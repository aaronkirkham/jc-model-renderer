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

static int32_t stoi_s(const std::string& str)
{
    if (str.length() == 0) {
        return -1;
    }

    return std::stoi(str);
}
}; // namespace util
