#include "avalanche_data_format.h"
#include "../../window.h"
#include "../hashlittle.h"

#include <fstream>

#include <bitset>

std::recursive_mutex                                     Factory<AvalancheDataFormat>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheDataFormat>> Factory<AvalancheDataFormat>::Instances;

AvalancheDataFormat::AvalancheDataFormat(const std::filesystem::path& file)
    : m_File(file)
{
    AddBuiltInTypes();
}

AvalancheDataFormat::AvalancheDataFormat(const std::filesystem::path& filename, const FileBuffer& buffer)
    : m_File(filename)
{
    AddBuiltInTypes();
}

void AvalancheDataFormat::AddBuiltInTypes()
{
    using namespace jc::AvalancheDataFormat;
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Unsigned, sizeof(uint8_t), 0, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Signed, sizeof(int8_t), 1, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Unsigned, sizeof(uint16_t), 2, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Signed, sizeof(int16_t), 3, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Unsigned, sizeof(uint32_t), 4, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Signed, sizeof(int32_t), 5, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Unsigned, sizeof(uint64_t), 6, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Signed, sizeof(int64_t), 7, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Float, sizeof(float), 8, 3);
    AddBuiltInType(TypeDefinitionType::Primitive, PrimitiveType::Float, sizeof(double), 9, 3);
    AddBuiltInType(TypeDefinitionType::String, PrimitiveType::Signed, 8, 10, 0);
    AddBuiltInType(TypeDefinitionType::Deferred, PrimitiveType::Signed, 16, 11, 0);
}

void AvalancheDataFormat::AddBuiltInType(jc::AvalancheDataFormat::TypeDefinitionType type,
                                         jc::AvalancheDataFormat::PrimitiveType primitive_type, uint32_t size,
                                         uint32_t type_index, uint16_t flags)
{
    using namespace jc::AvalancheDataFormat;

    const auto name = ADF_TYPE_NAMES[type_index];

    char str[64];
    snprintf(str, sizeof(str), "%s%u%u%u", name, static_cast<uint32_t>(type), size, size);

    auto type_hash = hashlittle(str);
    auto alignment = size;

    if (type == TypeDefinitionType::Deferred) {
        alignment = 8;
        type_hash = 0xDEFE88ED;
    }

    TypeDefinition definition{};
    definition.m_Type          = type;
    definition.m_Size          = size;
    definition.m_Alignment     = alignment;
    definition.m_TypeHash      = static_cast<TypeMemberHash>(type_hash);
    definition.m_NameIndex     = -1;
    definition.m_Flags         = flags;
    definition.m_PrimitiveType = primitive_type;

    auto typeDef          = std::make_unique<AdfTypeDefinition>();
    typeDef->m_Parent     = this;
    typeDef->m_Name       = str;
    typeDef->m_Definition = std::move(definition);

    m_TypeDefinitions.emplace_back(std::move(typeDef));
}

bool AvalancheDataFormat::Parse(const FileBuffer& data)
{
    std::istringstream stream(std::string{(char*)data.data(), data.size()});

    jc::AvalancheDataFormat::Header header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct "ADF"
    if (header.m_Magic != 0x41444620) {
        SPDLOG_ERROR("Invalid header magic.");
        return false;
    }

    // make sure it's the correct version
    if (header.m_Version != 4) {
        SPDLOG_ERROR("Unexpected ADF file version! (m_Version={})", header.m_Version);
        return false;
    }

    // read names
    std::vector<uint8_t> name_lengths;
    name_lengths.resize(header.m_StringCount);
    stream.seekg(header.m_StringOffset);
    stream.read((char*)&name_lengths.front(), sizeof(uint8_t) * header.m_StringCount);
    for (uint32_t i = 0; i < header.m_StringCount; ++i) {
        auto name = std::unique_ptr<char[]>(new char[name_lengths[i] + 1]);
        stream.read(name.get(), name_lengths[i] + 1);

        m_Names.emplace_back(name.get());
    }

    // read type definitions
    stream.seekg(header.m_TypeOffset);
    for (uint32_t i = 0; i < header.m_TypeCount; ++i) {
        jc::AvalancheDataFormat::TypeDefinition definition;
        stream.read((char*)&definition, sizeof(definition));

        auto typeDef          = std::make_unique<AdfTypeDefinition>();
        typeDef->m_Parent     = this;
        typeDef->m_Name       = m_Names[definition.m_NameIndex];
        typeDef->m_Definition = definition;

        SPDLOG_TRACE("TypeDef: \"{}\", m_Type=\"{}\", m_PrimitiveType={}, m_TypeHash={:x}", typeDef->m_Name,
                     jc::AvalancheDataFormat::TypeDefinitionStr(definition.m_Type),
                     static_cast<int32_t>(definition.m_PrimitiveType), static_cast<uint32_t>(definition.m_TypeHash));

        switch (definition.m_Type) {
            case jc::AvalancheDataFormat::TypeDefinitionType::Structure: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));

                // read members
                typeDef->m_Members.resize(count);
                stream.read((char*)&typeDef->m_Members.front(),
                            sizeof(jc::AvalancheDataFormat::MemeberDefinition) * count);
                break;
            }

            case jc::AvalancheDataFormat::TypeDefinitionType::Array:
            case jc::AvalancheDataFormat::TypeDefinitionType::InlineArray:
            case jc::AvalancheDataFormat::TypeDefinitionType::StringHash: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));
                break;
            }

