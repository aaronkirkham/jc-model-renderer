#include "avalanche_data_format.h"
#include "avalanche_archive.h"

#include "../../window.h"
#include "../file_loader.h"
#include "../hashlittle.h"

#include <bitset>
#include <fstream>

std::recursive_mutex                                     Factory<AvalancheDataFormat>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheDataFormat>> Factory<AvalancheDataFormat>::Instances;

AvalancheDataFormat::AvalancheDataFormat(const std::filesystem::path& filename, const FileBuffer& buffer)
    : m_Filename(filename)
    , m_Buffer(std::move(buffer))
{
    using namespace jc::AvalancheDataFormat;

    AddBuiltInType(EAdfType::Primitive, ScalarType::Unsigned, sizeof(uint8_t), "uint8", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Signed, sizeof(int8_t), "int8", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Unsigned, sizeof(uint16_t), "uint16", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Signed, sizeof(int16_t), "int16", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Unsigned, sizeof(uint32_t), "uint32", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Signed, sizeof(int32_t), "int32", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Unsigned, sizeof(uint64_t), "uint64", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Signed, sizeof(int64_t), "int64", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Float, sizeof(float), "float", 3);
    AddBuiltInType(EAdfType::Primitive, ScalarType::Float, sizeof(double), "double", 3);
    AddBuiltInType(EAdfType::String, ScalarType::Signed, 8, "String", 0); // NOTE: keep capitalization
    AddBuiltInType(EAdfType::Deferred, ScalarType::Signed, 16, "void", 0);

    // load internal types
    if (m_Buffer.size() > 0) {
        ParseTypes((const char*)m_Buffer.data(), m_Buffer.size());
    }

    // load type libraries
    if (filename.has_extension()) {
        ReadTypeLibrariesByExtension(filename.extension());
    }
}

AvalancheDataFormat::~AvalancheDataFormat()
{
    for (auto& type : m_Types) {
        std::free(type);
    }
}

