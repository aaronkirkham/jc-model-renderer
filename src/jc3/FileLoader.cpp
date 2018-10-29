#include <jc3/FileLoader.h>
#include <jc3/RenderBlockFactory.h>
#include <graphics/DDSTextureLoader.h>
#include <graphics/TextureManager.h>
#include <Window.h>
#include <graphics/UI.h>

#include <thread>
#include <istream>
#include <queue>

#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RuntimeContainer.h>

#include <import_export/ImportExportManager.h>

#include <zlib.h>
#include <fnv1.h>
#include <jc3/hashlittle.h>

// Credit to gibbed for most of the file formats
// http://gib.me

extern fs::path g_JC3Directory;
static uint64_t g_ArchiveLoadCount = 0;

std::unordered_map<uint32_t, std::string> NameHashLookup::LookupTable;

FileLoader::FileLoader()
{
    m_FileList = std::make_unique<DirectoryList>();
    const auto status_text_id = UI::Get()->PushStatusText("Loading dictionary...");

    std::thread([this, status_text_id] {
        try {
            const auto handle = GetModuleHandle(nullptr);
            const auto rc = FindResource(handle, MAKEINTRESOURCE(128), RT_RCDATA);
            if (rc == nullptr) {
                throw std::runtime_error("FileLoader - Failed to find dictionary resource");
            }

            const auto data = LoadResource(handle, rc);
            if (data == nullptr) {
                throw std::runtime_error("FileLoader - Failed to load dictionary resource");
            }

            // parse the file list json
            auto str = static_cast<const char*>(LockResource(data));
            auto& dictionary = json::parse(str);
            m_FileList->Parse(&dictionary);

            // parse the dictionary
            for (auto& it = dictionary.begin(); it != dictionary.end(); ++it) {
                const auto& key = it.key();
                const auto& data = it.value();

                const auto namehash = static_cast<uint32_t>(std::stoul(data["hash"].get<std::string>(), nullptr, 16));

                std::vector<std::string> paths;
                for (const auto& path : data["path"]) {
                    paths.emplace_back(path.get<std::string>());
                }

                m_Dictionary[namehash] = std::make_pair(key, paths);
            }
        }
        catch (const std::exception& e) {
            DEBUG_LOG(e.what());

            std::stringstream error;
            error << "Failed to read/parse file list dictionary.\n\nSome features will be disabled." << std::endl << std::endl;
            error << e.what();

            Window::Get()->ShowMessageBox(error.str());
        }

        UI::Get()->PopStatusText(status_text_id);
    }).detach();

    // init the namehash lookup table
    NameHashLookup::Init();

    // TODO: move the events to a better location?

    // handle file drops
    Window::Get()->Events().UnhandledDragDropped.connect([&](const fs::path& file) {
        // do we have a registered callback for this file type?
        if (m_FileTypeCallbacks.find(file.extension().string()) != m_FileTypeCallbacks.end()) {
            const auto size = fs::file_size(file);

            FileBuffer data;
            data.resize(size);

            // read the file
            std::ifstream stream(file, std::ios::binary);
            assert(!stream.fail());
            stream.read((char *)data.data(), size);
            stream.close();

            // run the callback handlers
            for (const auto& fn : m_FileTypeCallbacks[file.extension().string()]) {
                fn(file, data, true);
            }
        }
        else {
            std::stringstream info;
            info << "I don't know how to read the \"" << file.extension() << "\" extension." << std::endl << std::endl;
            info << "Want to help? Check out our GitHub page for information on how to contribute.";
            Window::Get()->ShowMessageBox(info.str(), MB_ICONASTERISK);

            DEBUG_LOG("[ERROR] Unknown file type \"" << file << "\".");
        }
    });

    // trigger the file type callbacks
    UI::Get()->Events().FileTreeItemSelected.connect([&](const fs::path& file, AvalancheArchive* archive) {
        // do we have a registered callback for this file type?
        if (m_FileTypeCallbacks.find(file.extension().string()) != m_FileTypeCallbacks.end()) {
            ReadFile(file, [&, file](bool success, FileBuffer data) {
                if (success) {
                    for (const auto& fn : m_FileTypeCallbacks[file.extension().string()]) {
                        fn(file, data, false);
                    }
                }
                else {
                    std::stringstream error;
                    error << "Failed to load \"" << file.filename() << "\"." << std::endl << std::endl;
                    error << "Make sure you have selected the correct Just Cause 3 directory.";
                    Window::Get()->ShowMessageBox(error.str());

                    DEBUG_LOG("[ERROR] Failed to load \"" << file << "\".");
                }
            });
        }
        else {
            std::stringstream info;
            info << "I don't know how to read the \"" << file.extension() << "\" extension." << std::endl << std::endl;
            info << "Want to help? Check out our GitHub page for information on how to contribute.";
            Window::Get()->ShowMessageBox(info.str(), MB_ICONASTERISK);

            DEBUG_LOG("[ERROR] Unknown file type \"" << file.extension() << "\". (" << file << ")");
        }
    });

    // save file
    UI::Get()->Events().SaveFileRequest.connect([&](const fs::path& file, const fs::path& directory) {
        DEBUG_LOG("SaveFileRequest - want to save file \"" << file << "\" to \"" << directory << "\"..");
        bool was_handled = false;
        // try use a handler
        if (m_SaveFileCallbacks.find(file.extension().string()) != m_SaveFileCallbacks.end()) {
            for (const auto& fn : m_SaveFileCallbacks[file.extension().string()]) {
                was_handled = fn(file, directory);
            }
        }

        // no handlers, fallback to just reading the raw data
        if (!was_handled) {
            FileLoader::Get()->ReadFile(file, [&, file, directory](bool success, FileBuffer data) {
                if (success) {
                    const auto& path = directory / file.filename();

                    // write the file data
                    std::ofstream stream(path, std::ios::binary);
                    if (!stream.fail()) {
                        stream.write((char *)data.data(), data.size());
                        stream.close();
                        return;
                    }
                }

                std::stringstream error;
                error << "Failed to save \"" << file.filename() << "\".";
                Window::Get()->ShowMessageBox(error.str());

                DEBUG_LOG("[ERROR] " << error.str());
            });
        }
    });

    // export file
    UI::Get()->Events().ExportFileRequest.connect([&](const fs::path& file, IImportExporter* exporter) {
        Window::Get()->ShowFolderSelection("Select a folder to export the file to.", [&, file, exporter](const fs::path& selected) {
            DEBUG_LOG("ExportFileRequest - want to export file '" << file << "' to '" << selected << "'");

            auto _exporter = exporter;
            if (!exporter) {
                DEBUG_LOG("ExportFileRequest - Finding a suitable exporter for '" << file.extension() << "'...");

                const auto& exporters = ImportExportManager::Get()->GetExportersForExtension(file.extension().string());
                if (exporters.size() > 0) {
                    DEBUG_LOG("ExportFileRequest - Using exporter '" << exporters.at(0)->GetName() << "'");

                    _exporter = exporters.at(0);
                }
            }

            // if we have a valid exporter, read the file and export it
            if (_exporter) {
                std::stringstream status_text;
                status_text << "Exporting \"" << file << "\"...";
                const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

                std::thread([&, file, selected, status_text_id] {
                    _exporter->Export(file, selected, [&, status_text_id](bool success) {
                        UI::Get()->PopStatusText(status_text_id);

                        if (!success) {
                            std::stringstream error;
                            error << "Failed to export \"" << file.filename() << "\".";
                            Window::Get()->ShowMessageBox(error.str());

                            DEBUG_LOG("[ERROR] " << error.str());
                        }
                    });
                }).detach();
            }
        });
    });
}

