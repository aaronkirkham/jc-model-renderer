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

                const auto res_data = LoadResource(handle, rc);
                if (res_data == nullptr) {
                    throw std::runtime_error("LoadResource failed");
                }

                const void* ptr = LockResource(res_data);
                if (ptr == nullptr) {
                    throw std::runtime_error("LockResource failed");
                }

                // parse the file list json
                std::vector<uint8_t> buffer(SizeofResource(handle, rc));
                memcpy(buffer.data(), ptr, buffer.size());
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
