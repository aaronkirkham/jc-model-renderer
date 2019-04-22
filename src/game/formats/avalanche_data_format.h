#pragma once

#include <filesystem>
#include <memory>
#include <queue>
#include <unordered_map>

#include "../../factory.h"
#include "../../graphics/types.h"
#include "../types.h"

class AdfInstanceInfo;
class AdfTypeDefinition;
class AvalancheDataFormat;

static constexpr const char* ADF_TYPE_NAMES[12] = {"uint8",  "int8",  "uint16", "int16",  "uint32", "int32",
                                                   "uint64", "int64", "float",  "double", "String", "void"};

class AdfInstanceMemberInfo
{
  public:
    std::string                                         m_Name = "";
    jc::AvalancheDataFormat::TypeDefinitionType         m_Type;
    AdfTypeDefinition*                                  m_TypeDef = nullptr;
    jc::AvalancheDataFormat::TypeMemberHash             m_DataType;
    FileBuffer                                          m_Data;
    std::string                                         m_StringData;
    std::vector<std::unique_ptr<AdfInstanceMemberInfo>> m_Members;
    uint32_t                                            m_ExpectedElementCount = 0;
    int64_t                                             m_Offset               = 0;
    int64_t                                             m_FileOffset           = 0;
    AdfInstanceInfo*                                    m_Adf                  = nullptr;

    AdfInstanceMemberInfo() = default;
    AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset, AdfInstanceInfo* adf,
                          uint32_t expected_element_count);
    AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset, AdfInstanceInfo* adf);
    AdfInstanceMemberInfo(const std::string& name, jc::AvalancheDataFormat::TypeMemberHash type, int64_t offset,
                          AdfInstanceInfo* adf);
    AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t offset, AdfInstanceInfo* adf,
                          jc::AvalancheDataFormat::Enum* enum_data);
    virtual ~AdfInstanceMemberInfo() = default;

    template <typename T> T GetData()
    {
        return CastBuffer<T>(&m_Data)[0];
    }
};

class AdfInstanceInfo
{
  private:
    std::queue<AdfInstanceMemberInfo*> m_Queue;

    void MemberSetup_Structure(AdfInstanceMemberInfo* info);
    void MemberSetup_Array(AdfInstanceMemberInfo* info);
    void MemberSetup_StringHash(AdfInstanceMemberInfo* info);

    void MemberSetupStructMember(AdfInstanceMemberInfo* info, jc::AvalancheDataFormat::TypeMemberHash type_hash,
                                 const std::string& name, int64_t offset);

  public:
    std::string                                         m_Name   = "";
    AdfTypeDefinition*                                  m_Type   = nullptr;
    uint32_t                                            m_Offset = 0;
    jc::AvalancheDataFormat::TypeMemberHash             m_TypeHash;
    std::vector<std::unique_ptr<AdfInstanceMemberInfo>> m_Members;
    std::unordered_map<int64_t, AdfInstanceMemberInfo*> m_MemberOffsets;
    AvalancheDataFormat*                                m_Parent = nullptr;

    FileBuffer m_Data;
    uint64_t   m_DataOffset = 0;

    AdfInstanceInfo()          = default;
    virtual ~AdfInstanceInfo() = default;

    template <typename T> T ReadData()
    {
        T result;
        std::memcpy(&result, &m_Data[m_DataOffset], sizeof(T));
        m_DataOffset += sizeof(T);
        return result;
    }

    std::string ReadString()
    {
        char* next = reinterpret_cast<char*>(&m_Data[m_DataOffset]);
        m_DataOffset += strlen(next);
        return std::string(next);
    }

    void ReadMembers();
    void MemberSetup(AdfInstanceMemberInfo* info);
};

class AdfTypeDefinition
{
  public:
    AvalancheDataFormat*                                    m_Parent = nullptr;
    std::string                                             m_Name   = "";
    jc::AvalancheDataFormat::TypeDefinition                 m_Definition;
    std::vector<jc::AvalancheDataFormat::MemeberDefinition> m_Members;
    std::vector<jc::AvalancheDataFormat::Enum>              m_Enums;

    AdfTypeDefinition()          = default;
    virtual ~AdfTypeDefinition() = default;
};

class AvalancheDataFormat : public Factory<AvalancheDataFormat>
{
    friend class AdfInstanceInfo;
    friend class AdfInstanceMemberInfo;

  private:
    std::filesystem::path m_File;

    // ADF data
    std::vector<std::string>                        m_Names;
    std::vector<std::pair<uint32_t, std::string>>   m_StringHashes;
    std::vector<std::unique_ptr<AdfTypeDefinition>> m_TypeDefinitions;
    std::vector<std::unique_ptr<AdfInstanceInfo>>   m_InstanceInfos;

    void AddBuiltInTypes();
    void AddBuiltInType(jc::AvalancheDataFormat::TypeDefinitionType type,
                        jc::AvalancheDataFormat::PrimitiveType primitive_type, uint32_t size, uint32_t type_index,
                        uint16_t flags);

  public:
    AvalancheDataFormat(const std::filesystem::path& file);
    AvalancheDataFormat(const std::filesystem::path& filename, const FileBuffer& buffer);
    virtual ~AvalancheDataFormat() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_File.string();
    }

    bool Parse(const FileBuffer& data);

    static void FileReadCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);

    AdfTypeDefinition* GetTypeDefinition(jc::AvalancheDataFormat::TypeMemberHash type_hash);

    AdfInstanceMemberInfo* GetMember(const std::string& name);
    AdfInstanceMemberInfo* GetMember(AdfInstanceMemberInfo* info, const std::string& name);
};