void FileLoader::ReadFile(const fs::path& filename, ReadFileCallback callback, uint8_t flags) noexcept
{
    uint64_t status_text_id = 0;

    if (!UseBatches) {
        std::stringstream status_text;
        status_text << "Reading \"" << filename << "\"...";
        status_text_id = UI::Get()->PushStatusText(status_text.str());
    }

    // are we trying to load textures?
    if (filename.extension() == ".dds" || filename.extension() == ".ddsc" || filename.extension() == ".hmddsc") {
        const auto& texture = TextureManager::Get()->GetTexture(filename, TextureManager::NO_CREATE);
        if (texture && texture->IsLoaded()) {
            UI::Get()->PopStatusText(status_text_id);
            return callback(true, texture->GetBuffer());
        }

        // try load the texture
        if (!(flags & SKIP_TEXTURE_LOADER)) {
            UI::Get()->PopStatusText(status_text_id);
            return ReadTexture(filename, callback);
        }
    }

    // check any loaded archives for the file
    const auto&[archive, entry] = GetStreamArchiveFromFile(filename);
    if (archive && entry.m_Offset != 0 && entry.m_Offset != -1) {
        auto buffer = archive->GetEntryBuffer(entry);

        UI::Get()->PopStatusText(status_text_id);
        return callback(true, std::move(buffer));
    }
#ifdef DEBUG
    else if (archive && (entry.m_Offset == 0 || entry.m_Offset == -1)) {
        DEBUG_LOG("NOTE: \"" << filename.filename() << "\" exists in archive but has been patched. Reading the patched version instead.");
    }
#endif

    // should we use file batching?
    if (UseBatches) {
        DEBUG_LOG("FileLoader::ReadFile - Using batches for \"" << filename << "\"");
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

void FileLoader::ReadFileBatched(const fs::path& filename, ReadFileCallback callback) noexcept
{
    std::lock_guard<std::recursive_mutex> _lk{ m_BatchesMutex };
    m_PathBatches[filename.string()].emplace_back(callback);
}

void FileLoader::RunFileBatches() noexcept
{
    std::thread([&] {
        std::lock_guard<std::recursive_mutex> _lk{ m_BatchesMutex };

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

            DEBUG_LOG("FileLoader::RunFileBatches - will read " << c << " files from \"" << batch.first << "\".");
        }
#endif

        // iterate over all the archives
        for (const auto& batch : m_Batches) {
            const auto& archive_path = batch.first;
            const auto sep = archive_path.find_last_of("/");

            // split the directory and archive
            const auto& directory = archive_path.substr(0, sep);
            const auto& archive = archive_path.substr(sep + 1, archive_path.length());

            // get the path names
            const auto& arc_file = g_JC3Directory / directory / (archive + ".arc");
            const auto& tab_file = g_JC3Directory / directory / (archive + ".tab");

            // if the arc/tab files don't exist, get out now
            if (!fs::exists(tab_file) || !fs::exists(arc_file)) {
                DEBUG_LOG("FileLoader::RunFileBatches - can't find .arc/.tab file (jc3: " << g_JC3Directory << ", dir: " << directory << ", archive: " << archive << ")");
                continue;
            }

            // read the archive table
            JustCause3::ArchiveTable::VfsArchive table;
            if (ReadArchiveTable(tab_file, &table)) {
                // open the arc file
                std::ifstream stream(arc_file, std::ios::binary);
                if (stream.fail()) {
                    DEBUG_LOG("FileLoader::RunFileBatches - invalid arc file stream!");
                    continue;
                }

#ifdef DEBUG
                g_ArchiveLoadCount++;
                DEBUG_LOG("[PERF] Archive load count: " << g_ArchiveLoadCount);
#endif

                // iterate over all the files
                for (const auto& file : batch.second) {
                    bool has_found_file = false;

                    // locate the data for the file
                    for (const auto& entry : table.m_Entries) {
                        // is this the correct file hash?
                        if (entry.m_NameHash == file.first) {
                            has_found_file = true;

                            // resize the buffer
                            FileBuffer buffer;
                            buffer.resize(entry.m_Size);

                            // read the file data
                            stream.seekg(entry.m_Offset);
                            stream.read((char *)buffer.data(), entry.m_Size);

                            // trigger the callbacks
                            for (const auto& callback : file.second) {
                                callback(true, buffer);
                            }
                        }
                    }

                    // have we not found the file?
                    if (!has_found_file) {
                        DEBUG_LOG("FileLoader::RunFileBatches ERROR - didn't find \"" << file.first << "\".");

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

bool FileLoader::ReadArchiveTable(const fs::path& filename, JustCause3::ArchiveTable::VfsArchive* output) noexcept
{
    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("FileLoader::ReadArchiveTable - Failed to open stream");
        return false;
    }

    JustCause3::ArchiveTable::TabFileHeader header;
    stream.read((char *)&header, sizeof(header));

    if (header.m_Magic != 0x424154) {
        DEBUG_LOG("FileLoader::ReadArchiveTable - Invalid header magic. Input file probably isn't a TAB file.");
        return false;
    }

    output->m_Index = 0;
    output->m_Version = 2;

    while (!stream.eof())
    {
        JustCause3::ArchiveTable::VfsTabEntry entry;
        stream.read((char *)&entry, sizeof(entry));

        // prevent the entry being added 2 times when we get to the null character at the end
        // failbit will be set because the stream can only read 1 more byte
        if (stream.fail()) {
            break;
        }

        output->m_Entries.emplace_back(entry);
    }

    DEBUG_LOG("FileLoader::ReadArchiveTable - " << output->m_Entries.size() << " files in archive");

    stream.close();
    return true;
}

std::unique_ptr<StreamArchive_t> FileLoader::ParseStreamArchive(FileBuffer* sarc_buffer, FileBuffer* toc_buffer)
{
    std::istringstream stream(std::string{ (char *)sarc_buffer->data(), sarc_buffer->size() });

    JustCause3::StreamArchive::SARCHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "SARC", 4) != 0) {
        DEBUG_LOG("FileLoader::ParseStreamArchive - Invalid header magic. Input file probably isn't a StreamArchive file.");
        return nullptr;
    }

    auto result = std::make_unique<StreamArchive_t>();
    result->m_Header = header;
    result->m_UsingTOC = (toc_buffer != nullptr);

    // read the toc header for the filelist
    if (toc_buffer) {
        DEBUG_LOG("USING .toc BUFFER FOR ARCHIVE FILELIST");

        std::istringstream toc_stream(std::string{ (char *)toc_buffer->data(), toc_buffer->size() });
        while (static_cast<size_t>(toc_stream.tellg()) < toc_buffer->size()) {
            StreamArchiveEntry_t entry;

            uint32_t length;
            toc_stream.read((char *)&length, sizeof(length));

            auto filename = std::unique_ptr<char[]>(new char[length + 1]);
            toc_stream.read(filename.get(), length);
            filename[length] = '\0';

            entry.m_Filename = filename.get();

            toc_stream.read((char *)&entry.m_Offset, sizeof(entry.m_Offset));
            toc_stream.read((char *)&entry.m_Size, sizeof(entry.m_Size));

            result->m_Files.emplace_back(entry);
        }
    }
    // if we didn't pass a TOC buffer, stream the file list from the SARC header
    else {
        DEBUG_LOG("USING ORIGINAL FOR ARCHIVE FILELIST");

        auto start_pos = stream.tellg();
        while (true) {
            StreamArchiveEntry_t entry;

            uint32_t length;
            stream.read((char *)&length, sizeof(length));

            auto filename = std::unique_ptr<char[]>(new char[length + 1]);
            stream.read(filename.get(), length);
            filename[length] = '\0';

            entry.m_Filename = filename.get();

            stream.read((char *)&entry.m_Offset, sizeof(entry.m_Offset));
            stream.read((char *)&entry.m_Size, sizeof(entry.m_Size));

            result->m_Files.emplace_back(entry);

            auto current_pos = stream.tellg();
            if (header.m_Size - (current_pos - start_pos) <= 15) {
                break;
            }
        }
    }

    return result;
}

void FileLoader::ReadStreamArchive(const fs::path& filename, ReadArchiveCallback callback) noexcept
{
    if (!fs::exists(filename)) {
        DEBUG_LOG("FileLoader::ReadStreamArchive - Input file doesn't exist");
        return callback(nullptr);
    }

    std::ifstream compressed_buffer(filename, std::ios::binary);
    if (compressed_buffer.fail()) {
        DEBUG_LOG("FileLoader::ReadStreamArchive - Failed to open stream");
        return callback(nullptr);
    }

    // TODO: need to read the header, check if the archive is compressed,
    // then just back to the start of the stream!

    FileBuffer buffer;
    if (DecompressArchiveFromStream(compressed_buffer, &buffer)) {
        compressed_buffer.close();

        // parse the stream archive
        auto result = ParseStreamArchive(&buffer);
        if (result) {
            result->m_Filename = filename;
            result->m_SARCBytes = std::move(buffer);
            return callback(std::move(result));
        }
    }

    return callback(nullptr);
}

void FileLoader::ReadStreamArchive(const fs::path& filename, const FileBuffer& data, ReadArchiveCallback callback) noexcept
{
    auto toc_filename = filename;
    toc_filename += ".toc";
    ReadFile(toc_filename, [&, filename, toc_filename, compressed_data = std::move(data), callback](bool success, FileBuffer data) {
        // decompress the archive data
        std::istringstream compressed_buffer(std::string{ (char*)compressed_data.data(), compressed_data.size() });

        // TODO: read the magic and make sure it's actually compressed AAF

        // decompress the archive data
        FileBuffer buffer;
        if (DecompressArchiveFromStream(compressed_buffer, &buffer)) {
            // parse the stream archive
            auto archive = ParseStreamArchive(&buffer, success ? &data : nullptr);
            if (archive) {
                archive->m_Filename = filename;
                archive->m_SARCBytes = std::move(buffer);
            }

            return callback(std::move(archive));
        }

        return callback(nullptr);
    });
}

void FileLoader::CompressArchive(std::ostream& stream, JustCause3::AvalancheArchive::Header* header, std::vector<JustCause3::AvalancheArchive::Chunk>* chunks) noexcept
{
    // write the header
    stream.write((char *)header, sizeof(JustCause3::AvalancheArchive::Header));

    // write all the blocks
    for (uint32_t i = 0; i < header->m_ChunkCount; ++i) {
        auto& chunk = chunks->at(i);

        // compress the chunk data
        // NOTE: 2 + 4 for the compression header & checksum
        // no way to tell zlib not to write them??
        auto decompressed_size = (uLong)chunk.m_UncompressedSize;
        auto compressed_size = compressBound(decompressed_size);

        DEBUG_LOG("data size: " << chunk.m_DataSize);
        DEBUG_LOG("expected compresed size: " << chunk.m_CompressedSize);

        FileBuffer result;
        result.reserve(compressed_size);
        auto res = compress(result.data(), &compressed_size, chunk.m_BlockData.data(), decompressed_size);
        assert(res == Z_OK);

        // store the compressed size
        chunk.m_CompressedSize = static_cast<uint32_t>(compressed_size) - 6; // remove the header and checksum from the total size
        DEBUG_LOG("actual compressed size: " << chunk.m_CompressedSize);

        // calculate the distance to the 16-byte boundary after we write our data
        auto pos = static_cast<uint32_t>(stream.tellp()) + JustCause3::AvalancheArchive::CHUNK_SIZE + chunk.m_CompressedSize;
        auto padding = JustCause3::DISTANCE_TO_BOUNDARY(pos, 16);

        // generate the data size
        chunk.m_DataSize = (JustCause3::AvalancheArchive::CHUNK_SIZE + chunk.m_CompressedSize + padding);
        DEBUG_LOG("data size: " << chunk.m_DataSize);

        // write the chunk
        stream.write((char *)&chunk.m_CompressedSize, sizeof(uint32_t));
        stream.write((char *)&chunk.m_UncompressedSize, sizeof(uint32_t));
        stream.write((char *)&chunk.m_DataSize, sizeof(uint32_t));
        stream.write((char *)&chunk.m_Magic, sizeof(uint32_t));

        // ignore the header when writing the data
        stream.write((char *)result.data() + 2, chunk.m_CompressedSize);

        // write the padding
        DEBUG_LOG("writing " << padding << " bytes of padding...");
        static uint32_t PADDING_BYTE = 0x30;
        for (decltype(padding) i = 0; i < padding; ++i) {
            stream.write((char *)&PADDING_BYTE, 1);
        }
    }
}

void FileLoader::CompressArchive(std::ostream& stream, StreamArchive_t* archive) noexcept
{
    assert(archive);

    auto& block_data = archive->m_SARCBytes;
    const auto num_chunks = 1 + static_cast<uint32_t>(block_data.size() / JustCause3::AvalancheArchive::MAX_CHUNK_DATA_SIZE);
    constexpr auto max_chunk_size = JustCause3::AvalancheArchive::MAX_CHUNK_DATA_SIZE;

    // generate the header
    JustCause3::AvalancheArchive::Header header;
    header.m_TotalUncompressedSize = archive->m_SARCBytes.size();
    header.m_UncompressedBufferSize = num_chunks > 1 ? max_chunk_size : archive->m_SARCBytes.size();
    header.m_ChunkCount = num_chunks;

    DEBUG_LOG("CompressArchive");
    DEBUG_LOG(" - m_ChunkCount=" << header.m_ChunkCount);
    DEBUG_LOG(" - TotalUncompressedSize=" << header.m_TotalUncompressedSize);
    DEBUG_LOG(" - UncompressedBufferSize=" << header.m_UncompressedBufferSize);

    // generate the chunks
    std::vector<JustCause3::AvalancheArchive::Chunk> chunks;
    uint32_t last_chunk_offset = 0;
    for (uint32_t i = 0; i < header.m_ChunkCount; ++i) {
        JustCause3::AvalancheArchive::Chunk chunk;

        // generate chunks if needed
        if (num_chunks > 1) {
            // calculate the chunk size
            const auto block_size = block_data.size() - last_chunk_offset;
            auto chunk_size = block_size > max_chunk_size ? max_chunk_size % block_size : block_size;

            std::vector<uint8_t> n_block_data(block_data.begin() + last_chunk_offset, block_data.begin() + last_chunk_offset + chunk_size);
            last_chunk_offset += chunk_size;

            chunk.m_UncompressedSize = chunk_size;
            chunk.m_BlockData = std::move(n_block_data);
        }
        // we only have a single chunk, use the total block size
        else {
            chunk.m_UncompressedSize = block_data.size();
            chunk.m_BlockData = block_data;
        }

        DEBUG_LOG("AAF chunk #" << i << ", UncompressedSize=" << chunk.m_UncompressedSize);

        chunks.emplace_back(std::move(chunk));
    }

    return CompressArchive(stream, &header, &chunks);
}

bool FileLoader::DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept
{
    // read the archive header
    JustCause3::AvalancheArchive::Header header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "AAF", 4) != 0) {
        DEBUG_LOG("FileLoader::DecompressArchiveFromStream - Invalid header magic. Input probably isn't an AvalancheArchiveFormat file.");
        return false;
    }

    DEBUG_LOG("AvalancheArchiveFormat v" << header.m_Version);
    DEBUG_LOG(" - m_ChunkCount=" << header.m_ChunkCount);
    DEBUG_LOG(" - m_TotalUncompressedSize=" << header.m_TotalUncompressedSize);
    DEBUG_LOG(" - m_UncompressedBufferSize=" << header.m_UncompressedBufferSize);

    output->reserve(header.m_TotalUncompressedSize);

    // read all the blocks
    for (uint32_t i = 0; i < header.m_ChunkCount; ++i) {
        auto base_position = stream.tellg();

        // read the chunk
        JustCause3::AvalancheArchive::Chunk chunk;
        stream.read((char *)&chunk.m_CompressedSize, sizeof(chunk.m_CompressedSize));
        stream.read((char *)&chunk.m_UncompressedSize, sizeof(chunk.m_UncompressedSize));
        stream.read((char *)&chunk.m_DataSize, sizeof(chunk.m_DataSize));
        stream.read((char *)&chunk.m_Magic, sizeof(chunk.m_Magic));

        DEBUG_LOG("AAF chunk #" << i << ", CompressedSize=" << chunk.m_CompressedSize << ", UncompressedSize=" << chunk.m_UncompressedSize << ", DataSize=" << chunk.m_DataSize);

        // make sure the block magic is correct
        if (chunk.m_Magic != 0x4D415745) {
            output->shrink_to_fit();

            DEBUG_LOG("FileLoader::DecompressArchiveFromStream - Invalid chunk magic.");
            return false;
        }

        // read the block data
        FileBuffer data;
        data.resize(chunk.m_CompressedSize);
        stream.read((char *)data.data(), chunk.m_CompressedSize);

        // decompress the block
        {
            uLong compressed_size = (uLong)chunk.m_CompressedSize;
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

void FileLoader::WriteTOC(const fs::path& filename, StreamArchive_t* archive) noexcept
{
    assert(archive);

    std::ofstream stream(filename, std::ios::binary);
    assert(!stream.fail());
    for (auto& entry : archive->m_Files) {
        auto length = static_cast<uint32_t>(entry.m_Filename.length());

        stream.write((char *)&length, sizeof(uint32_t));
        stream.write((char *)entry.m_Filename.c_str(), length);
        stream.write((char *)&entry.m_Offset, sizeof(uint32_t));
        stream.write((char *)&entry.m_Size, sizeof(uint32_t));
    }

    stream.close();
}

void FileLoader::ReadTexture(const fs::path& filename, ReadFileCallback callback) noexcept
{
    FileLoader::Get()->ReadFile(filename, [this, filename, callback](bool success, FileBuffer data) {
        using namespace JustCause3;

        if (!success) {
            DEBUG_LOG("ReadTexture -> failed to read \"" << filename << "\".");
            return callback(false, {});
        }

        // generic DDS handler
        if (filename.extension() == ".dds") {
            return callback(true, std::move(data));
        }

        std::istringstream stream(std::string{ (char *)data.data(), data.size() });

        // read the texture data
        AvalancheTexture::Header texture;
        stream.read((char *)&texture, sizeof(texture));

        // ensure the header magic is correct
        if (texture.m_Magic != 0x58545641) {
            DEBUG_LOG("ReadTexture -> failed to read \"" << filename << "\". (invalid magic)");
            return callback(false, {});
        }

#if 1
        {
            // clang-format off
            std::stringstream ss;
            ss << "AVTX m_Flags=" << texture.m_Flags << " (";

            if (texture.m_Flags & AvalancheTexture::STREAMED)       ss << "streamed, ";
            if (texture.m_Flags & AvalancheTexture::PLACEHOLDER)    ss << "placeholder, ";
            if (texture.m_Flags & AvalancheTexture::TILED)          ss << "tiled, ";
            if (texture.m_Flags & AvalancheTexture::SRGB)           ss << "SRGB, ";
            if (texture.m_Flags & AvalancheTexture::CUBE)           ss << "cube, ";
            if (texture.m_Flags == 0)                               ss << ", ";

            ss.seekp(-2, ss.cur);
            ss << ")";
            DEBUG_LOG(ss.str());
            // clang-format on
        }
#endif

        // find the best stream to use
        auto&[stream_index, load_source] = AvalancheTexture::FindBest(&texture);

        // find the rank
        const auto rank = AvalancheTexture::GetHighestRank(&texture, stream_index);

        FileBuffer out;
        out.resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + texture.m_Streams[stream_index].m_Size);

        // write the DDS header block to the output
        {
            DDS_HEADER header;
            ZeroMemory(&header, sizeof(header));
            header.size = sizeof(DDS_HEADER);
            header.flags = (0x1007 | 0x20000);
            header.width = texture.m_Width >> rank;
            header.height = texture.m_Height >> rank;
            header.depth = texture.m_Depth;
            header.mipMapCount = 1;
            header.ddspf = TextureManager::GetPixelFormat(texture.m_Format);
            header.caps = (8 | 0x1000);

            // write the magic and header
            std::memcpy(&out.front(), (char *)&DDS_MAGIC, sizeof(DDS_MAGIC));
            std::memcpy(&out.front() + sizeof(DDS_MAGIC), (char *)&header, sizeof(DDS_HEADER));
        }

        // TODO: convert texture to valid format.
        // the game will convert some textures if the surface format is over 0x3E8
        // this is the reason why some textures will fail to load (charactereyescube_v2/v3)

        // if we need to load the source file, request it
        if (load_source) {
            auto source_filename = filename;
            source_filename.replace_extension(".hmddsc");

            auto offset = texture.m_Streams[stream_index].m_Offset;
            auto size = texture.m_Streams[stream_index].m_Size;

            // read file callback
            auto cb = [&, source_filename, texture, offset, size, out, callback](bool success, FileBuffer data) mutable {
                if (success) {
                    std::memcpy(&out.front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), data.data() + offset, size);
                    return callback(true, std::move(out));
                }

                return callback(false, {});
            };

            if (FileLoader::UseBatches) {
                DEBUG_LOG("FileLoader::ReadTexture - Using batches for \"" << filename << "\"");
                FileLoader::Get()->ReadFileBatched(source_filename, cb);
            }
            else {
                FileLoader::Get()->ReadFile(source_filename, cb, SKIP_TEXTURE_LOADER);
            }

            return;
        }

        // read the texture data
        stream.seekg(texture.m_Streams[stream_index].m_Offset);
        stream.read((char *)out.data() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), texture.m_Streams[stream_index].m_Size);

        return callback(true, std::move(out));
    }, SKIP_TEXTURE_LOADER);
}

bool FileLoader::ParseCompressedTexture(FileBuffer* data, FileBuffer* outData) noexcept
{
    using namespace JustCause3;

    std::istringstream stream(std::string{ (char *)data->data(), data->size() });

    // read the texture data
    AvalancheTexture::Header texture;
    stream.read((char*)&texture, sizeof(texture));

    // ensure the header magic is correct
    if (texture.m_Magic != 0x58545641) {
        DEBUG_LOG("ParseCompressedTexture -> invalid magic");
        return false;
    }

    // find the best stream to use
    auto&[stream_index, load_source] = AvalancheTexture::FindBest(&texture, true);

    // find the rank
    const auto rank = AvalancheTexture::GetHighestRank(&texture, stream_index);

    //
    outData->resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + texture.m_Streams[stream_index].m_Size);

    // write the DDS header block to the output
    {
        DDS_HEADER header;
        ZeroMemory(&header, sizeof(header));
        header.size = sizeof(DDS_HEADER);
        header.flags = (0x1007 | 0x20000);
        header.width = texture.m_Width >> rank;
        header.height = texture.m_Height >> rank;
        header.depth = texture.m_Depth;
        header.mipMapCount = 1;
        header.ddspf = TextureManager::GetPixelFormat(texture.m_Format);
        header.caps = (8 | 0x1000);

        // write the magic and header
        std::memcpy(&outData->front(), (char *)&DDS_MAGIC, sizeof(DDS_MAGIC));
        std::memcpy(&outData->front() + sizeof(DDS_MAGIC), (char *)&header, sizeof(DDS_HEADER));
    }

    // read the texture data
    stream.seekg(texture.m_Streams[stream_index].m_Offset);
    stream.read((char *)outData->data() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), texture.m_Streams[stream_index].m_Size);

    return true;
}

void FileLoader::WriteRuntimeContainer(RuntimeContainer* runtime_container) noexcept
{
    std::ofstream stream("C:/users/aaron/desktop/rtpc.bin", std::ios::binary);
    assert(!stream.fail());

    assert(runtime_container);

    // generate the header
    JustCause3::RuntimeContainer::Header header;
    strncpy(header.m_Magic, "RTPC", 4);
    header.m_Version = 1;

    stream.write((char *)&header, sizeof(header));

    // get the inital write offset (header + root node)
    auto offset = static_cast<uint32_t>(sizeof(JustCause3::RuntimeContainer::Header) + sizeof(JustCause3::RuntimeContainer::Node));

    std::queue<RuntimeContainer*> instanceQueue;
    instanceQueue.push(runtime_container);

    while (!instanceQueue.empty()) {
        const auto& container = instanceQueue.front();

        const auto& properties = container->GetProperties();
        const auto& instances = container->GetContainers();

        // write the current node
        JustCause3::RuntimeContainer::Node _node;
        _node.m_NameHash = container->GetNameHash();
        _node.m_DataOffset = offset;
        _node.m_PropertyCount = properties.size();
        _node.m_InstanceCount = instances.size();

        stream.write((char *)&_node, sizeof(_node));

        auto _child_offset = offset + static_cast<uint32_t>(sizeof(JustCause3::RuntimeContainer::Property) * _node.m_PropertyCount);

        // calculate the property and instance write offsets
        const auto property_offset = offset;
        const auto child_offset = JustCause3::ALIGN_TO_BOUNDARY(_child_offset);
        const auto prop_data_offset = child_offset + (sizeof(JustCause3::RuntimeContainer::Node) * _node.m_InstanceCount);

        // write all the properties
#if 0
        stream.seekp(prop_data_offset);
        DEBUG_LOG("seek to " << prop_data_offset << " to write properties...");
        for (const auto& prop : properties) {
            if (prop->GetType() == PropertyType::RTPC_TYPE_INTEGER || prop->GetType() == PropertyType::RTPC_TYPE_FLOAT) {
            }
            else if (prop->GetType() == PropertyType::RTPC_TYPE_STRING) {
            }
            else {
            }

            JustCause3::RuntimeContainer::Property _prop;
            _prop.m_NameHash = prop->GetNameHash();
            _prop.m_Data = 0;
            _prop.m_Type = static_cast<uint8_t>(prop->GetType());

            stream.write((char *)&_prop, sizeof(_prop));
        }
#endif

        stream.seekp(property_offset);
        for (const auto& prop : properties) {
            if (prop->GetType() == PropertyType::RTPC_TYPE_INTEGER) {
                auto val = prop->GetValue<int32_t>();
                stream.write((char *)&val, sizeof(val));
            }
            else if (prop->GetType() == PropertyType::RTPC_TYPE_FLOAT) {
                auto val = prop->GetValue<float>();
                stream.write((char *)&val, sizeof(val));
            }
        }

        // queue all the child nodes
        for (const auto& child : instances) {
            instanceQueue.push(child);
        }

        instanceQueue.pop();
    }
}

std::shared_ptr<RuntimeContainer> FileLoader::ParseRuntimeContainer(const fs::path& filename, const FileBuffer& buffer) noexcept
{
    std::istringstream stream(std::string{ (char*)buffer.data(), buffer.size() });

    // read the runtimer container header
    JustCause3::RuntimeContainer::Header header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "RTPC", 4) != 0) {
        DEBUG_LOG("FileLoader::ParseRuntimeContainer - Invalid header magic. Input probably isn't a RuntimeContainer file.");
        return nullptr;
    }

    DEBUG_LOG("RuntimePropertyContainer v" << header.m_Version);

    // read the root node
    JustCause3::RuntimeContainer::Node rootNode;
    stream.read((char *)&rootNode, sizeof(rootNode));

    // create the root container
    auto root_container = RuntimeContainer::make(rootNode.m_NameHash, filename);

    std::queue<std::tuple<RuntimeContainer*, JustCause3::RuntimeContainer::Node>> instanceQueue;
    std::queue<std::tuple<RuntimeContainerProperty*, JustCause3::RuntimeContainer::Property>> propertyQueue;

    instanceQueue.push({ root_container.get(), rootNode });

    while (!instanceQueue.empty()) {
        const auto&[current_container, item] = instanceQueue.front();

        stream.seekg(item.m_DataOffset);

        // read all the node properties
        for (uint16_t i = 0; i < item.m_PropertyCount; ++i) {
            JustCause3::RuntimeContainer::Property prop;
            stream.read((char *)&prop, sizeof(prop));

            if (current_container) {
                auto container_property = new RuntimeContainerProperty{ prop.m_NameHash, prop.m_Type };
                current_container->AddProperty(container_property);

                propertyQueue.push({ container_property, prop });
            }
        }

        // seek to our current pos with 4-byte alignment
        stream.seekg(JustCause3::ALIGN_TO_BOUNDARY(stream.tellg()));

        // read all the node instances
        for (uint16_t i = 0; i < item.m_InstanceCount; ++i) {
            JustCause3::RuntimeContainer::Node node;
            stream.read((char *)&node, sizeof(node));

            auto next_container = new RuntimeContainer{ node.m_NameHash };
            if (current_container) {
                current_container->AddContainer(next_container);
            }

            instanceQueue.push({ next_container, node });
        }

        instanceQueue.pop();
    }

    // TODO: refactor this

    // grab all the property values
    while (!propertyQueue.empty()) {
        const auto&[current_property, prop] = propertyQueue.front();

        // read each type
        const auto& type = current_property->GetType();
        switch (type) {
        // integer
        case RTPC_TYPE_INTEGER: {
            current_property->SetValue(static_cast<int32_t>(prop.m_Data));
            break;
        }

        // float
        case RTPC_TYPE_FLOAT: {
            current_property->SetValue(static_cast<float>(prop.m_Data));
            break;
        }

        // string
        case RTPC_TYPE_STRING: {
            stream.seekg(prop.m_Data);

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
            stream.seekg(prop.m_Data);

            if (type == RTPC_TYPE_VEC2) {
                glm::vec2 result;
                stream.read((char *)&result, sizeof(result));
                current_property->SetValue(result);
            }
            else if (type == RTPC_TYPE_VEC3) {
                glm::vec3 result;
                stream.read((char *)&result, sizeof(result));
                current_property->SetValue(result);
            }
            else if (type == RTPC_TYPE_VEC4) {
                glm::vec4 result;
                stream.read((char *)&result, sizeof(result));
                current_property->SetValue(result);
            }
            else if (type == RTPC_TYPE_MAT4X4) {
                glm::mat4x4 result;
                stream.read((char *)&result, sizeof(result));
                current_property->SetValue(result);
            }

            break;
        }

        // lists
        case RTPC_TYPE_LIST_INTEGERS:
        case RTPC_TYPE_LIST_FLOATS:
        case RTPC_TYPE_LIST_BYTES: {
            stream.seekg(prop.m_Data);

            int32_t count;
            stream.read((char *)&count, sizeof(count));

            if (type == RTPC_TYPE_LIST_INTEGERS) {
                std::vector<int32_t> result;
                result.resize(count);
                stream.read((char *)result.data(), (count * sizeof(int32_t)));

                current_property->SetValue(result);
            }
            else if (type == RTPC_TYPE_LIST_FLOATS) {
                std::vector<float> result;
                result.resize(count);
                stream.read((char *)result.data(), (count * sizeof(float)));

                current_property->SetValue(result);
            }
            else if (type == RTPC_TYPE_LIST_BYTES) {
                std::vector<uint8_t> result;
                result.resize(count);
                stream.read((char *)result.data(), count);

                current_property->SetValue(result);
            }

            break;
        }

        // objectid
        case RTPC_TYPE_OBJECT_ID: {
            stream.seekg(prop.m_Data);

            uint32_t key, value;
            stream.read((char *)&key, sizeof(key));
            stream.read((char *)&value, sizeof(value));

            current_property->SetValue(std::make_pair(key, value));
            break;
        }

        // events
        case RTPC_TYPE_EVENTS: {
            stream.seekg(prop.m_Data);

            int32_t count;
            stream.read((char *)&count, sizeof(count));

            std::vector<std::pair<uint32_t, uint32_t>> result;
            result.resize(count);

            for (int32_t i = 0; i < count; ++i) {
                uint32_t key, value;
                stream.read((char *)&key, sizeof(key));
                stream.read((char *)&value, sizeof(value));

                result.emplace_back(std::make_pair(key, value));
            }
            break;
        }
        }

        propertyQueue.pop();
    }

    // generate container names if needed
    root_container->GenerateBetterNames();

    return root_container;
}

