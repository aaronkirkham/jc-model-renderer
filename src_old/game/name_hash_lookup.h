#pragma once

#include "window.h"

#include <util/byte_array_buffer.h>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

class NameHashLookup
{
  public:
    NameHashLookup()          = delete;
    virtual ~NameHashLookup() = delete;

    static std::unordered_map<uint32_t, std::string> LookupTable;

    static void Init()
    {
        std::thread([&] {
            try {
                std::vector<uint8_t> buffer;
                if (!Window::Get()->LoadInternalResource(512, &buffer)) {
                    throw std::runtime_error("LoadInternalResource failed.");
                }

                byte_array_buffer         buf(buffer);
                std::istream              stream(&buf);
                rapidjson::IStreamWrapper stream_wrapper(stream);

                rapidjson::Document doc;
                doc.ParseStream(stream_wrapper);

                assert(doc.IsArray());
                LookupTable.reserve(doc.Size());

                // generate the lookup table
                for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
                    assert(itr->IsString());
                    const uint32_t name_hash = ava::hashlittle(itr->GetString());
                    LookupTable[name_hash]   = itr->GetString();
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("Failed to load lookup table.");
                SPDLOG_ERROR(e.what());

                std::string error = "Failed to load namehash lookup table, some features will be unavailable.\n\n";
                error += e.what();
                Window::Get()->ShowMessageBox(error);
            }
        }).detach();
    }

    static std::string GetName(const uint32_t name_hash)
    {
        auto it = LookupTable.find(name_hash);
        if (it != LookupTable.end()) {
            return (*it).second;
        }

        return "";
    }
};
