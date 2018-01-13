#include <jc3/formats/AvalancheArchive.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

AvalancheArchive* g_CurrentLoadedArchive = nullptr;

AvalancheArchive::AvalancheArchive(const fs::path& file)
    : m_File(file)
{
#if 0
    // make sure this is an ee file
    if (file.extension() != ".ee") {
        throw std::invalid_argument("AvalancheArchive type only supports .ee files!");
    }
#endif

    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(file);
    m_StreamArchive->m_Filename = file;

    Initialise();
}

AvalancheArchive::AvalancheArchive(const fs::path& filename, const std::vector<uint8_t>& buffer)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);
    m_StreamArchive->m_Filename = filename;

    Initialise();
}

AvalancheArchive::~AvalancheArchive()
{
    DEBUG_LOG("AvalancheArchive::~AvalancheArchive");

    for (auto& rbm : m_LinkedRenderBlockModels) {
        delete rbm;
    }

    g_CurrentLoadedArchive = nullptr;
    safe_delete(m_StreamArchive);
}

void AvalancheArchive::FileReadCallback(const fs::path& filename, const std::vector<uint8_t>& data)
{
    // NOTE: will be deleted from g_CurrentLoadedArchive when the archive is closed
    new AvalancheArchive(filename, data);
}

void AvalancheArchive::Initialise()
{
    g_CurrentLoadedArchive = this;
    m_FileList = std::make_unique<DirectoryList>();
    m_FileList->Parse(m_StreamArchive, { ".rbm" });
}

void AvalancheArchive::LinkRenderBlockModel(RenderBlockModel* rbm)
{
    std::lock_guard<std::recursive_mutex> _lk{ m_LinkedRenderBlockModelsMutex };
    m_LinkedRenderBlockModels.emplace_back(rbm);
}