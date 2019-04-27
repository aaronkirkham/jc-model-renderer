#pragma once

#include <any>
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
    int64_t                                             m_Id   = -1;
    std::string                                         m_Name = "";
    jc::AvalancheDataFormat::TypeDefinitionType         m_Type;
    AdfTypeDefinition*                                  m_TypeDef = nullptr;
    jc::AvalancheDataFormat::TypeMemberHash             m_TypeHash;
    std::any                                            m_Data;
    std::vector<std::unique_ptr<AdfInstanceMemberInfo>> m_Members;
    uint32_t                                            m_ExpectedElementCount = 0;
    int64_t                                             m_Offset               = 0;
    int64_t                                             m_FileOffset           = 0;
    AdfInstanceInfo*                                    m_Adf                  = nullptr;
    bool                                                m_IsReferenceToId      = false;
    uint32_t                                            m_BitIndex             = 0;

    AdfInstanceMemberInfo() = delete;
    AdfInstanceMemberInfo(int64_t id, const std::string& name, AdfInstanceInfo* adf)
        : m_Id(id)
        , m_Name(name)
        , m_Adf(adf)
    {
    }
    virtual ~AdfInstanceMemberInfo() = default;

    void InitReference(AdfTypeDefinition* type, jc::AvalancheDataFormat::TypeMemberHash typehash);
    void InitNonPrimitive(AdfTypeDefinition* type, jc::AvalancheDataFormat::TypeMemberHash typehash, int64_t offset,
                          uint32_t element_count);
    void InitPrimitive(jc::AvalancheDataFormat::TypeMemberHash typehash, int64_t offset);
    void InitPointer(AdfTypeDefinition* type, int64_t offset);
    void InitEnum(AdfTypeDefinition* type, int64_t offset);
    void InitBitField(AdfTypeDefinition* type, int64_t offset, uint32_t bit_index);

    template <typename T> T As()
    {
        return std::any_cast<T>(m_Data);
    }

    AdfInstanceMemberInfo* GetMember(const std::string& name);
};

class AdfInstanceInfo
{
  private:
    std::queue<AdfInstanceMemberInfo*> m_Queue;

    void MemberSetup_Structure(AdfInstanceMemberInfo* info);
    void MemberSetup_Array(AdfInstanceMemberInfo* info);
    void MemberSetup_StringHash(AdfInstanceMemberInfo* info);

    void MemberSetupStructMember(AdfInstanceMemberInfo*                      info,
                                 jc::AvalancheDataFormat::MemeberDefinition* type_definition,
                                 jc::AvalancheDataFormat::TypeMemberHash type_hash, const std::string& name,
                                 int64_t offset);

  public:
    std::string                                         m_Name   = "";
    AdfTypeDefinition*                                  m_Type   = nullptr;
    uint32_t                                            m_Offset = 0;
    jc::AvalancheDataFormat::TypeMemberHash             m_TypeHash;
    std::vector<std::unique_ptr<AdfInstanceMemberInfo>> m_Members;
    std::unordered_map<int64_t, AdfInstanceMemberInfo*> m_MemberOffsets;
    AvalancheDataFormat*                                m_Parent = nullptr;
    FileBuffer                                          m_Data;
    int64_t                                             m_DataReadOffset = 0;

    AdfInstanceInfo()          = default;
    virtual ~AdfInstanceInfo() = default;

    template <typename T> T ReadData()
    {
        assert((m_DataReadOffset + sizeof(T)) <= m_Data.size());

        T result;
        std::memcpy(&result, &m_Data[m_DataReadOffset], sizeof(T));
        m_DataReadOffset += sizeof(T);
        return result;
    }

    std::string ReadString()
    {
        char* next = reinterpret_cast<char*>(&m_Data[m_DataReadOffset]);
        m_DataReadOffset += strlen(next);
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

    // type libraries
    std::vector<std::shared_ptr<AvalancheDataFormat>> m_TypeLibraries;

    void AddBuiltInTypes();
    void AddBuiltInType(jc::AvalancheDataFormat::TypeDefinitionType type,
                        jc::AvalancheDataFormat::PrimitiveType primitive_type, uint32_t size, uint32_t type_index,
                        uint16_t flags);

  public:
    AvalancheDataFormat(const std::filesystem::path& file, bool add_built_in_types = true);
    virtual ~AvalancheDataFormat() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_File.string();
    }

    bool Parse(const FileBuffer& data);
    void LoadTypeLibraries();

    static void ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);

    AdfTypeDefinition* GetTypeDefinition(jc::AvalancheDataFormat::TypeMemberHash type_hash);

    AdfInstanceMemberInfo* GetMember(const std::string& name);

    const std::vector<std::unique_ptr<AdfInstanceInfo>>& GetInstanceInfos()
    {
        return m_InstanceInfos;
    }
};