std::unique_ptr<AvalancheDataFormat> FileLoader::ReadAdf(const fs::path& filename) noexcept
{
    if (!fs::exists(filename)) {
        DEBUG_LOG("FileLoader::ReadAdf - Input file doesn't exist");
        return nullptr;
    }

    std::ifstream stream(filename, std::ios::binary | std::ios::ate);
    if (stream.fail()) {
        DEBUG_LOG("FileLoader::ReadAdf - Failed to open stream");
        return nullptr;
    }

    auto size = stream.tellg();
    stream.seekg(std::ios::beg);

    FileBuffer data;
    data.resize(size);
    stream.read((char *)data.data(), size);

    // parse the adf file
    auto adf = std::make_unique<AvalancheDataFormat>(filename);
    if (adf->Parse(data)) {
        return adf;
    }

    return nullptr;
}

std::tuple<StreamArchive_t*, StreamArchiveEntry_t> FileLoader::GetStreamArchiveFromFile(const fs::path& file, StreamArchive_t* archive) noexcept
{
    if (archive) {
        for (const auto& entry : archive->m_Files) {
            if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                return { archive, entry };
            }
        }
    }
    else {
        std::lock_guard<std::recursive_mutex> _lk{ AvalancheArchive::InstancesMutex };
        for (const auto& arc : AvalancheArchive::Instances) {
            const auto stream_arc = arc.second->GetStreamArchive();
            if (stream_arc) {
                for (const auto& entry : stream_arc->m_Files) {
                    if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                        return { stream_arc, entry };
                    }
                }
            }
        }
    }

    return { nullptr, StreamArchiveEntry_t{} };
}

