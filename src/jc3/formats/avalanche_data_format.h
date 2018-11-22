#pragma once

#include <filesystem>
#include <memory>
#include <queue>
#include <unordered_map>

#include "../../graphics/types.h"
#include "../types.h"

class AdfInstanceInfo;
class AdfTypeDefinition;
class AvalancheDataFormat;

class AdfInstanceMemberInfo
{
  public:
    std::string                                         m_Name;
    AdfTypeDefinition*                                  m_Type                 = nullptr;
    int64_t                                             m_Pos                  = 0;
    AdfInstanceInfo*                                    m_Adf                  = nullptr;
    uint32_t                                            m_ExpectedElementCount = 0;
    std::vector<std::unique_ptr<AdfInstanceMemberInfo>> m_Members;

    AdfInstanceMemberInfo(const std::string& name, AdfTypeDefinition* type, int64_t position, AdfInstanceInfo* adf,
                          uint32_t expected_element_count = 0);
    AdfInstanceMemberInfo(const std::string& name, uint32_t type_hash, int64_t position, AdfInstanceInfo* adf);
    virtual ~AdfInstanceMemberInfo() = default;
};

class AdfInstanceInfo
{
  private:
    std::queue<AdfInstanceMemberInfo*> m_Queue;

  public:
    std::string                                         m_Name     = "";
    AdfTypeDefinition*                                  m_Type     = nullptr;
    uint32_t                                            m_TypeHash = 0;
    std::vector<std::unique_ptr<AdfInstanceMemberInfo>> m_Members;
    std::unordered_map<int64_t, AdfInstanceMemberInfo*> m_MemberOffsets;

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

    void ReadMembers();
    void MemberSetup(AdfInstanceMemberInfo* info);
    void MemberSetup_Structure(AdfInstanceMemberInfo* info);
    void MemberSetup_Array(AdfInstanceMemberInfo* info);

    void MemberSetupStructMember(AdfInstanceMemberInfo* info, JustCause3::AvalancheDataFormat::TypeMemberHash type_hash,
                                 const std::string& name, int64_t offset);
};

class AdfTypeDefinition
{
  public:
    AvalancheDataFormat*                                            m_Parent = nullptr;
    std::string                                                     m_Name   = "";
    JustCause3::AvalancheDataFormat::TypeDefinition                 m_Definition;
    std::vector<JustCause3::AvalancheDataFormat::MemeberDefinition> m_Members;

    AdfTypeDefinition()          = default;
    virtual ~AdfTypeDefinition() = default;
};

class AvalancheDataFormat
{
  private:
    std::filesystem::path m_File;

    // ADF data
    std::vector<std::string>                        m_Names;
    std::vector<std::unique_ptr<AdfTypeDefinition>> m_TypeDefinitions;
    std::vector<std::unique_ptr<AdfInstanceInfo>>   m_InstanceInfos;

  public:
    AvalancheDataFormat(const std::filesystem::path& file);
    AvalancheDataFormat(const std::filesystem::path& filename, const FileBuffer& buffer);
    virtual ~AvalancheDataFormat();

    bool Parse(const FileBuffer& data);

    static void FileReadCallback(const std::filesystem::path& filename, const FileBuffer& data);

    AdfTypeDefinition* GetTypeDefinition(uint32_t type_hash);

    const std::string& GetName(int32_t index)
    {
        return m_Names[index];
    }
};
