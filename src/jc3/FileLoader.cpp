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

// Credits to gibbed for most of the archive format stuff
// http://gib.me

extern AvalancheArchive* g_CurrentLoadedArchive;
extern fs::path g_JC3Directory;

FileLoader::FileLoader()
{
    m_FileList = std::make_unique<DirectoryList>();
    const auto status_text_id = UI::Get()->PushStatusText("Loading dictionary...");

    std::thread([this, status_text_id] {
        fs::path dictionary = fs::current_path() / "dictionary.json";

        try {
            if (!fs::exists(dictionary)) {
                throw std::runtime_error("FileLoader - Failed to read file list dictionary");
            }

            std::ifstream file(dictionary);
            if (file.fail()) {
                throw std::runtime_error("FileLoader - Failed to read file list dictionary");
            }

            m_FileListDictionary = json::parse(file);
            file.close();

            m_FileList->Parse(&m_FileListDictionary, { ".rbm", ".ee", ".bl", ".nl" });
        }
        catch (...) {
            Window::Get()->ShowMessageBox("Failed to read/parse file list dictionary.\n\nSome features will be disabled.");
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
        auto texture = TextureManager::Get()->GetTexture(filename, false);

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
        const auto& [archive_name, namehash] = LocateFileInDictionary(filename);
        if (!archive_name.empty()) {
            FileBuffer buffer;
            if (ReadFileFromArchive(archive_name, namehash, &buffer)) {
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
    std::ifstream file(filename, std::ios::binary);
    if (file.fail()) {
        DEBUG_LOG("FileLoader::ReadArchiveTable - Failed to open stream");
        return false;
    }

    JustCause3::ArchiveTable::TabFileHeader header;
    file.read((char *)&header, sizeof(header));

    if (header.magic != 0x424154) {
        DEBUG_LOG("FileLoader::ReadArchiveTable - Invalid header magic. Input file probably isn't a TAB file.");
        return false;
    }

    output->index = 0;
    output->version = 2;

    while (!file.eof())
    {
        JustCause3::ArchiveTable::VfsTabEntry entry;
        file.read((char *)&entry, sizeof(entry));

        output->entries.emplace_back(entry);
    }

    DEBUG_LOG("FileLoader::ReadArchiveTable - " << output->entries.size() << " files in archive");

    file.close();
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

        auto buffer = new char[length + 1];
        stream.read(buffer, length);
        buffer[length] = '\0';

        entry.m_Filename = buffer;
        delete[] buffer;

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

bool FileLoader::ReadCompressedTexture(const FileBuffer& buffer, uint64_t size, FileBuffer* output) noexcept
{
    std::istringstream stream(std::string{ (char*)buffer.data(), buffer.size() });
    ParseCompressedTexture(stream, size, output);
    return true;
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

    ParseCompressedTexture(stream, std::numeric_limits<uint64_t>::max(), output);
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

bool FileLoader::ReadFileFromArchive(const std::string& archive, uint32_t namehash, FileBuffer* output) noexcept
{
    fs::path tab_file = g_JC3Directory / "archives_win64" / (archive + ".tab");
    fs::path arc_file = g_JC3Directory / "archives_win64" / (archive + ".arc");

    DEBUG_LOG(tab_file);
    DEBUG_LOG(arc_file);

    if (!fs::exists(tab_file) || !fs::exists(arc_file)) {
        DEBUG_LOG("FileLoader::ReadFileFromArchive - Can't find .arc/.tab file");
        return false;
    }

    bool found = false;

    // read the archive table
    JustCause3::ArchiveTable::VfsArchive archiveTable;
    if (ReadArchiveTable(tab_file.string(), &archiveTable)) {
        uint64_t offset = 0;
        uint64_t size = 0;

        // locate the data for the file
        for (auto& entry : archiveTable.entries) {
            if (entry.hash == namehash) {
                offset = entry.offset;
                size = entry.size;
                found = true;

                DEBUG_LOG("FileLoader::ReadFileFromArchive - Found file in archive at " << offset << " (size: " << size << " bytes)");
                break;
            }
        }

        // did we find what we're looking for?
        if (found) {
            std::ifstream stream(arc_file, std::ios::binary);
            stream.seekg(offset);

            output->resize(size);
            stream.read((char *)output->data(), size);
            stream.close();
        }
    }

#ifdef DEBUG
    if (!found) {
        __debugbreak();
    }
#endif

    return found;
}

std::tuple<std::string, uint32_t> FileLoader::LocateFileInDictionary(const fs::path& filename) noexcept
{
    // ergh, fs::path uses backslashes which is bad for us, is there a way
    // to stop that or should we switch back to strings?
    std::string _filename = filename.string();
    std::replace(_filename.begin(), _filename.end(), '\\', '/');

    for (auto it = m_FileListDictionary.begin(); it != m_FileListDictionary.end(); ++it) {
        auto archive = it.key();
        auto data = it.value();

        for (auto it2 = data.begin(); it2 != data.end(); ++it2) {
            auto key = it2.key();
            auto value = it2.value();

            if (value.is_string()) {
                auto valueStr = value.get<std::string>();
                if (valueStr.find(_filename) != std::string::npos) {
                    DEBUG_LOG("LocateFileInDictionary - Found '" << valueStr << "' (" << key << ") in archive '" << archive << "'");

                    auto namehash = static_cast<uint32_t>(std::stoul(key, nullptr, 0));
                    return { archive, namehash };
                }
            }
        }
    }

    return { "", 0 };
}

void FileLoader::ParseCompressedTexture(std::istream& stream, uint64_t size, FileBuffer* output)
{
    // figure out the file size if we need to
    if (size == std::numeric_limits<uint64_t>::max()) {
        stream.seekg(0, std::ios::end);
        size = static_cast<uint64_t>(stream.tellg());
        stream.seekg(0, std::ios::beg);
    }

    JustCause3::AvalancheTexture texture;
    stream.read((char *)&texture, sizeof(texture));

    // load avalanche texture
    if (texture.m_Magic == 0x58545641) {
        auto stream_index = 0;
        auto offset = texture.m_Streams[stream_index].m_Offset;

        auto mipCount = texture.m_Mips - texture.m_MipsRedisent + stream_index;
        auto width = texture.m_Width >> mipCount;
        auto height = texture.m_Height >> mipCount;
        auto depth = texture.m_Depth >> mipCount;

        stream.seekg(offset);
        auto block_size = (size - offset);

        auto block = new char[block_size + 1];
        stream.read(block, block_size);

        DDS_HEADER header;
        ZeroMemory(&header, sizeof(header));
        header.size = 124;
        header.flags = (0x1007 | 0x20000);
        header.width = width;
        header.height = height;
        header.depth = depth;
        header.mipMapCount = mipCount;
        header.ddspf = TextureManager::GetPixelFormat(texture.m_Format);
        header.caps = (8 | 0x1000);

        // ugly things
        output->resize(sizeof(uint32_t) + sizeof(header) + block_size);
        memcpy(&output->front(), (char *)&DDS_MAGIC, sizeof(uint32_t));
        memcpy(&output->front() + sizeof(uint32_t), (char *)&header, sizeof(header));
        memcpy(&output->front() + sizeof(uint32_t) + sizeof(header), block, block_size);

        delete[] block;
    }
    // lol - probably hmddsc file (raw data without headers?)
    else {
        stream.seekg(0, std::ios::beg);

        DDS_HEADER header;
        ZeroMemory(&header, sizeof(header));
        header.size = 124;
        header.flags = (0x1007 | 0x20000);
        header.width = 512;
        header.height = 512;
        header.depth = 1;
        header.mipMapCount = 0;
        header.ddspf = TextureManager::GetPixelFormat(DXGI_FORMAT_BC1_UNORM);
        header.caps = (8 | 0x1000);

        auto block = new char[size + 1];
        stream.read(block, size);

        output->resize(sizeof(uint32_t) + sizeof(header) + size);
        memcpy(&output->front(), (char *)&DDS_MAGIC, sizeof(uint32_t));
        memcpy(&output->front() + sizeof(uint32_t), (char *)&header, sizeof(header));
        memcpy(&output->front() + sizeof(uint32_t) + sizeof(header), block, size);

        delete[] block;
    }
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