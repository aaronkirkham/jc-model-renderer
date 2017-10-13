#include <jc3/formats/ExportedEntity.h>
#include <Window.h>
#include <jc3/FileLoader.h>

#include <zlib.h>

#include <jc3/formats/RenderBlockModel.h>

RenderBlockModel* test_model = nullptr;

ExportedEntity::ExportedEntity(const fs::path& file)
    : m_File(file)
{
    // make sure this is an ee file
    if (file.extension() != ".ee") {
        throw std::invalid_argument("ExportedEntity type only supports .ee files!");
    }

    // try and open the file
    std::ifstream fh(m_File, std::ios::binary);
    if (fh.fail()) {
        throw std::runtime_error("Failed to read ExportedEntity file!");
    }

    // read the archive header
    JustCause3::AvalancheArchiveHeader header;
    fh.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "AAF", 4) != 0) {
        throw std::runtime_error("Invalid file header. Input file probably isn't an ExportedEntity file.");
    }

    DEBUG_LOG("AvalancheArchiveFormat v" << header.m_Version);
    DEBUG_LOG(" - m_BlockCount=" << header.m_BlockCount);
    DEBUG_LOG(" - m_TotalUncompressedSize=" << header.m_TotalUncompressedSize);

    m_ArchiveChunks.reserve(header.m_BlockCount);

    // read all the compressed blocks
    for (uint32_t i = 0; i < header.m_BlockCount; ++i) {
        JustCause3::AvalancheArchiveChunk chunk;
        chunk.m_DataOffset = static_cast<uint64_t>(fh.tellg());
        fh.read((char *)&chunk.m_CompressedSize, sizeof(chunk.m_CompressedSize));
        fh.read((char *)&chunk.m_UncompressedSize, sizeof(chunk.m_UncompressedSize));

        uint32_t blockSize;
        uint32_t blockMagic;

        fh.read((char *)&blockSize, sizeof(blockSize));
        fh.read((char *)&blockMagic, sizeof(blockMagic));

        DEBUG_LOG(" ==== Block #" << i << " ====");
        DEBUG_LOG(" - Compressed Size: " << chunk.m_CompressedSize);
        DEBUG_LOG(" - Uncompressed Size: " << chunk.m_UncompressedSize);
        DEBUG_LOG(" - Size: " << blockSize);
        DEBUG_LOG(" - Magic: " << std::string((char *)&blockMagic, 4));

        // 'EWAM' magic
        if (blockMagic != 0x4D415745) {
            throw std::runtime_error("Invalid chunk!");
        }

        // read the block data
        std::vector<uint8_t> blockDataCompressed;
        blockDataCompressed.resize(blockSize);
        fh.read((char *)blockDataCompressed.data(), blockSize);

        uLong size = blockSize;
        uLong out_size = chunk.m_UncompressedSize;

        // uncompress the block data
        std::vector<uint8_t> blockDataUncompressed;
        blockDataUncompressed.resize(chunk.m_UncompressedSize);
        auto res = uncompress(blockDataUncompressed.data(), &out_size, blockDataCompressed.data(), size);

        assert(res == Z_OK);
        assert(out_size == chunk.m_UncompressedSize);

        chunk.m_BlockData = std::move(blockDataUncompressed);

        FileLoader::Get()->ParseStreamArchive(chunk.m_BlockData, &chunk.m_FileEntries);
        m_ArchiveChunks.emplace_back(chunk);
    }

    fh.close();

#if 1
    {
        std::string model = "models/jc_characters/animals/cow/cow_mesh_body_lod1.rbm";
        uint32_t offset = 0;
        uint32_t size = 0;
        JustCause3::AvalancheArchiveChunk chunk;

        uint32_t i = 0;
        for (auto& chunk : m_ArchiveChunks) {
            for (auto& entry : chunk.m_FileEntries) {
                if (entry.m_Filename == model) {
                    offset = entry.m_Offset;
                    size = entry.m_Size;
                    chunk = chunk;

                    DEBUG_LOG("Found '" << model << "' in chunk " << i);
                }
            }

            i++;
        }

        auto first = m_ArchiveChunks[0].m_BlockData.begin() + offset;
        auto last = m_ArchiveChunks[0].m_BlockData.begin() + offset + size;
        std::vector<uint8_t> data(first, last);

        test_model = new RenderBlockModel(data);

        Renderer::Get()->Events().RenderFrame.connect([](RenderContext_t* context) {
            if (test_model) {
                test_model->Draw(context);
            }
        });
    }
#endif
}

ExportedEntity::~ExportedEntity()
{
}