#ifdef DEBUG
            case jc::AvalancheDataFormat::TypeDefinitionType::Pointer: {
                uint32_t unknown;
                stream.read((char*)&unknown, sizeof(unknown));

                __debugbreak();
                break;
            }
#endif

            case jc::AvalancheDataFormat::TypeDefinitionType::BitField: {
                uint32_t unknown;
                stream.read((char*)&unknown, sizeof(unknown));
                break;
            }

            case jc::AvalancheDataFormat::TypeDefinitionType::Enumeration: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));

                // read enums
                typeDef->m_Enums.resize(count);
                stream.read((char*)&typeDef->m_Enums.front(), sizeof(jc::AvalancheDataFormat::Enum) * count);
                break;
            }

            default: {
#ifdef DEBUG
                __debugbreak();
#endif
                break;
            }
        }

        m_TypeDefinitions.emplace_back(std::move(typeDef));
    }

    // read string hashes
    stream.seekg(header.m_StringHashOffset);
    m_StringHashes.reserve(header.m_StringHashCount);
    for (uint32_t i = 0; i < header.m_StringHashCount; ++i) {
        std::string value;
        uint64_t    hash;

        std::getline(stream, value, '\0');
        stream.read((char*)&hash, sizeof(hash));

        m_StringHashes.emplace_back(std::make_pair(hash, value));
    }

    // read instances
    stream.seekg(header.m_InstanceOffset);
    for (uint32_t i = 0; i < header.m_InstanceCount; ++i) {
        jc::AvalancheDataFormat::InstanceInfo instance;
        stream.read((char*)&instance, sizeof(instance));

        auto instanceInfo        = std::make_unique<AdfInstanceInfo>();
        instanceInfo->m_Name     = m_Names[instance.m_NameIndex];
        instanceInfo->m_Type     = GetTypeDefinition(instance.m_TypeHash);
        instanceInfo->m_Offset   = instance.m_Offset;
        instanceInfo->m_TypeHash = instance.m_TypeHash;
        instanceInfo->m_Parent   = this;

        const auto pos = stream.tellg();

        // read the instance data
        instanceInfo->m_Data.resize(instance.m_Size);
        stream.seekg(instance.m_Offset);
        stream.read((char*)instanceInfo->m_Data.data(), instance.m_Size);

        SPDLOG_TRACE("Instance Name=\"{}\", m_Size={}, m_Type={}", m_Names[instance.m_NameIndex], instance.m_Size,
                     jc::AvalancheDataFormat::TypeDefinitionStr(instanceInfo->m_Type->m_Definition.m_Type));

        instanceInfo->ReadMembers();
        m_InstanceInfos.emplace_back(std::move(instanceInfo));

        // get back to the correct file offset
        stream.seekg(pos);
    }

    return true;
}

void AvalancheDataFormat::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    if (auto adf = AvalancheDataFormat::make(filename)) {
        adf->Parse(data);
    }
}

bool AvalancheDataFormat::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
#if 0
    if (auto adf = AvalancheDataFormat::get(filename.string())) {
        using namespace jc::AvalancheDataFormat;

        std::ofstream stream(path, std::ios::binary);
        if (stream.fail()) {
            return false;
        }

        const auto& instance_infos = adf->GetInstanceInfos();
        for (const auto& instance : instance_infos) {
            //
        }

        Header header;
        header.m_InstanceCount    = instance_infos.size();
        header.m_InstanceOffset   = 0;
        header.m_TypeCount        = 0;
        header.m_TypeOffset       = 0;
        header.m_StringHashCount  = 0;
        header.m_StringHashOffset = 0;
        header.m_StringCount      = 0;
        header.m_StringOffset     = 0;
        header.m_FileSize         = 0;
        stream.write((char*)&header, sizeof(header));
        stream.close();
    }
