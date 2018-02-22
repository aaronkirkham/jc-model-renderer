#include <jc3/formats/AvalancheArchive.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

AvalancheArchive* g_CurrentLoadedArchive = nullptr;

AvalancheArchive::AvalancheArchive(const fs::path& file, bool skip_init)
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

    if (!skip_init) {
        Initialise();
    }
}

AvalancheArchive::AvalancheArchive(const fs::path& filename, const FileBuffer& buffer, bool skip_init)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);
    m_StreamArchive->m_Filename = filename;

    if (!skip_init) {
        Initialise();
    }
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

void AvalancheArchive::FileReadCallback(const fs::path& filename, const FileBuffer& data)
{
    // NOTE: will be deleted from g_CurrentLoadedArchive when the archive is closed
    new AvalancheArchive(filename, data);
}

void AvalancheArchive::Initialise()
{
    DEBUG_LOG("AvalancheArchive::Initialise");

    g_CurrentLoadedArchive = this;
    m_FileList = std::make_unique<DirectoryList>();
    m_FileList->Parse(m_StreamArchive, { ".rbm" });
}

void AvalancheArchive::LinkRenderBlockModel(RenderBlockModel* rbm)
{
    std::lock_guard<std::recursive_mutex> _lk{ m_LinkedRenderBlockModelsMutex };
    m_LinkedRenderBlockModels.emplace_back(rbm);
}

void AvalancheArchive::Export(const fs::path& directory, std::function<void()> callback)
{
    std::stringstream status_text;
    status_text << "Exporting \"" << m_File << "\"...";
    const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

    std::thread t([this, directory, callback, status_text_id] {
        auto path = directory / m_File.stem();
        DEBUG_LOG("AvalancheArchive::Export - Exporting archive to " << path << "...");

        // create the directory if we need to
        if (!fs::exists(path)) {
            fs::create_directory(path);
        }

        // export all the files from the archive
        for (const auto& archiveEntry : m_StreamArchive->m_Files) {
            const auto file_path = path / archiveEntry.m_Filename;

            // ensure the file is valid
            if (archiveEntry.m_Offset != 0) {
                // create the directories for the file
                fs::create_directories(file_path.parent_path());

                // get the file contents from the archive buffer
                auto buffer = m_StreamArchive->GetEntryFromArchive(archiveEntry);

                // write the file
                std::ofstream file(file_path, std::ios::binary);

                if (file.fail()) {
                    DEBUG_LOG("[ERROR] AvalancheArchive::Export - Failed to open " << file_path.string());
                    continue;
                }

                file.write((char *)&buffer.front(), buffer.size());
                file.close();
            }
        }

        UI::Get()->PopStatusText(status_text_id);

        if (callback) {
            callback();
        }
    });

    t.detach();
}