bool FileLoader::ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash, FileBuffer* output) noexcept
{
    const auto& tab_file = g_JC3Directory / directory / (archive + ".tab");
    const auto& arc_file = g_JC3Directory / directory / (archive + ".arc");

    const auto arc_hash = fnv_1_32::hash(arc_file.string().c_str(), arc_file.string().length());

    DEBUG_LOG(tab_file);
    DEBUG_LOG(arc_file);

    // ensure the arc/tab file exists
    if (!fs::exists(tab_file) || !fs::exists(arc_file)) {
        DEBUG_LOG("FileLoader::ReadFileFromArchive - Can't find .arc/.tab file");
        return false;
    }

    // read the archive table
    JustCause3::ArchiveTable::VfsArchive archiveTable;
    if (ReadArchiveTable(tab_file.string(), &archiveTable)) {
        uint64_t offset = 0;
        uint64_t size = 0;

        // locate the data for the file
        for (const auto& entry : archiveTable.m_Entries) {
            if (entry.m_NameHash == namehash) {
                DEBUG_LOG("FileLoader::ReadFileFromArchive - Found file in archive at " << entry.m_Offset << " (size: " << entry.m_Size << " bytes)");

                // open the file stream
                std::ifstream stream(arc_file, std::ios::binary | std::ios::ate);
                if (stream.fail()) {
                    DEBUG_LOG("[ERROR] FileLoader::ReadFileFromArchive - Failed to open stream");
                    return false;
                }

                g_ArchiveLoadCount++;
                DEBUG_LOG("[PERF] Archive load count: " << g_ArchiveLoadCount);

                // TODO: caching

                stream.seekg(entry.m_Offset);
                output->resize(entry.m_Size);

                stream.read((char *)output->data(), entry.m_Size);
                stream.close();

                return true;
            }
        }
    }

#ifdef DEBUG
    __debugbreak();
#endif
    return false;
}

