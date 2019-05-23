#include "avalanche_archive.h"
#include "render_block_model.h"

#include "../../graphics/ui.h"
#include "../../util.h"
#include "../../window.h"
#include "../file_loader.h"
#include "../hashlittle.h"

#include <zlib.h>

std::recursive_mutex                                  Factory<AvalancheArchive>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheArchive>> Factory<AvalancheArchive>::Instances;

extern bool g_IsJC4Mode;

AvalancheArchive::AvalancheArchive(const std::filesystem::path& filename)
    : m_Filename(filename)
{
    m_FileList = std::make_unique<DirectoryList>();
    UI::Get()->SwitchToTab(TreeViewTab_Archives);
}

AvalancheArchive::AvalancheArchive(const std::filesystem::path& filename, const FileBuffer& buffer, bool external)
    : m_Filename(filename)
    , m_FromExternalSource(external)
{
    m_FileList = std::make_unique<DirectoryList>();
    UI::Get()->SwitchToTab(TreeViewTab_Archives);

    // parse the buffer
    Parse(buffer);
}

void AvalancheArchive::Add(const std::filesystem::path& filename, const std::filesystem::path& root)
{
    // TODO: only add files which are a supported format
    // generic textures, models, etc

    if (std::filesystem::is_directory(filename)) {
        for (const auto& f : std::filesystem::directory_iterator(filename)) {
            Add(f.path(), root);
        }
    } else {
        std::ifstream stream(filename, std::ios::binary);
        if (stream.fail()) {
            SPDLOG_ERROR("Failed to open file stream!");
#ifdef DEBUG
            __debugbreak();
#endif
            return;
        }

        // strip the root directory from the path
        const auto& name = filename.lexically_relative(root);
        const auto  size = std::filesystem::file_size(filename);

        // read the file buffer
        FileBuffer buffer;
        buffer.resize(size);
        stream.read((char*)buffer.data(), size);
        stream.close();
        WriteFileToBuffer(name, buffer);
    }
}

bool AvalancheArchive::HasFile(const std::filesystem::path& filename)
{
    const auto& _filename = filename.generic_string();
    const auto  it        = std::find_if(m_Entries.begin(), m_Entries.end(),
                                 [_filename](const ArchiveEntry_t& item) { return item.m_Filename == _filename; });

    return it != m_Entries.end();
}

void AvalancheArchive::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    AvalancheArchive::make(filename, data, external);
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

        // write the compressed archive
        archive->Compress([archive, path, status_text_id](bool success, FileBuffer data) {
            if (success) {
                std::ofstream stream(path, std::ios::binary);
                if (stream.fail()) {
                    SPDLOG_ERROR("Failed to open stream! \"{}\"", path.string());
                    return;
                }

                stream.write((char*)data.data(), data.size());
                stream.close();

                // write TOC
                if (archive->IsUsingTOC()) {
                    auto toc_path = path;
                    toc_path += ".toc";

                    SPDLOG_INFO("Archive was loaded with a TOC. Writing new TOC to \"{}\"", toc_path.string());

                    // write the .toc file
                    if (!archive->WriteTOC(toc_path)) {
                        SPDLOG_ERROR("Failed to write TOC!");
                    }
                }
            }

            UI::Get()->PopStatusText(status_text_id);
        });

        return true;
    }

    return false;
}

void AvalancheArchive::Parse(const FileBuffer& buffer, ParseCallback_t callback)
{
    std::thread([&, buffer = std::move(buffer), callback] {
        if (buffer.size() == 0) {
            if (callback) {
                callback(false);
            }

            return;
        }

        // do we need to decompress the AAF?
        char magic[4];
        std::memcpy(&magic, buffer.data(), sizeof(magic));
        if (!strncmp(magic, "AAF", 4)) {
            Decompress(buffer, [&, callback](bool success, FileBuffer data) {
                if (success) {
                    ParseSARC(std::move(data), callback);
                } else if (callback) {
                    callback(false);
                }
            });
        } else {
            ParseSARC(std::move(buffer), callback);
        }
    }).detach();
}

