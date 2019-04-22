#include "avalanche_data_format.h"
#include "../../window.h"
#include "../hashlittle.h"

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

    TypeDefinition definition;
    definition.m_Type                = type;
    definition.m_Size                = size;
    definition.m_Alignment           = alignment;
    definition.m_TypeHash            = static_cast<TypeMemberHash>(type_hash);
    definition.m_NameIndex           = -1;
    definition.m_Flags               = flags;
    definition.m_PrimitiveType       = primitive_type;
    definition.m_ElementTypeHash     = static_cast<TypeMemberHash>(0);
    definition.m_ArraySizeOrBitCount = 0;

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
        LOG_ERROR("Invalid header magic.");
        return false;
    }

    // make sure it's the correct version
    if (header.m_Version != 4) {
        LOG_ERROR("Unexpected ADF file version! (m_Version={})", header.m_Version);
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

        LOG_TRACE("TypeDef: \"{}\", m_Type=\"{}\", m_PrimitiveType={}, m_TypeHash={:x}", typeDef->m_Name,
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
            case jc::AvalancheDataFormat::TypeDefinitionType::Pointer:
            case jc::AvalancheDataFormat::TypeDefinitionType::StringHash: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));
                break;
            }

            case jc::AvalancheDataFormat::TypeDefinitionType::BitField: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));
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
        // instanceInfo->m_DataOffset = instance.m_Offset;

        LOG_TRACE("Instance Name=\"{}\", m_Size={}, m_Type={}", m_Names[instance.m_NameIndex], instance.m_Size,
                  jc::AvalancheDataFormat::TypeDefinitionStr(instanceInfo->m_Type->m_Definition.m_Type));

        instanceInfo->ReadMembers();
        m_InstanceInfos.emplace_back(std::move(instanceInfo));

        // get back to the correct file offset
        stream.seekg(pos);
    }

    return true;
}

void AvalancheDataFormat::FileReadCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    auto adf = AvalancheDataFormat::make(filename);
    assert(adf);
    adf->Parse(data);
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
            const auto found = GetMember(member.get(), name);
            if (found) {
                return found;
            }
        }
    }

    return nullptr;
}

