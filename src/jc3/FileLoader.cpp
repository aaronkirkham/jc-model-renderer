#include <jc3/FileLoader.h>
#include <jc3/RenderBlockFactory.h>
#include <graphics/DDSTextureLoader.h>
#include <graphics/TextureManager.h>
#include <Window.h>

#include <thread>
#include <istream>

#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/ExportedEntity.h>

#include <zlib.h>

// SARC file extensions: .ee, .bl, .nl

// Credits to gibbed for most of the archive format stuff
// http://gib.me

FileLoader::FileLoader()
{
    m_LoadingDictionary = true;
    m_FileList = std::make_unique<DirectoryList>();

    std::thread load([this] {
        std::ifstream file(fs::current_path() / "assets/dictionary.json");
        if (file.fail()) {
            throw std::runtime_error("FileLoader::FileLoader - Failed to read file list dictionary");
        }

        m_FileListDictionary = json::parse(file);
        file.close();

        // parse
        for (auto it = m_FileListDictionary.begin(); it != m_FileListDictionary.end(); ++it)
        {
            auto data = it.value();
            for (auto it2 = data.begin(); it2 != data.end(); ++it2)
            {
                auto value = it2.value();
                if (value.is_string())
                {
                    auto valueStr = value.get<std::string>();
                    if (valueStr.find(".rbm") != std::string::npos ||
                        valueStr.find(".ee") != std::string::npos || 
                        valueStr.find(".bl") != std::string::npos ||
                        valueStr.find(".nl") != std::string::npos)
                    {
                        m_FileList->Add(valueStr);
                    }
                }
            }
        }

        m_LoadingDictionary = false;
    });

    load.detach();
}

void FileLoader::ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output)
{
    std::ifstream file(filename, std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error("FileLoader::ReadArchiveTable - Failed to read input file.");
    }

    JustCause3::ArchiveTable::TabFileHeader header;
    file.read((char *)&header, sizeof(header));

    if (header.magic != 0x424154) {
        throw std::runtime_error("FileLoader::ReadArchiveTable - File isn't a .tab file.");
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
}

StreamArchive_t* FileLoader::ParseStreamArchive(std::istream& stream)
{
    StreamArchive_t* result = new StreamArchive_t;

    JustCause3::StreamArchive::SARCHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "SARC", 4) != 0) {
        throw std::runtime_error("Invalid file header. Input file probably isn't a StreamArchive file.");
    }

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

        //DEBUG_LOG("StreamArchiveEntry - m_Filename='" << entry.m_Filename.c_str() << "', m_Offset=" << entry.m_Offset << ", m_Size=" << entry.m_Size);

        result->m_Files.emplace_back(entry);

        auto current_pos = stream.tellg();
        if (header.m_Size - (current_pos - start_pos) <= 15) {
            break;
        }
    }

    return result;
}

std::vector<uint8_t> FileLoader::DecompressArchiveFromStream(std::istream& stream)
{
    std::vector<uint8_t> bytes;

    // read the archive header
    JustCause3::AvalancheArchiveHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "AAF", 4) != 0) {
        throw std::runtime_error("Invalid file header. Input file probably isn't an AvalancheArchiveFormat file.");
    }

    DEBUG_LOG("AvalancheArchiveFormat v" << header.m_Version);
    DEBUG_LOG(" - m_BlockCount=" << header.m_BlockCount);
    DEBUG_LOG(" - m_TotalUncompressedSize=" << header.m_TotalUncompressedSize);

    bytes.reserve(header.m_TotalUncompressedSize);

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

        DEBUG_LOG(" ==== Block #" << i << " ====");
        DEBUG_LOG(" - Compressed Size: " << chunk.m_CompressedSize);
        DEBUG_LOG(" - Uncompressed Size: " << chunk.m_UncompressedSize);
        DEBUG_LOG(" - Size: " << blockSize);
        DEBUG_LOG(" - Magic: " << std::string((char *)&blockMagic, 4));

        // 'EWAM' magic
        if (blockMagic != 0x4D415745) {
            throw std::runtime_error("Invalid chunk!");
        }

        // set the chunk offset
        chunk.m_DataOffset = static_cast<uint64_t>(stream.tellg());

        // read the block data
        std::vector<uint8_t> data;
        data.resize(blockSize);
        stream.read((char *)data.data(), blockSize);

        // decompress the block
        {
            uLong compressed_size = (uLong)header.m_BlockSize;
            uLong decompressed_size = (uLong)chunk.m_UncompressedSize;

            std::vector<uint8_t> result;
            result.resize(chunk.m_UncompressedSize);
            auto res = uncompress(result.data(), &decompressed_size, data.data(), compressed_size);

            assert(res == Z_OK);
            assert(decompressed_size == chunk.m_UncompressedSize);

            bytes.insert(bytes.end(), result.begin(), result.end());
        }

        stream.seekg((uint64_t)base_position + blockSize);
    }

    return bytes;
}

