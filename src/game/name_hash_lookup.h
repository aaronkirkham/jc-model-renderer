#pragma once

#include "../window.h"

#include <json.hpp>

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
                const auto handle = GetModuleHandle(nullptr);
                const auto rc     = FindResource(handle, MAKEINTRESOURCE(512), RT_RCDATA);
                if (rc == nullptr) {
                    throw std::runtime_error("FindResource failed");
                }

                const auto data = LoadResource(handle, rc);
                if (data == nullptr) {
                    throw std::runtime_error("LoadResource failed");
                }

                // parse the file list json
                auto  str        = static_cast<const char*>(LockResource(data));
                auto& dictionary = nlohmann::json::parse(str);

                // generate the lookup table
                for (auto& it = dictionary.begin(); it != dictionary.end(); ++it) {
                    const auto namehash = static_cast<uint32_t>(std::stoul(it.key(), nullptr, 16));
                    LookupTable.insert(std::make_pair(namehash, it.value().get<std::string>()));
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("Failed to load lookup table. ({})", e.what());

                std::string error = "Failed to load namehash lookup table. (";
                error += e.what();
                error += ")\n\n";
                error += "Some features will be disabled.";
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
