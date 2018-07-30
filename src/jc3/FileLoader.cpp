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

    // trigger the file type callbacks
    UI::Get()->Events().FileTreeItemSelected.connect([&](const fs::path& file, std::shared_ptr<AvalancheArchive> archive) {
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

            DEBUG_LOG("[ERROR] Unknown file type \"" << file << "\".");
        }
    });

    // save file
    UI::Get()->Events().SaveFileRequest.connect([&](const fs::path& file) {
        Window::Get()->ShowFolderSelection("Select a folder to save the file to.", [&, file](const std::string& selected) {
            DEBUG_LOG("SaveFileRequest - want to save file '" << file << "' to '" << selected << "'");

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
    });

    // export file
    UI::Get()->Events().ExportFileRequest.connect([&](const fs::path& file, IImportExporter* exporter) {
        Window::Get()->ShowFolderSelection("Select a folder to export the file to.", [&, file, exporter](const std::string& selected) {
            DEBUG_LOG("ExportFileRequest - want to export file '" << file << "' to '" << selected << "'");

            auto use = exporter;
            if (!exporter) {
                DEBUG_LOG("ExportFileRequest - Finding a suitable exporter for '" << file.extension() << "'...");

                const auto& exporters = ImportExportManager::Get()->GetExportersForExtension(file.extension().string());
                if (exporters.size() > 0) {
                    DEBUG_LOG("ExportFileRequest - Using exporter '" << exporters.at(0)->GetName() << "'");

                    use = exporters.at(0);
                }
            }

            // if we have a valid exporter, read the file and export it
            if (use) {
                std::stringstream status_text;
                status_text << "Exporting \"" << file << "\"...";
                const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

                ReadFile(file, [&, file, selected, use, status_text_id](bool success, FileBuffer data) {
                    DEBUG_LOG("ExportFileRequest - Finished reading file, exporting now...");

                    if (success) {
                        use->Export(file, data, selected, [status_text_id] {
                            UI::Get()->PopStatusText(status_text_id);
                        });
                    }
                    else {
                        UI::Get()->PopStatusText(status_text_id);

                        std::stringstream error;
                        error << "Failed to export \"" << file.filename() << "\".";
                        Window::Get()->ShowMessageBox(error.str());

                        DEBUG_LOG("[ERROR] " << error.str());
                    }
                });
            }
        });
    });
}

void FileLoader::ReadFile(const fs::path& filename, ReadFileCallback callback, uint8_t flags) noexcept
{
    uint64_t status_text_id = 0;

    if (!m_UseBatches) {
        std::stringstream status_text;
        status_text << "Reading \"" << filename << "\"...";
        status_text_id = UI::Get()->PushStatusText(status_text.str());
    }

    // are we looking for a texture?
    if (!(flags & ReadFileFlags::NO_TEX_CACHE) && (filename.extension() == ".dds" || filename.extension() == ".ddsc" || filename.extension() == ".hmddsc")) {
        const auto& texture = TextureManager::Get()->GetTexture(filename, TextureManager::NO_CREATE);

        // is the texture loaded?
        if (texture && texture->GetResource()) {
            UI::Get()->PopStatusText(status_text_id);
            return callback(true, texture->GetBuffer());
        }
    }

    // check any loaded archives for the file
    const auto&[archive, entry] = GetStreamArchiveFromFile(filename);
    if (archive && entry.m_Offset != 0) {
        auto buffer = archive->GetEntryFromArchive(entry);

        UI::Get()->PopStatusText(status_text_id);
        return callback(true, std::move(buffer));
    }

    // should we use file batching?
    if (m_UseBatches) {
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

StreamArchive_t* FileLoader::ParseStreamArchive(std::istream& stream)
{
    JustCause3::StreamArchive::SARCHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "SARC", 4) != 0) {
        DEBUG_LOG("FileLoader::ParseStreamArchive - Invalid header magic. Input file probably isn't a StreamArchive file.");
        return nullptr;
    }

    auto result = new StreamArchive_t;

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

    return result;
}

bool FileLoader::DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept
{
    // read the archive header
    JustCause3::AvalancheArchiveHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "AAF", 4) != 0) {
        DEBUG_LOG("FileLoader::DecompressArchiveFromStream - Invalid header magic. Input probably isn't an AvalancheArchiveFormat file.");
        return false;
    }

    DEBUG_LOG("AvalancheArchiveFormat v" << header.m_Version);
    DEBUG_LOG(" - m_BlockCount=" << header.m_BlockCount);
    DEBUG_LOG(" - m_TotalUncompressedSize=" << header.m_TotalUncompressedSize);

    output->reserve(header.m_TotalUncompressedSize);

    // read all the blocks
    for (uint32_t i = 0; i < header.m_BlockCount; ++i) {
        auto base_position = stream.tellg();

        JustCause3::AvalancheArchiveChunk chunk;
        stream.read((char *)&chunk.m_CompressedSize, sizeof(chunk.m_CompressedSize));
        stream.read((char *)&chunk.m_UncompressedSize, sizeof(chunk.m_UncompressedSize));

        uint32_t blockSize;
        uint32_t blockMagic;

        stream.read((char *)&blockSize, sizeof(blockSize));
        stream.read((char *)&blockMagic, sizeof(blockMagic));

#if 0
        DEBUG_LOG(" ==== Block #" << i << " ====");
        DEBUG_LOG(" - Compressed Size: " << chunk.m_CompressedSize);
        DEBUG_LOG(" - Uncompressed Size: " << chunk.m_UncompressedSize);
        DEBUG_LOG(" - Size: " << blockSize);
        DEBUG_LOG(" - Magic: " << std::string((char *)&blockMagic, 4));
#endif

        // 'EWAM' magic
        if (blockMagic != 0x4D415745) {
            output->shrink_to_fit();

            DEBUG_LOG("FileLoader::DecompressArchiveFromStream - Invalid chunk magic.");
            return false;
        }

        // set the chunk offset
        chunk.m_DataOffset = static_cast<uint64_t>(stream.tellg());

        // read the block data
        FileBuffer data;
        data.resize(blockSize);
        stream.read((char *)data.data(), blockSize);

        // decompress the block
        {
            uLong compressed_size = (uLong)header.m_BlockSize;
            uLong decompressed_size = (uLong)chunk.m_UncompressedSize;

            FileBuffer result;
            result.resize(chunk.m_UncompressedSize);
            auto res = uncompress(result.data(), &decompressed_size, data.data(), compressed_size);

            assert(res == Z_OK);
            assert(decompressed_size == chunk.m_UncompressedSize);

            output->insert(output->end(), result.begin(), result.end());
        }

        stream.seekg((uint64_t)base_position + blockSize);
    }

    return true;
}

