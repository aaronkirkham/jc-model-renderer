#include "pch.h"

#include "name_hash_lookup.h"

#include "app/app.h"

#include <AvaFormatLib/util/byte_array_buffer.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

namespace jcmr
{
static constexpr i32 INTERNAL_RESOURCE_FILELIST = 512;

using LookupTable = std::unordered_map<u32, std::string>;

static bool        s_namehash_lookup_table_is_loaded = false;
static LookupTable s_namehash_lookup_table;

void load_namehash_lookup_table()
{
    ASSERT(!s_namehash_lookup_table_is_loaded);

    auto buffer = App::load_internal_resource(INTERNAL_RESOURCE_FILELIST);

    byte_array_buffer buf(buffer);
    std::istream      buf_stream(&buf);

    rapidjson::IStreamWrapper stream(buf_stream);
    rapidjson::Document       document;
    document.ParseStream(stream);

    ASSERT(document.IsArray());
    s_namehash_lookup_table.reserve(document.Size());

    for (auto iter = document.Begin(); iter != document.End(); ++iter) {
        ASSERT(iter->IsString());
        auto namehash                     = ava::hashlittle(iter->GetString());
        s_namehash_lookup_table[namehash] = iter->GetString();
    }

    s_namehash_lookup_table_is_loaded = true;
    LOG_INFO("namehash lookup table is loaded. {} entries.", s_namehash_lookup_table.size());
}

const char* find_in_namehash_lookup_table(u32 namehash)
{
    ASSERT(s_namehash_lookup_table_is_loaded);

    auto iter = s_namehash_lookup_table.find(namehash);
    if (iter == s_namehash_lookup_table.end()) {
        return "";
    }

    return (*iter).second.c_str();
}
} // namespace jcmr