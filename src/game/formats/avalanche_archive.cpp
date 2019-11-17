#include <AvaFormatLib.h>
#include <spdlog/spdlog.h>

#include "avalanche_archive.h"
#include "directory_list.h"

#include "graphics/ui.h"

#include "game/file_loader.h"

std::recursive_mutex                                  Factory<AvalancheArchive>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheArchive>> Factory<AvalancheArchive>::Instances;

extern bool g_IsJC4Mode;

AvalancheArchive::AvalancheArchive(const std::filesystem::path& filename)
    : m_Filename(filename)
{
    m_FileList = std::make_unique<DirectoryList>();
    UI::Get()->SwitchToTab(TreeViewTab_Archives);
}

AvalancheArchive::AvalancheArchive(const std::filesystem::path& filename, const std::vector<uint8_t>& buffer,
                                   bool external)
    : m_Filename(filename)
    , m_FromExternalSource(external)
{
    m_FileList = std::make_unique<DirectoryList>();
    UI::Get()->SwitchToTab(TreeViewTab_Archives);

    // parse the buffer
    Parse(buffer);
}

void AvalancheArchive::Parse(const std::vector<uint8_t>& buffer, ParseCallback_t callback)
{
    if (buffer.empty()) {
        if (callback) {
            callback(false);
        }
        return;
    }

    char magic[4];
    std::memcpy(&magic, buffer.data(), sizeof(magic));
    const bool needs_aaf_decompress = strncmp(magic, "AAF", 4) == 0;

    std::thread([&, needs_aaf_decompress, buffer, callback] {
        try {
            if (needs_aaf_decompress) {
                std::vector<uint8_t> out_buffer;
                ava::AvalancheArchiveFormat::Decompress(buffer, &out_buffer);
                ava::StreamArchive::Parse(out_buffer, &m_Entries);

                m_Buffer = std::move(out_buffer);
            } else {
                ava::StreamArchive::Parse(buffer, &m_Entries);
                m_Buffer = std::move(buffer);
            }

            // parse TOC if this file didn't come from an external source
            if (!m_FromExternalSource) {
                auto toc_filename = m_Filename;
                toc_filename += ".toc";

                return FileLoader::Get()->ReadFile(
                    toc_filename, [&, callback](bool success, std::vector<uint8_t> data) {
                        if (success) {
                            uint32_t num_added = 0, num_patched = 0;
                            ava::StreamArchive::ParseTOC(data, &m_Entries, &num_added, &num_patched);

                            m_UsingTOC = true;

                            SPDLOG_INFO("Added {} and patched {} files from TOC.", num_added, num_patched);
                        }

                        // add entries to the filelist
                        for (const auto& entry : m_Entries) {
                            m_FileList->Add(entry.m_Filename);
                        }

                        if (callback) {
                            callback(true);
                        }
                    });
            }

            // add entries to the filelist
            for (const auto& entry : m_Entries) {
                m_FileList->Add(entry.m_Filename);
            }

            if (callback) {
                callback(true);
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Failed to parse stream archive. (\"{}\")", e.what());
#ifdef DEBUG
            __debugbreak();
#endif
        }
    }).detach();
}

void AvalancheArchive::ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                        bool external)
{
    if (!AvalancheArchive::exists(filename.string())) {
        AvalancheArchive::make(filename, data, external);
    }
}

bool AvalancheArchive::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    const auto archive = AvalancheArchive::get(filename.string());
    if (archive) {
        std::filesystem::create_directories(path.parent_path());

        std::string status_text    = "Repacking \"" + path.filename().string() + "\"...";
        const auto  status_text_id = UI::Get()->PushStatusText(status_text);

        // TODO: we should read the archive filelist, grab a list of files which have offset 0 (in patches)
        // and check if we have edited any of those files. if so we need to include it in the repack of the SARC

        std::thread([archive, path, status_text_id] {
            try {
                std::vector<uint8_t> compressed_buffer;
                ava::AvalancheArchiveFormat::Compress(archive->GetBuffer(), &compressed_buffer);

                std::ofstream stream(path, std::ios::binary);
                stream.write((char*)compressed_buffer.data(), compressed_buffer.size());
                stream.close();

                // file was opened with a TOC, rebuild the TOC buffer
                const auto& entries = archive->GetEntries();
                if (archive->IsUsingTOC() && !entries.empty()) {
                    auto toc_path = path;
                    toc_path += ".toc";

                    SPDLOG_INFO("Archive was loaded with a TOC. Writing new TOC file to \"{}\"", toc_path.string());

                    std::vector<uint8_t> toc_buffer;
                    ava::StreamArchive::WriteTOC(&toc_buffer, entries);

                    // write the TOC file
                    std::ofstream stream(toc_path, std::ios::binary);
                    stream.write((char*)toc_buffer.data(), toc_buffer.size());
                    stream.close();
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("Failed to compress stream archive. (\"{}\")", e.what());
#ifdef _DEBUG
                __debugbreak();
#endif
            }

            UI::Get()->PopStatusText(status_text_id);
        }).detach();

        return true;
    }

    return false;
}

void AvalancheArchive::AddFile(const std::filesystem::path& filename, const std::filesystem::path& root)
{
    // TODO: only add files which are a supported format
    // generic textures, models, etc

    if (std::filesystem::is_directory(filename)) {
        for (const auto& f : std::filesystem::directory_iterator(filename)) {
            AddFile(f.path(), root);
        }
    } else {
        try {
            // strip the root directory from the path
            const auto& name = filename.lexically_relative(root);
            const auto  size = std::filesystem::file_size(filename);

            // read the file buffer
            std::vector<uint8_t> buffer(size);
            std::ifstream        stream(filename, std::ios::binary);
            stream.read((char*)buffer.data(), size);
            stream.close();

            // write the file buffer to the SARC buffer
            ava::StreamArchive::WriteEntry(&m_Buffer, &m_Entries, name.string(), buffer);
        } catch (const std::exception& e) {
            SPDLOG_ERROR(e.what());
#ifdef _DEBUG
            __debugbreak();
#endif
        }
    }
}

bool AvalancheArchive::HasFile(const std::filesystem::path& filename)
{
    using namespace ava::StreamArchive;

    const auto& _filename = filename.generic_string();
    const auto  it        = std::find_if(m_Entries.begin(), m_Entries.end(),
                                 [_filename](const ArchiveEntry& item) { return item.m_Filename == _filename; });

    return it != m_Entries.end();
}
