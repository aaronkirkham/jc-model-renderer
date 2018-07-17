#include <jc3/formats/AvalancheArchive.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/ModelManager.h>
#include <Window.h>
#include <graphics/UI.h>

AvalancheArchive* g_CurrentLoadedArchive = nullptr;

AvalancheArchive::AvalancheArchive(const fs::path& file)
    : m_File(file)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(file);

    Initialise();
}

AvalancheArchive::AvalancheArchive(const fs::path& filename, const FileBuffer& buffer)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);
    if (m_StreamArchive) {
        m_StreamArchive->m_Filename = filename;
    }

 
    Initialise();
}

AvalancheArchive::~AvalancheArchive()
{
    DEBUG_LOG("AvalancheArchive::~AvalancheArchive");

    // delete all models from this archive
    const auto& models = ModelManager::Get()->GetArchiveModels();
    for (auto& model : models) {
        delete model;
    }

    g_CurrentLoadedArchive = nullptr;
    safe_delete(m_StreamArchive);
}

void AvalancheArchive::FileReadCallback(const fs::path& filename, const FileBuffer& data)
{
    // NOTE: will be deleted from g_CurrentLoadedArchive when the archive is closed
    new AvalancheArchive(filename, data);
}

void AvalancheArchive::Initialise()
{
    DEBUG_LOG("AvalancheArchive::Initialise");

    if (g_CurrentLoadedArchive) {
        delete g_CurrentLoadedArchive;
    }

    g_CurrentLoadedArchive = this;
    m_FileList = std::make_unique<DirectoryList>();
    m_FileList->Parse(m_StreamArchive);//, { ".rbm" });
}