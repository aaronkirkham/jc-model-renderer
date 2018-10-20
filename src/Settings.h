#pragma once

#include <StdInc.h>
#include <singleton.h>

class Settings : public Singleton<Settings>
{
  private:
    fs::path m_SettingsFile = (fs::current_path() / "settings.json");
    json     m_Settings     = json::object();

  public:
    Settings()
    {
        if (fs::exists(m_SettingsFile)) {
            try {
                std::ifstream settings_file(m_SettingsFile);
                m_Settings = json::parse(settings_file);
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
