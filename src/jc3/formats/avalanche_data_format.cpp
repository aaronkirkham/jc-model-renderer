#include "avalanche_data_format.h"
#include "../../window.h"

AvalancheDataFormat::AvalancheDataFormat(const std::filesystem::path& file)
    : m_File(file)
{
}

AvalancheDataFormat::AvalancheDataFormat(const std::filesystem::path& filename, const FileBuffer& buffer)
    : m_File(filename)
{
}

bool AvalancheDataFormat::Parse(const FileBuffer& data)
{
    std::istringstream stream(std::string{(char*)data.data(), data.size()});

    JustCause3::AvalancheDataFormat::Header header;
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

    //
    std::vector<uint8_t> name_lengths;
    name_lengths.resize(header.m_StringCount);

    // read names
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
        JustCause3::AvalancheDataFormat::TypeDefinition definition;
        stream.read((char*)&definition, sizeof(definition));

        auto typeDef          = std::make_unique<AdfTypeDefinition>();
        typeDef->m_Parent     = this;
        typeDef->m_Name       = m_Names[definition.m_NameIndex];
        typeDef->m_Definition = definition;

        LOG_INFO("Definition: \"{}\", type: {}", typeDef->m_Name,
                 JustCause3::AvalancheDataFormat::TypeDefintionStr(definition.m_Type));

        switch (definition.m_Type) {
            case JustCause3::AvalancheDataFormat::TypeDefinitionType::Structure: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));

                LOG_INFO("Reading {} type definition structures...", count);

                // read members
                typeDef->m_Members.resize(count);
                stream.read((char*)&typeDef->m_Members.front(),
                            sizeof(JustCause3::AvalancheDataFormat::MemeberDefinition) * count);

                for (const auto& m : typeDef->m_Members) {
                    LOG_INFO("Read member \"{}\", TypeHash: {}", m_Names[m.m_NameIndex], (uint32_t)m.m_TypeHash);
                }

                break;
            }

            case JustCause3::AvalancheDataFormat::TypeDefinitionType::Array:
            case JustCause3::AvalancheDataFormat::TypeDefinitionType::InlineArray:
            case JustCause3::AvalancheDataFormat::TypeDefinitionType::Pointer:
            case JustCause3::AvalancheDataFormat::TypeDefinitionType::StringHash: {
                uint32_t count;
                stream.read((char*)&count, sizeof(count));
                LOG_INFO("Type Definition is Array/InlineArray/Pointer/StringHash (count={})", count);
                break;
            }
        }

        m_TypeDefinitions.emplace_back(std::move(typeDef));
    }

    // read instances
    stream.seekg(header.m_InstanceOffset);
    for (uint32_t i = 0; i < header.m_InstanceCount; ++i) {
        JustCause3::AvalancheDataFormat::InstanceInfo instance;
        stream.read((char*)&instance, sizeof(instance));

        auto instanceInfo        = std::make_unique<AdfInstanceInfo>();
        instanceInfo->m_Name     = m_Names[instance.m_NameIndex];
        instanceInfo->m_Type     = GetTypeDefinition(instance.m_TypeHash);
        instanceInfo->m_TypeHash = instance.m_TypeHash;

        // read the instance data
        instanceInfo->m_Data.resize(instance.m_Size);
        stream.seekg(instance.m_Offset);
        stream.read((char*)instanceInfo->m_Data.data(), instance.m_Size);
        // instanceInfo->m_DataOffset = instance.m_Offset;

        LOG_INFO("Instance Name=\"{}\", Size={}, Type={}", m_Names[instance.m_NameIndex], instance.m_Size,
                 JustCause3::AvalancheDataFormat::TypeDefintionStr(instanceInfo->m_Type->m_Definition.m_Type));

        instanceInfo->ReadMembers();
        m_InstanceInfos.emplace_back(std::move(instanceInfo));
    }

    LOG_INFO("finished..");

    return true;
}

void AvalancheDataFormat::FileReadCallback(const std::filesystem::path& filename, const FileBuffer& data) {}