StreamArchive_t* FileLoader::ReadStreamArchive(const std::vector<uint8_t>& data)
{
    // TODO: need to read the header, check if the archive is compressed,
    // then just back to the start of the stream!

    // decompress the archive data
    std::istringstream stream(std::string{ (char*)data.data(), data.size() });
    auto m_SARCBytes = DecompressArchiveFromStream(stream);

    // parse the stream archive
    std::istringstream stream2(std::string{ (char*)m_SARCBytes.data(), m_SARCBytes.size() });
    auto result = ParseStreamArchive(stream2);

    result->m_SARCBytes = std::move(m_SARCBytes);
    return result;
}

StreamArchive_t* FileLoader::ReadStreamArchive(const fs::path& filename)
{
    if (!fs::exists(filename)) {
        throw std::runtime_error("FileLoader::ReadStreamArchive - Input file doesn't exist.");
    }

    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        throw std::runtime_error("FileLoader::ReadStreamArchive - Failed to read input file.");
    }

    // TODO: need to read the header, check if the archive is compressed,
    // then just back to the start of the stream!

    auto m_SARCBytes = DecompressArchiveFromStream(stream);
    stream.close();

    // parse the stream archive
    std::istringstream stream2(std::string{ (char*)m_SARCBytes.data(), m_SARCBytes.size() });
    auto result = ParseStreamArchive(stream2);

    result->m_SARCBytes = std::move(m_SARCBytes);
    return result;
}

void FileLoader::ReadCompressedTexture(const std::vector<uint8_t>& buffer, uint64_t size, std::vector<uint8_t>* output)
{
    std::istringstream stream(std::string{ (char*)buffer.data(), buffer.size() });
    ParseCompressedTexture(stream, size, output);
}

void FileLoader::ReadCompressedTexture(const fs::path& filename, std::vector<uint8_t>* output)
{
    if (!fs::exists(filename)) {
        DEBUG_LOG("FileLoader::ReadCompressedTexture - can't find input file '" << filename.string().c_str() << "'");
        return;
    }

    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("FileLoader::ReadCompressedTexture - can't find input file '" << filename.string().c_str() << "'");
        return;
    }

    ParseCompressedTexture(stream, std::numeric_limits<uint64_t>::max(), output);
    stream.close();
}

void FileLoader::CacheLoadedArchive(StreamArchive_t* archive)
{
    if (std::find(m_LoadedArchives.begin(), m_LoadedArchives.end(), archive) == m_LoadedArchives.end()) {
        m_LoadedArchives.emplace_back(archive);
    }
}

void FileLoader::RemoveLoadedArchive(StreamArchive_t* archive)
{
    if (std::find(m_LoadedArchives.begin(), m_LoadedArchives.end(), archive) != m_LoadedArchives.end()) {
        m_LoadedArchives.erase(std::remove(m_LoadedArchives.begin(), m_LoadedArchives.end(), archive), m_LoadedArchives.end());
    }
}

std::tuple<StreamArchive_t*, StreamArchiveEntry_t> FileLoader::GetStreamArchiveFromFile(const fs::path& file)
{
    for (auto& archive : m_LoadedArchives) {
        for (auto& entry : archive->m_Files) {
            if (entry.m_Filename == file) {
                return { archive, entry };
            }
        }
    }

    return { nullptr, StreamArchiveEntry_t{} };
}

