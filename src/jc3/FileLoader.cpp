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

// Credits to gibbed for most of the archive format stuff
// http://gib.me

extern AvalancheArchive* g_CurrentLoadedArchive;
extern fs::path g_JC3Directory;

static uint64_t g_ArchiveLoadCount = 0;

FileLoader::FileLoader()
{
    m_FileList = std::make_unique<DirectoryList>();
    const auto status_text_id = UI::Get()->PushStatusText("Loading dictionary...");

    std::thread([this, status_text_id] {
        try {
            const auto handle = GetModuleHandle(nullptr);
            const auto rc = FindResource(handle, MAKEINTRESOURCE(128), MAKEINTRESOURCE(256));
            if (rc == nullptr) {
                throw std::runtime_error("FileLoader - Failed to find dictionary resource");
            }

            const auto data = LoadResource(handle, rc);
            if (data == nullptr) {
                throw std::runtime_error("FileLoader - Failed to load dictionary resource");
            }

            // TODO: something with the load resource stuff is breaking this.
            // this is ugly but works for now
            auto str = static_cast<const char*>(LockResource(data));
            auto l = SizeofResource(handle, rc);
            std::vector<uint8_t> d;
            d.resize(l);
            std::memcpy((char *)d.data(), str, l);

            m_FileListDictionary = json::parse(d);
            m_FileList->Parse(&m_FileListDictionary, { ".rbm", ".ee", ".bl", ".nl" }); // TODO: dds, ddsc, hmddsc
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

    // TODO: move the events to a better location?

    // trigger the file type callbacks
    UI::Get()->Events().FileTreeItemSelected.connect([&](const fs::path& filename) {
        // do we have a registered callback for this file type?
        if (m_FileTypeCallbacks.find(filename.extension().string()) != m_FileTypeCallbacks.end()) {
            ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                if (success) {
                    for (const auto& fn : m_FileTypeCallbacks[filename.extension().string()]) {
                        fn(filename, data);
                    }
                }
                else {
                    std::stringstream error;
                    error << "Failed to load \"" << filename << "\"." << std::endl << std::endl;
                    error << "Make sure you have selected the correct Just Cause 3 directory.";
                    Window::Get()->ShowMessageBox(error.str());

                    DEBUG_LOG("[ERROR] Failed to load \"" << filename << "\".");
                }
            });
        }
        else {
            std::stringstream info;
            info << "I don't know how to read the \"" << filename.extension() << "\" extension." << std::endl << std::endl;
            info << "Want to help? Check out our GitHub page for information on how to contribute.";
            Window::Get()->ShowMessageBox(info.str(), MB_ICONASTERISK);
        }
    });

    // save file
    UI::Get()->Events().SaveFileRequest.connect([&](const fs::path& filename) {
        Window::Get()->ShowFolderSelection("Select a folder to save the file to.", [&, filename](const std::string& selected) {
            DEBUG_LOG("SaveFileRequest - want to save file '" << filename << "' to '" << selected << "'");
        });
    });

    // export file
    UI::Get()->Events().ExportFileRequest.connect([&](const fs::path& filename, IImportExporter* exporter) {
        Window::Get()->ShowFolderSelection("Select a folder to export the file to.", [&, filename, exporter](const std::string& selected) {
            DEBUG_LOG("ExportFileRequest - want to export file '" << filename << "' to '" << selected << "'");

            auto use = exporter;
            if (!exporter) {
                DEBUG_LOG("ExportFileRequest - Finding a suitable exporter for '" << filename << "'...");

                const auto& exporters = ImportExportManager::Get()->GetExportersForExtension(filename.extension().string());
                if (exporters.size() > 0) {
                    DEBUG_LOG("ExportFileRequest - Using exporter '" << exporters.at(0)->GetName() << "'");

                    use = exporters.at(0);
                }
            }

            // if we have a valid exporter, read the file and export it
            if (use) {
                std::stringstream status_text;
                status_text << "Exporting \"" << filename << "\"...";
                const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

                FileLoader::Get()->ReadFile(filename, [&, filename, selected, use, status_text_id](bool success, FileBuffer data) {
                    DEBUG_LOG("ExportFileRequest - Finished reading file, exporting now...");

                    if (success) {
                        use->Export(filename, data, selected, [status_text_id] {
                            UI::Get()->PopStatusText(status_text_id);
                        });
                    }
                    else {
                        UI::Get()->PopStatusText(status_text_id);

                        std::stringstream error;
                        error << "Failed to export \"" << filename.filename() << "\".";
                        Window::Get()->ShowMessageBox(error.str());

                        DEBUG_LOG("[ERROR] " << error.str());
                    }
                });
            }
        });
    });
}

void FileLoader::ReadFile(const fs::path& filename, ReadFileCallback callback) noexcept
{
    std::stringstream status_text;
    status_text << "Reading \"" << filename << "\"...";
    const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

    // are we looking for a texture?
    if (filename.extension() == ".dds" || filename.extension() == ".ddsc" || filename.extension() == ".hmddsc") {
        auto& texture = TextureManager::Get()->GetTexture(filename, false);

        // is the texture loaded?
        if (texture && texture->GetResource()) {
            UI::Get()->PopStatusText(status_text_id);
            return callback(true, texture->GetBuffer());
        }
    }

    // check any loaded archives for the file
    if (g_CurrentLoadedArchive) {
        const auto& [archive, entry] = GetStreamArchiveFromFile(filename);
        if (archive && entry.m_Offset != 0) {
            auto buffer = archive->GetEntryFromArchive(entry);

            UI::Get()->PopStatusText(status_text_id);
            return callback(true, std::move(buffer));
        }
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

bool FileLoader::ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output) noexcept
{
    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("FileLoader::ReadArchiveTable - Failed to open stream");
        return false;
    }

    JustCause3::ArchiveTable::TabFileHeader header;
    stream.read((char *)&header, sizeof(header));

    if (header.magic != 0x424154) {
        DEBUG_LOG("FileLoader::ReadArchiveTable - Invalid header magic. Input file probably isn't a TAB file.");
        return false;
    }

    output->index = 0;
    output->version = 2;

    while (!stream.eof())
    {
        JustCause3::ArchiveTable::VfsTabEntry entry;
        stream.read((char *)&entry, sizeof(entry));

        // prevent the entry being added 2 times when we get to the null character at the end
        // failbit will be set because the stream can only read 1 more byte
        if (stream.fail()) {
            break;
        }

        output->entries.emplace_back(entry);
    }

    DEBUG_LOG("FileLoader::ReadArchiveTable - " << output->entries.size() << " files in archive");

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

    StreamArchive_t* result = new StreamArchive_t;

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
        DEBUG_LOG("FileLoader::ReadStreamArchive - Failed to open stream")
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

RuntimeContainer* FileLoader::ReadRuntimeContainer(const FileBuffer& buffer) noexcept
{
    std::istringstream stream(std::string{ (char*)buffer.data(), buffer.size() });

    // read the runtimer container header
    JustCause3::RuntimeContainer::Header header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "RTPC", 4) != 0) {
        DEBUG_LOG("FileLoader::ReadRuntimeContainer - Invalid header magic. Input probably isn't a RuntimeContainer file.");
        return nullptr;
    }

    DEBUG_LOG("RuntimePropertyContainer v" << header.m_Version);

    // read the root node
    JustCause3::RuntimeContainer::Node rootNode;
    stream.read((char *)&rootNode, sizeof(rootNode));

    // create the root container
    auto root_container = new RuntimeContainer{ rootNode.m_NameHash };

    std::queue<std::tuple<RuntimeContainer*, JustCause3::RuntimeContainer::Node>> instanceQueue;
    std::queue<std::tuple<RuntimeContainerProperty*, JustCause3::RuntimeContainer::Property>> propertyQueue;

    instanceQueue.push({ root_container, rootNode });

    while (!instanceQueue.empty()) {
        const auto&[current_container, item] = instanceQueue.front();

        stream.seekg(item.m_DataOffset);

#if 0
        DEBUG_LOG(" ====== NODE ======");
        DEBUG_LOG("  - m_NameHash: 0x" << std::setw(4) << std::hex << item.m_NameHash << " (" << DebugNameHash(item.m_NameHash) << ")");
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
        DEBUG_LOG("   > NODES");
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

    return root_container;
}

std::tuple<StreamArchive_t*, StreamArchiveEntry_t> FileLoader::GetStreamArchiveFromFile(const fs::path& file) noexcept
{
    if (g_CurrentLoadedArchive) {
        auto streamArchive = g_CurrentLoadedArchive->GetStreamArchive();

        for (auto& entry : streamArchive->m_Files) {
            if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                return { streamArchive, entry };
            }
        }
    }

    return { nullptr, StreamArchiveEntry_t{} };
}

