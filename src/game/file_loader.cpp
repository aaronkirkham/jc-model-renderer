#include <istream>
#include <queue>
#include <thread>

#include "file_loader.h"
#include "name_hash_lookup.h"

#include "../graphics/dds_texture_loader.h"
#include "../graphics/texture_manager.h"
#include "../graphics/ui.h"

#include "formats/avalanche_archive.h"
#include "formats/render_block_model.h"
#include "formats/runtime_container.h"
#include "render_block_factory.h"

#include "jc4/oodle.h"

#include "../import_export/import_export_manager.h"

#include "../fnv1.h"
#include "../settings.h"
#include "hashlittle.h"
#include <zlib.h>

// Credit to gibbed for most of the file formats
// http://gib.me

static uint64_t g_ArchiveLoadCount = 0;
extern bool     g_IsJC4Mode;

std::unordered_map<uint32_t, std::string> NameHashLookup::LookupTable;

FileLoader::FileLoader()
{
    // init
    Init();

    // init the namehash lookup table
    NameHashLookup::Init();

    // TODO: move the events to a better location?

    // trigger the file type callbacks
    UI::Get()->Events().FileTreeItemSelected.connect([&](const std::filesystem::path& file, AvalancheArchive* archive) {
        // do we have a registered callback for this file type?
        if (m_FileTypeCallbacks.find(file.extension().string()) != m_FileTypeCallbacks.end()) {
            ReadFile(file, [&, file](bool success, FileBuffer data) {
                if (success) {
                    for (const auto& fn : m_FileTypeCallbacks[file.extension().string()]) {
                        fn(file, data, false);
                    }
                } else {
                    LOG_ERROR("(FileTreeItemSelected) Failed to load \"{}\"", file.string());

                    std::string error = "Failed to load \"" + file.filename().string() + "\".\n\n";
                    error += "Make sure you have selected the correct Just Cause directory.";
                    Window::Get()->ShowMessageBox(error);
                }
            });
        } else {
            LOG_ERROR("(FileTreeItemSelected) Unknown file type extension \"{}\" (\"{}\")", file.extension().string(),
                      file.string());

            std::string error = "Unable to read the \"" + file.extension().string() + "\" extension.\n\n";
            error += "Want to help? Check out our GitHub page for information on how to contribute.";
            Window::Get()->ShowMessageBox(error);
        }
    });

    // save file
    UI::Get()->Events().SaveFileRequest.connect(
        [&](const std::filesystem::path& file, const std::filesystem::path& directory, bool is_dropzone) {
            bool was_handled = false;

            // keep the full path if we are saving to the dropzone
            std::filesystem::path path = directory;
            path /= (is_dropzone ? file : file.filename());

            std::filesystem::create_directories(path.parent_path());

            // try use a handler
            if (m_SaveFileCallbacks.find(file.extension().string()) != m_SaveFileCallbacks.end()) {
                for (const auto& fn : m_SaveFileCallbacks[file.extension().string()]) {
                    was_handled = fn(file, path);
                }
            }

            // no handlers, fallback to just reading the raw data
            if (!was_handled) {
                FileLoader::Get()->ReadFile(file, [&, file, path](bool success, FileBuffer data) {
                    if (success) {
                        // write the file data
                        std::ofstream stream(path, std::ios::binary);
                        if (!stream.fail()) {
                            stream.write((char*)data.data(), data.size());
                            stream.close();
                            return;
                        }
                    }

                    LOG_ERROR("(SaveFileRequest) Failed to save \"{}\"", file.filename().string());
                    Window::Get()->ShowMessageBox("Failed to save \"" + file.filename().string() + "\".");
                });
            }
        });

    // import file
    UI::Get()->Events().ImportFileRequest.connect([&](IImportExporter* importer, ImportFinishedCallback callback) {
        std::string filter = importer->GetName();
        filter.push_back('\0');
        filter.append("*");
        filter.append(importer->GetExportExtension());
        filter.push_back('\0');

        Window::Get()->ShowFileSelection(
            "Select a file to import", filter, [&, importer, callback](const std::filesystem::path& selected) {
                LOG_INFO("(ImportFileRequest) Want to import file \"{}\"", selected.string());
                std::thread([&, importer, selected, callback] { importer->Import(selected, callback); }).detach();
            });
    });

    // export file
    UI::Get()->Events().ExportFileRequest.connect([&](const std::filesystem::path& file, IImportExporter* exporter) {
        Window::Get()->ShowFolderSelection(
            "Select a folder to export the file to.", [&, file, exporter](const std::filesystem::path& selected) {
                auto _exporter = exporter;
                if (!exporter) {
                    const auto& exporters = ImportExportManager::Get()->GetExporters(file.extension().string());
                    if (exporters.size() > 0) {
                        _exporter = exporters.at(0);
                    }
                }

                // if we have a valid exporter, read the file and export it
                if (_exporter) {
                    std::string status_text    = "Exporting \"" + file.generic_string() + "\"...";
                    const auto  status_text_id = UI::Get()->PushStatusText(status_text);

                    std::thread([&, file, selected, status_text_id] {
                        _exporter->Export(file, selected, [&, file, status_text_id](bool success) {
                            UI::Get()->PopStatusText(status_text_id);

                            if (!success) {
                                LOG_ERROR("(ExportFileRequest) Failed to export \"{}\"", file.filename().string());
                                Window::Get()->ShowMessageBox("Failed to export \"" + file.filename().string() + "\".");
                            }
                        });
                    }).detach();
                }
            });
    });
}

FileLoader::~FileLoader()
{
    Shutdown();
}

