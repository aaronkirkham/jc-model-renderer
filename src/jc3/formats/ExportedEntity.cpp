#include <jc3/formats/ExportedEntity.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

ExportedEntity::ExportedEntity(const fs::path& file)
    : m_File(file)
{
    // make sure this is an ee file
    if (file.extension() != ".ee") {
        throw std::invalid_argument("ExportedEntity type only supports .ee files!");
    }

    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(file);
    FileLoader::Get()->CacheLoadedArchive(m_StreamArchive);

    m_StreamArchive->m_Filename = file;
}

ExportedEntity::ExportedEntity(const fs::path& filename, const std::vector<uint8_t>& buffer)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);
    FileLoader::Get()->CacheLoadedArchive(m_StreamArchive);

    m_StreamArchive->m_Filename = filename;
}

ExportedEntity::~ExportedEntity()
{
    if (m_StreamArchive) {
        FileLoader::Get()->RemoveLoadedArchive(m_StreamArchive);
        delete m_StreamArchive;
    }
}