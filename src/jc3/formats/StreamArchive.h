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
        if (HasFile(filename)) {
            __debugbreak();
        }
        else {
            auto last_entry = m_Files.back();

            // generate the entry info
            StreamArchiveEntry_t entry;
            entry.m_Filename = filename;
            entry.m_Offset = (last_entry.m_Offset + last_entry.m_Size);
            entry.m_Size = buffer.size();
            m_Files.emplace_back(entry);

            // rebuild archive data
            RebuildSARC(true, entry, buffer);
        }
    }

    bool RemoveFile(const std::string& filename)
    {
#if 0
        auto it = std::find_if(m_Files.begin(), m_Files.end(), [&](const StreamArchiveEntry_t& entry) {
            return entry.m_Filename == filename;
        });

        if (it != m_Files.end()) {
            // remove the file
            const auto& entry = (*it);
            m_Files.erase(std::remove(m_Files.begin(), m_Files.end(), entry), m_Files.end());

            // rebuild archive data
            RebuildSARC(false, (*it));

            // TODO: this is more complicated.
            // if we remove a file which is "above" another file(s) in the stream, then we also need to
            // recalculate all the offsets for each file which is underneath the one we deleted
            return true;
        }
#endif

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
        auto end = m_SARCBytes.begin() + entry.m_Offset + entry.m_Size;

        return std::vector<uint8_t>(start, end);
    }

    template <typename T>
    size_t sarc_insert(uint64_t offset, T& val) {
        m_SARCBytes.insert(m_SARCBytes.begin() + offset, (uint8_t *)&val, (uint8_t *)&val + sizeof(T));
        return offset + sizeof(T);
    }

    size_t sarc_insert(uint64_t offset, const std::string& val) {
        m_SARCBytes.insert(m_SARCBytes.begin() + offset, val.begin(), val.end());
        return offset + val.length();
    }

    void RebuildSARC(bool added_file, const StreamArchiveEntry_t& entry, const std::vector<uint8_t>& buffer)
    {
        // calculate the binary size of the new entry
        const auto entry_raw_size = (sizeof(uint32_t) + entry.m_Filename.length() + sizeof(uint32_t) + sizeof(uint32_t));

        // add
        if (added_file) {
            // update the header
            m_Header.m_Size += entry_raw_size;
            std::memcpy(m_SARCBytes.data(), &m_Header, sizeof(m_Header));

            // calculate the next insert position
            uint64_t pos = sizeof(JustCause3::StreamArchive::SARCHeader);
            for (const auto& file : m_Files) {
                if (file.m_Filename != entry.m_Filename) {
                    pos += (sizeof(uint32_t) + file.m_Filename.length() + sizeof(uint32_t) + sizeof(uint32_t));
                }
            }

            // TODO: resize the sarc buffer
            // need to calculate the new size based on header + buffer sizes

            // insert the data info
            auto length = static_cast<uint32_t>(entry.m_Filename.length());
            pos = sarc_insert(pos, length);
            pos = sarc_insert(pos, entry.m_Filename);
            pos = sarc_insert(pos, entry.m_Offset);
            pos = sarc_insert(pos, entry.m_Size);

            // insert the buffer
            m_SARCBytes.insert(m_SARCBytes.begin() + entry.m_Offset, buffer.begin(), buffer.end());
        }

#if 1
        std::ofstream out("E:/jc3-packing/test.bin", std::ios::binary);
        out.write((char *)m_SARCBytes.data(), m_SARCBytes.size());
        out.close();
#endif
    }
};