AdfInstanceMemberInfo* AvalancheDataFormat::GetMember(AdfInstanceMemberInfo* info, const std::string& name)
{
    assert(info);

    if (info->m_Name == name) {
        return info;
    }

    for (const auto& member : info->m_Members) {
        if (member->m_Name == name || member->m_StringData == name) {
            return member.get();
        }

        const auto found = GetMember(member.get(), name);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

void AdfInstanceInfo::ReadMembers()
{
    m_Queue = {};
    m_Members.emplace_back(std::make_unique<AdfInstanceMemberInfo>(m_Name, m_Type, 0, this, 0));

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
    for (const auto& member : info->m_TypeDef->m_Members) {
        const auto& name = info->m_TypeDef->m_Parent->m_Names[member.m_NameIndex];
        MemberSetupStructMember(info, member.m_TypeHash, name, info->m_Offset + member.m_Offset);
    }
}

void AdfInstanceInfo::MemberSetupStructMember(AdfInstanceMemberInfo*                  info,
                                              jc::AvalancheDataFormat::TypeMemberHash type_hash,
                                              const std::string& name, int64_t offset)
{
    using namespace jc::AvalancheDataFormat;

    info->m_Adf->m_DataOffset = offset;

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
            LOG_TRACE("\"{}\" is a primivite", name);

            auto new_info = std::make_unique<AdfInstanceMemberInfo>(name, type_hash, offset, this);
            info->m_Members.emplace_back(std::move(new_info));
            break;
        }

        case TypeMemberHash::String: {
            LOG_TRACE("\"{}\" is a string (at {})", name, offset);

            auto dest_offset = info->m_Adf->ReadData<uint32_t>();
            auto new_info    = std::make_unique<AdfInstanceMemberInfo>(name, type_hash, dest_offset, this);
            info->m_Members.emplace_back(std::move(new_info));
            break;
        }

        default: {
            const auto definition = info->m_TypeDef->m_Parent->GetTypeDefinition(type_hash);
            if (!definition) {
                LOG_ERROR("\"{}\" - unknown type definition: {:x}", name, static_cast<uint32_t>(type_hash));
                break;
            }

            switch (definition->m_Definition.m_Type) {
                case TypeDefinitionType::InlineArray:
                case TypeDefinitionType::Structure:
                case TypeDefinitionType::StringHash: {
                    auto new_info =
                        std::make_unique<AdfInstanceMemberInfo>(name, definition, info->m_Adf->m_DataOffset, this,
                                                                definition->m_Definition.m_ArraySizeOrBitCount);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::Array: {
                    auto dest_offset = info->m_Adf->ReadData<uint32_t>();
                    info->m_Adf->m_DataOffset += 4;
                    auto element_count = info->m_Adf->ReadData<int64_t>();

#ifdef DEBUG
                    if (dest_offset == 0 && element_count == 0) {
                        __debugbreak();
                    }
#endif

                    if (m_MemberOffsets.find(dest_offset) == m_MemberOffsets.end()) {
                        auto new_info =
                            std::make_unique<AdfInstanceMemberInfo>(name, definition, dest_offset, this, element_count);
                        info->m_Members.emplace_back(std::move(new_info));
                    }

                    break;
                }

                case TypeDefinitionType::Pointer: {
                    LOG_WARN("\"{}\" is a pointer", name);

                    auto new_info = std::make_unique<AdfInstanceMemberInfo>(name, definition, offset, this);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::BitField: {
                    LOG_WARN("\"{}\" is a bitfield", name);
                    // m_ArraySizeOrBitCount
                    break;
                }

                case TypeDefinitionType::Enumeration: {
                    auto  enum_index = info->m_Adf->ReadData<uint32_t>();
                    auto& enum_data  = definition->m_Enums[enum_index];

                    auto new_info = std::make_unique<AdfInstanceMemberInfo>(name, definition, offset, this, &enum_data);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case TypeDefinitionType::Deferred: {
                    LOG_WARN("\"{}\" is a deferred pointer-reference", name);

                    auto new_info = std::make_unique<AdfInstanceMemberInfo>(name, definition, offset, this);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                default: {
                    LOG_TRACE("\"{}\" is {}", name, static_cast<int32_t>(definition->m_Definition.m_Type));
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

    const auto& definition  = info->m_TypeDef->m_Definition;
    const auto  elementType = info->m_TypeDef->m_Parent->GetTypeDefinition(definition.m_ElementTypeHash);

    if (!elementType) {
        LOG_ERROR("Couldn't find a type definition for \"{}\" m_ElementTypeHash={:x}",
                  info->m_Adf->m_Parent->m_Names[definition.m_NameIndex],
                  static_cast<uint32_t>(definition.m_ElementTypeHash));

#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

    switch (definition.m_ElementTypeHash) {
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
            LOG_TRACE("Primivite {:x} size is {}", static_cast<uint32_t>(definition.m_TypeHash),
                      elementType->m_Definition.m_Size);

            info->m_DataType = definition.m_ElementTypeHash;
            info->m_Data.resize((elementType->m_Definition.m_Size * info->m_ExpectedElementCount));
            std::memcpy(info->m_Data.data(), &m_Data[info->m_Offset], info->m_Data.size());
            break;
        }

        case TypeMemberHash::Deferred: {
            LOG_WARN("Element type is a deferred pointer-reference.");
#ifdef DEBUG
            __debugbreak();
#endif
            break;
        }

        default: {
            LOG_TRACE("Need to setup non-primivite {:x}. (m_ExpectedElementCount={}, m_ElementTypeHash={:x}, Size={})",
                      static_cast<uint32_t>(definition.m_TypeHash), info->m_ExpectedElementCount,
                      static_cast<uint32_t>(definition.m_ElementTypeHash), elementType->m_Definition.m_Size);

            for (uint32_t i = 0; i < info->m_ExpectedElementCount; ++i) {
                const auto name = info->m_Name + "[" + std::to_string(i) + "]";
                MemberSetupStructMember(info, definition.m_ElementTypeHash, name,
                                        (info->m_Offset + (elementType->m_Definition.m_Size * i)));
            }

            break;
        }
    }
}

void AdfInstanceInfo::MemberSetup_StringHash(AdfInstanceMemberInfo* info)
{
    info->m_Adf->m_DataOffset = info->m_Offset;
    const auto  hash          = info->m_Adf->ReadData<uint32_t>();
    const auto& string_hashes = info->m_Adf->m_Parent->m_StringHashes;

    auto it = std::find_if(string_hashes.begin(), string_hashes.end(),
                           [&](const std::pair<uint32_t, std::string>& value) { return value.first == hash; });

    if (it != string_hashes.end()) {
        info->m_StringData = (*it).second;
        LOG_TRACE("StringHash value: \"{}\" ({:x})", info->m_StringData, hash);
    }
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset,
                                             AdfInstanceInfo* adf, uint32_t expected_element_count)
{
    m_Name                 = name;
    m_Type                 = type->m_Definition.m_Type;
    m_TypeDef              = type;
    m_Offset               = offset;
    m_FileOffset           = offset + adf->m_Offset;
    m_Adf                  = adf;
    m_ExpectedElementCount = expected_element_count;

    adf->MemberSetup(this);
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset,
                                             AdfInstanceInfo* adf)
{
    m_Name       = name;
    m_Type       = jc::AvalancheDataFormat::TypeDefinitionType::Deferred;
    m_TypeDef    = type;
    m_DataType   = jc::AvalancheDataFormat::TypeMemberHash::Deferred;
    m_Offset     = offset;
    m_FileOffset = offset + adf->m_Offset;
    m_Adf        = adf;

    adf->m_DataOffset = offset;
    const auto value  = adf->ReadData<uint32_t>();

    LOG_INFO("Deferred pointer-reference read: {}", value);
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, jc::AvalancheDataFormat::TypeMemberHash type,
                                             int64_t offset, AdfInstanceInfo* adf)
{
    m_Name       = name;
    m_Type       = jc::AvalancheDataFormat::TypeDefinitionType::Primitive;
    m_DataType   = type;
    m_Offset     = offset;
    m_FileOffset = offset + adf->m_Offset;
    m_Adf        = adf;

    //
    switch (type) {
        case jc::AvalancheDataFormat::TypeMemberHash::Int8:
        case jc::AvalancheDataFormat::TypeMemberHash::UInt8: {
            LOG_TRACE("read int8/uint8");

            adf->m_DataOffset = offset;
            m_Data.push_back(adf->ReadData<uint8_t>());
            break;
        }

        case jc::AvalancheDataFormat::TypeMemberHash::Int16:
        case jc::AvalancheDataFormat::TypeMemberHash::UInt16: {
            LOG_TRACE("read int16/uint16");

            adf->m_DataOffset = offset;
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            break;
        }

        case jc::AvalancheDataFormat::TypeMemberHash::Int32:
        case jc::AvalancheDataFormat::TypeMemberHash::UInt32:
        case jc::AvalancheDataFormat::TypeMemberHash::Float: {
            LOG_TRACE("read int32/uint32/float");

            adf->m_DataOffset = offset;
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            break;
        }

        case jc::AvalancheDataFormat::TypeMemberHash::Int64:
        case jc::AvalancheDataFormat::TypeMemberHash::UInt64:
        case jc::AvalancheDataFormat::TypeMemberHash::Double: {
            LOG_TRACE("read int64/uint64/double");

            adf->m_DataOffset = offset;
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            m_Data.push_back(adf->ReadData<uint8_t>());
            break;
        }

        case jc::AvalancheDataFormat::TypeMemberHash::String: {
            adf->m_DataOffset = offset;
            m_StringData      = adf->ReadString();

            LOG_TRACE("Read String: \"{}\"", m_StringData);
            break;
        }
    }
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset,
                                             AdfInstanceInfo* adf, jc::AvalancheDataFormat::Enum* enum_data)
{
    m_Name       = name;
    m_Type       = type->m_Definition.m_Type;
    m_TypeDef    = type;
    m_Offset     = offset;
    m_FileOffset = offset + adf->m_Offset;
    m_Adf        = adf;

    // init enum member data
    {
        auto member = std::make_unique<AdfInstanceMemberInfo>();

        member->m_Name     = adf->m_Parent->m_Names[enum_data->m_NameIndex];
        member->m_Type     = jc::AvalancheDataFormat::TypeDefinitionType::Primitive;
        member->m_DataType = jc::AvalancheDataFormat::TypeMemberHash::Int32;
        member->m_Adf      = adf;

        member->m_Data.resize(sizeof(enum_data->m_Value));
        std::memcpy(member->m_Data.data(), &enum_data->m_Value, sizeof(enum_data->m_Value));

        m_Members.emplace_back(std::move(member));
    }
}
