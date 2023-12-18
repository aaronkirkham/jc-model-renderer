#ifndef JCMR_APP_SETTINGS_H_HEADER_GUARD
#define JCMR_APP_SETTINGS_H_HEADER_GUARD

#include "platform.h"

#include <rapidjson/document.h>

namespace jcmr
{
struct Settings {
    Settings();
    virtual ~Settings();

    bool load();
    bool save();

    void set(const char* key, const char* value)
    {
        if (!m_root.HasMember(key)) {
            m_root.AddMember(rapidjson::GenericStringRef<char>(key), rapidjson::GenericStringRef<char>(value),
                             m_root.GetAllocator());
        } else {
            m_root[key].Set(value);
        }

        save();
    }

    template <typename T> void set(const char* key, T value)
    {
        if (!m_root.HasMember(key)) {
            m_root.AddMember(rapidjson::GenericStringRef<char>(key), value, m_root.GetAllocator());
        } else {
            m_root[key].template Set<T>(value);
        }

        save();
    }

    template <typename T> T get(const char* key)
    {
        ASSERT(m_root.HasMember(key));
        return m_root[key].template Get<T>();
    }

    template <typename T> T get(const char* key, T fallback)
    {
        if (!m_root.HasMember(key)) {
            return fallback;
        }

        return m_root[key].template Get<T>();
    }

  private:
    rapidjson::Document m_root;
};
} // namespace jcmr

#endif // JCMR_APP_SETTINGS_H_HEADER_GUARD
