#include <jc3/formats/ExportedEntity.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

ExportedEntity* g_CurrentLoadedArchive = nullptr;

ExportedEntity::ExportedEntity(const fs::path& file)
    : m_File(file)
{
    // make sure this is an ee file
    if (file.extension() != ".ee") {
        throw std::invalid_argument("ExportedEntity type only supports .ee files!");
    }

    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(file);
    m_StreamArchive->m_Filename = file;

    Initialise();
}

ExportedEntity::ExportedEntity(const fs::path& filename, const std::vector<uint8_t>& buffer)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);
    m_StreamArchive->m_Filename = filename;

    Initialise();
}

ExportedEntity::~ExportedEntity()
{
    DEBUG_LOG("ExportedEntity::~ExportedEntity");

    for (auto& rbm : m_LinkedRenderBlockModels) {
        delete rbm;
    }

    g_CurrentLoadedArchive = nullptr;
    safe_delete(m_StreamArchive);
}

void ExportedEntity::Initialise()
{
    g_CurrentLoadedArchive = this;
    m_FileList = std::make_unique<DirectoryList>();

    for (auto& file : m_StreamArchive->m_Files) {
        if (file.m_Filename.find(".rbm") != std::string::npos) {
        //    file.m_Filename.find(".ee") != std::string::npos ||
        //    file.m_Filename.find(".bl") != std::string::npos ||
        //    file.m_Filename.find(".nl") != std::string::npos)
            m_FileList->Add(file.m_Filename);
        }
    }
}

void ExportedEntity::LinkRenderBlockModel(RenderBlockModel* rbm)
{
    std::lock_guard<std::recursive_mutex> _lk{ m_LinkedRenderBlockModelsMutex };
    m_LinkedRenderBlockModels.emplace_back(rbm);
}