void FileLoader::Init()
{
    Shutdown();

    // load oo2core_7_win64.dll
    if (g_IsJC4Mode) {
        std::filesystem::path filename = Window::Get()->GetJustCauseDirectory();
        filename /= "oo2core_7_win64.dll";

        if (!std::filesystem::exists(filename)) {
            LOG_ERROR("Can't find oo2core_7_win64.dll! ({})", filename.string());
            Window::Get()->ShowMessageBox(
                "Can't find oo2core_7_win64.dll.\n\nPlease make sure your Just Cause 4 directory is valid.",
                MB_ICONERROR | MB_OK);
            return;
        }

        oo2core_7_win64 = LoadLibrary(filename.string().c_str());
        if (!oo2core_7_win64) {
            LOG_ERROR("Failed to load oo2core_7_win64.dll! (ErrorCode={})", GetLastError());
            std::string error = "Failed to load oo2core_7_win64.dll\n\nErrorCode=" + std::to_string(GetLastError());
            Window::Get()->ShowMessageBox(error, MB_ICONERROR | MB_OK);
            return;
        }

        oo2::OodleLZ_Compress_original = (oo2::OodleLZ_Compress_t)GetProcAddress(oo2core_7_win64, "OodleLZ_Compress");
        oo2::OodleLZ_Decompress_original =
            (oo2::OodleLZ_Decompress_t)GetProcAddress(oo2core_7_win64, "OodleLZ_Decompress");
    }

    // init the filelist
    m_FileList = std::make_unique<DirectoryList>();

    const auto status_text_id = UI::Get()->PushStatusText("Loading dictionary...");
    std::thread([this, status_text_id] {
        try {
            const auto handle = GetModuleHandle(nullptr);
            const auto rc     = FindResource(handle, MAKEINTRESOURCE(g_IsJC4Mode ? 256 : 128), RT_RCDATA);
            if (rc == nullptr) {
                throw std::runtime_error("FindResource failed");
            }

            const auto data = LoadResource(handle, rc);
            if (data == nullptr) {
                throw std::runtime_error("LoadResource failed");
            }

            // parse the file list json
            auto  str        = static_cast<const char*>(LockResource(data));
            auto& dictionary = nlohmann::json::parse(str, (str + SizeofResource(handle, rc)));
            m_FileList->Parse(&dictionary);

            // parse the dictionary
            for (auto& it = dictionary.begin(); it != dictionary.end(); ++it) {
                const auto& key  = it.key();
                const auto& data = it.value();

                const auto namehash = static_cast<uint32_t>(std::stoul(data["hash"].get<std::string>(), nullptr, 16));

                std::vector<std::string> paths;
                for (const auto& path : data["path"]) {
                    paths.emplace_back(path.get<std::string>());
                }

                m_Dictionary[namehash] = std::make_pair(key, std::move(paths));
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to load file list dictionary. ({})", e.what());

            std::string error = "Failed to load file list dictionary. (";
            error += e.what();
            error += ")\n\n";
            error += "Some features will be disabled.";
            Window::Get()->ShowMessageBox(error);
        }

        UI::Get()->PopStatusText(status_text_id);
    }).detach();
}

void FileLoader::Shutdown()
{
    // release oo2core_7_win64.dll
    if (oo2core_7_win64) {
        FreeLibrary(oo2core_7_win64);

        oo2core_7_win64                  = nullptr;
        oo2::OodleLZ_Compress_original   = nullptr;
        oo2::OodleLZ_Decompress_original = nullptr;
    }

    //
    m_FileList = nullptr;
}

void FileLoader::ReadFile(const std::filesystem::path& filename, ReadFileCallback callback, uint8_t flags) noexcept
{
    uint64_t status_text_id = 0;

    if (!UseBatches) {
        std::string status_text = "Reading \"" + filename.generic_string() + "\"...";
        status_text_id          = UI::Get()->PushStatusText(status_text);
    }

    // are we trying to load textures?
    if (filename.extension() == ".dds" || filename.extension() == ".ddsc" || filename.extension() == ".hmddsc"
        || filename.extension() == ".atx1" || filename.extension() == ".atx2") {
        const auto& texture = TextureManager::Get()->GetTexture(filename, TextureManager::TextureCreateFlags_NoCreate);
        if (texture && texture->IsLoaded()) {
            UI::Get()->PopStatusText(status_text_id);
            return callback(true, *texture->GetBuffer());
        }

        // try load the texture
        if (!(flags & ReadFileFlags_SkipTextureLoader)) {
            UI::Get()->PopStatusText(status_text_id);
            return ReadTexture(filename, callback);
        }
    }

    // check any loaded archives for the file
    const auto& [archive, entry] = GetStreamArchiveFromFile(filename);
    if (archive && entry.m_Offset != 0 && entry.m_Offset != -1) {
        auto buffer = archive->GetEntryBuffer(entry);

        UI::Get()->PopStatusText(status_text_id);
        return callback(true, std::move(buffer));
    }
#ifdef DEBUG
    else if (archive && (entry.m_Offset == 0 || entry.m_Offset == -1)) {
        LOG_INFO("\"{}\" exists in archive but has been patched. Will read the patch version instead.",
                 filename.filename().string());
    }
#endif

    // should we use file batching?
    if (UseBatches) {
        LOG_TRACE("Using batches for \"{}\"", filename.string());
        return ReadFileBatched(filename, callback);
    }

    // finally, lets read it directory from the arc file
    std::thread([this, filename, callback, status_text_id] {
        const auto& [directory_name, archive_name, namehash] = LocateFileInDictionary(filename);
        if (!directory_name.empty()) {
            FileBuffer buffer;
            if (ReadFileFromArchive(directory_name, archive_name, namehash, &buffer)) {
                UI::Get()->PopStatusText(status_text_id);
                return callback(true, std::move(buffer));
            }
        }

        UI::Get()->PopStatusText(status_text_id);
        return callback(false, {});
    }).detach();
}

void FileLoader::ReadFileBatched(const std::filesystem::path& filename, ReadFileCallback callback) noexcept
{
    std::lock_guard<std::recursive_mutex> _lk{m_BatchesMutex};
    m_PathBatches[filename.string()].emplace_back(callback);
}

void FileLoader::RunFileBatches() noexcept
{
    std::thread([&] {
        std::lock_guard<std::recursive_mutex> _lk{m_BatchesMutex};

        for (const auto& path : m_PathBatches) {
            const auto& [directory_name, archive_name, namehash] = LocateFileInDictionary(path.first);
            if (!directory_name.empty()) {
                const auto& archive_path = (directory_name + "/" + archive_name);

                for (const auto& callback : path.second) {
                    m_Batches[archive_path][namehash].emplace_back(callback);
                }
            }
            // nothing found, callback
            else {
                LOG_ERROR("Didn't find {}", path.first);
                for (const auto& callback : path.second) {
                    callback(false, {});
                }
            }
        }

#ifdef DEBUG
        for (const auto& batch : m_Batches) {
            int c = 0;
            for (const auto& file : batch.second) {
                c += file.second.size();
            }

            LOG_INFO("Will read {} files from \"{}\"", c, batch.first);
        }
#endif

        // iterate over all the archives
        for (const auto& batch : m_Batches) {
            const auto& archive_path = batch.first;
            const auto  sep          = archive_path.find_last_of("/");

            // split the directory and archive
            const auto& directory = archive_path.substr(0, sep);
            const auto& archive   = archive_path.substr(sep + 1, archive_path.length());

            // get the path names
            auto&      jc_directory = Window::Get()->GetJustCauseDirectory();
            const auto tab_file     = jc_directory / directory / (archive + ".tab");
            const auto arc_file     = jc_directory / directory / (archive + ".arc");

            // ensure the tab file exists
            if (!std::filesystem::exists(tab_file)) {
                LOG_ERROR("Can't find TAB file (\"{}\")", tab_file.string());
                continue;
            }

            // ensure the arc file exists
            if (!std::filesystem::exists(arc_file)) {
                LOG_ERROR("Can't find ARC file (\"{}\")", arc_file.string());
                continue;
            }

            std::vector<TabFileEntry>                            entries;
            std::vector<jc::ArchiveTable::VfsTabCompressedBlock> blocks;
            if (ReadArchiveTable(tab_file, &entries, &blocks)) {
                // open the arc file
                std::ifstream stream(arc_file, std::ios::binary);
                if (stream.fail()) {
                    LOG_ERROR("Invalid ARC file stream!");
                    continue;
                }

#ifdef DEBUG
                g_ArchiveLoadCount++;
                LOG_INFO("Total archive load count: {}", g_ArchiveLoadCount);
#endif

                // iterate over all the files
                for (const auto& file : batch.second) {
                    const auto it = std::find_if(entries.begin(), entries.end(), [&](const TabFileEntry& entry) {
                        return entry.m_NameHash == file.first;
                    });

                    if (it != entries.end()) {
                        const auto& entry = (*it);

                        //
                        stream.seekg(entry.m_Offset);

                        // read the file buffer
                        FileBuffer buffer;
                        buffer.resize(entry.m_CompressedSize);
                        stream.read((char*)buffer.data(), entry.m_CompressedSize);

                        // trigger the callbacks
                        for (const auto& callback : file.second) {
                            callback(true, std::move(buffer));
                        }
                    } else {
                        LOG_ERROR("Didn't find {}", file.first);

                        for (const auto& callback : file.second) {
                            callback(false, {});
                        }
                    }
                }

                stream.close();
            }
        }

        // clear the batches
        m_PathBatches.clear();
        m_Batches.clear();
    }).detach();
}

void FileLoader::ReadFileFromDisk(const std::filesystem::path& filename) noexcept
{
    const auto& extension = filename.extension().string();

    // do we have a registered callback for this file type?
    if (m_FileTypeCallbacks.find(extension) != m_FileTypeCallbacks.end()) {
        const auto size = std::filesystem::file_size(filename);

        FileBuffer data;
        data.resize(size);

        // read the file
        std::ifstream stream(filename, std::ios::binary);
        assert(!stream.fail());
        stream.read((char*)data.data(), size);
        stream.close();

        // run the callback handlers
        for (const auto& fn : m_FileTypeCallbacks[extension]) {
            fn(filename.filename(), data, true);
        }
    } else {
        LOG_ERROR("Unknown file type extension \"{}\"", filename.filename().string());

        std::string error = "Unable to read the \"" + extension + "\" extension.\n\n";
        error += "Want to help? Check out our GitHub page for information on how to contribute.";
        Window::Get()->ShowMessageBox(error);
    }
}

std::unique_ptr<StreamArchive_t> FileLoader::ParseStreamArchive(const FileBuffer* sarc_buffer, FileBuffer* toc_buffer)
{
    std::istringstream stream(std::string{(char*)sarc_buffer->data(), sarc_buffer->size()});

    jc::StreamArchive::SARCHeader header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "SARC", 4) != 0) {
        LOG_ERROR("Invalid header magic");
        return nullptr;
    }

    auto result        = std::make_unique<StreamArchive_t>();
    result->m_Header   = header;
    result->m_UsingTOC = (toc_buffer != nullptr);

    // jc4
    if (header.m_Version == 3) {
        uint32_t strings_length;
        stream.read((char*)&strings_length, sizeof(strings_length));

        std::map<uint32_t, std::string> filenames;

        uint32_t data_offset = static_cast<uint32_t>(stream.tellg()) + strings_length;

        while (static_cast<uint32_t>(stream.tellg()) < data_offset) {
            std::string filename;
            std::getline(stream, filename, '\0');

            const auto hash = hashlittle(filename.c_str());
            filenames[hash] = std::move(filename);
        }

        assert(stream.tellg() == data_offset);

        std::filesystem::path jc_directory = Window::Get()->GetJustCauseDirectory();

        while (stream.tellg() < header.m_Size) {
            uint32_t a, fileOffset, uncompressedSize, namehash, e;
            stream.read((char*)&a, sizeof(a));
            stream.read((char*)&fileOffset, sizeof(fileOffset));
            stream.read((char*)&uncompressedSize, sizeof(uncompressedSize));
            stream.read((char*)&namehash, sizeof(namehash));
            stream.read((char*)&e, sizeof(e));

            result->m_Files.emplace_back(StreamArchiveEntry_t{filenames[namehash], fileOffset, uncompressedSize});
        }

        return result;
    }

