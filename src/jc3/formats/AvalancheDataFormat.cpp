#include <jc3/formats/AvalancheDataFormat.h>
#include <jc3/FileLoader.h>
#include <Window.h>

AvalancheDataFormat::AvalancheDataFormat(const fs::path& file)
    : m_File(file)
{
}

AvalancheDataFormat::AvalancheDataFormat(const fs::path& filename, const FileBuffer& buffer)
    : m_File(filename)
{
}

AvalancheDataFormat::~AvalancheDataFormat()
{
    DEBUG_LOG("AvalancheDataFormat::~AvalancheDataFormat");
}

bool AvalancheDataFormat::Parse(const FileBuffer& data)
{
    std::istringstream stream(std::string{ (char*)data.data(), data.size() });

    JustCause3::AvalancheDataFormat::Header header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct "ADF"
    if (header.m_Magic != 0x41444620) {
        DEBUG_LOG("AvalancheDataFormat::Parse - Invalid header magic. Input probably isn't an AvalancheDataFormat file.");
        return false;
    }

    DEBUG_LOG(" - m_Version=" << header.m_Version);

    // make sure it's the correct version
    if (header.m_Version != 4) {
        DEBUG_LOG("AvalancheDataFormat::Parse - Unexpected AvalancheDataFormat file version!");
        return false;
    }

    DEBUG_LOG(" - m_InstanceCount=" << header.m_InstanceCount);
    DEBUG_LOG(" - m_InstanceOffset=" << header.m_InstanceOffset);
    DEBUG_LOG(" - m_TypeCount=" << header.m_TypeCount);
    DEBUG_LOG(" - m_TypeOffset=" << header.m_TypeOffset);
    DEBUG_LOG(" - m_StringHashCount=" << header.m_StringHashCount);
    DEBUG_LOG(" - m_StringHashOffset=" << header.m_StringHashOffset);
    DEBUG_LOG(" - m_StringCount=" << header.m_StringCount);
    DEBUG_LOG(" - m_StringOffset=" << header.m_StringOffset);

    // 
    std::vector<uint8_t> name_lengths;
    name_lengths.resize(header.m_StringCount);

    // read names
    stream.seekg(header.m_StringOffset);
    stream.read((char *)&name_lengths.front(), sizeof(uint8_t) * header.m_StringCount);
    for (uint32_t i = 0; i < header.m_StringCount; ++i) {
        auto name = std::unique_ptr<char[]>(new char[name_lengths[i] + 1]);
        stream.read(name.get(), name_lengths[i] + 1);

        DEBUG_LOG("string: " << name.get());
        m_Names.emplace_back(name.get());
    }

    // read type definitions
    stream.seekg(header.m_TypeOffset);
    for (uint32_t i = 0; i < header.m_TypeCount; ++i) {
        JustCause3::AvalancheDataFormat::TypeDefinition definition;
        stream.read((char *)&definition, sizeof(definition));

        DEBUG_LOG("definition type: " << JustCause3::AvalancheDataFormat::TypeDefintionStr(definition.m_Type));

        m_Definitions.emplace_back(definition);
    }

    // read instances
    stream.seekg(header.m_InstanceOffset);
    for (uint32_t i = 0; i < header.m_InstanceCount; ++i) {
        JustCause3::AvalancheDataFormat::InstanceInfo instance;
        stream.read((char *)&instance, sizeof(instance));

        auto type = GetTypeDefinition(instance.m_TypeHash);

        DEBUG_LOG("instance name: " << m_Names[instance.m_NameIndex] << ", size=" << instance.m_Size << ", type=" << JustCause3::AvalancheDataFormat::TypeDefintionStr(type));

        m_InstanceInfos.emplace_back(instance);
    }

    // read instance members
    const auto pos = stream.tellg();
    for (const auto& instance : m_InstanceInfos) {
        std::vector<uint8_t> data;
        data.resize(instance.m_Size);
        stream.seekg(instance.m_Offset);
        stream.read((char *)data.data(), instance.m_Size);

        // member data
        {

        }
    }

    stream.seekg(pos);

    return true;
}

void AvalancheDataFormat::FileReadCallback(const fs::path& filename, const FileBuffer& data)
{
}

JustCause3::AvalancheDataFormat::TypeDefinitionType AvalancheDataFormat::GetTypeDefinition(uint32_t type_hash)
{
    auto find_it = std::find_if(m_Definitions.begin(), m_Definitions.end(), [&](const JustCause3::AvalancheDataFormat::TypeDefinition& item) {
        return item.m_NameHash == type_hash;
    });

    return (*find_it).m_Type;
}