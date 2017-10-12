#include <jc3/FileLoader.h>
#include <jc3/RenderBlockFactory.h>
#include <graphics/DDSTextureLoader.h>
#include <graphics/TextureManager.h>
#include <Window.h>

#include <thread>

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
                    if (valueStr.find(".rbm") != std::string::npos)
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

std::string FileLoader::TrimArchiveName(const std::string& filename)
{
    auto result = filename.substr(filename.find_last_of("/") + 1);
    result.resize(result.rfind("."));
    return result;
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

#pragma pack(push, 1)
struct TextureElement {
    uint32_t offset;
    uint32_t size;
    uint16_t unknown;
    uint8_t unknown2;
    bool external;
};
#pragma pack(pop)

// Credits to gibbed for most of this
// http://gib.me
void FileLoader::ReadCompressedTexture(const fs::path& filename, std::vector<uint8_t>* output)
{
    static uint32_t headerLength = 0x80;
    static uint32_t elementsCount = 8;

    std::ifstream file(filename, std::ios::binary);
    if (file.fail()) {
        DEBUG_LOG("FileLoader::ReadCompressedTexture - can't find input file '" << filename.string().c_str() << "'");
        return;
    }

    // get the file size
    file.seekg(0, std::ios::end);
    auto fileSize = static_cast<int32_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    // read the ddsc file
    JustCause3::DDSC ddsc;
    file.read((char *)&ddsc, sizeof(ddsc));

    // make sure this is the correct file
    if (ddsc.magic != 0x58545641) {
        return;
    }

    // read the elements
    TextureElement elements[8];
    uint32_t biggestIndex = 0, biggestSize = 0;

    for (uint32_t i = 0; i < elementsCount; i++) {
        file.read((char *)&elements[i], sizeof(TextureElement));

        if (elements[i].size == 0) {
            continue;
        }

        if ((false && elements[i].external) || (elements[i].external == false) && elements[i].size > biggestSize)
        {
            biggestSize = elements[i].size;
            biggestIndex = i;
        }
    }

    // find the texture rank
    uint32_t rank = 0;
    for (uint32_t i = 0; i < elementsCount; ++i)
    {
        if (i == biggestIndex) {
            continue;
        }

        if (elements[i].size > elements[biggestIndex].size) {
            ++rank;
        }
    }

    auto length = (fileSize - headerLength);
    auto block = new char[length];

    file.seekg(headerLength);
    file.read(block, length);

    DDS_HEADER header;
    ZeroMemory(&header, sizeof(DDS_HEADER));
    header.size = 124;
    header.flags = (0x1007 | 0x20000);
    header.width = ddsc.width >> rank;
    header.height = ddsc.height >> rank;
    header.pitchOrLinearSize = 0;
    header.depth = ddsc.depth;
    header.mipMapCount = 1;
    header.ddspf = TextureManager::GetDDSCPixelFormat(ddsc.format);
    header.caps = (8 | 0x1000);
    header.caps2 = 0;

    // ugly things
    output->resize(sizeof(uint32_t) + sizeof(DDS_HEADER) + length);
    memcpy(&output->front(), (void *)&DDS_MAGIC, sizeof(uint32_t));
    memcpy(&output->front() + sizeof(uint32_t), (void *)&header, sizeof(DDS_HEADER));
    memcpy(&output->front() + sizeof(uint32_t) + sizeof(DDS_HEADER), block, length);

    delete[] block;
    file.close();
}

std::string FileLoader::GetFileName(uint32_t hash)
{
    auto s = std::to_string(hash);
    auto val = m_FileListDictionary[s];

    if (!val.is_string()) {
        return "Unknown File - " + s;
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