#ifdef DEBUG
    uint32_t _num_toc_added_files   = 0;
    uint32_t _num_toc_patched_files = 0;
#endif

    const auto start_pos = stream.tellg();
    while (true) {
        uint32_t length;
        stream.read((char*)&length, sizeof(length));

        auto filename = std::unique_ptr<char[]>(new char[length + 1]);
        stream.read(filename.get(), length);
        filename[length] = '\0';

        // read the archive entry data
        StreamArchiveEntry_t entry;
        entry.m_Filename = filename.get();
        stream.read((char*)&entry.m_Offset, sizeof(entry.m_Offset));
        stream.read((char*)&entry.m_Size, sizeof(entry.m_Size));

        result->m_Files.emplace_back(std::move(entry));

        if (header.m_Size - (stream.tellg() - start_pos) <= 15) {
            break;
        }
    }

    // do we have a TOC buffer?
    if (toc_buffer) {
        std::istringstream toc_stream(std::string{(char*)toc_buffer->data(), toc_buffer->size()});
        while (static_cast<size_t>(toc_stream.tellg()) < toc_buffer->size()) {
            uint32_t length;
            toc_stream.read((char*)&length, sizeof(length));

            auto filename = std::unique_ptr<char[]>(new char[length + 1]);
            toc_stream.read(filename.get(), length);
            filename[length] = '\0';

            // read the archive entry data
            StreamArchiveEntry_t entry;
            entry.m_Filename = filename.get();
            entry.m_Patched  = true;
            toc_stream.read((char*)&entry.m_Offset, sizeof(entry.m_Offset));
            toc_stream.read((char*)&entry.m_Size, sizeof(entry.m_Size));

            const auto it =
                std::find_if(result->m_Files.begin(), result->m_Files.end(),
                             [&](const StreamArchiveEntry_t& item) { return item.m_Filename == entry.m_Filename; });
            if (it == result->m_Files.end()) {
                LOG_INFO("need to add {} from toc", entry.m_Filename);

                result->m_Files.emplace_back(std::move(entry));

#ifdef DEBUG
                _num_toc_added_files++;
#endif
            } else {
#ifdef DEBUG
                if ((*it).m_Offset != entry.m_Offset || (*it).m_Size != entry.m_Size) {
                    _num_toc_patched_files++;
                }
#endif

                // update file offset & size
                (*it).m_Offset = entry.m_Offset;
                (*it).m_Size   = entry.m_Size;
            }
        }
    }

#ifdef DEBUG
    if (_num_toc_added_files > 0 || _num_toc_patched_files > 0) {
        LOG_INFO("Added {}, patched {} files from TOC buffer", _num_toc_added_files, _num_toc_patched_files);
    }
#endif

    return result;
}

void FileLoader::ReadStreamArchive(const std::filesystem::path& filename, const FileBuffer& data, bool external_source,
                                   ReadArchiveCallback callback) noexcept
{
    static auto read_archive = [&, filename](const FileBuffer* data,
                                             FileBuffer*       toc_buffer) -> std::unique_ptr<StreamArchive_t> {
        // decompress the archive data
        std::istringstream compressed_buffer(std::string{(char*)data->data(), data->size()});

        // do we need to decompress the AAF?
        char magic[4];
        std::memcpy(&magic, data->data(), sizeof(magic));
        if (!strncmp(magic, "AAF", 4)) {
            LOG_INFO("Stream archive is compressed. Decompressing now...");

            FileBuffer buffer;
            if (DecompressArchiveFromStream(compressed_buffer, &buffer)) {
                auto archive = ParseStreamArchive(&buffer, toc_buffer);
                if (archive) {
                    archive->m_Filename  = filename;
                    archive->m_SARCBytes = std::move(buffer);
                }

                return archive;
            }

            LOG_ERROR("Failed to decompressed buffer.");
            return nullptr;
        }

        auto archive = ParseStreamArchive(data, toc_buffer);
        if (archive) {
            archive->m_Filename  = filename;
            archive->m_SARCBytes = std::move(*data);
        }

        return archive;
    };

    // if we are not loading from an external source, look for the toc
    if (!external_source) {
        auto toc_filename = filename;
        toc_filename += ".toc";

        ReadFile(toc_filename,
                 [&, toc_filename, compressed_data = std::move(data), callback](bool success, FileBuffer data) {
                     return callback(read_archive(&compressed_data, success ? &data : nullptr));
                 });
    } else {
        std::thread([this, compressed_data = std::move(data), callback] {
            callback(read_archive(&compressed_data, nullptr));
        }).detach();
    }
}