#endif

    return false;
}

AdfTypeDefinition* AvalancheDataFormat::GetTypeDefinition(jc::AvalancheDataFormat::TypeMemberHash type_hash)
{
    auto it = std::find_if(m_TypeDefinitions.begin(), m_TypeDefinitions.end(),
                           [&](const std::unique_ptr<AdfTypeDefinition>& definition) {
                               return definition->m_Definition.m_TypeHash == type_hash;
                           });

    if (it == m_TypeDefinitions.end()) {
        return nullptr;
    }

    return (*it).get();
}

AdfInstanceMemberInfo* AvalancheDataFormat::GetMember(const std::string& name)
{
    for (const auto& instance : m_InstanceInfos) {
        for (const auto& member : instance->m_Members) {
            const auto found = member->GetMember(name);
            if (found) {
                return found;
            }
        }
    }

    return nullptr;
}

void AdfInstanceInfo::ReadMembers()
{
    m_Queue = {};

    auto info = std::make_unique<AdfInstanceMemberInfo>(0, m_Name, this);
    info->InitNonPrimitive(m_Type, jc::AvalancheDataFormat::TypeMemberHash::Int64, 0, 0);
    m_Members.emplace_back(std::move(info));

    while (m_Queue.size() > 0) {
        const auto info = m_Queue.front();
        switch (info->m_TypeDef->m_Definition.m_Type) {
            case jc::AvalancheDataFormat::TypeDefinitionType::Structure: {
                MemberSetup_Structure(info);
                break;
            }

            case jc::AvalancheDataFormat::TypeDefinitionType::Array:
            case jc::AvalancheDataFormat::TypeDefinitionType::InlineArray: {
                MemberSetup_Array(info);
                break;
            }

            case jc::AvalancheDataFormat::TypeDefinitionType::StringHash: {
                MemberSetup_StringHash(info);
                break;
            }
        }

        m_Queue.pop();
    }
}

void AdfInstanceInfo::MemberSetup(AdfInstanceMemberInfo* info)
{
    m_Queue.push(info);
}

void AdfInstanceInfo::MemberSetup_Structure(AdfInstanceMemberInfo* info)
{
    for (auto& member : info->m_TypeDef->m_Members) {
        MemberSetupStructMember(info, &member, member.m_TypeHash,
                                info->m_TypeDef->m_Parent->m_Names[member.m_NameIndex],
                                info->m_Offset + member.m_Offset);
    }
}