void AvalancheDataFormat::GetInstance(uint32_t index, jc::AvalancheDataFormat::SInstanceInfo* instance_info)
{
    using namespace jc::AvalancheDataFormat;

    Header header_out{};
    bool   byte_swap_out;
    if (!ParseHeader((Header*)m_Buffer.data(), m_Buffer.size(), &header_out, &byte_swap_out)) {
#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

    if (header_out.m_Version != 4) {
#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

    const Instance* instance = (Instance*)&m_Buffer[header_out.m_FirstInstanceOffset + (sizeof(Instance) * index)];

    instance_info->m_NameHash = instance->m_NameHash;
    instance_info->m_TypeHash = instance->m_TypeHash;
    instance_info->m_Name     = m_Strings[instance->m_Name].c_str();

    const auto type = FindType(instance->m_TypeHash);
    if (type) {
        instance_info->m_Instance     = &m_Buffer[instance->m_PayloadOffset];
        instance_info->m_InstanceSize = instance->m_PayloadSize;
        return;
    }

    instance_info->m_Instance     = nullptr;
    instance_info->m_InstanceSize = 0;
}

void AvalancheDataFormat::ReadInstance(uint32_t name_hash, uint32_t type_hash, void** out_instance)
{
    using namespace jc::AvalancheDataFormat;

    Header header_out{};
    bool   byte_swap_out;
    if (!ParseHeader((Header*)m_Buffer.data(), m_Buffer.size(), &header_out, &byte_swap_out)) {
#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

    // find the instance
    Instance* current_instance = nullptr;
    auto      instance_buffer  = &m_Buffer[header_out.m_FirstInstanceOffset];
    for (uint32_t i = 0; i < header_out.m_InstanceCount; ++i) {
        current_instance = (Instance*)instance_buffer;
        if (current_instance->m_NameHash == name_hash && current_instance->m_TypeHash == type_hash) {
            break;
        }

        instance_buffer += sizeof(Instance);
    }

    if (!current_instance) {
#ifdef DEBUG
        SPDLOG_ERROR("Can't find instance with name hash {:x} and type hash {:x}", name_hash, type_hash);
        __debugbreak();
#endif
        return;
    }

    const auto type = FindType(current_instance->m_TypeHash);
    const auto name = m_Strings[current_instance->m_Name];

    SPDLOG_INFO("Reading instance \"{}\"...", name);

    auto payload = &m_Buffer[current_instance->m_PayloadOffset];

    // alloc the memory for the result
    const auto mem = std::malloc(current_instance->m_PayloadSize);
    std::memcpy(mem, payload, current_instance->m_PayloadSize);

    // adjust the relative offsets
    uint64_t current_offset = 0;
    uint64_t v72            = 0;
    for (auto size = *(uint32_t*)&payload[current_instance->m_PayloadSize]; size;
         *(uint64_t*)((uint32_t)(current_offset - 4) + (uint64_t)mem) = (uint64_t)mem + v72) {

        current_offset = (current_offset + size);
        size           = *(uint32_t*)(current_offset + (uint64_t)mem);
        v72            = *(uint32_t*)((uint32_t)(current_offset - 4) + (uint64_t)mem);

        if (v72 == 1) {
            v72 = 0;
        }
    }

    *out_instance = mem;
}

void AvalancheDataFormat::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    /*if (auto adf = AvalancheDataFormat::make(filename)) {
        adf->Parse(data);
    }*/
}

bool AvalancheDataFormat::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    return false;
}

void AvalancheDataFormat::AddBuiltInType(jc::AvalancheDataFormat::EAdfType   type,
                                         jc::AvalancheDataFormat::ScalarType scalar_type, uint32_t size,
                                         const char* name, uint16_t flags)
{
    char type_name[64];
    snprintf(type_name, sizeof(type_name), "%s%u%u%u", name, static_cast<uint32_t>(type), size, size);

    auto type_hash = hashlittle(type_name);
    auto alignment = size;

    if (type == jc::AvalancheDataFormat::EAdfType::Deferred) {
        alignment = 8;
        type_hash = 0xDEFE88ED;
    }

    // create the type def
    const auto def             = new jc::AvalancheDataFormat::Type;
    def->m_AdfType             = type;
    def->m_Size                = size;
    def->m_Align               = alignment;
    def->m_TypeHash            = type_hash;
    def->m_Name                = -1;
    def->m_Flags               = flags;
    def->m_ScalarType          = scalar_type;
    def->m_SubTypeHash         = 0;
    def->m_ArraySizeOrBitCount = 0;
    def->m_MemberCount         = 0;
    m_Types.emplace_back(std::move(def));
}

void AvalancheDataFormat::ParseTypes(const char* data, uint64_t data_size, bool is_internal_types)
{
    using namespace jc::AvalancheDataFormat;

    Header header_out{};
    bool   byte_swap_out;
    if (!ParseHeader((Header*)data, data_size, &header_out, &byte_swap_out)) {
#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

#ifdef _DEBUG
    if (byte_swap_out) {
        __debugbreak();
    }
#endif

    // load string hashes
    uint32_t   string_hash_offset = 0;
    const auto string_hashes      = &data[header_out.m_FirstStringHashOffset];
    for (uint32_t i = 0; i < header_out.m_StringHashCount; ++i) {
        const auto string = &string_hashes[string_hash_offset];
        const auto hash   = *(uint64_t*)&string_hashes[string_hash_offset + (strlen(string) + 1)];

        m_StringHashes[hash] = string;
        string_hash_offset += (strlen(string) + 1) + 8;
    }

    // load strings
    auto                 strings_buffer = &data[header_out.m_FirstStringDataOffset];
    std::vector<uint8_t> string_lens(strings_buffer, &strings_buffer[header_out.m_StringCount]);
    uint32_t             idx = 0;
    for (uint32_t i = 0; i < header_out.m_StringCount; ++i) {
        const auto size = string_lens[i] + 1;
        auto       name = std::unique_ptr<char[]>(new char[size]);
        strcpy_s(name.get(), size, &strings_buffer[header_out.m_StringCount + idx]);

        m_Strings.emplace_back(name.get());

        // keep a list of strings which are only available in the internal types.
        // NOTE: this is for later when we want to export
        if (is_internal_types) {
            m_InternalStrings.emplace_back(name.get());
        }

        idx += size;
    }

    // parse types
    if (header_out.m_Version > 3) {
        auto types_buffer = &data[header_out.m_FirstTypeOffset];
        auto _type_data   = types_buffer;
        for (uint32_t i = 0; i < header_out.m_TypeCount; ++i) {
            const auto current_type = (Type*)_type_data;
            const auto size         = current_type->m_MemberCount
                                  * ((current_type->m_AdfType == EAdfType::Enumeration) ? sizeof(Enum) : sizeof(Member))
                              + sizeof(Type);

            // get the name
            const auto name = GetInstanceNameFromStrings(
                &data[header_out.m_FirstStringDataOffset],
                &data[header_out.m_FirstStringDataOffset + header_out.m_StringCount], current_type->m_Name);

            // do we already have this type?
            if (FindType(current_type->m_TypeHash)) {
                goto NEXT_TYPE;
            }

            SPDLOG_TRACE("Adding type \"{}\"...", name);

            // copy the type (and members)
            {
                auto type = std::malloc(size);
                std::memcpy(type, current_type, size);

                const auto it = std::find_if(m_Strings.begin(), m_Strings.end(),
                                             [name](const std::string& str) { return str == name; });

                // adjust the type name index
                const auto _cast_type = (Type*)type;
                _cast_type->m_Name    = std::distance(m_Strings.begin(), it);

                // adjust all member name indices
                const auto is_enum = (_cast_type->m_AdfType == EAdfType::Enumeration);
                for (uint32_t x = 0; x < _cast_type->m_MemberCount; ++x) {
                    const auto ptr =
                        (void*)((char*)type + sizeof(Type) + ((is_enum ? sizeof(Enum) : sizeof(Member)) * x));
                    const auto name_index = *(uint64_t*)ptr;

                    // find the original name in the new strings list
                    const auto name = GetInstanceNameFromStrings(
                        &data[header_out.m_FirstStringDataOffset],
                        &data[header_out.m_FirstStringDataOffset + header_out.m_StringCount], name_index);
                    const auto it = std::find_if(m_Strings.begin(), m_Strings.end(),
                                                 [name](const std::string& str) { return str == name; });

                    // update the new string index
                    *(uint64_t*)ptr = std::distance(m_Strings.begin(), it);
                }

                m_Types.emplace_back((Type*)type);
            }

        NEXT_TYPE:
            _type_data += size;
        }
    } else {
#ifdef DEBUG
        __debugbreak();
#endif
    }
}

void AvalancheDataFormat::ReadTypeLibrary(const std::filesystem::path& filename)
{
    const auto buffer = FileLoader::Get()->GetAdfTypeLibraries()->GetEntryBuffer(filename.string());
    if (buffer.size() > 0) {
        ParseTypes((char*)buffer.data(), buffer.size());
    }
}

void AvalancheDataFormat::ReadTypeLibrariesByExtension(const std::filesystem::path& extension)
{
    // blo_adf, flo_adf, epe_adf
    if (extension == ".blo_adf" || extension == ".flo_adf" || extension == ".epe_adf") {
        ReadTypeLibrary("AttachedEffects.adf");
        ReadTypeLibrary("Damageable.adf");
        ReadTypeLibrary("DamageController.adf");
        ReadTypeLibrary("LocationGameObjectAdf.adf");
        ReadTypeLibrary("PhysicsGameObject.adf");
        ReadTypeLibrary("RigidObject.adf");
        ReadTypeLibrary("TargetSystem.adf");
    }
}

bool AvalancheDataFormat::ParseHeader(jc::AvalancheDataFormat::Header* data, uint64_t data_size,
                                      jc::AvalancheDataFormat::Header* header_out, bool* byte_swap_out,
                                      const char** description_out)
{
    if (data_size < 24) {
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    // needs byte-swap
    if (data->m_Magic == 0x20464441) {
        *byte_swap_out = true;
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    if (data->m_Magic != 0x41444620) {
        return false;
    }

    *byte_swap_out = false;
    switch (data->m_Version) {
        case 2: {
            header_out->m_Magic               = data->m_Magic;
            header_out->m_Version             = data->m_Version;
            header_out->m_InstanceCount       = data->m_InstanceCount;
            header_out->m_FirstInstanceOffset = data->m_FirstInstanceOffset;
            header_out->m_TypeCount           = data->m_TypeCount;
            header_out->m_FirstTypeOffset     = data->m_FirstTypeOffset;
            return true;
        }

        case 3: {
            header_out->m_Magic                 = data->m_Magic;
            header_out->m_Version               = data->m_Version;
            header_out->m_InstanceCount         = data->m_InstanceCount;
            header_out->m_FirstInstanceOffset   = data->m_FirstInstanceOffset;
            header_out->m_TypeCount             = data->m_TypeCount;
            header_out->m_FirstTypeOffset       = data->m_FirstTypeOffset;
            header_out->m_StringHashCount       = data->m_StringHashCount;
            header_out->m_FirstStringHashOffset = data->m_FirstStringHashOffset;
            return true;
        }

        case 4: {
            header_out->m_Magic                 = data->m_Magic;
            header_out->m_Version               = data->m_Version;
            header_out->m_InstanceCount         = data->m_InstanceCount;
            header_out->m_FirstInstanceOffset   = data->m_FirstInstanceOffset;
            header_out->m_TypeCount             = data->m_TypeCount;
            header_out->m_FirstTypeOffset       = data->m_FirstTypeOffset;
            header_out->m_StringHashCount       = data->m_StringHashCount;
            header_out->m_FirstStringHashOffset = data->m_FirstStringHashOffset;
            header_out->m_StringCount           = data->m_StringCount;
            header_out->m_FirstStringDataOffset = data->m_FirstStringDataOffset;
            header_out->m_FileSize              = data->m_FileSize;
            header_out->unknown                 = data->unknown;
            header_out->m_Flags                 = data->m_Flags;
            header_out->m_IncludedLibraries     = data->m_IncludedLibraries;
            header_out->m_Description           = data->m_Description;

            if (description_out && data->m_Description) {
                *description_out = (const char*)&data->m_Description;
            }
            return true;
        }
    }

    return false;
}

const char* AvalancheDataFormat::GetInstanceNameFromStrings(const char* string_lengths, const char* strings,
                                                            uint64_t string_index)
{
    int64_t  v3 = 0;
    int64_t  v4 = 0;
    uint64_t v5 = 0;
    int32_t  v6 = 0;

    if (string_index >= 2) {
        do {
            v6 = string_lengths[v5];
            v5 += 2i64;
            v3 += v6 + 1;
            v4 += string_lengths[v5 - 1] + 1;
        } while (v5 < string_index - 1);
    }
    if (v5 < string_index)
        strings += string_lengths[v5] + 1;

    return &strings[v4 + v3];
}

jc::AvalancheDataFormat::Type* AvalancheDataFormat::FindType(uint32_t type_hash)
{
    const auto it = std::find_if(m_Types.begin(), m_Types.end(), [&](const jc::AvalancheDataFormat::Type* type) {
        return type->m_TypeHash == type_hash;
    });

    if (it == m_Types.end()) {
        return nullptr;
    }

    return *it;
}