void FileLoader::CompressArchive(std::ostream& stream, jc::AvalancheArchive::Header* header,
                                 std::vector<jc::AvalancheArchive::Chunk>* chunks) noexcept
{
    // write the header
    stream.write((char*)header, sizeof(jc::AvalancheArchive::Header));

    // write all the blocks
    for (uint32_t i = 0; i < header->m_ChunkCount; ++i) {
        auto& chunk = chunks->at(i);

        // compress the chunk data
        // NOTE: 2 + 4 for the compression header & checksum
        // no way to tell zlib not to write them??
        auto decompressed_size = (uLong)chunk.m_UncompressedSize;
        auto compressed_size   = compressBound(decompressed_size);

        LOG_INFO("m_DataSize={}, m_CompressedSize={}", chunk.m_DataSize, chunk.m_CompressedSize);

        FileBuffer result;
        result.reserve(compressed_size);
        auto res = compress(result.data(), &compressed_size, chunk.m_BlockData.data(), decompressed_size);
        assert(res == Z_OK);

        // store the compressed size
        chunk.m_CompressedSize =
            static_cast<uint32_t>(compressed_size) - 6; // remove the header and checksum from the total size
        LOG_INFO("Actual compressed size: {}", chunk.m_CompressedSize);

        // calculate the distance to the 16-byte boundary after we write our data
        auto pos = static_cast<uint32_t>(stream.tellp()) + jc::AvalancheArchive::CHUNK_SIZE + chunk.m_CompressedSize;
        auto padding = jc::DISTANCE_TO_BOUNDARY(pos, 16);

        // generate the data size
        chunk.m_DataSize = (jc::AvalancheArchive::CHUNK_SIZE + chunk.m_CompressedSize + padding);
        LOG_INFO("data size: {}", chunk.m_DataSize);

        // write the chunk
        stream.write((char*)&chunk.m_CompressedSize, sizeof(uint32_t));
        stream.write((char*)&chunk.m_UncompressedSize, sizeof(uint32_t));
        stream.write((char*)&chunk.m_DataSize, sizeof(uint32_t));
        stream.write((char*)&chunk.m_Magic, sizeof(uint32_t));

        // ignore the header when writing the data
        stream.write((char*)result.data() + 2, chunk.m_CompressedSize);

        // write the padding
        LOG_INFO("Writing {} bytes of padding", padding);
        static uint32_t PADDING_BYTE = 0x30;
        for (decltype(padding) i = 0; i < padding; ++i) {
            stream.write((char*)&PADDING_BYTE, 1);
        }
    }
}

void FileLoader::CompressArchive(std::ostream& stream, StreamArchive_t* archive) noexcept
{
    assert(archive);

    auto&      block_data = archive->m_SARCBytes;
    const auto num_chunks = 1 + static_cast<uint32_t>(block_data.size() / jc::AvalancheArchive::MAX_CHUNK_DATA_SIZE);
    constexpr auto max_chunk_size = jc::AvalancheArchive::MAX_CHUNK_DATA_SIZE;

    // generate the header
    jc::AvalancheArchive::Header header;
    header.m_TotalUncompressedSize  = archive->m_SARCBytes.size();
    header.m_UncompressedBufferSize = num_chunks > 1 ? max_chunk_size : archive->m_SARCBytes.size();
    header.m_ChunkCount             = num_chunks;

    LOG_INFO("ChunkCount={}, TotalUncompressedSize={}, UncompressedBufferSize={}", header.m_ChunkCount,
             header.m_TotalUncompressedSize, header.m_UncompressedBufferSize);

    // generate the chunks
    std::vector<jc::AvalancheArchive::Chunk> chunks;
    uint32_t                                 last_chunk_offset = 0;
    for (uint32_t i = 0; i < header.m_ChunkCount; ++i) {
        jc::AvalancheArchive::Chunk chunk;

        // generate chunks if needed
        if (num_chunks > 1) {
            // calculate the chunk size
            const auto block_size = block_data.size() - last_chunk_offset;
            auto       chunk_size = block_size > max_chunk_size ? max_chunk_size % block_size : block_size;

            std::vector<uint8_t> n_block_data(block_data.begin() + last_chunk_offset,
                                              block_data.begin() + last_chunk_offset + chunk_size);
            last_chunk_offset += chunk_size;

            chunk.m_UncompressedSize = chunk_size;
            chunk.m_BlockData        = std::move(n_block_data);
        }
        // we only have a single chunk, use the total block size
        else {
            chunk.m_UncompressedSize = block_data.size();
            chunk.m_BlockData        = block_data;
        }

        LOG_INFO("AAF chunk #{}, UncompressedSize={}", i, chunk.m_UncompressedSize);

        chunks.emplace_back(std::move(chunk));
    }

    return CompressArchive(stream, &header, &chunks);
}

bool FileLoader::DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept
{
    // read the archive header
    jc::AvalancheArchive::Header header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "AAF", 4) != 0) {
        LOG_ERROR("Invalid header magic");
        return false;
    }

    LOG_INFO("ChunkCount={}, TotalUncompressedSize={}, UncompressedBufferSize={}", header.m_ChunkCount,
             header.m_TotalUncompressedSize, header.m_UncompressedBufferSize);

    output->reserve(header.m_TotalUncompressedSize);

    // read all the blocks
    for (uint32_t i = 0; i < header.m_ChunkCount; ++i) {
        auto base_position = stream.tellg();

        // read the chunk
        jc::AvalancheArchive::Chunk chunk;
        stream.read((char*)&chunk.m_CompressedSize, sizeof(chunk.m_CompressedSize));
        stream.read((char*)&chunk.m_UncompressedSize, sizeof(chunk.m_UncompressedSize));
        stream.read((char*)&chunk.m_DataSize, sizeof(chunk.m_DataSize));
        stream.read((char*)&chunk.m_Magic, sizeof(chunk.m_Magic));

        LOG_INFO("AAF chunk #{}, CompressedSize={}, UncompressedSize={}, DataSize={}", i, chunk.m_CompressedSize,
                 chunk.m_UncompressedSize, chunk.m_DataSize);

        // make sure the block magic is correct
        if (chunk.m_Magic != 0x4D415745) {
            output->shrink_to_fit();
            LOG_ERROR("Invalid chunk magic");
            return false;
        }

        // read the block data
        FileBuffer data;
        data.resize(chunk.m_CompressedSize);
        stream.read((char*)data.data(), chunk.m_CompressedSize);

        // decompress the block
        {
            uLong compressed_size   = (uLong)chunk.m_CompressedSize;
            uLong decompressed_size = (uLong)chunk.m_UncompressedSize;

            FileBuffer result;
            result.resize(chunk.m_UncompressedSize);
            auto res = uncompress(result.data(), &decompressed_size, data.data(), compressed_size);

            assert(res == Z_OK);
            assert(decompressed_size == chunk.m_UncompressedSize);

            output->insert(output->end(), result.begin(), result.end());
            chunk.m_BlockData = std::move(result);
        }

        // goto the next block
        stream.seekg((uint64_t)base_position + chunk.m_DataSize);
    }

    assert(output->size() == header.m_TotalUncompressedSize);

    return true;
}

