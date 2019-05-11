#include "avalanche_archive.h"
#include "render_block_model.h"

#include "../../graphics/ui.h"
#include "../../window.h"
#include "../file_loader.h"
#include "../hashlittle.h"

std::recursive_mutex                                  Factory<AvalancheArchive>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheArchive>> Factory<AvalancheArchive>::Instances;

AvalancheArchive::AvalancheArchive(const std::filesystem::path& filename)
    : m_File(filename)
{
    m_StreamArchive = std::make_unique<StreamArchive_t>();
    m_FileList      = std::make_unique<DirectoryList>();

    UI::Get()->SwitchToTab(TreeViewTab_Archives);
}

AvalancheArchive::AvalancheArchive(const std::filesystem::path& filename, const FileBuffer& buffer, bool external)
    : m_File(filename)
{
    // read the stream archive
    FileLoader::Get()->ReadStreamArchive(filename, std::move(buffer), external,
                                         [&](std::unique_ptr<StreamArchive_t> archive) {
                                             assert(archive);
                                             m_StreamArchive = std::move(archive);

                                             // parse the file list
                                             m_FileList = std::make_unique<DirectoryList>();
                                             m_FileList->Parse(m_StreamArchive.get());
                                         });

    UI::Get()->SwitchToTab(TreeViewTab_Archives);
}

void AvalancheArchive::AddFile(const std::filesystem::path& filename, const FileBuffer& data)
{
    assert(m_StreamArchive);
    assert(m_FileList);

    const auto& name = filename.generic_string();

    if (!m_StreamArchive->HasFile(name)) {
        m_FileList->Add(name);
    }

    m_StreamArchive->AddFile(name, data);
    m_HasUnsavedChanged = true;
}

void AvalancheArchive::Add(const std::filesystem::path& filename, const std::filesystem::path& root)
{
    // TODO: only add files which are a supported format
    // generic textures, models, etc

    if (std::filesystem::is_directory(filename)) {
        for (const auto& f : std::filesystem::directory_iterator(filename)) {
            Add(f.path(), root);
        }
    } else {
        std::ifstream stream(filename, std::ios::binary);
        if (stream.fail()) {
            SPDLOG_ERROR("Failed to open file stream!");
#ifdef DEBUG
            __debugbreak();
#endif
            return;
        }

        // strip the root directory from the path
        const auto& name = filename.lexically_relative(root);
        const auto  size = std::filesystem::file_size(filename);

        // read the file buffer
        FileBuffer buffer;
        buffer.resize(size);
        stream.read((char*)buffer.data(), size);
        stream.close();

        AddFile(name, buffer);
    }
}

bool AvalancheArchive::HasFile(const std::filesystem::path& filename)
{
    assert(m_StreamArchive);
    return m_StreamArchive->HasFile(filename.generic_string());
}

void AvalancheArchive::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    AvalancheArchive::make(filename, data, external);
}

bool AvalancheArchive::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    auto archive = AvalancheArchive::get(filename.string());
    if (archive) {
        assert(archive->m_StreamArchive);

        std::filesystem::create_directories(path.parent_path());

        std::string status_text    = "Repacking \"" + path.filename().string() + "\"...";
        const auto  status_text_id = UI::Get()->PushStatusText(status_text);

        // TODO: we should read the archive filelist, grab a list of files which have offset 0 (in patches)
        // and check if we have edited any of those files. if so we need to include it in the repack of the SARC

        std::thread([&, archive, path, status_text_id] {
            auto toc_path = path;
            toc_path += ".toc";

            // write the .toc file
            const auto toc_result = FileLoader::Get()->WriteTOC(toc_path, archive->m_StreamArchive.get());

            // TODO: only write toc if it was loaded with it
            if (archive->m_StreamArchive->m_UsingTOC && !toc_result) {
                SPDLOG_ERROR("Failed to write TOC!");
            }

            // generate the .ee
            std::ofstream stream(path, std::ios::binary);
            FileLoader::Get()->CompressArchive(stream, archive->m_StreamArchive.get());
            stream.close();

            UI::Get()->PopStatusText(status_text_id);
        }).detach();

        return true;
    }

    return false;
}