std::tuple<std::string, std::string, uint32_t> FileLoader::LocateFileInDictionary(const fs::path& filename) noexcept
{
#ifdef DEBUG
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif

    // we are looking for forward slashes only!
    auto& _filename = filename.string();
    std::replace(_filename.begin(), _filename.end(), '\\', '/');

    std::string directory_name, archive_name;
    std::string _path;
    auto _namehash = hashlittle(_filename.c_str());

    // try find the namehash first to save some time
    auto find_it = m_Dictionary.find(_namehash);
    if (find_it == m_Dictionary.end()) {
        // namehash doesn't exist, look for part of the filename
        find_it = std::find_if(m_Dictionary.begin(), m_Dictionary.end(), [&](const std::pair<uint32_t, std::pair<fs::path, std::vector<std::string>>>& item) {
            const auto& item_fn = item.second.first;
            return item_fn == _filename || (item_fn.string().find(_filename) != std::string::npos && item_fn.extension() == filename.extension());
        });
    }

    if (find_it != m_Dictionary.end()) {
        const auto& file = (*find_it);
        _path = file.second.second.at(0); // TODO: some hierarchy (patches_win64 over archives_win64)
        _namehash = file.first;

        // get the directory and archive name
        const auto pos = _path.find_last_of("/");
        directory_name = _path.substr(0, pos);
        archive_name = _path.substr(pos + 1, _path.length());
    }

#ifdef DEBUG
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    DEBUG_LOG("[PERF] FileLoader::LocateFileInDictionary (" << filename.string() << ") - took " << duration << " ms.");
#endif

    return { directory_name, archive_name, _namehash };
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