#pragma once

#include <StdInc.h>
#include <jc3/Types.h>

class AdfTypeDefinition
{
private:
    JustCause3::AvalancheDataFormat::TypeDefinition m_Definition;

public:
    AdfTypeDefinition() = default;
    virtual ~AdfTypeDefinition() = default;
};

class AvalancheDataFormat
{
private:
    fs::path m_File;

    // ADF data
    std::vector<std::string> m_Names;
    std::vector<JustCause3::AvalancheDataFormat::TypeDefinition> m_Definitions;
    std::vector<JustCause3::AvalancheDataFormat::InstanceInfo> m_InstanceInfos;

    // hash to type definition
    JustCause3::AvalancheDataFormat::TypeDefinitionType GetTypeDefinition(uint32_t type_hash);

public:
    AvalancheDataFormat(const fs::path& file);
    AvalancheDataFormat(const fs::path& filename, const FileBuffer& buffer);
    virtual ~AvalancheDataFormat();

    bool Parse(const FileBuffer& data);

    static void FileReadCallback(const fs::path& filename, const FileBuffer& data);
};