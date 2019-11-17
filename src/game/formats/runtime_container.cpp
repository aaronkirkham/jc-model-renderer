#include <AvaFormatLib.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <spdlog/spdlog.h>

#include <sstream>

#include <glm/glm.hpp>

#include "runtime_container.h"

#include "game/file_loader.h"
#include "game/formats/render_block_model.h"
#include "game/name_hash_lookup.h"

extern bool g_IsJC4Mode;

std::recursive_mutex                                  Factory<RuntimeContainer>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RuntimeContainer>> Factory<RuntimeContainer>::Instances;

RuntimeContainerProperty::RuntimeContainerProperty(uint32_t name_hash, ava::RuntimePropertyContainer::EVariantType type)
    : m_NameHash(name_hash)
    , m_Type(type)
{
    m_Name = NameHashLookup::GetName(name_hash);

    if (m_Name.empty()) {
        std::stringstream ss;
        ss << "Unknown (" << std::hex << std::setw(4) << name_hash << ")";
        m_Name = ss.str();
    }
}

std::string RuntimeContainerProperty::GetTypeName(ava::RuntimePropertyContainer::EVariantType type)
{
    using namespace ava::RuntimePropertyContainer;

    switch (type) {
        case T_VARIANT_UNASSIGNED:
            return "unassigned";
        case T_VARIANT_INTEGER:
            return "integer";
        case T_VARIANT_FLOAT:
            return "float";
        case T_VARIANT_STRING:
            return "string";
        case T_VARIANT_VEC2:
            return "vec2";
        case T_VARIANT_VEC3:
            return "vec3";
        case T_VARIANT_VEC4:
            return "vec4";
        case T_VARIANT_MAT4x4:
            return "mat4x4";
        case T_VARIANT_VEC_INTS:
            return "vec_int";
        case T_VARIANT_VEC_FLOATS:
            return "vec_float";
        case T_VARIANT_VEC_BYTES:
            return "vec_byte";
        case T_VARIANT_OBJECTID:
            return "object_id";
        case T_VARIANT_VEC_EVENTS:
            return "vec_events";
    }

    return "unknown";
}

RuntimeContainer::RuntimeContainer(uint32_t name_hash, const std::filesystem::path& filename)
    : m_NameHash(name_hash)
    , m_Filename(filename)
{
    m_Name = NameHashLookup::GetName(name_hash);
}

RuntimeContainer::~RuntimeContainer()
{
    // delete all the properties
    for (auto& prop : m_Properties) {
        delete prop;
    }

    // delete all the containers
    for (auto& container : m_Containers) {
        delete container;
    }

    m_Properties.clear();
    m_Containers.clear();
}

void RuntimeContainer::GenerateBetterNames()
{
    std::string tmp;
    auto        _name  = GetProperty("name", false);
    auto        _class = GetProperty("_class", false);

    // append the real name
    if (_name) {
        tmp.append(_name->GetValue<std::string>());
    }

    // append the class name
    if (_class) {
        if (_name) {
            tmp.append(" (");
        }

        tmp.append(_class->GetValue<std::string>());

        if (_name) {
            tmp.append(")");
        }
    }
    // if we don't have a class name, look for the class hash (mainly used in jc4)
    else if (_class = GetProperty("_class_hash", false)) {
        if (_name) {
            tmp.append(" (");
        }

        tmp.append(NameHashLookup::GetName(_class->GetValue<int32_t>()));

        if (_name) {
            tmp.append(")");
        }
    }

    if (m_Name.empty()) {
        m_Name = tmp;
    } else {
        m_Name = tmp + " (" + m_Name + ")";
    }

    for (const auto& container : m_Containers) {
        container->GenerateBetterNames();
    }
}

RuntimeContainerProperty* RuntimeContainer::GetProperty(uint32_t name_hash, bool include_children)
{
    // find the property
    for (auto& prop : m_Properties) {
        if (prop->GetNameHash() == name_hash) {
            return prop;
        }
    }

    if (include_children) {
        // try find the property in child containers
        for (auto& container : m_Containers) {
            auto prop = container->GetProperty(name_hash);
            if (prop) {
                return prop;
            }
        }
    }

    return nullptr;
}