void AvalancheArchive::ParseSARC(const FileBuffer& buffer, ParseCallback_t callback)
{
    util::byte_array_buffer buf(buffer.data(), buffer.size());
    std::istream            stream(&buf);

    jc::StreamArchive::Header header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp(header.m_Magic, "SARC", 4) != 0) {
        SPDLOG_ERROR("Invalid header magic");
        if (callback) {
            callback(false);
        }

        return;
    }

    // copy the buffer (needed to read entries from it later)
    m_Buffer = buffer;

    // just cause 4
    if (header.m_Version == 3) {
        uint32_t strings_length;
        stream.read((char*)&strings_length, sizeof(uint32_t));

        std::map<uint32_t, std::string> filenames;

        const auto data_offset = (static_cast<uint64_t>(stream.tellg()) + strings_length);
        while (static_cast<uint64_t>(stream.tellg()) < data_offset) {
            std::string filename;
            std::getline(stream, filename, '\0');

            const auto hash = hashlittle(filename.c_str());
            filenames[hash] = std::move(filename);
        }

        assert(stream.tellg() == data_offset);

        while (stream.tellg() < header.m_Size) {
            uint32_t name_offset, file_offset, uncompressed_size, namehash, extension_hash;

            stream.read((char*)&name_offset, sizeof(uint32_t));
            stream.read((char*)&file_offset, sizeof(uint32_t));
            stream.read((char*)&uncompressed_size, sizeof(uint32_t));
            stream.read((char*)&namehash, sizeof(uint32_t));
            stream.read((char*)&extension_hash, sizeof(uint32_t));

            ArchiveEntry_t entry{};
            entry.m_Filename = filenames[namehash];
            entry.m_Offset   = file_offset;
            entry.m_Size     = uncompressed_size;
            entry.m_Patched  = false;
            m_Entries.emplace_back(std::move(entry));
        }
    }
    // just cause 3
    else {
        const auto start_pos = stream.tellg();
        while (true) {
            uint32_t length;
            stream.read((char*)&length, sizeof(uint32_t));

            // read the filename string
            auto filename = std::unique_ptr<char[]>(new char[length + 1]);
            stream.read(filename.get(), length);
            filename[length] = '\0';

            // read archive entry
            ArchiveEntry_t entry{};
            entry.m_Filename = filename.get();
            entry.m_Patched  = false;
            stream.read((char*)&entry.m_Offset, sizeof(entry.m_Offset));
            stream.read((char*)&entry.m_Size, sizeof(entry.m_Size));
            m_Entries.emplace_back(std::move(entry));

            if (header.m_Size - (stream.tellg() - start_pos) <= 15) {
                break;
            }
        }

        // if we are not loading from an external source, look for the toc
        if (!m_FromExternalSource) {
            auto toc_filename = m_Filename;
            toc_filename += ".toc";

            return FileLoader::Get()->ReadFile(toc_filename, [&, callback](bool success, FileBuffer data) {
                if (success) {
                    ParseTOC(data);
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
    }

    // add entries to the filelist
    for (const auto& entry : m_Entries) {
        m_FileList->Add(entry.m_Filename);
    }

    if (callback) {
        callback(true);
    }
}

void AvalancheArchive::ParseTOC(const FileBuffer& buffer)
{
#ifdef DEBUG
    uint32_t num_added   = 0;
    uint32_t num_patched = 0;
#endif

    util::byte_array_buffer buf(buffer.data(), buffer.size());
    std::istream            stream(&buf);

    while (static_cast<size_t>(stream.tellg()) < buffer.size()) {
        uint32_t length;
        stream.read((char*)&length, sizeof(uint32_t));

        // read the filename string
        const auto filename = std::unique_ptr<char[]>(new char[length + 1]);
        stream.read(filename.get(), length);
        filename[length] = '\0';

        // read archive entry
        ArchiveEntry_t entry{};
        entry.m_Filename = filename.get();
        entry.m_Patched  = true;
        stream.read((char*)&entry.m_Offset, sizeof(entry.m_Offset));
        stream.read((char*)&entry.m_Size, sizeof(entry.m_Size));

        const auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                                     [&](const ArchiveEntry_t& item) { return item.m_Filename == entry.m_Filename; });
        if (it == m_Entries.end()) {
            m_Entries.emplace_back(std::move(entry));

#ifdef DEBUG
            ++num_added;
#endif
        } else {
#ifdef DEBUG
            if ((*it).m_Offset != entry.m_Offset || (*it).m_Size != entry.m_Size) {
                ++num_patched;
            }
#endif

            // update file offset and size
            (*it).m_Offset = entry.m_Offset;
            (*it).m_Size   = entry.m_Size;
        }
    }

#ifdef DEBUG
    if (num_added > 0 || num_patched > 0) {
        SPDLOG_INFO("Added {} and patched {} files from TOC.", num_added, num_patched);
    }
#endif
}

bool AvalancheArchive::WriteTOC(const std::filesystem::path& filename)
{
    std::ofstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        SPDLOG_ERROR("Failed to open stream! \"{}\"", filename.string());
        return false;
    }

    if (m_Entries.size() == 0) {
        SPDLOG_ERROR("Nothing to write.");
        return false;
    }

    for (const auto& entry : m_Entries) {
        const auto length = static_cast<uint32_t>(entry.m_Filename.length());

        stream.write((char*)&length, sizeof(uint32_t));
        stream.write((char*)entry.m_Filename.c_str(), length);
        stream.write((char*)&entry.m_Offset, sizeof(uint32_t));
        stream.write((char*)&entry.m_Size, sizeof(uint32_t));
    }

    stream.close();
    return true;
}

void AvalancheArchive::Compress(CompressCallback_t callback)
{
    using namespace jc::AvalancheArchive;

    assert(m_Buffer.size() > 0);

    std::thread([&, callback] {
        std::ostringstream stream;
        const auto         num_chunks = (1 + (m_Buffer.size() / MAX_CHUNK_DATA_SIZE));

        // create the header
        Header header;
        header.m_TotalUncompressedSize  = m_Buffer.size();
        header.m_UncompressedBufferSize = (num_chunks > 1 ? MAX_CHUNK_DATA_SIZE : m_Buffer.size());
        header.m_ChunkCount             = num_chunks;

        // write the header
        stream.write((char*)&header, sizeof(header));

        SPDLOG_INFO("ChunkCount={}, TotalUncompressedSize={}, UncompressedBufferSize={}", header.m_ChunkCount,
                    header.m_TotalUncompressedSize, header.m_UncompressedBufferSize);

        // create the chunks
        std::vector<Chunk> chunks;
        uint32_t           last_chunk_offset = 0;
        for (uint32_t i = 0; i < num_chunks; ++i) {
            Chunk chunk;

            // split the data if we have more than 1 chunk
            if (num_chunks > 1) {
                // calculate the chunk size
                const auto block_size = (m_Buffer.size() - last_chunk_offset);
                const auto chunk_size =
                    (block_size > MAX_CHUNK_DATA_SIZE ? (MAX_CHUNK_DATA_SIZE % block_size) : block_size);

                FileBuffer n_block_data(m_Buffer.begin() + last_chunk_offset,
                                        m_Buffer.begin() + last_chunk_offset + chunk_size);
                last_chunk_offset += chunk_size;

                // update the chunk
                chunk.m_UncompressedSize = chunk_size;
                chunk.m_BlockData        = std::move(n_block_data);
            } else {
                chunk.m_UncompressedSize = m_Buffer.size();
                chunk.m_BlockData        = m_Buffer;
            }

            // compress the chunk data
            auto       decompressed_size = (uLong)chunk.m_UncompressedSize;
            auto       compressed_size   = compressBound(decompressed_size);
            FileBuffer compressed_block(compressed_size);
            const auto result =
                compress(compressed_block.data(), &compressed_size, chunk.m_BlockData.data(), decompressed_size);

            if (result != Z_OK) {
                SPDLOG_ERROR("Failed to compressed chunk #{} (CompressedSize={}, DecompressedSize={}).", i,
                             compressed_size, decompressed_size);
#ifdef DEBUG
                __debugbreak();
#endif

                return callback(false, {});
            }

            // update the chunk
            // NOTE: -6 for the ZLIB magic (2) and checksum (4)
            chunk.m_CompressedSize = (static_cast<uint32_t>(compressed_size) - 6);

            // calculate the distance to the 16-byte boundary after we write our data
            const auto pos     = (static_cast<uint32_t>(stream.tellp()) + CHUNK_SIZE + chunk.m_CompressedSize);
            auto       padding = jc::DISTANCE_TO_BOUNDARY(pos, 16);

            // calculate the data size
            chunk.m_DataSize = (CHUNK_SIZE + chunk.m_CompressedSize + padding);

            // write the chunk
            stream.write((char*)&chunk.m_CompressedSize, sizeof(chunk.m_CompressedSize));
            stream.write((char*)&chunk.m_UncompressedSize, sizeof(chunk.m_UncompressedSize));
            stream.write((char*)&chunk.m_DataSize, sizeof(chunk.m_DataSize));
            stream.write((char*)&chunk.m_Magic, sizeof(chunk.m_Magic));

            // write the chunk data
            // NOTE: +2 to skip the ZLIB magic
            stream.write((char*)compressed_block.data() + 2, chunk.m_CompressedSize);

            // write the padding
            static uint32_t PADDING_BYTE = 0x30;
            SPDLOG_INFO("Writing {} bytes of padding", padding);
            for (decltype(padding) i = 0; i < padding; ++i) {
                stream.write((char*)&PADDING_BYTE, 1);
            }
        }

        // TODO: better way?
        FileBuffer  buffer;
        const auto& str = stream.str();
        buffer.insert(buffer.end(), str.begin(), str.end());
        return callback(true, std::move(buffer));
    }).detach();
}

void AvalancheArchive::Decompress(const FileBuffer& buffer, DecompressCallback_t callback)
{
    using namespace jc::AvalancheArchive;

    assert(buffer.size() > 0);

    std::thread([&, buffer, callback] {
        util::byte_array_buffer buf(buffer.data(), buffer.size());
        std::istream            stream(&buf);

        // read the archive header
        Header header;
        stream.read((char*)&header, sizeof(header));

        // ensure the header magic is correct
        if (strncmp(header.m_Magic, "AAF", 4) != 0) {
            SPDLOG_ERROR("Invalid header magic");
            return callback(false, {});
        }

        SPDLOG_INFO("ChunkCount={}, TotalUncompressedSize={}, UncompressedBufferSize={}", header.m_ChunkCount,
                    header.m_TotalUncompressedSize, header.m_UncompressedBufferSize);

        FileBuffer out_buffer;
        out_buffer.reserve(header.m_TotalUncompressedSize);

        // read all the chunks
        for (uint32_t i = 0; i < header.m_ChunkCount; ++i) {
            const auto base_position = stream.tellg();

            // read the chunk
            Chunk chunk;
            stream.read((char*)&chunk.m_CompressedSize, sizeof(chunk.m_CompressedSize));
            stream.read((char*)&chunk.m_UncompressedSize, sizeof(chunk.m_UncompressedSize));
            stream.read((char*)&chunk.m_DataSize, sizeof(chunk.m_DataSize));
            stream.read((char*)&chunk.m_Magic, sizeof(chunk.m_Magic));

            SPDLOG_INFO("AAF chunk #{}, CompressedSize={}, UncompressedSize={}, DataSize={}", i, chunk.m_CompressedSize,
                        chunk.m_UncompressedSize, chunk.m_DataSize);

            // make sure the block magic is correct
            if (chunk.m_Magic != 0x4D415745) {
                SPDLOG_ERROR("Invalid chunk magic");
                return callback(false, {});
            }

            // read the block data
            FileBuffer block_data;
            block_data.resize(chunk.m_CompressedSize);
            stream.read((char*)block_data.data(), chunk.m_CompressedSize);

            // decompress the block data
            auto       compressed_size   = (uLong)chunk.m_CompressedSize;
            auto       decompressed_size = (uLong)chunk.m_UncompressedSize;
            FileBuffer decompressed_block(decompressed_size);
            const auto result =
                uncompress(decompressed_block.data(), &decompressed_size, block_data.data(), compressed_size);

            if (result != Z_OK || decompressed_size != chunk.m_UncompressedSize) {
                SPDLOG_ERROR("Failed to decompress chunk #{} (CompressedSize={}, DecompressedSize={}).", i,
                             compressed_size, decompressed_size);
#ifdef DEBUG
                __debugbreak();
#endif

                return callback(false, {});
            }

            out_buffer.insert(out_buffer.end(), decompressed_block.begin(), decompressed_block.end());

            // goto the next block
            stream.seekg(static_cast<uint64_t>(base_position) + chunk.m_DataSize);
        }

        if (out_buffer.size() != header.m_TotalUncompressedSize) {
            SPDLOG_ERROR("Failed to decompress stream archive. (Expecting={}, Got={})", header.m_TotalUncompressedSize,
                         out_buffer.size());
#ifdef DEBUG
            __debugbreak();
#endif

            return callback(false, {});
        }

        return callback(true, std::move(out_buffer));
    }).detach();
}

void AvalancheArchive::WriteFileToBuffer(const std::filesystem::path& filename, const FileBuffer& data)
{
    // TODO: remove when we support JC4 versions of Stream Archive.
    if (g_IsJC4Mode) {
#ifdef DEBUG
        __debugbreak();
#endif
        return;
    }

    // add the file to the file list dictionary
    const auto& _filename = filename.generic_string();
    if (m_FileList && !HasFile(_filename)) {
        m_FileList->Add(_filename);
    }

    ArchiveEntry_t* entry = nullptr;
    const auto      it    = std::find_if(m_Entries.begin(), m_Entries.end(),
                                 [_filename](const ArchiveEntry_t& item) { return item.m_Filename == _filename; });

    // if we don't already have this file, add a new entry to the end
    if (it == m_Entries.end()) {
        ArchiveEntry_t new_entry{};
        new_entry.m_Filename = _filename;
        new_entry.m_Offset   = 1;
        new_entry.m_Size     = data.size();
        new_entry.m_Patched  = false;
        m_Entries.emplace_back(std::move(new_entry));

        entry = &m_Entries.back();
    } else {
        entry         = &(*it);
        entry->m_Size = data.size();
    }

    assert(entry);

    jc::StreamArchive::Header header;
    header.m_Version = g_IsJC4Mode ? 3 : 2;

    FileBuffer temp_buffer;
    auto       pos = sizeof(header);

    // calculate header and data size
    uint32_t header_size = 0;
    uint32_t data_size   = 0;
    for (const auto& _entry : m_Entries) {
        // length + string length + offset + size
        const auto raw_entry_size =
            (sizeof(uint32_t) + CalcStringLength(_entry.m_Filename) + sizeof(uint32_t) + sizeof(uint32_t));
        const auto raw_data_size = (_entry.m_Offset != 0 ? _entry.m_Size : 0);

        header_size += raw_entry_size;
        data_size += jc::ALIGN_TO_BOUNDARY(raw_data_size);
    }

    // update header
    header.m_Size = jc::ALIGN_TO_BOUNDARY(header_size, 16);

    // allocate the temp buffer
    temp_buffer.resize(sizeof(header) + header.m_Size + data_size);

    // write the header
    std::memcpy(temp_buffer.data(), &header, sizeof(header));

    // update all entries
    uint32_t current_data_offset = (sizeof(header) + header.m_Size);
    for (auto& _entry : m_Entries) {
        const bool is_the_file = _entry.m_Filename == _filename;
        const auto data_offset = (_entry.m_Offset != 0 ? current_data_offset : 0);

        // calculate aligned string length
        uint32_t   alignment = 0;
        const auto length    = CalcStringLength(_entry.m_Filename, &alignment);

        // copy the entry to the temp buffer
        CopyToBuffer(&temp_buffer, &pos, length);
        CopyAlignedStringToBuffer(&temp_buffer, &pos, _entry.m_Filename, alignment);
        CopyToBuffer(&temp_buffer, &pos, data_offset);
        CopyToBuffer(&temp_buffer, &pos, _entry.m_Size);

        // copy the entry data
        if (data_offset != 0) {
            if (!is_the_file) {
                std::memcpy(&temp_buffer[data_offset], &m_Buffer[_entry.m_Offset], _entry.m_Size);
            } else {
                std::memcpy(&temp_buffer[data_offset], data.data(), data.size());
            }

            // update the entry offset
            _entry.m_Offset = data_offset;
        }

        auto next_offset    = current_data_offset + (data_offset != 0 ? _entry.m_Size : 0);
        current_data_offset = jc::ALIGN_TO_BOUNDARY(next_offset);
    }

    // copy the new buffer
    m_Buffer            = std::move(temp_buffer);
    m_HasUnsavedChanged = true;
}