void AdfInstanceInfo::MemberSetupStructMember(AdfInstanceMemberInfo*                      info,
                                              jc::AvalancheDataFormat::MemeberDefinition* type_definition,
                                              jc::AvalancheDataFormat::TypeMemberHash     type_hash,
                                              const std::string& name, int64_t offset)
{
    using namespace jc::AvalancheDataFormat;

    info->m_Adf->m_DataReadOffset = offset;
    auto new_info                 = std::make_unique<AdfInstanceMemberInfo>(-1, name, this);

    switch (type_hash) {
        case TypeMemberHash::Int8:
        case TypeMemberHash::UInt8:
        case TypeMemberHash::Int16:
        case TypeMemberHash::UInt16:
        case TypeMemberHash::Int32:
        case TypeMemberHash::UInt32:
        case TypeMemberHash::Int64:
        case TypeMemberHash::UInt64:
        case TypeMemberHash::Float:
        case TypeMemberHash::Double: {
            new_info->InitPrimitive(type_hash, offset);
            info->m_Members.emplace_back(std::move(new_info));
            break;
        }

        case TypeMemberHash::String: {
            const auto dest_offset = info->m_Adf->ReadData<uint32_t>();
            new_info->InitPrimitive(type_hash, dest_offset);
            info->m_Members.emplace_back(std::move(new_info));
            break;
        }

        default: {
            const auto definition = info->m_TypeDef->m_Parent->GetTypeDefinition(type_hash);
            if (!definition) {
                SPDLOG_ERROR("Struct member \"{}\" - unknown type definition: {:x}", name,
                             static_cast<uint32_t>(type_hash));
                break;
            }

            switch (definition->m_Definition.m_Type) {
                case TypeDefinitionType::InlineArray:
                case TypeDefinitionType::Structure:
                case TypeDefinitionType::StringHash: {
                    new_info->InitNonPrimitive(definition, type_hash, offset,
                                               definition->m_Definition.m_ArraySizeOrBitCount);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::Array: {
                    const auto dest_offset   = info->m_Adf->ReadData<uint32_t>();
                    const auto unknown       = info->m_Adf->ReadData<uint32_t>();
                    const auto element_count = info->m_Adf->ReadData<int64_t>();

#ifdef DEBUG
                    if (dest_offset == 0 && element_count == 0) {
                        __debugbreak();
                        break;
                    }
#endif

                    int64_t    id;
                    const auto find_it = m_MemberOffsets.find(dest_offset);
                    if (find_it == m_MemberOffsets.end()) {
                        // use the last id
                        id = m_Members.size();
                        new_info->InitNonPrimitive(definition, type_hash, dest_offset, element_count);

                        /*uint32_t insert_index = id;
                        for (auto i = 0; i < m_Members.size(); ++i) {
                            if (dest_offset < m_Members[i]->m_Offset) {
                                insert_index = i;
                                SPDLOG_INFO("Correct insert index should be {}", insert_index);
                            }
                        }*/

                        m_MemberOffsets[dest_offset] = new_info.get();
                        m_Members.emplace_back(std::move(new_info));
                    } else {
                        id = (*find_it).second->m_Id;
                        SPDLOG_INFO("Member offset already exists. Using reference to id {}", id);
                    }

                    auto ref_info = std::make_unique<AdfInstanceMemberInfo>(id, name, this);
                    ref_info->InitReference(definition, type_hash);
                    info->m_Members.emplace_back(std::move(ref_info));
                    break;
                }

                case TypeDefinitionType::Pointer: {
                    SPDLOG_WARN("Struct member \"{}\" is a pointer", name);
#ifdef DEBUG
                    __debugbreak();
#endif

                    new_info->InitPointer(definition, offset);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::BitField: {
                    new_info->InitBitField(definition, offset, type_definition->m_BitIndex);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::Enumeration: {
                    new_info->InitEnum(definition, offset);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::Deferred: {
                    const auto unknown   = info->m_Adf->ReadData<uint32_t>();
                    const auto unknown2  = info->m_Adf->ReadData<uint32_t>();
                    const auto type_hash = info->m_Adf->ReadData<uint32_t>();

                    const auto member_def =
                        info->m_TypeDef->m_Parent->GetTypeDefinition(static_cast<TypeMemberHash>(type_hash));
                    if (member_def) {
                        // TODO: we should really keep a reference instanceinfo, for later when we write back to a file.

                        new_info->InitPointer(member_def, offset);
                        info->m_Members.emplace_back(std::move(new_info));
                    } else {
                        SPDLOG_ERROR("Unable to find typedef for type {:x}", type_hash);
                    }
                    break;
                }

                default: {
                    SPDLOG_TRACE("Struct member \"{}\" is {}", name,
                                 static_cast<int32_t>(definition->m_Definition.m_Type));
#ifdef DEBUG
                    __debugbreak();
#endif
                    break;
                }
            }

            break;
        }
    }
}

void AdfInstanceInfo::MemberSetup_Array(AdfInstanceMemberInfo* info)
{
    using namespace jc::AvalancheDataFormat;

    assert(info);
    assert(info->m_TypeDef);

    auto&      definition  = info->m_TypeDef->m_Definition;
    const auto elementType = info->m_TypeDef->m_Parent->GetTypeDefinition(definition.m_ElementTypeHash);

    if (!elementType) {
        SPDLOG_ERROR("Couldn't find a type definition for \"{}\" m_ElementTypeHash={:x}",
                     info->m_Adf->m_Parent->m_Names[definition.m_NameIndex],
                     static_cast<uint32_t>(definition.m_ElementTypeHash));

#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

    switch (definition.m_ElementTypeHash) {
        case TypeMemberHash::Int8:
        case TypeMemberHash::UInt8: {
            std::vector<uint8_t> buffer;
            buffer.resize(info->m_ExpectedElementCount);
            std::memcpy((char*)&buffer.front(), &m_Data[info->m_Offset], buffer.size());

            info->m_Data     = std::move(buffer);
            info->m_TypeHash = definition.m_ElementTypeHash;
            break;
        }

        case TypeMemberHash::Int16:
        case TypeMemberHash::UInt16: {
            std::vector<uint8_t> buffer;
            buffer.resize(sizeof(uint16_t) * info->m_ExpectedElementCount);
            std::memcpy((char*)&buffer.front(), &m_Data[info->m_Offset], buffer.size());

            info->m_Data     = CastBuffer<uint16_t>(&buffer);
            info->m_TypeHash = definition.m_ElementTypeHash;
            break;
        }

        case TypeMemberHash::Int32:
        case TypeMemberHash::UInt32: {
            std::vector<uint8_t> buffer;
            buffer.resize(sizeof(uint32_t) * info->m_ExpectedElementCount);
            std::memcpy((char*)&buffer.front(), &m_Data[info->m_Offset], buffer.size());

            info->m_Data     = CastBuffer<uint32_t>(&buffer);
            info->m_TypeHash = definition.m_ElementTypeHash;
            break;
        }

        case TypeMemberHash::Int64:
        case TypeMemberHash::UInt64: {
            std::vector<uint8_t> buffer;
            buffer.resize(sizeof(uint64_t) * info->m_ExpectedElementCount);
            std::memcpy((char*)&buffer.front(), &m_Data[info->m_Offset], buffer.size());

            info->m_Data     = CastBuffer<uint64_t>(&buffer);
            info->m_TypeHash = definition.m_ElementTypeHash;
            break;
        }

        case TypeMemberHash::Float: {
            std::vector<uint8_t> buffer;
            buffer.resize(sizeof(float) * info->m_ExpectedElementCount);
            std::memcpy((char*)&buffer.front(), &m_Data[info->m_Offset], buffer.size());

            info->m_Data     = CastBuffer<float>(&buffer);
            info->m_TypeHash = definition.m_ElementTypeHash;
            break;
        }

        case TypeMemberHash::Double: {
            std::vector<uint8_t> buffer;
            buffer.resize(sizeof(double) * info->m_ExpectedElementCount);
            std::memcpy((char*)&buffer.front(), &m_Data[info->m_Offset], buffer.size());

            info->m_Data     = CastBuffer<double>(&buffer);
            info->m_TypeHash = definition.m_ElementTypeHash;
            break;
        }

        case TypeMemberHash::Deferred: {
            SPDLOG_WARN("Array element is a deferred pointer-reference.");
#ifdef DEBUG
            __debugbreak();
#endif
            break;
        }

        default: {
            for (uint32_t i = 0; i < info->m_ExpectedElementCount; ++i) {
                const auto name = info->m_Name + "[" + std::to_string(i) + "]";
                MemberSetupStructMember(info, nullptr, definition.m_ElementTypeHash, name,
                                        (info->m_Offset + (elementType->m_Definition.m_Size * i)));
            }

            break;
        }
    }
}

void AdfInstanceInfo::MemberSetup_StringHash(AdfInstanceMemberInfo* info)
{
    info->m_Adf->m_DataReadOffset = info->m_Offset;
    const auto  hash              = info->m_Adf->ReadData<uint32_t>();
    const auto& string_hashes     = info->m_Adf->m_Parent->m_StringHashes;

    auto it = std::find_if(string_hashes.begin(), string_hashes.end(),
                           [&](const std::pair<uint32_t, std::string>& value) { return value.first == hash; });

    if (it != string_hashes.end()) {
        info->m_Data = (*it).second;
        SPDLOG_TRACE("StringHash value: \"{}\" ({:x})", (*it).second, hash);
    }
}

void AdfInstanceMemberInfo::InitReference(AdfTypeDefinition* type, jc::AvalancheDataFormat::TypeMemberHash typehash)
{
    m_Type            = type->m_Definition.m_Type;
    m_TypeDef         = type;
    m_TypeHash        = typehash;
    m_IsReferenceToId = true;
}

void AdfInstanceMemberInfo::InitNonPrimitive(AdfTypeDefinition* type, jc::AvalancheDataFormat::TypeMemberHash typehash,
                                             int64_t offset, uint32_t element_count)
{
    m_Type                 = type->m_Definition.m_Type;
    m_TypeDef              = type;
    m_TypeHash             = typehash;
    m_Offset               = offset;
    m_FileOffset           = (m_Adf->m_Offset + offset);
    m_ExpectedElementCount = element_count;

    m_Adf->MemberSetup(this);
}

void AdfInstanceMemberInfo::InitPrimitive(jc::AvalancheDataFormat::TypeMemberHash typehash, int64_t offset)
{
    m_Type       = jc::AvalancheDataFormat::TypeDefinitionType::Primitive;
    m_TypeHash   = typehash;
    m_Offset     = offset;
    m_FileOffset = (m_Adf->m_Offset + offset);

    using namespace jc::AvalancheDataFormat;

    m_Adf->m_DataReadOffset = offset;
    switch (typehash) {
        case TypeMemberHash::Int8:
        case TypeMemberHash::UInt8: {
            m_Data = m_Adf->ReadData<uint8_t>();
            break;
        }

        case TypeMemberHash::Int16:
        case TypeMemberHash::UInt16: {
            m_Data = m_Adf->ReadData<uint16_t>();
            break;
        }

        case TypeMemberHash::Int32:
        case TypeMemberHash::UInt32: {
            m_Data = m_Adf->ReadData<uint32_t>();
            break;
        }

        case TypeMemberHash::Int64:
        case TypeMemberHash::UInt64: {
            m_Data = m_Adf->ReadData<uint64_t>();
            break;
        }

        case TypeMemberHash::Float: {
            m_Data = m_Adf->ReadData<float>();
            break;
        }

        case TypeMemberHash::Double: {
            m_Data = m_Adf->ReadData<double>();
            break;
        }

        case TypeMemberHash::String: {
            m_Data = m_Adf->ReadString();
            break;
        }
    }
}

void AdfInstanceMemberInfo::InitPointer(AdfTypeDefinition* type, int64_t offset)
{
    m_Type       = jc::AvalancheDataFormat::TypeDefinitionType::Deferred;
    m_TypeDef    = type;
    m_TypeHash   = jc::AvalancheDataFormat::TypeMemberHash::Deferred;
    m_Offset     = offset;
    m_FileOffset = (m_Adf->m_Offset + offset);

    m_Adf->MemberSetup(this);
}

void AdfInstanceMemberInfo::InitEnum(AdfTypeDefinition* type, int64_t offset)
{
    m_Type       = type->m_Definition.m_Type;
    m_TypeDef    = type;
    m_TypeHash   = type->m_Definition.m_TypeHash;
    m_Offset     = offset;
    m_FileOffset = (m_Adf->m_Offset + offset);

    // read the enum index and get the data
    const auto  enum_index = m_Adf->ReadData<uint32_t>();
    const auto& enum_data  = type->m_Enums[enum_index];

    // init enum member data
    auto member = std::make_unique<AdfInstanceMemberInfo>(-1, m_Adf->m_Parent->m_Names[enum_data.m_NameIndex], m_Adf);
    member->m_Type     = jc::AvalancheDataFormat::TypeDefinitionType::Primitive;
    member->m_TypeHash = jc::AvalancheDataFormat::TypeMemberHash::Int32;
    member->m_Data     = enum_data.m_Value;

    m_Members.emplace_back(std::move(member));
}

void AdfInstanceMemberInfo::InitBitField(AdfTypeDefinition* type, int64_t offset, uint32_t bit_index)
{
    m_Type       = type->m_Definition.m_Type;
    m_TypeDef    = type;
    m_TypeHash   = type->m_Definition.m_TypeHash;
    m_Offset     = offset;
    m_FileOffset = (m_Adf->m_Offset + offset);
    m_BitIndex   = bit_index;

    // TODO: handle cases when these are false.
    assert(type->m_Definition.m_ArraySizeOrBitCount == 1);
    assert(type->m_Definition.m_PrimitiveType == jc::AvalancheDataFormat::PrimitiveType::Unsigned);

    auto value    = m_Adf->ReadData<uint64_t>();
    auto bitfield = std::bitset<64>(value);
    m_Data        = bitfield.test(bit_index);

    SPDLOG_INFO("Value={}, BitIndex={}, Bit={}", value, bit_index, bitfield.test(bit_index) ? "True" : "False");
}

AdfInstanceMemberInfo* AdfInstanceMemberInfo::GetMember(const std::string& name)
{
    // resolve a reference, if the member points to another member
    static const auto ResolveReference = [&](AdfInstanceMemberInfo* member) {
        if (member->m_IsReferenceToId) {
            const auto& reference = member->m_Adf->m_Members.at(member->m_Id);
            return reference.get();
        }

        return member;
    };

    if (m_Name == name) {
        return ResolveReference(this);
    }

    for (const auto& member : m_Members) {
        if (member->m_Name == name) {
            return ResolveReference(member.get());
        }
    }

    return nullptr;
}
