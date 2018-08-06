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

template <typename T>
inline T ALIGN_TO_BOUNDARY(T& value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (value + (alignment - (value % alignment)));
    }

    return value;
}

template <typename T>
inline uint32_t DISTANCE_TO_BOUNDARY(T& value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (alignment - (value % alignment));
    }

    return 0;
}

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

                const auto namehash = static_cast<uint32_t>(std::stoul(data["namehash"].get<std::string>(), nullptr, 16));

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

#if 0
    // handle file drops
    Window::Get()->Events().FileDropped.connect([&](const fs::path& file) {
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
                fn(file, data);
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
#endif

    // trigger the file type callbacks
    UI::Get()->Events().FileTreeItemSelected.connect([&](const fs::path& file, AvalancheArchive* archive) {
        // do we have a registered callback for this file type?
        if (m_FileTypeCallbacks.find(file.extension().string()) != m_FileTypeCallbacks.end()) {
            ReadFile(file, [&, file](bool success, FileBuffer data) {
                if (success) {
                    for (const auto& fn : m_FileTypeCallbacks[file.extension().string()]) {
                        fn(file, data);
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
    UI::Get()->Events().SaveFileRequest.connect([&](const fs::path& file) {
#if 0
        Window::Get()->ShowFolderSelection("Select a folder to save the file to.", [&, file](const std::string& selected) {
            DEBUG_LOG("SaveFileRequest - want to save file \"" << file << "\" to \"" << selected << "\"..");

            std::stringstream status_text;
            status_text << "Saving \"" << file << "\"...";
            const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

            ReadFile(file, [&, file, selected, status_text_id](bool success, FileBuffer data) {
                if (success) {
                    fs::path file_with_path = selected;
                    file_with_path /= file.filename();

                    std::ofstream file(file_with_path, std::ios::binary);
                    file.write((char *)data.data(), data.size());
                    file.close();

                    UI::Get()->PopStatusText(status_text_id);
                }
                else {
                    UI::Get()->PopStatusText(status_text_id);

                    std::stringstream error;
                    error << "Failed to save \"" << file.filename() << "\".";
                    Window::Get()->ShowMessageBox(error.str());

                    DEBUG_LOG("[ERROR] " << error.str());
                }
            });
        });
#endif
    });

    // export file
    UI::Get()->Events().ExportFileRequest.connect([&](const fs::path& file, IImportExporter* exporter) {
        Window::Get()->ShowFolderSelection("Select a folder to export the file to.", [&, file, exporter](const std::string& selected) {
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
    if (archive && entry.m_Offset != 0) {
        auto buffer = archive->GetEntryBuffer(entry);

        UI::Get()->PopStatusText(status_text_id);
        return callback(true, std::move(buffer));
    }

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
                        if (entry.m_Hash == file.first) {
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

std::unique_ptr<StreamArchive_t> FileLoader::ParseStreamArchive(std::istream& stream)
{
    JustCause3::StreamArchive::SARCHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "SARC", 4) != 0) {
        DEBUG_LOG("FileLoader::ParseStreamArchive - Invalid header magic. Input file probably isn't a StreamArchive file.");
        return nullptr;
    }

    auto result = std::make_unique<StreamArchive_t>();
    result->m_Header = header;

    auto start_pos = stream.tellg();
    while (true) {
        StreamArchiveEntry_t entry;

        uint32_t length;
        stream.read((char *)&length, sizeof(length));

        auto filename = std::unique_ptr<char[]>(new char[length]);
        stream.read(filename.get(), length);

        entry.m_Filename = filename.get();
        entry.m_Filename.resize(length);

        stream.read((char *)&entry.m_Offset, sizeof(entry.m_Offset));
        stream.read((char *)&entry.m_Size, sizeof(entry.m_Size));

        result->m_Files.emplace_back(entry);

        auto current_pos = stream.tellg();
        if (header.m_Size - (current_pos - start_pos) <= 15) {
            break;
        }
    }

    return result;
}

void FileLoader::CompressArchive(JustCause3::AvalancheArchive::Header* header, std::vector<JustCause3::AvalancheArchive::Chunk>* chunks) noexcept
{
    std::ofstream stream("C:/users/aaron/desktop/main_character.bin", std::ios::binary);
    assert(!stream.fail());
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

        // store the compressed size
        chunk.m_CompressedSize = static_cast<uint32_t>(compressed_size) - 6; // remove the header and checksum from the total size
        DEBUG_LOG("actual compressed size: " << chunk.m_CompressedSize);

        assert(res == Z_OK);

        // calculate the distance to the 16-byte boundary after we write our data
        auto pos = static_cast<uint32_t>(stream.tellp()) + JustCause3::AvalancheArchive::CHUNK_SIZE + chunk.m_CompressedSize;
        auto padding = DISTANCE_TO_BOUNDARY(pos, 16);

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

    stream.close();
}

void FileLoader::CompressArchive(StreamArchive_t* archive) noexcept
{
    assert(archive);

    // TODO: support chunks. how do avalanche determine when to create a new block?
    // there seems to be no obvious patterns with the buffer sizes

    // generate the header
    JustCause3::AvalancheArchive::Header header;
    strcpy(header.m_Magic, "AAF");
    header.m_Version = 1;
    strcpy(header.m_Magic2, "AVALANCHEARCHIVEFORMATISCOOL");
    header.m_TotalUncompressedSize = archive->m_SARCBytes.size();
    header.m_UncompressedBufferSize = archive->m_SARCBytes.size();
    header.m_ChunkCount = 1;

    // TODO: figure out how m_UncompressedBufferSize is calculated for multiple blocks!

    DEBUG_LOG("CompressArchive");
    DEBUG_LOG(" - TotalUncompressedSize=" << header.m_TotalUncompressedSize);
    DEBUG_LOG(" - UncompressedBufferSize=" << header.m_UncompressedBufferSize);

    auto& block_data = archive->m_SARCBytes;

    // generate the chunks
    std::vector<JustCause3::AvalancheArchive::Chunk> chunks;
    for (uint32_t i = 0; i < header.m_ChunkCount; ++i) {
        // TODO: chunks should copy the block_data.begin() + offset

        JustCause3::AvalancheArchive::Chunk chunk;
        chunk.m_Magic = 0x4D415745;
        chunk.m_CompressedSize = 0; // NOTE: generated later
        chunk.m_UncompressedSize = block_data.size();
        chunk.m_DataSize = 0; // NOTE: generated later
        chunk.m_BlockData = block_data;

        chunks.emplace_back(std::move(chunk));
    }

    return CompressArchive(&header, &chunks);
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

std::unique_ptr<StreamArchive_t> FileLoader::ReadStreamArchive(const FileBuffer& data) noexcept
{
    // TODO: need to read the header, check if the archive is compressed,
    // then just back to the start of the stream!

    // decompress the archive data
    std::istringstream compressed_buffer(std::string{ (char*)data.data(), data.size() });

    // decompress the archive data
    FileBuffer buffer;
    if (DecompressArchiveFromStream(compressed_buffer, &buffer)) {
        // parse the stream archive
        std::istringstream stream(std::string{ (char *)buffer.data(), buffer.size() });
        auto archive = ParseStreamArchive(stream);
        if (archive) {
            archive->m_SARCBytes = std::move(buffer);
            return archive;
        }
    }

    return nullptr;
}

std::unique_ptr<StreamArchive_t> FileLoader::ReadStreamArchive(const fs::path& filename) noexcept
{
    if (!fs::exists(filename)) {
        DEBUG_LOG("FileLoader::ReadStreamArchive - Input file doesn't exist");
        return nullptr;
    }

    std::ifstream compressed_buffer(filename, std::ios::binary);
    if (compressed_buffer.fail()) {
        DEBUG_LOG("FileLoader::ReadStreamArchive - Failed to open stream");
        return nullptr;
    }

    // TODO: need to read the header, check if the archive is compressed,
    // then just back to the start of the stream!

    FileBuffer buffer;
    if (DecompressArchiveFromStream(compressed_buffer, &buffer)) {
        compressed_buffer.close();

        // parse the stream archive
        std::istringstream stream(std::string{ (char*)buffer.data(), buffer.size() });
        auto result = ParseStreamArchive(stream);
        if (result) {
            result->m_Filename = filename;
            result->m_SARCBytes = std::move(buffer);
            return result;
        }
    }

    return nullptr;
}

void FileLoader::ReadTexture(const fs::path& filename, ReadFileCallback callback) noexcept
{
    FileLoader::Get()->ReadFile(filename, [this, filename, callback](bool success, FileBuffer data) {
        if (!success) {
            DEBUG_LOG("ReadTexture -> failed to read \"" << filename << "\".");
            return callback(false, {});
        }

        std::istringstream stream(std::string{ (char *)data.data(), data.size() });

        // read the texture data
        JustCause3::AvalancheTexture texture;
        stream.read((char *)&texture, sizeof(texture));

        // ensure the header magic is correct
        if (texture.m_Magic != 0x58545641) {
            DEBUG_LOG("ReadTexture -> failed to read \"" << filename << "\". (invalid magic)");
            return callback(false, {});
        }

        // find the best stream to use
        auto&[stream_index, load_source] = JustCause3::FindBestTexture(&texture);

        // find the rank
        auto rank = JustCause3::GetHighestTextureRank(&texture, stream_index);

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

#if 0
        DEBUG_LOG(" ====== NODE ======");
        DEBUG_LOG("  - m_NameHash: 0x" << std::setw(4) << std::hex << item.m_NameHash << " (" << NameHashLookup::GetName(item.m_NameHash) << ")");
        DEBUG_LOG("  - m_PropertyCount: " << item.m_PropertyCount);
        DEBUG_LOG("  - m_InstanceCount: " << item.m_InstanceCount << " (queue: " << instanceQueue.size() << ")");
        DEBUG_LOG("  - m_DataOffset: " << item.m_DataOffset << " (current: " << stream.tellg() << ")");
#endif

        // read all the node properties
#if 0
        DEBUG_LOG("   > PROPERTIES");
#endif
        for (uint16_t i = 0; i < item.m_PropertyCount; ++i) {
            JustCause3::RuntimeContainer::Property prop;
            stream.read((char *)&prop, sizeof(prop));

            if (current_container) {
                auto container_property = new RuntimeContainerProperty{ prop.m_NameHash, prop.m_Type };
                current_container->AddProperty(container_property);

                propertyQueue.push({ container_property, prop });
            }

#if 0
            DEBUG_LOG("    - m_NameHash: 0x" << std::setw(4) << std::hex << prop.m_NameHash << " (" << DebugNameHash(prop.m_NameHash) << ")" << ", m_Type: " << RuntimeContainerProperty::GetTypeName(prop.m_Type));
#endif
        }

        // seek to our current pos with 4-byte alignment
        stream.seekg(ALIGN_TO_BOUNDARY(stream.tellg()));

        // read all the node instances
#if 0
        DEBUG_LOG("   > INSTANCES");
#endif
        for (uint16_t i = 0; i < item.m_InstanceCount; ++i) {
            JustCause3::RuntimeContainer::Node node;
            stream.read((char *)&node, sizeof(node));

            auto next_container = new RuntimeContainer{ node.m_NameHash };
            if (current_container) {
                current_container->AddContainer(next_container);
            }

#if 0
            DEBUG_LOG("    - m_NameHash: 0x" << std::setw(4) << std::hex << node.m_NameHash << " (" << DebugNameHash(node.m_NameHash) << ")");
            DEBUG_LOG("    - m_DataOffset: " << node.m_DataOffset);
            DEBUG_LOG("    -----------------------");
#endif

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
    root_container->GenerateNamesIfNeeded();

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
            for (const auto& entry : stream_arc->m_Files) {
                if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                    return { stream_arc, entry };
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
            if (entry.m_Hash == namehash) {
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

void FileLoader::RegisterReadCallback(const std::vector<std::string>& filetypes, FileTypeCallback fn)
{
    for (const auto& filetype : filetypes) {
        m_FileTypeCallbacks[filetype].emplace_back(fn);
    }
}