bool FileLoader::WriteTOC(const std::filesystem::path& filename, StreamArchive_t* archive) noexcept
{
    assert(archive);

    std::ofstream stream(filename, std::ios::binary);
    if (stream.fail()) {
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    for (const auto& entry : archive->m_Files) {
        auto length = static_cast<uint32_t>(entry.m_Filename.length());

        stream.write((char*)&length, sizeof(uint32_t));
        stream.write((char*)entry.m_Filename.c_str(), length);
        stream.write((char*)&entry.m_Offset, sizeof(uint32_t));
        stream.write((char*)&entry.m_Size, sizeof(uint32_t));
    }

    stream.close();
    return true;
}

static void DEBUG_TEXTURE_FLAGS(const jc::AvalancheTexture::Header* header)
{
    using namespace jc::AvalancheTexture;

    if (header->m_Flags != 0) {
        // clang-format off
        std::stringstream ss;
        ss << "AVTX m_Flags=" << header->m_Flags << " (";

        if (header->m_Flags & STREAMED)       ss << "streamed, ";
        if (header->m_Flags & PLACEHOLDER)    ss << "placeholder, ";
        if (header->m_Flags & TILED)          ss << "tiled, ";
        if (header->m_Flags & SRGB)           ss << "SRGB, ";
        if (header->m_Flags & CUBE)           ss << "cube, ";

        ss.seekp(-2, ss.cur);
        ss << ")";
		LOG_INFO(ss.str());

        // clang-format on
    }
}

void FileLoader::ReadTexture(const std::filesystem::path& filename, ReadFileCallback callback) noexcept
{
    FileLoader::Get()->ReadFile(
        filename,
        [this, filename, callback](bool success, FileBuffer data) {
            if (!success) {
                // ddsc failed. look for the source file instead
                if (filename.extension() == ".ddsc") {
                    auto source_filename = filename;
                    source_filename.replace_extension(g_IsJC4Mode ? ".atx1" : ".hmddsc"); // TODO: ATX2

                    // we should look for .atx2 first, if that doesn't exist, look for .atx1

                    auto cb = [&, callback](bool success, FileBuffer data) {
                        if (success) {
                            FileBuffer d;
                            ReadHMDDSC(&data, &d);
                            return callback(success, std::move(d));
                        }

                        return callback(success, {});
                    };

                    if (FileLoader::UseBatches) {
                        FileLoader::Get()->ReadFileBatched(source_filename, cb);
                    } else {
                        FileLoader::Get()->ReadFile(source_filename, cb, ReadFileFlags_SkipTextureLoader);
                    }

                    return;
                }

                return callback(false, {});
            }

            // generic DDS handler
            if (filename.extension() == ".dds") {
                return callback(true, std::move(data));
            }

            // source file buffer
            if (filename.extension() == ".hmddsc" || filename.extension() == ".atx1"
                || filename.extension() == ".atx2") {
                FileBuffer d;
                ReadHMDDSC(&data, &d);
                return callback(true, std::move(d));
            }

            std::istringstream stream(std::string{(char*)data.data(), data.size()});

            // read the texture data
            jc::AvalancheTexture::Header texture;
            stream.read((char*)&texture, sizeof(texture));

            // ensure the header magic is correct
            if (texture.m_Magic != 0x58545641) {
                LOG_ERROR("Invalid header magic");
                return callback(false, {});
            }

            DEBUG_TEXTURE_FLAGS(&texture);

            // figure out which source file to load
            uint8_t source = 0;
            if (g_IsJC4Mode) {
                auto atx2 = filename;
                atx2.replace_extension(".atx2");

                bool has_atx2 = FileLoader::Get()->HasFileInDictionary(hashlittle(atx2.string().c_str()));
                if (!has_atx2) {
                    auto atx1 = filename;
                    atx1.replace_extension(".atx1");

                    bool has_atx1 = FileLoader::Get()->HasFileInDictionary(hashlittle(atx1.string().c_str()));
                    source        = static_cast<uint8_t>(has_atx1);
                } else {
                    source = 2;
                }
            } else {
                auto hmddsc = filename;
                hmddsc.replace_extension(".hmddsc");
                if (FileLoader::Get()->HasFileInDictionary(hashlittle(hmddsc.string().c_str()))) {
                    source = 1;
                }
            }

            // find the best stream to use
            const auto stream_index = jc::AvalancheTexture::FindBest(&texture, source);

            // find the rank
            const auto rank = jc::AvalancheTexture::GetHighestRank(&texture, stream_index);

            // generate the DDS header
            DDS_HEADER header{};
            header.size        = sizeof(DDS_HEADER);
            header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
            header.width       = texture.m_Width >> rank;
            header.height      = texture.m_Height >> rank;
            header.depth       = texture.m_Depth;
            header.mipMapCount = 1;
            header.ddspf       = TextureManager::GetPixelFormat(texture.m_Format);
            header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

            //
            bool dxt10header = (header.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));

            // resize the output buffer
            FileBuffer out;
            out.resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + (dxt10header ? sizeof(DDS_HEADER_DXT10) : 0)
                       + texture.m_Streams[stream_index].m_Size);

            // write the DDS magic and header
            std::memcpy(&out.front(), (char*)&DDS_MAGIC, sizeof(DDS_MAGIC));
            std::memcpy(&out.front() + sizeof(DDS_MAGIC), (char*)&header, sizeof(DDS_HEADER));

            // write the DXT10 header
            if (dxt10header) {
                DDS_HEADER_DXT10 dx10header{};
                dx10header.dxgiFormat        = texture.m_Format;
                dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
                dx10header.arraySize         = 1;

                std::memcpy(&out.front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), (char*)&dx10header,
                            sizeof(DDS_HEADER_DXT10));
            }

            // if we need to load the source file, request it
            if (source > 0) {
                auto source_filename = filename;

                if (g_IsJC4Mode) {
                    std::string source_extension = ".atx" + std::to_string(source);
                    source_filename.replace_extension(source_extension);
                } else {
                    source_filename.replace_extension(".hmddsc");
                }

                LOG_INFO("Loading source: \"{}\".", source_filename.string());

                auto offset = texture.m_Streams[stream_index].m_Offset;
                auto size   = texture.m_Streams[stream_index].m_Size;

                // read file callback
                auto cb = [&, dxt10header, offset, size, out, callback](bool success, FileBuffer data) mutable {
                    if (success) {
                        std::memcpy(&out.front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER)
                                        + (dxt10header ? sizeof(DDS_HEADER_DXT10) : 0),
                                    data.data() + offset, size);

                        return callback(true, std::move(out));
                    }

                    return callback(false, {});
                };

                if (FileLoader::UseBatches) {
                    LOG_INFO("Using batches for \"{}\"", source_filename.string());
                    FileLoader::Get()->ReadFileBatched(source_filename, cb);
                } else {
                    FileLoader::Get()->ReadFile(source_filename, cb, ReadFileFlags_SkipTextureLoader);
                }

                return;
            }

            // read the texture data
            stream.seekg(texture.m_Streams[stream_index].m_Offset);
            stream.read((char*)&out.front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER)
                            + (dxt10header ? sizeof(DDS_HEADER_DXT10) : 0),
                        texture.m_Streams[stream_index].m_Size);

            return callback(true, std::move(out));
        },
        ReadFileFlags_SkipTextureLoader);
}

