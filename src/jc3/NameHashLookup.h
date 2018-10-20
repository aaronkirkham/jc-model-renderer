#pragma once

#include <Window.h>
#include <string>
#include <unordered_map>

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
                const auto rc     = FindResource(handle, MAKEINTRESOURCE(256), RT_RCDATA);
                if (rc == nullptr) {
                    throw std::runtime_error("NameHashLookup - Failed to find dictionary resource");
                }

                const auto data = LoadResource(handle, rc);
                if (data == nullptr) {
                    throw std::runtime_error("NameHashLookup - Failed to load dictionary resource");
                }

                // parse the file list json
                auto  str        = static_cast<const char*>(LockResource(data));
                auto& dictionary = json::parse(str);

                // generate the lookup table
                for (auto& it = dictionary.begin(); it != dictionary.end(); ++it) {
                    const auto namehash = static_cast<uint32_t>(std::stoul(it.key(), nullptr, 16));
                    LookupTable.insert(std::make_pair(namehash, it.value().get<std::string>()));
                }
            } catch (const std::exception& e) {
                DEBUG_LOG(e.what());

                std::stringstream error;
                error << "Failed to read/parse namehash lookup table.\n\nSome features will be disabled." << std::endl
                      << std::endl;
                error << e.what();

                Window::Get()->ShowMessageBox(error.str());
            }
        })
            .detach();
    }

    static std::string GetName(const uint32_t name_hash)
    {
        assert(LookupTable.size() != 0);
        auto it = LookupTable.find(name_hash);
        if (it != LookupTable.end()) {
            return (*it).second;
        }

        return "";
    }
};
