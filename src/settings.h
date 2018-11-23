#pragma once

#include <filesystem>
#include <fstream>

#include "singleton.h"

#include <json.hpp>

class Settings : public Singleton<Settings>
{
  private:
    std::filesystem::path m_SettingsFile = (std::filesystem::current_path() / "settings.json");
    nlohmann::json        m_Settings     = nlohmann::json::object();

  public:
    Settings()
    {
        if (std::filesystem::exists(m_SettingsFile)) {
            try {
                std::ifstream settings_file(m_SettingsFile);
                m_Settings = nlohmann::json::parse(settings_file);
            } catch (...) {
            }
        }
    }

    virtual ~Settings() = default;

    template <typename T> void SetValue(const std::string& key, const T& value)
    {
        m_Settings[key] = value;

        std::ofstream settings(m_SettingsFile);
        settings << m_Settings.dump(4) << std::endl;
    }

    template <typename T> T GetValue(const std::string& key)
    {
        T value;
        try {
            value = m_Settings[key].get<T>();
        } catch (...) {
        }

        return value;
    }

    template <typename T> T GetValue(const std::string& key, T def)
    {
        T value = def;
        try {
            value = m_Settings[key].get<T>();
        } catch (...) {
        }

        return value;
    }
};