bool FileLoader::ReadAVTX(FileBuffer* data, FileBuffer* outData) noexcept
{
    std::istringstream stream(std::string{(char*)data->data(), data->size()});

    // read the texture data
    jc::AvalancheTexture::Header texture;
    stream.read((char*)&texture, sizeof(texture));

    // ensure the header magic is correct
    if (texture.m_Magic != 0x58545641) {
        LOG_ERROR("Invalid header magic");
        return false;
    }

    DEBUG_TEXTURE_FLAGS(&texture);

    // find the best stream to use
    const auto stream_index = jc::AvalancheTexture::FindBest(&texture, 0);

    // find the rank
    const auto rank = jc::AvalancheTexture::GetHighestRank(&texture, stream_index);

    // generate the DDS header
    DDS_HEADER header{};
    header.size        = sizeof(DDS_HEADER);
    header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
    header.width       = texture.m_Width >> rank;
    header.height      = texture.m_Height >> rank;
    header.depth       = texture.m_Depth;
    header.mipMapCount = texture.m_Mips;
    header.ddspf       = TextureManager::GetPixelFormat(texture.m_Format);
    header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

    //
    bool dxt10header = (header.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));

    // resize the output buffer
    outData->resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + (dxt10header ? sizeof(DDS_HEADER_DXT10) : 0)
                    + texture.m_Streams[stream_index].m_Size);

    // write the DDS magic and header
    std::memcpy(&outData->front(), (char*)&DDS_MAGIC, sizeof(DDS_MAGIC));
    std::memcpy(&outData->front() + sizeof(DDS_MAGIC), (char*)&header, sizeof(DDS_HEADER));

    // write the DXT10 header
    if (dxt10header) {
        DDS_HEADER_DXT10 dx10header{};
        dx10header.dxgiFormat        = texture.m_Format;
        dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
        dx10header.arraySize         = 1;

        std::memcpy(&outData->front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), (char*)&dx10header,
                    sizeof(DDS_HEADER_DXT10));
    }

    // read the texture data
    stream.seekg(texture.m_Streams[stream_index].m_Offset);
    stream.read((char*)&outData->front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER)
                    + (dxt10header ? sizeof(DDS_HEADER_DXT10) : 0),
                texture.m_Streams[stream_index].m_Size);

    return true;
}

void FileLoader::ReadHMDDSC(FileBuffer* data, FileBuffer* outData) noexcept
{
    assert(data && data->size() > 0);

    DDS_HEADER header{};
    header.size        = sizeof(DDS_HEADER);
    header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
    header.width       = 512;
    header.height      = 512;
    header.depth       = 1;
    header.mipMapCount = 1;
    header.ddspf       = TextureManager::GetPixelFormat(DXGI_FORMAT_BC1_UNORM);
    header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

    outData->resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + data->size());

    std::memcpy(&outData->front(), (char*)&DDS_MAGIC, sizeof(DDS_MAGIC));
    std::memcpy(&outData->front() + sizeof(DDS_MAGIC), (char*)&header, sizeof(DDS_HEADER));
    std::memcpy(&outData->front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), data->data(), data->size());
}

bool FileLoader::WriteAVTX(Texture* texture, FileBuffer* outData) noexcept
{
    if (!texture || !texture->IsLoaded()) {
        return false;
    }

    jc::AvalancheTexture::Header header;

    auto tex_buffer = texture->GetBuffer();
    auto tex_desc   = texture->GetDesc();

    // calculate the offset to skip
    const auto dds_header_size = (sizeof(DDS_MAGIC) + sizeof(DDS_HEADER));

    header.m_Dimension    = 2;
    header.m_Format       = tex_desc->Format;
    header.m_Width        = tex_desc->Width;
    header.m_Height       = tex_desc->Height;
    header.m_Depth        = 1;
    header.m_Flags        = 0;
    header.m_Mips         = tex_desc->MipLevels;
    header.m_MipsRedisent = tex_desc->MipLevels;

    // set the stream
    header.m_Streams[0].m_Offset    = sizeof(header);
    header.m_Streams[0].m_Size      = (tex_buffer->size() - dds_header_size);
    header.m_Streams[0].m_Alignment = 16;
    header.m_Streams[0].m_IsTile    = false;
    header.m_Streams[0].m_Source    = 0;

    outData->resize(sizeof(header) + header.m_Streams[0].m_Size);
    std::memcpy(&outData->front(), (char*)&header, sizeof(header));
    std::memcpy(&outData->front() + sizeof(header), (char*)tex_buffer->data() + dds_header_size,
                header.m_Streams[0].m_Size);

    return true;
}

