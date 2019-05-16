#pragma once

#include <any>
#include <filesystem>
#include <memory>
#include <queue>
#include <unordered_map>

#include "../../factory.h"
#include "../../graphics/types.h"
#include "../types.h"

static constexpr const char* ADF_BUILTIN_TYPE_NAMES[12] = {"uint8",  "int8",  "uint16", "int16",  "uint32", "int32",
                                                           "uint64", "int64", "float",  "double", "String", "void"};

class AvalancheDataFormat : public Factory<AvalancheDataFormat>
{
  private:
    std::filesystem::path                           m_Filename;
    FileBuffer                                      m_Buffer;
    std::vector<jc::AvalancheDataFormat::Type*>     m_Types;
    std::vector<jc::AvalancheDataFormat::Instance*> m_Instances;
    std::vector<const char*>                        m_Strings;
    std::map<uint32_t, const char*>                 m_StringHashes;

    void        AddBuiltInType(jc::AvalancheDataFormat::EAdfType type, jc::AvalancheDataFormat::ScalarType scalar_type,
                               uint32_t size, uint32_t type_index, uint16_t flags);
    void        ParseTypes(const char* data, uint64_t data_size);
    void        LoadTypeLibraries();
    bool        ParseHeader(jc::AvalancheDataFormat::Header* data, uint64_t data_size,
                            jc::AvalancheDataFormat::Header* header_out, bool* byte_swap_out);
    const char* GetInstanceNameFromStrings(const char* string_lengths, const char* strings, uint64_t string_index);
    // const char* GetStringFromHash(const char* string_hashes, uint32_t string_hash_count, uint64_t hash);
    jc::AvalancheDataFormat::Type* FindType(uint32_t type_hash);

  public:
    AvalancheDataFormat(const std::filesystem::path& file, const FileBuffer& buffer);
    virtual ~AvalancheDataFormat() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    void GetInstance(uint32_t index, jc::AvalancheDataFormat::SInstanceInfo* instance_info);
    void ReadInstance(uint32_t name_hash, uint32_t type_hash, void** out_instance);

    const char* HashLookup(const uint32_t hash)
    {
        const auto it = m_StringHashes.find(hash);
        if (it == m_StringHashes.end()) {
            return "";
        }

        return it->second;
    }

    static void ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);
};
