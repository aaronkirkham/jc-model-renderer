#ifndef JCMR_APP_UTILS_H_HEADER_GUARD
#define JCMR_APP_UTILS_H_HEADER_GUARD

#include <algorithm>
#include <string>

namespace jcmr::utils
{
static std::string get_extension(const std::string& filename)
{
    auto index = filename.find_last_of('.');
    if (index == std::string::npos) {
        return "";
    }

    return filename.substr(index + 1);
}

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

static std::string replace(std::string string, const char* search, const char* replace)
{
    return string.replace(string.find(search), strlen(search), replace);
}

// static void replace(std::string& data, const char* search, const char* replace)
// {
//     size_t pos = data.find(search);
//     while (pos != std::string::npos) {
//         data.replace(pos, strlen(search), replace);
//         pos = data.find(search, pos + strlen(replace));
//     }
// }

template <typename... Args> static std::string format(const std::string& format, Args... args)
{
    size_t                  size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

static bool find_insensitive(const std::string& haystack, const std::string& needle)
{
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char ch1, char ch2) { return toupper(ch1) == toupper(ch2); });
    return (it != haystack.end());
}
} // namespace jcmr::utils

#endif // JCMR_APP_UTILS_H_HEADER_GUARD
