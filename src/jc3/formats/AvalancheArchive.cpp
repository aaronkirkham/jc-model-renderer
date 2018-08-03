#include <jc3/formats/AvalancheArchive.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

std::recursive_mutex Factory<AvalancheArchive>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheArchive>> Factory<AvalancheArchive>::Instances;

AvalancheArchive::AvalancheArchive(const fs::path& file)
    : m_File(file)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(file);
    assert(m_StreamArchive);

    // parse the file list
    m_FileList = std::make_unique<DirectoryList>();
    m_FileList->Parse(m_StreamArchive.get());
}

AvalancheArchive::AvalancheArchive(const fs::path& filename, const FileBuffer& buffer)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);
    assert(m_StreamArchive);
    m_StreamArchive->m_Filename = filename;

    // parse the file list
    m_FileList = std::make_unique<DirectoryList>();
    m_FileList->Parse(m_StreamArchive.get());
}

void AvalancheArchive::AddFile(const fs::path& filename, const FileBuffer& data)
{
    assert(m_StreamArchive);
    assert(m_FileList);

    m_StreamArchive->AddFile(filename.string(), data);
    m_FileList->Add(filename.string());
}

bool AvalancheArchive::HasFile(const fs::path& filename)
{
    auto find_it = std::find_if(m_StreamArchive->m_Files.begin(), m_StreamArchive->m_Files.end(), [&](const StreamArchiveEntry_t& entry) {
        return entry.m_Filename == filename || entry.m_Filename.find(filename.string()) != std::string::npos;
    });

    return find_it != m_StreamArchive->m_Files.end();
}

void AvalancheArchive::FileReadCallback(const fs::path& filename, const FileBuffer& data)
{
    AvalancheArchive::make(filename, data);
}