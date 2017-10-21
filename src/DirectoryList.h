#pragma once

#include <json.hpp>
#include <string.h>

class DirectoryList
{
private:
    nlohmann::json m_Structure;

    void split(std::string str, nlohmann::json& current)
    {
        auto current_pos = str.find('/');
        if (current_pos != std::string::npos) {
            auto directory = str.substr(0, current_pos);
            str.erase(0, current_pos + 1);

            if (current.is_object() || current.is_null()) {
                split(str, current[directory]);
            }
            else {
                auto temp = current;
                current = nullptr;
                current["/"] = temp;
                split(str, current[directory]);
            }
        }
        else {
            if (!current.is_object()) {
                current.emplace_back(str);
            }
            else {
                current["/"].emplace_back(str);
            }
        }
    }

public:
    DirectoryList() = default;
    virtual ~DirectoryList() = default;

    void Add(const std::string& filename) {
        split(filename, m_Structure);
    }

    nlohmann::json* GetStructure() { return &m_Structure; }
};