AdfTypeDefinition* AvalancheDataFormat::GetTypeDefinition(uint32_t type_hash)
{
    auto it = std::find_if(m_TypeDefinitions.begin(), m_TypeDefinitions.end(),
                           [&](const std::unique_ptr<AdfTypeDefinition>& definition) {
                               return definition->m_Definition.m_NameHash == type_hash;
                           });

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
    m_Members.emplace_back(std::make_unique<AdfInstanceMemberInfo>(m_Name, m_Type, 0, this));

    while (m_Queue.size() > 0) {
        AdfInstanceMemberInfo* info = m_Queue.front();
        LOG_TRACE("ReadMemebrs at {}", info->m_Name);

        switch (info->m_TypeDef->m_Definition.m_Type) {
            case JustCause3::AvalancheDataFormat::TypeDefinitionType::Structure: {
                MemberSetup_Structure(info);
                break;
            }

            case JustCause3::AvalancheDataFormat::TypeDefinitionType::Array:
            case JustCause3::AvalancheDataFormat::TypeDefinitionType::InlineArray: {
                MemberSetup_Array(info);
                break;
            }

            case JustCause3::AvalancheDataFormat::TypeDefinitionType::StringHash: {
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
    const auto type_hash = info->m_TypeDef->m_Definition.m_ElementTypeHash;

    for (const auto& member : info->m_TypeDef->m_Members) {
        const auto& name = info->m_TypeDef->m_Parent->GetName(member.m_NameIndex);
        MemberSetupStructMember(info, member.m_TypeHash, name, info->m_Offset + member.m_Offset);
    }
}

void AdfInstanceInfo::MemberSetupStructMember(AdfInstanceMemberInfo*                          info,
                                              JustCause3::AvalancheDataFormat::TypeMemberHash type_hash,
                                              const std::string& name, int64_t offset)
{
    info->m_Adf->m_DataOffset = offset;

    switch (type_hash) {
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int8:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt8:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int16:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt16:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int32:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt32:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int64:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt64:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Float:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Double: {
            LOG_TRACE("{} is a primivite", name);
            break;
        }

        case JustCause3::AvalancheDataFormat::TypeMemberHash::String: {
            LOG_TRACE("{} is a string (at {})", name, offset);

            auto dest_offset = info->m_Adf->ReadData<int64_t>();

            LOG_TRACE("dest_offset={}", dest_offset);

            auto new_info =
                std::make_unique<AdfInstanceMemberInfo>(name, static_cast<uint32_t>(type_hash), dest_offset, this);
            info->m_Members.emplace_back(std::move(new_info));
            break;
        }

        default: {
            const auto& definition = info->m_TypeDef->m_Parent->GetTypeDefinition(static_cast<uint32_t>(type_hash));

            switch (definition->m_Definition.m_Type) {
                case JustCause3::AvalancheDataFormat::TypeDefinitionType::InlineArray:
                case JustCause3::AvalancheDataFormat::TypeDefinitionType::Structure:
                case JustCause3::AvalancheDataFormat::TypeDefinitionType::StringHash: {
                    LOG_TRACE("{} is a structure", name);

                    auto new_info = std::make_unique<AdfInstanceMemberInfo>(
                        name, definition, info->m_Adf->m_DataOffset, this, definition->m_Definition.m_ElementLength);
                    info->m_Members.emplace_back(std::move(new_info));
                    break;
                }

                case JustCause3::AvalancheDataFormat::TypeDefinitionType::Array: {
                    LOG_TRACE("{} is an array", name);

                    auto dest_offset   = info->m_Adf->ReadData<int64_t>();
                    auto element_count = info->m_Adf->ReadData<int64_t>();

                    LOG_TRACE(" - DestOffset={}, ElementCount={}", dest_offset, element_count);

                    if (dest_offset == 0 && element_count == 0) {
                        __debugbreak();
                    }

                    if (m_MemberOffsets.find(dest_offset) == m_MemberOffsets.end()) {
                        auto new_info =
                            std::make_unique<AdfInstanceMemberInfo>(name, definition, dest_offset, this, element_count);
                        info->m_Members.emplace_back(std::move(new_info));
                    }

                    break;
                }

                case JustCause3::AvalancheDataFormat::TypeDefinitionType::Pointer: {
                    LOG_TRACE("{} is a pointer", name);
                    break;
                }

                default: {
                    __debugbreak();
                    break;
                }
            }

            break;
        }
    }
}

void AdfInstanceInfo::MemberSetup_Array(AdfInstanceMemberInfo* info)
{
    int32_t element_size = 0;
    bool    is_primitive = true;

    switch ((JustCause3::AvalancheDataFormat::TypeMemberHash)info->m_TypeDef->m_Definition.m_ElementTypeHash) {
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int8:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt8: {
            element_size = 1;
            break;
        }

        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int16:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt16: {
            element_size = 2;
            break;
        }

        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int32:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt32:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Float: {
            element_size = 4;
            break;
        }

        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int64:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt64:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Double: {
            element_size = 8;
            break;
        }

        case JustCause3::AvalancheDataFormat::TypeMemberHash::String: {
            is_primitive = false;
            element_size = 8;
            break;
        }

        default: {
            is_primitive = false;
            element_size = info->m_TypeDef->m_Parent->GetTypeDefinition(info->m_TypeDef->m_Definition.m_ElementTypeHash)
                               ->m_Definition.m_Size;
            break;
        }
    }

    if (is_primitive) {
        LOG_INFO("Need to setup primitive. (ElementCount: {})", info->m_ExpectedElementCount);

        info->m_Members.emplace_back(std::make_unique<AdfInstanceMemberInfo>(
            "", info->m_TypeDef->m_Definition.m_ElementTypeHash, info->m_Offset, this, info->m_ExpectedElementCount));
    } else {
        LOG_INFO("Need to setup non-primivite. (ElementCount: {})", info->m_ExpectedElementCount);

        for (uint32_t i = 0; i < info->m_ExpectedElementCount; ++i) {
            const auto element_type_hash = info->m_TypeDef->m_Definition.m_ElementTypeHash;
            MemberSetupStructMember(info,
                                    static_cast<JustCause3::AvalancheDataFormat::TypeMemberHash>(element_type_hash), "",
                                    (info->m_Offset + element_size * i));
        }
    }
}

void AdfInstanceInfo::MemberSetup_StringHash(AdfInstanceMemberInfo* info)
{
    const auto value = info->m_Adf->ReadData<uint32_t>();
    LOG_INFO("StringHash value: {}", value);
    __debugbreak();
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset,
                                             AdfInstanceInfo* adf, uint32_t expected_element_count)
{
    m_Name                 = name;
    m_Type                 = type->m_Definition.m_Type;
    m_TypeDef              = type;
    m_Offset               = offset;
    m_Adf                  = adf;
    m_ExpectedElementCount = expected_element_count;

    LOG_INFO("m_Name=\"{}\", m_Offset={}", name, offset);

    adf->MemberSetup(this);
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, uint32_t type_hash, int64_t offset,
                                             AdfInstanceInfo* adf)
{
    m_Name     = name;
    m_Type     = JustCause3::AvalancheDataFormat::TypeDefinitionType::Primitive;
    m_TypeHash = type_hash;
    m_Offset   = offset;
    m_Adf      = adf;

    //
    switch (static_cast<JustCause3::AvalancheDataFormat::TypeMemberHash>(type_hash)) {
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int8:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt8: {
            LOG_INFO("read byte..");

            adf->m_DataOffset = offset;
            m_Data.push_back(adf->ReadData<uint8_t>());
            break;
        }

        case JustCause3::AvalancheDataFormat::TypeMemberHash::String: {
            adf->m_DataOffset = offset;
            m_StringData      = adf->ReadString();

            LOG_INFO("Read String: \"{}\"", m_StringData);
            break;
        }
    }
}

AdfInstanceMemberInfo::AdfInstanceMemberInfo(const std::string& name, uint32_t type_hash, int64_t offset,
                                             AdfInstanceInfo* adf, uint32_t element_count)
{
    m_Name     = name;
    m_Type     = JustCause3::AvalancheDataFormat::TypeDefinitionType::Primitive;
    m_TypeHash = type_hash;
    m_Offset   = offset;
    m_Adf      = adf;

    switch (static_cast<JustCause3::AvalancheDataFormat::TypeMemberHash>(type_hash)) {
        case JustCause3::AvalancheDataFormat::TypeMemberHash::Int8:
        case JustCause3::AvalancheDataFormat::TypeMemberHash::UInt8: {
            LOG_INFO("read {} bytes..", element_count);

            m_Data.resize(element_count);
            std::memcpy(m_Data.data(), &adf->m_Data[offset], element_count);

            break;
        }
    }
}
