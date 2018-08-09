#pragma once

#include <vector>
#include <jc3/Types.h>

struct StreamArchiveEntry_t
{
    std::string m_Filename;
    uint32_t m_Offset;
    uint32_t m_Size;
};

struct StreamArchive_t
{
    fs::path m_Filename;
    JustCause3::StreamArchive::SARCHeader m_Header;
    std::vector<uint8_t> m_SARCBytes;
    std::vector<StreamArchiveEntry_t> m_Files;
    bool m_UsingTOC = false;

    void AddFile(const std::string& filename, const std::vector<uint8_t>& buffer)
    {
        auto it = std::find_if(m_Files.begin(), m_Files.end(), [&](const StreamArchiveEntry_t& entry) {
            return entry.m_Filename == filename;
        });

        StreamArchiveEntry_t* entry = nullptr;

        // if we don't have the file, add it to the end
        if (it == m_Files.end()) {
            // find the last entry with data in the buffer
            auto last = std::find_if(m_Files.rbegin(), m_Files.rend(), [](const StreamArchiveEntry_t& entry) {
                return entry.m_Offset != 0;
            });

            // add a new entry
            StreamArchiveEntry_t new_entry;
            new_entry.m_Filename = filename;
            new_entry.m_Offset = 1; // NOTE: generated later, keep it above 0 or it will be tret as a patched file
            new_entry.m_Size = buffer.size();
            m_Files.emplace_back(std::move(new_entry));

            entry = &m_Files.back();
        }
        else {
            entry = &(*it);
            entry->m_Size = buffer.size();
        }

        assert(entry);

        std::vector<uint8_t> temp_buffer;
        uint64_t pos = sizeof(JustCause3::StreamArchive::SARCHeader);

        // calculate the header and data size
        uint32_t header_size = 0;
        uint32_t data_size = 0;
        for (auto& _entry : m_Files) {
            auto _raw_entry_size = (sizeof(uint32_t) + calc_string_length(_entry.m_Filename) + sizeof(uint32_t) + sizeof(uint32_t));
            header_size += _raw_entry_size;

            auto size = _entry.m_Offset != 0 ? _entry.m_Size : 0;
            data_size += JustCause3::ALIGN_TO_BOUNDARY(size);
        }

        // update the header
        m_Header.m_Size = JustCause3::ALIGN_TO_BOUNDARY(header_size, 16);

        temp_buffer.resize(pos + m_Header.m_Size + data_size);

        // write the header
        std::memcpy(temp_buffer.data(), &m_Header, sizeof(m_Header));

        // update all entries
        bool _has_wrote_entry = false;
        uint32_t current_data_offset = sizeof(JustCause3::StreamArchive::SARCHeader) + m_Header.m_Size;
        for (auto& _entry : m_Files) {
            const bool _is_the_file = _entry.m_Filename == filename;
            if (!_has_wrote_entry && _is_the_file) {
                _has_wrote_entry = true;
            }

            auto data_offset = _entry.m_Offset != 0 ? current_data_offset : 0;

            // calculate the aligned length for the entry filename
            uint32_t alignment = 0;
            auto length = calc_string_length(_entry.m_Filename, &alignment);

            // insert the entry header
            pos = sarc_copy(&temp_buffer, pos, length);
            pos = sarc_copy_aligned_string(&temp_buffer, pos, _entry.m_Filename, alignment);
            pos = sarc_copy(&temp_buffer, pos, data_offset);
            pos = sarc_copy(&temp_buffer, pos, _entry.m_Size);

            // copy the buffer data
            if (data_offset != 0) {
                if (!_is_the_file) {
                    std::memcpy(&temp_buffer[data_offset], &m_SARCBytes[_entry.m_Offset], _entry.m_Size);
                }
                else {
                    std::memcpy(&temp_buffer[data_offset], buffer.data(), buffer.size());
                }

                // update the data offset
                _entry.m_Offset = data_offset;
            }

            auto next_offset = current_data_offset + (data_offset != 0 ? _entry.m_Size : 0);
            current_data_offset = JustCause3::ALIGN_TO_BOUNDARY(next_offset);
        }

        m_SARCBytes = std::move(temp_buffer);

#if 0
        std::ofstream out("C:/users/aaron/desktop/packing-test.bin", std::ios::binary);
        out.write((char *)m_SARCBytes.data(), m_SARCBytes.size());
        out.close();
#endif
    }

    bool RemoveFile(const std::string& filename)
    {
        return false;
    }

    bool HasFile(const std::string& filename)
    {
        auto it = std::find_if(m_Files.begin(), m_Files.end(), [&](const StreamArchiveEntry_t& entry) {
            return entry.m_Filename == filename;
        });

        return it != m_Files.end();
    }

    std::vector<uint8_t> GetEntryBuffer(const StreamArchiveEntry_t& entry)
    {
        auto start = m_SARCBytes.begin() + entry.m_Offset;
        auto end = start + entry.m_Size;

        return std::vector<uint8_t>(start, end);
    }

    template <typename T>
    size_t sarc_copy(FileBuffer* buffer, uint64_t offset, T& val) {
        std::memcpy(buffer->data() + offset, (char *)&val, sizeof(T));
        return offset + sizeof(T);
    }

    size_t sarc_copy_aligned_string(FileBuffer* buffer, uint64_t offset, const std::string& val, uint32_t alignment) {
        static uint8_t empty = 0;

        std::memcpy(buffer->data() + offset, val.data(), val.length());
        std::memcpy(buffer->data() + offset + val.length(), &empty, alignment);
        return offset + val.length() + (sizeof(uint8_t) * alignment);
    }

    // calculate string length with alignment
    uint32_t calc_string_length(const std::string& value, uint32_t* alignment = nullptr) {
        auto length = value.length();
        auto align = JustCause3::DISTANCE_TO_BOUNDARY(length, 4);

        if (alignment) {
            *alignment = align;
        }

        return length + align;
    }
};