bool FileLoader::ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash, FileBuffer* output) noexcept
{
    fs::path tab_file = g_JC3Directory / directory / (archive + ".tab");
    fs::path arc_file = g_JC3Directory / directory / (archive + ".arc");

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
        for (const auto& entry : archiveTable.entries) {
            if (entry.hash == namehash) {
                DEBUG_LOG("FileLoader::ReadFileFromArchive - Found file in archive at " << entry.offset << " (size: " << entry.size << " bytes)");

                // open the file stream
                std::ifstream stream(arc_file, std::ios::binary | std::ios::ate);
                if (stream.fail()) {
                    DEBUG_LOG("[ERROR] FileLoader::ReadFileFromArchive - Failed to open stream");
                    return false;
                }

                g_ArchiveLoadCount++;
                DEBUG_LOG("[PERF] Archive load count: " << g_ArchiveLoadCount);

                // TODO: caching

                stream.seekg(entry.offset);
                output->resize(entry.size);

                stream.read((char *)output->data(), entry.size);
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
    // ergh, fs::path uses backslashes which is bad for us, is there a way
    // to stop that or should we switch back to strings?
    std::string _filename = filename.string();
    std::replace(_filename.begin(), _filename.end(), '\\', '/');

    // iterate all the root objects
    for (auto it = m_FileListDictionary.begin(); it != m_FileListDictionary.end(); ++it) {
        const auto& directory_name = it.key();
        const auto& directory_obj = it.value();

        // iterate the files in the archive object
        for (auto it2 = directory_obj.begin(); it2 != directory_obj.end(); ++it2) {
            const auto& archive_name = it2.key();
            const auto& archive_obj = it2.value();

            // iterate the data in the files object
            for (auto it3 = archive_obj.begin(); it3 != archive_obj.end(); ++it3) {
                const auto& key = it3.key();
                const auto& value = it3.value();

                if (value.is_string()) {
                    const auto& value_str = value.get<std::string>();
                    if (value_str.find(_filename) != std::string::npos) {
                        DEBUG_LOG("FileLoader::LocateFileInDictionary - Found '" << value_str << "' (" << key << ") in '/" << directory_name << "/" << archive_name << "'");

                        auto namehash = static_cast<uint32_t>(std::stoul(key, nullptr, 16));
                        return { directory_name, archive_name, namehash };
                    }
                }
            }
        }
    }

    // DEBUG_LOG("[ERROR] FileLoader::LocateFileInDictionary - Can't find '" << filename << "'");
    return { "", "", 0 };
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