#pragma once

#include <json.hpp>
#include <string.h>

#include <jc3/formats/StreamArchive.h>

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

    void Parse(nlohmann::json* structure, const std::vector<std::string>& include_only = {}) {
        if (structure) {
            m_Structure.clear();

            // iterate all the root objects
            for (auto it = structure->begin(); it != structure->end(); ++it) {
                const auto& directory_obj = it.value();

                // iterate the files in the archive object
                for (auto it2 = directory_obj.begin(); it2 != directory_obj.end(); ++it2) {
                    const auto& archive_obj = it2.value();
                    
                    // iterate the data in the files object
                    for (auto it3 = archive_obj.begin(); it3 != archive_obj.end(); ++it3) {
                        const auto& value = it3.value();
                        if (value.is_string()) {
                            const auto& value_str = value.get<std::string>();

                            // should we only parse specific includes?
                            if (!include_only.empty()) {
                                for (const auto& extension : include_only) {
                                    if (value_str.find(extension) != std::string::npos) {
                                        split(value_str, m_Structure);
                                    }
                                }
                            }
                            else {
                                split(value_str, m_Structure);
                            }
                        }
                    }
                }
            }
        }
    }

    void Parse(StreamArchive_t* archive, const std::vector<std::string>& include_only = {}) {
        if (archive) {
            m_Structure.clear();

            for (auto& file : archive->m_Files) {
                if (!include_only.empty()) {
                    for (auto& extension : include_only) {
                        if (file.m_Filename.find(extension) != std::string::npos) {
                            split(file.m_Filename, m_Structure);
                        }
                    }
                }
                else {
                    split(file.m_Filename, m_Structure);
                }
            }
        }
    }

    nlohmann::json* GetStructure() { return &m_Structure; }
};