RuntimeContainerProperty* RuntimeContainer::GetProperty(const std::string& name, bool include_children)
{
    return GetProperty(ava::hashlittle(name.c_str()), include_children);
}

RuntimeContainer* RuntimeContainer::GetContainer(uint32_t name_hash, bool include_children)
{
    // find the container
    for (auto& container : m_Containers) {
        if (container->GetNameHash() == name_hash) {
            return container;
        }

        if (include_children) {
            // if the container has a child we're looking for, use that
            auto child_container = container->GetContainer(name_hash);
            if (child_container) {
                return child_container;
            }
        }
    }

    return nullptr;
}

RuntimeContainer* RuntimeContainer::GetContainer(const std::string& name, bool include_children)
{
    return GetContainer(ava::hashlittle(name.c_str()), include_children);
}

std::vector<RuntimeContainer*> RuntimeContainer::GetAllContainers(const std::string& class_name)
{
    std::vector<RuntimeContainer*> result;

    const auto _class = GetProperty("_class");
    assert(_class);

    //
    auto _class_name = _class->GetValue<std::string>();
    if (_class_name == class_name) {
        result.emplace_back(this);
    }

    for (const auto& child : m_Containers) {
        auto r = child->GetAllContainers(class_name);
        std::copy(r.begin(), r.end(), std::back_inserter(result));
    }

    return result;
}

std::vector<RuntimeContainerProperty*> RuntimeContainer::GetSortedProperties()
{
    auto properties = GetProperties();
    std::sort(properties.begin(), properties.end(), [&](RuntimeContainerProperty* a, RuntimeContainerProperty* b) {
        auto& name_a = a->GetName();
        auto& name_b = b->GetName();

        return std::lexicographical_compare(name_a.begin(), name_a.end(), name_b.begin(), name_b.end());
    });

    return properties;
}

void RuntimeContainer::ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                        bool external)
{
    if (!RuntimeContainer::exists(filename.string())) {
        const auto rtpc = FileLoader::Get()->ParseRuntimeContainer(filename, data);
    }
}

