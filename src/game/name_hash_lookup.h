#pragma once

#include <json.hpp>

#include "window.h"

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

                const auto& dictionary = nlohmann::json::parse(buffer.begin(), buffer.end());

                // generate the lookup table
                for (auto& it = dictionary.begin(); it != dictionary.end(); ++it) {
                    const auto namehash = static_cast<uint32_t>(std::stoul(it.key(), nullptr, 16));
                    LookupTable.insert(std::make_pair(namehash, it.value().get<std::string>()));
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
