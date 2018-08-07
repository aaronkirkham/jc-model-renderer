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

    void AddFile(const std::string& filename, const std::vector<uint8_t>& buffer)
    {
        auto it = std::find_if(m_Files.begin(), m_Files.end(), [&](const StreamArchiveEntry_t& entry) {
            return entry.m_Filename == filename;
        });

        if (it != m_Files.end()) {
            auto& entry = (*it);

            std::vector<uint8_t> temp_buffer;
            uint64_t pos = sizeof(JustCause3::StreamArchive::SARCHeader);

            // update the entry size
            auto old_size = entry.m_Size;
            auto size_diff = static_cast<int32_t>(old_size - buffer.size()); // TODO: make sure this is correct if the file is smaller!
            entry.m_Size = buffer.size();

            // calculate the size
            uint32_t size = 0;
            uint32_t buffer_size = 0;
            for (auto& _entry : m_Files) {
                auto _entry_size = (sizeof(uint32_t) + calc_string_length(_entry.m_Filename) + sizeof(uint32_t) + sizeof(uint32_t));
                size += _entry_size;

                if (_entry.m_Offset != 0) {
                    buffer_size += (_entry_size + _entry.m_Size);
                }
            }

            // resize
            temp_buffer.resize(pos + buffer_size);

            // write the header
            m_Header.m_Size = JustCause3::ALIGN_TO_BOUNDARY(size, 16);
            std::memcpy(temp_buffer.data(), &m_Header, sizeof(m_Header));

            // update all entries
            bool _has_wrote_entry = false;
            for (auto& _entry : m_Files) {
                if (!_has_wrote_entry && _entry.m_Filename == filename) {
                    _has_wrote_entry = true;
                }

                if (_entry.m_Offset != 0) {
                    // calcaulate the aligned length
                    uint32_t alignment = 0;
                    auto length = calc_string_length(_entry.m_Filename, &alignment);

                    // calcaulte offset, if we have already written the file data, adjust every other
                    // offset (these files are underneath our data) by the size difference of the old and new buffer
                    auto offset = static_cast<uint32_t>(_has_wrote_entry ? (_entry.m_Offset + size_diff) : _entry.m_Offset);

                    pos = sarc_insert(&temp_buffer, pos, length);
                    pos = sarc_insert_string(&temp_buffer, pos, _entry.m_Filename, alignment);
                    pos = sarc_insert(&temp_buffer, pos, offset);
                    pos = sarc_insert(&temp_buffer, pos, _entry.m_Size);

                    // update the entry offset
                    auto old_offset = _entry.m_Offset;
                    _entry.m_Offset = offset;

                    // copy the buffer
                    std::memcpy(&temp_buffer[_entry.m_Offset], &m_SARCBytes[old_offset], _entry.m_Size);
                }
            }

            //
            m_SARCBytes = std::move(temp_buffer);

            // TODO: move all bytes of entries below us to their new offset
        }
        else {
            // calculate the binary size of the new entry
            const auto entry_raw_size = (sizeof(uint32_t) + calc_string_length(filename) + sizeof(uint32_t) + sizeof(uint32_t));

            // update the header
            auto new_size = m_Header.m_Size + entry_raw_size;
            m_Header.m_Size = JustCause3::ALIGN_TO_BOUNDARY(new_size, 16); // TODO: confirm alignment isn't 8
            std::memcpy(m_SARCBytes.data(), &m_Header, sizeof(m_Header));

            // calculate the next insert position
            uint64_t pos = sizeof(JustCause3::StreamArchive::SARCHeader);
            uint32_t _index = 0;
            for (auto& file : m_Files) {
                pos += (sizeof(uint32_t) + calc_string_length(file.m_Filename));

                if (file.m_Offset != 0) {
                    // update the current file offset
                    file.m_Offset += entry_raw_size;

                    // update the entry offset
                    m_SARCBytes.erase(m_SARCBytes.begin() + pos, m_SARCBytes.begin() + pos + sizeof(uint32_t));
                    sarc_insert(&m_SARCBytes, pos, file.m_Offset);
                }

                pos += (sizeof(uint32_t) + sizeof(uint32_t));
            }

            auto last_entry = m_Files.back();

            // generate the entry info
            StreamArchiveEntry_t entry;
            entry.m_Filename = filename;
            entry.m_Offset = (last_entry.m_Offset + last_entry.m_Size);
            entry.m_Size = buffer.size();

            // calcaulte the aligned length
            uint32_t alignment = 0;
            auto length = calc_string_length(entry.m_Filename, &alignment);

            // insert the data info
            pos = sarc_insert(&m_SARCBytes, pos, length);
            pos = sarc_insert_string(&m_SARCBytes, pos, entry.m_Filename, alignment);
            pos = sarc_insert(&m_SARCBytes, pos, entry.m_Offset);
            pos = sarc_insert(&m_SARCBytes, pos, entry.m_Size);

            // insert the buffer
            m_SARCBytes.insert(m_SARCBytes.begin() + entry.m_Offset, buffer.begin(), buffer.end());

            m_Files.emplace_back(entry);
        }

        auto total_size = m_SARCBytes.size();
        auto final_alignment = JustCause3::DISTANCE_TO_BOUNDARY(total_size, 4);

        std::stringstream ss; ss << "adding " << final_alignment << " bytes of padding...\n";
        OutputDebugStringA(ss.str().c_str());

        if (final_alignment > 0) {
            m_SARCBytes.resize(total_size + final_alignment);
        }

#if 1
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
    size_t sarc_insert(FileBuffer* buffer, uint64_t offset, T& val) {
        buffer->insert(buffer->begin() + offset, (uint8_t *)&val, (uint8_t *)&val + sizeof(T));
        return offset + sizeof(T);
    }

    // insert string into buffer with padding
    size_t sarc_insert_string(FileBuffer* buffer, uint64_t offset, const std::string& val, uint32_t alignment) {
        static uint8_t empty = 0x0;

        buffer->insert(buffer->begin() + offset, val.begin(), val.end());
        offset += val.length();
        buffer->insert(buffer->begin() + offset, alignment, empty);

        return offset + (sizeof(uint8_t) * alignment);
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