std::shared_ptr<RuntimeContainer> FileLoader::ParseRuntimeContainer(const std::filesystem::path& filename,
                                                                    const FileBuffer&            buffer) noexcept
{
    std::istringstream stream(std::string{(char*)buffer.data(), buffer.size()});

    // read the runtimer container header
    jc::RuntimeContainer::Header header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "RTPC", 4) != 0) {
        LOG_ERROR("Invalid header magic");
        return nullptr;
    }

    LOG_INFO("RTPC v{}", header.m_Version);

    // read the root node
    jc::RuntimeContainer::Node rootNode;
    stream.read((char*)&rootNode, sizeof(rootNode));

    // create the root container
    auto root_container = RuntimeContainer::make(rootNode.m_NameHash, filename);

    std::queue<std::tuple<RuntimeContainer*, jc::RuntimeContainer::Node>>             instanceQueue;
    std::queue<std::tuple<RuntimeContainerProperty*, jc::RuntimeContainer::Property>> propertyQueue;

    instanceQueue.push({root_container.get(), rootNode});

    while (!instanceQueue.empty()) {
        const auto& [current_container, item] = instanceQueue.front();

        stream.seekg(item.m_DataOffset);

        // read all the node properties
        for (uint16_t i = 0; i < item.m_PropertyCount; ++i) {
            jc::RuntimeContainer::Property prop;
            stream.read((char*)&prop, sizeof(prop));

            if (current_container) {
                auto container_property = new RuntimeContainerProperty{prop.m_NameHash, prop.m_Type};
                current_container->AddProperty(container_property);

                propertyQueue.push({container_property, prop});
            }
        }

        // seek to our current pos with 4-byte alignment
        stream.seekg(jc::ALIGN_TO_BOUNDARY(stream.tellg()));

        // read all the node instances
        for (uint16_t i = 0; i < item.m_InstanceCount; ++i) {
            jc::RuntimeContainer::Node node;
            stream.read((char*)&node, sizeof(node));

            auto next_container = new RuntimeContainer{node.m_NameHash};
            if (current_container) {
                current_container->AddContainer(next_container);
            }

            instanceQueue.push({next_container, node});
        }

        instanceQueue.pop();
    }

    // TODO: refactor this

    // grab all the property values
    while (!propertyQueue.empty()) {
        const auto& [current_property, prop] = propertyQueue.front();

        // read each type
        const auto type = current_property->GetType();
        switch (type) {
            // integer
            case RTPC_TYPE_INTEGER: {
                current_property->SetValue(*(int32_t*)&prop.m_DataOffset);
                break;
            }

            // float
            case RTPC_TYPE_FLOAT: {
                current_property->SetValue(*(float*)&prop.m_DataOffset);
                break;
            }

            // string
            case RTPC_TYPE_STRING: {
                stream.seekg(prop.m_DataOffset);

                std::string buffer;
                std::getline(stream, buffer, '\0');

                current_property->SetValue(buffer);
                break;
            }

            // vec2, vec3, vec4, mat4x4
            case RTPC_TYPE_VEC2:
            case RTPC_TYPE_VEC3:
            case RTPC_TYPE_VEC4:
            case RTPC_TYPE_MAT4X4: {
                stream.seekg(prop.m_DataOffset);

                if (type == RTPC_TYPE_VEC2) {
                    glm::vec2 result;
                    stream.read((char*)&result, sizeof(result));
                    current_property->SetValue(result);
                } else if (type == RTPC_TYPE_VEC3) {
                    glm::vec3 result;
                    stream.read((char*)&result, sizeof(result));
                    current_property->SetValue(result);
                } else if (type == RTPC_TYPE_VEC4) {
                    glm::vec4 result;
                    stream.read((char*)&result, sizeof(result));
                    current_property->SetValue(result);
                } else if (type == RTPC_TYPE_MAT4X4) {
                    glm::mat4x4 result;
                    stream.read((char*)&result, sizeof(result));
                    current_property->SetValue(result);
                }

                break;
            }

            // lists
            case RTPC_TYPE_LIST_INTEGERS:
            case RTPC_TYPE_LIST_FLOATS:
            case RTPC_TYPE_LIST_BYTES: {
                stream.seekg(prop.m_DataOffset);

                int32_t count;
                stream.read((char*)&count, sizeof(count));

                if (type == RTPC_TYPE_LIST_INTEGERS) {
                    std::vector<int32_t> result;
                    result.resize(count);
                    stream.read((char*)result.data(), (count * sizeof(int32_t)));

                    current_property->SetValue(result);
                } else if (type == RTPC_TYPE_LIST_FLOATS) {
                    std::vector<float> result;
                    result.resize(count);
                    stream.read((char*)result.data(), (count * sizeof(float)));

                    current_property->SetValue(result);
                } else if (type == RTPC_TYPE_LIST_BYTES) {
                    std::vector<uint8_t> result;
                    result.resize(count);
                    stream.read((char*)result.data(), count);

                    current_property->SetValue(result);
                }

                break;
            }

            // objectid
            case RTPC_TYPE_OBJECT_ID: {
                stream.seekg(prop.m_DataOffset);

                uint32_t key, value;
                stream.read((char*)&key, sizeof(key));
                stream.read((char*)&value, sizeof(value));

                current_property->SetValue(std::make_pair(key, value));
                break;
            }

            // events
            case RTPC_TYPE_EVENTS: {
                stream.seekg(prop.m_DataOffset);

                int32_t count;
                stream.read((char*)&count, sizeof(count));

                std::vector<std::pair<uint32_t, uint32_t>> result;
                result.resize(count);

                for (int32_t i = 0; i < count; ++i) {
                    uint32_t key, value;
                    stream.read((char*)&key, sizeof(key));
                    stream.read((char*)&value, sizeof(value));

                    result.emplace_back(std::make_pair(key, value));
                }

                current_property->SetValue(result);
                break;
            }

            default: {
#ifdef DEBUG
                __debugbreak();
#endif
                break;
            }
        }

        propertyQueue.pop();
    }

    // generate container names if needed
    root_container->GenerateBetterNames();

    return root_container;
}

std::shared_ptr<AvalancheDataFormat> FileLoader::ReadAdf(const std::filesystem::path& filename) noexcept
{
    if (!std::filesystem::exists(filename)) {
        LOG_ERROR("Input file doesn't exist!");
        return nullptr;
    }

    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        LOG_ERROR("Failed to open file stream!");
        return nullptr;
    }

    const auto size = std::filesystem::file_size(filename);

    FileBuffer data;
    data.resize(size);
    stream.read((char*)data.data(), size);
    stream.close();

    // parse the adf file
    auto adf = AvalancheDataFormat::make(filename);
    if (!adf->Parse(data)) {
        LOG_ERROR("Failed to parse ADF!");
        return nullptr;
    }

    return std::move(adf);
}

std::tuple<StreamArchive_t*, StreamArchiveEntry_t>
FileLoader::GetStreamArchiveFromFile(const std::filesystem::path& file, StreamArchive_t* archive) noexcept
{
    if (archive) {
        for (const auto& entry : archive->m_Files) {
            if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                return {archive, entry};
            }
        }
    } else {
        std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
        for (const auto& arc : AvalancheArchive::Instances) {
            const auto stream_arc = arc.second->GetStreamArchive();
            if (stream_arc) {
                for (const auto& entry : stream_arc->m_Files) {
                    if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                        return {stream_arc, entry};
                    }
                }
            }
        }
    }

    return {nullptr, StreamArchiveEntry_t{}};
}

bool FileLoader::ReadArchiveTable(const std::filesystem::path& filename, std::vector<TabFileEntry>* output,
                                  std::vector<jc::ArchiveTable::VfsTabCompressedBlock>* output_blocks) noexcept
{
    std::ifstream stream(filename, std::ios::ate | std::ios::binary);
    if (stream.fail()) {
        LOG_ERROR("Failed to open file stream!");
        return false;
    }

    // NOTE: TabFileHeader m_Version didn't change

    if (g_IsJC4Mode) {
        const auto length = stream.tellg();
        stream.seekg(std::ios::beg);

        jc4::ArchiveTable::TabFileHeader header;
        stream.read((char*)&header, sizeof(header));

        if (header.m_Magic != 0x424154) {
            LOG_ERROR("Invalid header magic");
            return false;
        }

        // read compressed blocked
        uint32_t _count = 0;
        stream.read((char*)&_count, sizeof(_count));
        output_blocks->resize(_count);
        stream.read((char*)output_blocks->data(), (sizeof(jc::ArchiveTable::VfsTabCompressedBlock) * _count));

        while (static_cast<int32_t>(stream.tellg()) + 20 <= length) {
            jc4::ArchiveTable::VfsTabEntry entry;
            stream.read((char*)&entry, sizeof(entry));

            output->emplace_back(std::move(entry));
        }
    } else {
        stream.seekg(std::ios::beg);

        jc3::ArchiveTable::TabFileHeader header;
        stream.read((char*)&header, sizeof(header));

        if (header.m_Magic != 0x424154) {
            LOG_ERROR("Invalid header magic");
            return false;
        }

        while (!stream.eof()) {
            jc3::ArchiveTable::VfsTabEntry entry;
            stream.read((char*)&entry, sizeof(entry));

            // prevent the entry being added 2 times when we get to the null character at the end
            // failbit will be set because the stream can only read 1 more byte
            if (stream.fail()) {
                break;
            }

            output->emplace_back(std::move(entry));
        }
    }

    stream.close();
    return true;
}

bool FileLoader::ReadArchiveTableEntry(const std::filesystem::path& table, const std::filesystem::path& filename,
                                       TabFileEntry*                                         output,
                                       std::vector<jc::ArchiveTable::VfsTabCompressedBlock>* output_blocks) noexcept
{
    auto namehash = hashlittle(filename.generic_string().c_str());
    return ReadArchiveTableEntry(table, namehash, output, output_blocks);
}