void WriteNode(std::ofstream& stream, RuntimeContainer* node, std::unordered_map<std::string, uint32_t>& string_offsets,
               std::vector<std::pair<uint32_t, ava::RuntimePropertyContainer::RtpcContainer>>& raw_nodes)
{
    using namespace ava::RuntimePropertyContainer;

    auto properties = node->GetProperties();
    auto containers = node->GetContainers();

    const uint32_t prop_offset            = static_cast<uint32_t>(stream.tellp());
    const uint32_t _child_offset_no_align = (prop_offset + (sizeof(RtpcContainerVariant) * properties.size()));
    uint32_t       child_offset           = ava::math::align(_child_offset_no_align);
    const uint32_t prop_data_offset       = (child_offset + (sizeof(RtpcContainer) * containers.size()));

    std::vector<RtpcContainerVariant> _raw_properties;
    _raw_properties.reserve(properties.size());

    if (g_IsJC4Mode) {
        std::sort(properties.begin(), properties.end(),
                  [](const RuntimeContainerProperty* a, const RuntimeContainerProperty* b) {
                      return a->GetNameHash() < b->GetNameHash();
                  });
    }

    // write property data
    stream.seekp(prop_data_offset);
    for (const auto& prop : properties) {
        auto       current_pos = static_cast<uint32_t>(stream.tellp());
        const auto type        = prop->GetType();

        // alignment
        if (type != T_VARIANT_INTEGER && type != T_VARIANT_FLOAT && type != T_VARIANT_STRING) {
            uint32_t alignment = 4;

            if (type == T_VARIANT_VEC4 || type == T_VARIANT_MAT4x4) {
                alignment = 16;
            }

            // write padding bytes
            uint32_t padding = ava::math::align_distance(current_pos, alignment);
            current_pos += padding;

            static uint8_t PADDING_BYTE = 0x50;
            for (decltype(padding) i = 0; i < padding; ++i) {
                stream.write((char*)&PADDING_BYTE, 1);
            }
        }

        RtpcContainerVariant _raw_property{prop->GetNameHash(), current_pos, prop->GetType()};
        switch (type) {
            case T_VARIANT_INTEGER: {
                auto value                 = std::any_cast<int32_t>(prop->GetValue());
                _raw_property.m_DataOffset = static_cast<uint32_t>(value);
                break;
            }

            case T_VARIANT_FLOAT: {
                auto value                 = std::any_cast<float>(prop->GetValue());
                _raw_property.m_DataOffset = *(uint32_t*)&value;
                break;
            }

            case T_VARIANT_STRING: {
                auto& value = std::any_cast<std::string>(prop->GetValue());

                auto it = string_offsets.find(value);
                if (it == string_offsets.end()) {
                    string_offsets[value] = _raw_property.m_DataOffset;

                    // write the null-terminated string value
                    stream.write(value.c_str(), value.length());
                    stream << '\0';
                }
                // update the current property data offset
                else {
                    _raw_property.m_DataOffset = (*it).second;
                }

                break;
            }

            case T_VARIANT_VEC2:
            case T_VARIANT_VEC3:
            case T_VARIANT_VEC4:
            case T_VARIANT_MAT4x4: {
                if (type == T_VARIANT_VEC2) {
                    auto& value = std::any_cast<glm::vec2>(prop->GetValue());
                    stream.write((char*)&value.x, sizeof(value));
                } else if (type == T_VARIANT_VEC3) {
                    auto& value = std::any_cast<glm::vec3>(prop->GetValue());
                    stream.write((char*)&value.x, sizeof(value));
                } else if (type == T_VARIANT_VEC4) {
                    auto& value = std::any_cast<glm::vec4>(prop->GetValue());
                    stream.write((char*)&value.x, sizeof(value));
                } else if (type == T_VARIANT_MAT4x4) {
                    auto& value = std::any_cast<glm::mat4x4>(prop->GetValue());
                    stream.write((char*)&value[0], sizeof(value));
                }

                break;
            }

            case T_VARIANT_VEC_INTS:
            case T_VARIANT_VEC_FLOATS:
            case T_VARIANT_VEC_BYTES: {
                if (type == T_VARIANT_VEC_INTS) {
                    auto& value = std::any_cast<std::vector<int32_t>>(prop->GetValue());
                    auto  count = static_cast<int32_t>(value.size());

                    stream.write((char*)&count, sizeof(count));
                    stream.write((char*)value.data(), (count * sizeof(int32_t)));
                } else if (type == T_VARIANT_VEC_FLOATS) {
                    auto& value = std::any_cast<std::vector<float>>(prop->GetValue());
                    auto  count = static_cast<int32_t>(value.size());

                    stream.write((char*)&count, sizeof(count));
                    stream.write((char*)value.data(), (count * sizeof(float)));
                } else if (type == T_VARIANT_VEC_BYTES) {
                    auto& value = std::any_cast<std::vector<uint8_t>>(prop->GetValue());
                    auto  count = static_cast<int32_t>(value.size());

                    stream.write((char*)&count, sizeof(count));
                    stream.write((char*)value.data(), (count * sizeof(uint8_t)));
                }

                break;
            }

            case T_VARIANT_OBJECTID: {
                auto& value = std::any_cast<std::pair<uint32_t, uint32_t>>(prop->GetValue());
                stream.write((char*)&value.first, (sizeof(uint32_t) * 2));
                break;
            }

            case T_VARIANT_VEC_EVENTS: {
                auto& value = std::any_cast<std::vector<std::pair<uint32_t, uint32_t>>>(prop->GetValue());
                auto  count = static_cast<int32_t>(value.size());

                stream.write((char*)&count, sizeof(count));
                stream.write((char*)value.data(), (count * (sizeof(uint32_t) * 2)));
                break;
            }
        }

        _raw_properties.emplace_back(std::move(_raw_property));
    }

    auto child_data_offset = static_cast<uint32_t>(stream.tellp());

    // write properties
    stream.seekp(prop_offset);
    stream.write((char*)_raw_properties.data(), (sizeof(RtpcContainerVariant) * _raw_properties.size()));

    std::sort(containers.begin(), containers.end(),
              [](const RuntimeContainer* a, const RuntimeContainer* b) { return a->GetNameHash() < b->GetNameHash(); });

    // write child containers
    stream.seekp(ava::math::align(child_data_offset));
    for (auto child : containers) {
        RtpcContainer _node{child->GetNameHash(), static_cast<uint32_t>(stream.tellp()),
                            static_cast<uint16_t>(child->GetProperties().size()),
                            static_cast<uint16_t>(child->GetContainers().size())};
        raw_nodes.emplace_back(std::make_pair(child_offset, std::move(_node)));

        child_offset += sizeof(RtpcContainer);

        WriteNode(stream, child, string_offsets, raw_nodes);
    }
}

