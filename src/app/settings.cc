#include "pch.h"

#include "app/log.h"
#include "app/profile.h"
#include "app/settings.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <fstream>

namespace jcmr
{
Settings::Settings()
{
    load();
}

Settings::~Settings()
{
    if (!save()) {
        LOG_ERROR("Settings : failed to save settings!");
    }
}

bool Settings::load()
{
    ProfileBlock _("Settings load");

    std::ifstream stream("settings.json");
    if (!stream.is_open()) {
        m_root.SetObject();
        return false;
    }

    rapidjson::IStreamWrapper stream_wrapper(stream);
    m_root.ParseStream(stream_wrapper);
    return !m_root.HasParseError();
}

bool Settings::save()
{
    ProfileBlock _("Settings save");

    std::ofstream stream("settings.json");
    if (!stream.is_open()) {
        return false;
    }

    rapidjson::OStreamWrapper                          stream_wrapper(stream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(stream_wrapper);
    return m_root.Accept(writer);
}
} // namespace jcmr