bool FileLoader::ReadArchiveTableEntry(const std::filesystem::path& table, uint32_t name_hash, TabFileEntry* output,
                                       std::vector<jc::ArchiveTable::VfsTabCompressedBlock>* output_blocks) noexcept
{
    std::vector<TabFileEntry> entries;
    if (ReadArchiveTable(table, &entries, output_blocks)) {
        auto it = std::find_if(entries.begin(), entries.end(),
                               [&](const TabFileEntry& entry) { return entry.m_NameHash == name_hash; });

        if (it != entries.end()) {
            *output = (*it);
            return true;
        }
    }

    return false;
}

bool FileLoader::ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash,
                                     FileBuffer* output) noexcept
{
    std::filesystem::path jc_directory = Window::Get()->GetJustCauseDirectory();
    const auto            tab_file     = jc_directory / directory / (archive + ".tab");
    const auto            arc_file     = jc_directory / directory / (archive + ".arc");

    // ensure the tab file exists
    if (!std::filesystem::exists(tab_file)) {
        LOG_ERROR("Can't find TAB file (\"{}\")", tab_file.string());
        return false;
    }

    // ensure the arc file exists
    if (!std::filesystem::exists(arc_file)) {
        LOG_ERROR("Can't find ARC file (\"{}\")", arc_file.string());
        return false;
    }

    TabFileEntry                                         entry;
    std::vector<jc::ArchiveTable::VfsTabCompressedBlock> blocks;
    if (ReadArchiveTableEntry(tab_file, namehash, &entry, &blocks)) {
        LOG_INFO("NameHash={}, Offset={}, CompressedSize={}, UncompressedSize={}, CompressionType={}, "
                 "CompressedBlockIndex={}",
                 namehash, entry.m_Offset, entry.m_CompressedSize, entry.m_UncompressedSize, entry.m_CompressionType,
                 entry.m_CompressedBlockIndex);

        // read the arc file
        std::ifstream stream(arc_file, std::ios::ate | std::ios::binary);
        if (stream.fail()) {
            LOG_ERROR("Failed to open file stream!");
            return false;
        }

        g_ArchiveLoadCount++;
        LOG_INFO("Current archive load count: {}", g_ArchiveLoadCount);

        //
        stream.seekg(entry.m_Offset);

        // file is compressed
        if (entry.m_CompressedSize != entry.m_UncompressedSize
            && entry.m_CompressionType != jc4::ArchiveTable::CompressionType_None) {
            LOG_WARN("File is compressed with {}.",
                     (static_cast<jc4::ArchiveTable::CompressionType>(entry.m_CompressionType)
                              == jc4::ArchiveTable::CompressionType_Zlib
                          ? "zlib"
                          : "oodle"));

            assert(entry.m_CompressionType == jc4::ArchiveTable::CompressionType_Oodle);

            // doesn't have compression blocks
            if (entry.m_CompressedBlockIndex == 0) {
                FileBuffer data;
                data.resize(entry.m_CompressedSize);

                // read the file from the archive
                stream.read((char*)data.data(), data.size());

                // uncompress the data
                FileBuffer uncompressed_buffer;
                uncompressed_buffer.resize(entry.m_UncompressedSize);
                const auto size = oo2::OodleLZ_Decompress(&data, &uncompressed_buffer);

                LOG_INFO("OodleLZ_Decompress={}", size);

                if (size != entry.m_UncompressedSize) {
                    LOG_ERROR("Failed to decompress the file.");
                    return false;
                }

                *output = std::move(uncompressed_buffer);
                return true;
            } else {
                FileBuffer data;
                data.resize(entry.m_UncompressedSize);

                auto     block_index        = entry.m_CompressedBlockIndex;
                auto     total_compressed   = entry.m_CompressedSize;
                uint32_t total_uncompressed = 0;
                while (total_compressed > 0) {
                    const auto& block = blocks[block_index];

                    // read the compressed block
                    FileBuffer compressedBlock;
                    compressedBlock.resize(block.m_CompressedSize);
                    stream.read((char*)compressedBlock.data(), block.m_CompressedSize);

                    const auto size =
                        oo2::OodleLZ_Decompress(compressedBlock.data(), block.m_CompressedSize,
                                                data.data() + total_uncompressed, block.m_UncompressedSize);
                    if (size != block.m_UncompressedSize) {
                        LOG_ERROR("Failed to decompress the block.");
                        return false;
                    }

                    //
                    total_compressed -= block.m_CompressedSize;
                    total_uncompressed += block.m_UncompressedSize;
                    block_index++;
                }

                LOG_INFO("OodleLZ_Decompress={}", total_uncompressed);

                *output = std::move(data);
                return output->size() > 0;
            }
        } else {
            assert(entry.m_CompressedSize != 0);

            FileBuffer data;
            data.resize(entry.m_CompressedSize);

            // read the file from the archive
            stream.read((char*)data.data(), data.size());

            *output = std::move(data);
            return output->size() > 0;
        }
    }

    return false;
}

bool FileLoader::HasFileInDictionary(uint32_t name_hash) noexcept
{
    return m_Dictionary.find(name_hash) != m_Dictionary.end();
}

DictionaryLookupResult FileLoader::LocateFileInDictionary(const std::filesystem::path& filename) noexcept
{
    auto name_hash = hashlittle(filename.generic_string().c_str());
    return LocateFileInDictionary(name_hash);
}

DictionaryLookupResult FileLoader::LocateFileInDictionary(uint32_t name_hash) noexcept
{
    auto it = m_Dictionary.find(name_hash);
    if (it == m_Dictionary.end()) {
        return {};
    }

    const auto& file = (*it);
    const auto& path = file.second.second.at(0);
    const auto  pos  = path.find_last_of("/");

    return {path.substr(0, pos), path.substr(pos + 1, path.length()), file.first};
}

DictionaryLookupResult FileLoader::LocateFileInDictionaryByPartOfName(const std::filesystem::path& filename) noexcept
{
    const auto& _filename = filename.generic_string();

    auto it =
        std::find_if(m_Dictionary.begin(), m_Dictionary.end(),
                     [&](const std::pair<uint32_t, std::pair<std::filesystem::path, std::vector<std::string>>>& item) {
                         const auto& item_fn = item.second.first;
                         return item_fn == _filename
                                || (item_fn.generic_string().find(_filename) != std::string::npos
                                    && item_fn.extension() == filename.extension());
                     });

    if (it == m_Dictionary.end()) {
        return {};
    }

    const auto& file = (*it);
    const auto& path = file.second.second.at(0);
    const auto  pos  = path.find_last_of("/");

    return {path.substr(0, pos), path.substr(pos + 1, path.length()), file.first};
}

void FileLoader::RegisterReadCallback(const std::vector<std::string>& extensions, FileTypeCallback fn)
{
    for (const auto& extension : extensions) {
        m_FileTypeCallbacks[extension].emplace_back(fn);
    }
}

void FileLoader::RegisterSaveCallback(const std::vector<std::string>& extensions, FileSaveCallback fn)
{
    for (const auto& extension : extensions) {
        m_SaveFileCallbacks[extension].emplace_back(fn);
    }
}