void FileLoader::ReadFileFromArchive(const std::string& archive, const std::string& filename, uint32_t namehash)
{
    std::thread load([this, archive, filename, namehash] {
        fs::path jc3_dir = "D:/Steam/steamapps/common/Just Cause 3";
        fs::path tab_file = jc3_dir / "archives_win64" / (archive + ".tab");
        fs::path arc_file = jc3_dir / "archives_win64" / (archive + ".arc");

        // read the archive table
        JustCause3::ArchiveTable::VfsArchive archiveTable;
        ReadArchiveTable(tab_file.string(), &archiveTable);

        uint64_t offset = 0;
        uint64_t size = 0;
        bool found = false;

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

        assert(found);

        // did we find what we're looking for?
        if (found) {
            std::ifstream stream(arc_file, std::ios::binary);
            stream.seekg(offset);

            std::vector<uint8_t> data;
            data.resize(size);
            stream.read((char *)data.data(), size);

            DEBUG_LOG("finished reading data from file.");

            // TODO: a proper handler for this stuff
            if (filename.find(".ee") != std::string::npos ||
                filename.find(".bl") != std::string::npos ||
                filename.find(".nl") != std::string::npos) {
                new ExportedEntity(filename, data);
            }
            else if (filename.find(".rbm") != std::string::npos) {
                new RenderBlockModel(filename, data);
            }

            stream.close();
        }
    });

    load.detach();
}

std::string FileLoader::GetFileName(uint32_t hash)
{
    auto filenamehash = std::to_string(hash);
    auto val = m_FileListDictionary[filenamehash];

    if (!val.is_string()) {
        return "Unknown File - " + filenamehash;
    }

    return val.get<std::string>();
}

std::string FileLoader::GetArchiveName(const std::string& filename)
{
    for (auto it = m_FileListDictionary.begin(); it != m_FileListDictionary.end(); ++it)
    {
        auto archive = it.key();
        auto data = it.value();
        for (auto it2 = data.begin(); it2 != data.end(); ++it2)
        {
            auto value = it2.value().get<std::string>();
            if (value == filename) {
                return archive;
            }
        }
    }

    return "(Unknown Archive)";
}

void FileLoader::LocateFileInDictionary(const std::string& filename, DictionaryLookupCallback callback)
{
    std::thread lookup([this, filename, callback] {
        while (m_LoadingDictionary) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        for (auto it = m_FileListDictionary.begin(); it != m_FileListDictionary.end(); ++it) {
            auto archive = it.key();
            auto data = it.value();

            for (auto it2 = data.begin(); it2 != data.end(); ++it2) {
                auto key = it2.key();
                auto value = it2.value();

                if (value.is_string()) {
                    auto valueStr = value.get<std::string>();
                    if (valueStr.find(filename) != std::string::npos) {
                        DEBUG_LOG("LocateFileInDictionary - Found '" << valueStr << "' (" << key << ") in archive '" << archive << "'");

                        auto namehash = static_cast<uint32_t>(std::stoul(key, nullptr, 0));
                        return callback(true, archive, namehash, valueStr);
                    }
                }
            }
        }

        return callback(false, "", 0, filename);
    });

    lookup.detach();
}

void FileLoader::ParseCompressedTexture(std::istream& stream, uint64_t size, std::vector<uint8_t>* output)
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
        header.pitchOrLinearSize = 0;
        header.depth = depth;
        header.mipMapCount = mipCount;
        header.ddspf = TextureManager::GetPixelFormat(texture.m_Format);
        header.caps = (8 | 0x1000);
        header.caps2 = 0;

        // ugly things
        output->resize(sizeof(uint32_t) + sizeof(header) + block_size);
        memcpy(&output->front(), (char *)&DDS_MAGIC, sizeof(uint32_t));
        memcpy(&output->front() + sizeof(uint32_t), (char *)&header, sizeof(header));
        memcpy(&output->front() + sizeof(uint32_t) + sizeof(header), block, block_size);

        delete[] block;
    }
}