StreamArchive_t* FileLoader::ReadStreamArchive(const FileBuffer& data) noexcept
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

StreamArchive_t* FileLoader::ReadStreamArchive(const fs::path& filename) noexcept
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

bool FileLoader::ReadCompressedTexture(const FileBuffer* buffer, const FileBuffer* hmddsc_buffer, FileBuffer* output) noexcept
{
    //std::istringstream stream(std::string{ (char *)buffer.data(), buffer.size() });
    //std::istringstream hmddsc_stream(std::string{ (char *)hmddsc_buffer.data(), hmddsc_buffer.size() });

    return ParseCompressedTexture(buffer, hmddsc_buffer, output);
}

bool FileLoader::ReadCompressedTexture(const fs::path& filename, FileBuffer* output) noexcept
{
    if (!fs::exists(filename)) {
        DEBUG_LOG("FileLoader::ReadCompressedTexture - Input file doesn't exist");
        return false;
    }

    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("FileLoader::ReadCompressedTexture - Failed to open stream");
        return false;
    }

    //ParseCompressedTexture(stream, std::numeric_limits<uint64_t>::max(), output);
    stream.close();
    return true;
}

template <typename T>
inline T ALIGN_TO_BOUNDARY(T& value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (value + (alignment - (value % alignment)));
    }

    return value;
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

