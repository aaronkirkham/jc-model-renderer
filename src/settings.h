#pragma once

#include "singleton.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <filesystem>
#include <fstream>

class Settings : public Singleton<Settings>
{
  private:
    std::filesystem::path m_SettingsFile = (std::filesystem::current_path() / "settings.json");
    rapidjson::Document   m_Doc;

  public:
    Settings()
    {
        if (std::filesystem::exists(m_SettingsFile)) {
            try {
                std::ifstream             settings(m_SettingsFile);
                rapidjson::IStreamWrapper stream_wrapper(settings);
                m_Doc.ParseStream(stream_wrapper);
            } catch (...) {
            }
        }
    }

    virtual ~Settings() = default;

    template <typename T> void SetValue(const char* key, const T& value)
    {
        using namespace rapidjson;

        if (m_Doc.HasMember(key)) {
            m_Doc[key].Set(value);
        } else {
            auto& allocator = m_Doc.GetAllocator();

            rapidjson::Value val;
            val.Set(value, allocator);

            m_Doc.AddMember(StringRef(key), val, allocator);
        }

        StringBuffer         buffer;
        Writer<StringBuffer> writer(buffer);
        m_Doc.Accept(writer);

        std::ofstream settings(m_SettingsFile);
        settings << buffer.GetString() << std::endl;
    }

    template <typename T> T GetValue(const char* key, const T& def)
    {
        if (m_Doc.HasMember(key) && m_Doc[key].Is<T>()) {
            return m_Doc[key].Get<T>();
        }

        return def;
    }
};