bool RuntimeContainer::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    if (auto rc = RuntimeContainer::get(filename.string())) {
        using namespace ava::RuntimePropertyContainer;

        std::ofstream stream(path, std::ios::binary);
        if (stream.fail()) {
            return false;
        }

        // generate the header
        RtpcHeader header;
        stream.write((char*)&header, sizeof(header));

        //
        std::vector<std::pair<uint32_t, RtpcContainer>> raw_nodes;
        std::unordered_map<std::string, uint32_t>       string_offsets;

        const auto current_pos = static_cast<uint32_t>(stream.tellp());
        stream.seekp(current_pos + sizeof(RtpcContainer));

        RtpcContainer _node{rc->GetNameHash(), (current_pos + sizeof(RtpcContainer)),
                            static_cast<uint16_t>(rc->GetProperties().size()),
                            static_cast<uint16_t>(rc->GetContainers().size())};
        raw_nodes.emplace_back(std::make_pair(current_pos, std::move(_node)));

        WriteNode(stream, rc.get(), string_offsets, raw_nodes);

        // write the nodes
        for (const auto& node : raw_nodes) {
            stream.seekp(node.first);
            stream.write((char*)&node.second, sizeof(node.second));
        }

        stream.close();
        return true;
    }

    return false;
}

void RuntimeContainer::DrawUI(int32_t index, uint8_t depth)
{
    using namespace ava::RuntimePropertyContainer;

    // skip "root"
    if (m_NameHash == 0xAA7D522A) {
        // draw children
        if (m_Containers.size() > 0) {
            auto _depth = ++depth;
            for (int i = 0; i < m_Containers.size(); ++i) {
                m_Containers[i]->DrawUI(i, _depth);
            }
        }

        return;
    }

    std::string title = m_Name + "##" + std::to_string(index);

    if (ImGui::TreeNode(title.c_str())) {
        for (const auto& prop : GetSortedProperties()) {
            // special case for hashes
            // TODO: Move into a function which gets run once, we need lots of custom cases for these (vehicles)
            // instead of just showing the hash, we should do a NameHashLookup and show the string value. If the
            // string is changed, when we save the EPE we need to convert the string back to a hash and save that
            if (prop->GetType() == T_VARIANT_INTEGER
                && (prop->GetNameHash() == 0x932E9257 || // ragdoll
                    prop->GetNameHash() == 0x26FA86FE || // skeleton
                    prop->GetNameHash() == 0x0F94740B || // model
                    prop->GetNameHash() == 0xB498C27D || // filepath
                    prop->GetName().rfind("hash") != std::string::npos)) {
                auto value = std::any_cast<int32_t>(prop->GetValue());
                ImGui::InputInt(prop->GetName().c_str(), (int32_t*)&value, 0, 0,
                                ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);
                continue;
            }

            switch (prop->GetType()) {
                case T_VARIANT_INTEGER: {
                    auto value = std::any_cast<int32_t>(prop->GetValue());
                    if (ImGui::InputInt(prop->GetName().c_str(), &value)) {
                        prop->SetValue(value);
                    }
                    break;
                }

                case T_VARIANT_FLOAT: {
                    auto value = std::any_cast<float>(prop->GetValue());
                    if (ImGui::InputFloat(prop->GetName().c_str(), &value)) {
                        prop->SetValue(value);
                    }
                    break;
                }

                case T_VARIANT_STRING: {
                    auto& value = std::any_cast<std::string>(prop->GetValue());
                    if (ImGui::InputText(prop->GetName().c_str(), &value)) {
                        prop->SetValue(value);
                    }
                    break;
                }

                case T_VARIANT_VEC2: {
                    auto& value = std::any_cast<glm::vec2>(prop->GetValue());
                    if (ImGui::InputFloat2(prop->GetName().c_str(), &value.x)) {
                        prop->SetValue(glm::vec2{value.x, value.y});
                    }
                    break;
                }

                case T_VARIANT_VEC3: {
                    auto& value = std::any_cast<glm::vec3>(prop->GetValue());
                    if (ImGui::InputFloat3(prop->GetName().c_str(), &value.x)) {
                        prop->SetValue(glm::vec3{value.x, value.y, value.z});
                    }
                    break;
                }

                case T_VARIANT_VEC4: {
                    auto& value = std::any_cast<glm::vec4>(prop->GetValue());
                    if (ImGui::InputFloat4(prop->GetName().c_str(), &value.x)) {
                        prop->SetValue(glm::vec4{value.x, value.y, value.z, value.w});
                    }
                    break;
                }

                case T_VARIANT_MAT4x4: {
                    auto& value = prop->GetValue<glm::mat4>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::InputFloat4("m0", &value[0][0]);
                    ImGui::InputFloat4("m1", &value[1][0]);
                    ImGui::InputFloat4("m2", &value[2][0]);
                    ImGui::InputFloat4("m3", &value[3][0]);
                    break;
                }

                case T_VARIANT_VEC_INTS: {
                    auto& values = prop->GetValue<std::vector<int32_t>>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::Text(RuntimeContainerProperty::GetTypeName(prop->GetType()).c_str());
                    for (auto& value : values) {
                        ImGui::InputInt(".", &value);
                    }
                    break;
                }

                case T_VARIANT_VEC_FLOATS: {
                    auto& values = prop->GetValue<std::vector<float>>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::Text(RuntimeContainerProperty::GetTypeName(prop->GetType()).c_str());
                    for (auto& value : values) {
                        ImGui::InputFloat(".", &value);
                    }
                    break;
                }

                case T_VARIANT_VEC_BYTES: {
                    auto& values = prop->GetValue<std::vector<uint8_t>>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::Text(RuntimeContainerProperty::GetTypeName(prop->GetType()).c_str());
                    // for (auto& value : values) {
                    // ImGui::InputInt(".", &value);
                    //}
                    break;
                }

                case T_VARIANT_OBJECTID: {
                    auto& value = prop->GetValue<std::pair<uint32_t, uint32_t>>();
                    if (ImGui::InputScalarN(prop->GetName().c_str(), ImGuiDataType_U32, (void*)&value.first, 2)) {
                        prop->SetValue(std::make_pair(value.first, value.second));
                    }

                    break;
                }

                case T_VARIANT_VEC_EVENTS: {
                    break;
                }
            }
        }

        // draw children
        if (m_Containers.size() > 0) {
            auto _depth = ++depth;
            for (int i = 0; i < m_Containers.size(); ++i) {
                m_Containers[i]->DrawUI(i, _depth);
            }
        }

        ImGui::TreePop();
    }
}

void RuntimeContainer::ContextMenuUI(const std::filesystem::path& filename)
{
    if (!g_IsJC4Mode) {
        if (ImGui::Selectable("Load models")) {
            auto rc = get(filename.string());

            // load the runtime container
            if (!rc) {
                RuntimeContainer::Load(filename, [&, filename](std::shared_ptr<RuntimeContainer> rc) {
                    RenderBlockModel::LoadFromRuntimeContainer(filename, rc);
                });
            } else {
                RenderBlockModel::LoadFromRuntimeContainer(filename, rc);
            }
        }
    }
}

void RuntimeContainer::Load(const std::filesystem::path&                           filename,
                            std::function<void(std::shared_ptr<RuntimeContainer>)> callback)
{
    FileLoader::Get()->ReadFile(filename, [&, filename, callback](bool success, std::vector<uint8_t> data) {
        if (success) {
            auto result = FileLoader::Get()->ParseRuntimeContainer(filename, data);
            callback(result);
        } else {
            SPDLOG_ERROR("Failed to read runtime container \"{}\"", filename.string());
        }
    });
}