AvalancheDataFormat* FileLoader::ReadAdf(const fs::path& filename) noexcept
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
    auto adf = new AvalancheDataFormat(filename);
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

bool FileLoader::ParseCompressedTexture(const FileBuffer* ddsc_buffer, const FileBuffer* hmddsc_buffer, FileBuffer* output) noexcept
{
    std::istringstream ddsc_stream(std::string{ (char *)ddsc_buffer->data(), ddsc_buffer->size() });

    // read the texture data
    JustCause3::AvalancheTexture texture;
    ddsc_stream.read((char *)&texture, sizeof(texture));

    // ensure the header magic is correct
    if (texture.m_Magic != 0x58545641) {
        return false;
    }

    // loop over all streams
    uint32_t big_size = 0;
    uint32_t stream_index = 0;
    for (auto i = 0; i < 8; ++i) {
        const auto& stream = texture.m_Streams[i];

        // skip this stream if we have no data
        if (stream.m_Size == 0) {
            continue;
        }

        // is this the biggest size?
        if ((hmddsc_buffer != nullptr && stream.m_IsSource) || (!stream.m_IsSource && stream.m_Size > big_size)) {
            big_size = stream.m_Size;
            stream_index = i;
        }
    }

    // find the rank
    uint32_t rank = 0;
    for (auto i = 0; i < 8; ++i) {
        if (i == stream_index) {
            continue;
        }

        if (texture.m_Streams[i].m_Size > texture.m_Streams[stream_index].m_Size) {
            ++rank;
        }
    }

    // select the stream offset
    const auto stream_offset = texture.m_Streams[stream_index].m_Offset;

    // create the block
    const auto block_size = ((hmddsc_buffer ? hmddsc_buffer->size() : ddsc_buffer->size()) - stream_offset);
    auto block = std::unique_ptr<char[]>(new char[block_size + 1]);

    // resize the output buffer
    output->resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + block_size);

    // write the DDS header block to the output
    {
        DDS_HEADER header;
        ZeroMemory(&header, sizeof(header));
        header.size = 124;
        header.flags = (0x1007 | 0x20000);
        header.width = texture.m_Width >> rank;
        header.height = texture.m_Height >> rank;
        header.depth = texture.m_Depth;
        header.mipMapCount = 1;
        header.ddspf = TextureManager::GetPixelFormat(texture.m_Format);
        header.caps = (8 | 0x1000);

        // write the magic and header
        memcpy(&output->front(), (char *)&DDS_MAGIC, sizeof(DDS_MAGIC));
        memcpy(&output->front() + sizeof(DDS_MAGIC), (char *)&header, sizeof(DDS_HEADER));
    }

    // do we have a hmddsc buffer to parse?
    if (hmddsc_buffer != nullptr) {
        std::istringstream hmddsc_stream(std::string{ (char *)hmddsc_buffer->data(), hmddsc_buffer->size() });

        // seek to the block position
        hmddsc_stream.seekg(stream_offset);
        
        // read the block data
        hmddsc_stream.read(block.get(), block_size);
    }
    else {
        // seek to the block position
        ddsc_stream.seekg(stream_offset);

        // read the block data
        ddsc_stream.read(block.get(), block_size);
    }

    // write the block data
    memcpy(&output->front() + sizeof(DDS_MAGIC) + sizeof(DDS_HEADER), block.get(), block_size);

    return true;
}

void FileLoader::RegisterCallback(const std::string& filetype, FileTypeCallback fn)
{
    m_FileTypeCallbacks[filetype].emplace_back(fn);
}

void FileLoader::RegisterCallback(const std::vector<std::string>& filetypes, FileTypeCallback fn)
{
    for (const auto& filetype : filetypes) {
        m_FileTypeCallbacks[filetype].emplace_back(fn);
    }
}