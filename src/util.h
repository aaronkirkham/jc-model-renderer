#pragma once

#include <string>
#include <vector>

namespace util
{
static std::vector<std::string> split(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;
    size_t                   prev_pos = 0;
    size_t                   pos      = 0;

    while ((pos = str.find(delimiter, pos)) != std::string::npos) {
        auto part = str.substr(prev_pos, (pos - prev_pos));
        prev_pos  = ++pos;

        result.emplace_back(part);
    }

    result.emplace_back(str.substr(prev_pos, (pos - prev_pos)));
    return result;
}

static std::string join(const std::vector<std::string>& strings, const std::string& delimiter)
{
    std::string out;
    if (auto i = strings.begin(), e = strings.end(); i != e) {
        out += *i++;
        for (; i != e; ++i) {
            out.append(delimiter).append(*i);
        }
    }

    return out;
}

static void replace(std::string& data, const char* search, const char* replace)
{
    size_t pos = data.find(search);
    while (pos != std::string::npos) {
        data.replace(pos, strlen(search), replace);
        pos = data.find(search, pos + strlen(replace));
    }
}

template <typename... Args> static std::string format(const std::string& format, Args... args)
{
    size_t                  size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

// case-insensitive string find
static bool find(const std::string& haystack, const std::string& needle)
{
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char ch1, char ch2) { return toupper(ch1) == toupper(ch2); });
    return (it != haystack.end());
}

static GUID GUID_from_string(const std::string& string, GUID fallback = {})
{
    GUID guid{};
    auto result = sscanf_s(string.c_str(), "{%8x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}",
                           &guid.Data1, &guid.Data2, &guid.Data3, &guid.Data4[0], &guid.Data4[1], &guid.Data4[2],
                           &guid.Data4[3], &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);
    if (result == 11) {
        return guid;
    }

    return fallback;
}

static std::string GUID_to_string(const GUID& guid)
{
    return format("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", guid.Data1, guid.Data2, guid.Data3,
                  guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                  guid.Data4[6], guid.Data4[7]);
}

static int32_t stoi_s(const std::string& str)
{
    if (str.length() == 0) {
        return -1;
    }

    return std::stoi(str);
}
}; // namespace util
