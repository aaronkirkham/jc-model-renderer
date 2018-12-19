#include "runtime_container.h"
#include "../file_loader.h"
#include "../hashlittle.h"
#include "../name_hash_lookup.h"
#include "render_block_model.h"

#include <misc/cpp/imgui_stdlib.h>

extern bool g_IsJC4Mode;

std::recursive_mutex                                  Factory<RuntimeContainer>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RuntimeContainer>> Factory<RuntimeContainer>::Instances;

RuntimeContainerProperty::RuntimeContainerProperty(uint32_t name_hash, uint8_t type)
    : m_NameHash(name_hash)
    , m_Type(static_cast<PropertyType>(type))
{
    m_Name = NameHashLookup::GetName(name_hash);

    if (m_Name.empty()) {
        std::stringstream ss;
        ss << "Unknown (" << std::hex << std::setw(4) << name_hash << ")";
        m_Name = ss.str();
    }
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
    const auto  _class = GetProperty("_class", false);
    const auto  _name  = GetProperty("name", false);

    if (_class) {
        tmp.append(_class->GetValue<std::string>());
        if (_name) {
            tmp.append(" (");
        }
    }

    if (_name) {
        tmp.append(_name->GetValue<std::string>());
        if (_class) {
            tmp.append(")");
        }
    }

    if (m_Name.empty()) {
        m_Name = tmp;
    } else if (!tmp.empty()) {
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
    return GetProperty(hashlittle(name.c_str()), include_children);
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
    return GetContainer(hashlittle(name.c_str()), include_children);
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

void RuntimeContainer::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    const auto rtpc = FileLoader::Get()->ParseRuntimeContainer(filename, data);
    assert(rtpc);
}

bool RuntimeContainer::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    const auto rc = RuntimeContainer::get(filename.string());
    if (rc) {
        using namespace jc::RuntimeContainer;

		std::ofstream stream(path, std::ios::binary);
        if (stream.fail()) {
            return false;
        }

        // generate the header
        Header header;
        header.m_Version = g_IsJC4Mode ? 3 : 1;
        stream.write((char*)&header, sizeof(header));

        //
        std::queue<std::pair<uint32_t, RuntimeContainer*>> instance_queue;
        instance_queue.push({sizeof(Header), rc.get()});

        //
        std::vector<std::pair<uint32_t, Node>>    raw_nodes;
        std::unordered_map<std::string, uint32_t> string_offsets;

        stream.seekp(static_cast<uint32_t>(stream.tellp()) + sizeof(Node));
        while (!instance_queue.empty()) {
            const auto& [offset, container] = instance_queue.front();

            auto properties = container->GetProperties();
            auto containers = container->GetContainers();

            // sort properties
            std::sort(properties.begin(), properties.end(),
                      [&](RuntimeContainerProperty* a, RuntimeContainerProperty* b) {
                          return a->GetNameHash() < b->GetNameHash();
                      });

            // sort containers
            std::sort(containers.begin(), containers.end(),
                      [&](RuntimeContainer* a, RuntimeContainer* b) { return a->GetNameHash() < b->GetNameHash(); });

            const uint32_t prop_offset            = static_cast<uint32_t>(stream.tellp());
            const uint32_t _child_offset_no_align = (prop_offset + (sizeof(Property) * properties.size()));
            uint32_t       child_offset           = jc::ALIGN_TO_BOUNDARY(_child_offset_no_align);
            const uint32_t prop_data_offset       = (child_offset + (sizeof(Node) * containers.size()));

            Node _node{container->GetNameHash(), prop_offset, static_cast<uint16_t>(properties.size()),
                       static_cast<uint16_t>(containers.size())};
            raw_nodes.emplace_back(std::make_pair(offset, std::move(_node)));

            std::vector<Property> _raw_properties;
            _raw_properties.reserve(properties.size());

            // write property data
            stream.seekp(prop_data_offset);
            for (const auto& prop : properties) {
                auto       current_pos = static_cast<uint32_t>(stream.tellp());
                const auto type        = prop->GetType();

                // alignment
                if (type != RTPC_TYPE_INTEGER && type != RTPC_TYPE_FLOAT && type != RTPC_TYPE_STRING) {
                    const auto padding = jc::DISTANCE_TO_BOUNDARY(current_pos);
                    current_pos += padding;

                    static uint8_t PADDING_BYTE = 0x50;
                    stream.write((char*)&PADDING_BYTE, padding);
                }

                Property _raw_property{prop->GetNameHash(), current_pos, prop->GetType()};
                switch (type) {
                    case RTPC_TYPE_INTEGER: {
                        auto value                 = std::any_cast<int32_t>(prop->GetValue());
                        _raw_property.m_DataOffset = static_cast<uint32_t>(value);
                        break;
                    }

                    case RTPC_TYPE_FLOAT: {
                        auto value                 = std::any_cast<float>(prop->GetValue());
                        _raw_property.m_DataOffset = static_cast<uint32_t>(value);
                        break;
                    }

                    case RTPC_TYPE_STRING: {
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

                    case RTPC_TYPE_VEC2:
                    case RTPC_TYPE_VEC3:
                    case RTPC_TYPE_VEC4:
                    case RTPC_TYPE_MAT4X4: {
                        if (type == RTPC_TYPE_VEC2) {
                            auto& value = std::any_cast<glm::vec2>(prop->GetValue());
                            stream.write((char*)&value, sizeof(value));
                        } else if (type == RTPC_TYPE_VEC3) {
                            auto& value = std::any_cast<glm::vec3>(prop->GetValue());
                            stream.write((char*)&value, sizeof(value));
                        } else if (type == RTPC_TYPE_VEC4) {
                            auto& value = std::any_cast<glm::vec4>(prop->GetValue());
                            stream.write((char*)&value, sizeof(value));
                        } else if (type == RTPC_TYPE_MAT4X4) {
                            auto& value = std::any_cast<glm::mat4x4>(prop->GetValue());
                            stream.write((char*)&value, sizeof(value));
                        }

                        break;
                    }

                    case RTPC_TYPE_LIST_INTEGERS:
                    case RTPC_TYPE_LIST_FLOATS:
                    case RTPC_TYPE_LIST_BYTES: {
                        if (type == RTPC_TYPE_LIST_INTEGERS) {
                            auto& value = std::any_cast<std::vector<int32_t>>(prop->GetValue());
                            auto  count = static_cast<uint32_t>(value.size());

                            stream.write((char*)&count, sizeof(count));
                            stream.write((char*)value.data(), (value.size() * sizeof(int32_t)));
                        } else if (type == RTPC_TYPE_LIST_FLOATS) {
                            auto& value = std::any_cast<std::vector<float>>(prop->GetValue());
                            auto  count = static_cast<uint32_t>(value.size());

                            stream.write((char*)&count, sizeof(count));
                            stream.write((char*)value.data(), (value.size() * sizeof(float)));
                        } else if (type == RTPC_TYPE_LIST_BYTES) {
                            auto& value = std::any_cast<std::vector<uint8_t>>(prop->GetValue());
                            auto  count = static_cast<uint32_t>(value.size());

                            stream.write((char*)&count, sizeof(count));
                            stream.write((char*)value.data(), (value.size() * sizeof(uint8_t)));
                        }

                        break;
                    }

                    case RTPC_TYPE_OBJECT_ID: {
                        auto& value = std::any_cast<std::pair<uint32_t, uint32_t>>(prop->GetValue());
                        stream.write((char*)&value, (sizeof(uint32_t) * 2));
                        break;
                    }

                    case RTPC_TYPE_EVENTS: {
                        auto& value = std::any_cast<std::vector<std::pair<uint32_t, uint32_t>>>(prop->GetValue());

                        auto count = static_cast<uint32_t>(value.size());
                        stream.write((char*)&count, sizeof(count));
                        stream.write((char*)value.data(), (value.size() * (sizeof(uint32_t) * 2)));
                        break;
                    }
                }

                _raw_properties.emplace_back(std::move(_raw_property));
            }

            auto current_pos       = static_cast<uint32_t>(stream.tellp());
            auto child_data_offset = jc::ALIGN_TO_BOUNDARY(current_pos);

            // write properties
            stream.seekp(prop_offset);
            stream.write((char*)_raw_properties.data(), (sizeof(Property) * _raw_properties.size()));

            // write child containers
            stream.seekp(child_data_offset);
            for (auto child : containers) {
                instance_queue.push({child_offset, child});
                child_offset += sizeof(Node);
            }

            instance_queue.pop();
        }

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
    // skip "root"
    if (m_NameHash == 0xaa7d522a) {
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
            switch (prop->GetType()) {
                case RTPC_TYPE_INTEGER: {
                    auto value = std::any_cast<int32_t>(prop->GetValue());
                    if (ImGui::InputInt(prop->GetName().c_str(), &value)) {
                        prop->SetValue(value);
                    }
                    break;
                }

                case RTPC_TYPE_FLOAT: {
                    auto value = std::any_cast<float>(prop->GetValue());
                    if (ImGui::InputFloat(prop->GetName().c_str(), &value)) {
                        prop->SetValue(value);
                    }
                    break;
                }

                case RTPC_TYPE_STRING: {
                    auto& value = std::any_cast<std::string>(prop->GetValue());
                    if (ImGui::InputText(prop->GetName().c_str(), &value)) {
                        prop->SetValue(value);
                    }
                    break;
                }

                case RTPC_TYPE_VEC2: {
                    auto& value = std::any_cast<glm::vec2>(prop->GetValue());
                    if (ImGui::InputFloat2(prop->GetName().c_str(), &value.x)) {
                        prop->SetValue(glm::vec2{value.x, value.y});
                    }
                    break;
                }

                case RTPC_TYPE_VEC3: {
                    auto& value = std::any_cast<glm::vec3>(prop->GetValue());
                    if (ImGui::InputFloat3(prop->GetName().c_str(), &value.x)) {
                        prop->SetValue(glm::vec3{value.x, value.y, value.z});
                    }
                    break;
                }

                case RTPC_TYPE_VEC4: {
                    auto& value = std::any_cast<glm::vec4>(prop->GetValue());
                    if (ImGui::InputFloat4(prop->GetName().c_str(), &value.x)) {
                        prop->SetValue(glm::vec4{value.x, value.y, value.z, value.w});
                    }
                    break;
                }

                case RTPC_TYPE_MAT4X4: {
                    auto& value = prop->GetValue<glm::mat4>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::InputFloat4("m0", &value[0][0]);
                    ImGui::InputFloat4("m1", &value[1][0]);
                    ImGui::InputFloat4("m2", &value[2][0]);
                    ImGui::InputFloat4("m3", &value[3][0]);
                    break;
                }

                case RTPC_TYPE_LIST_INTEGERS: {
                    auto& values = prop->GetValue<std::vector<int32_t>>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::Text(RuntimeContainerProperty::GetTypeName(prop->GetType()).c_str());
                    for (auto& value : values) {
                        ImGui::InputInt(".", &value);
                    }
                    break;
                }

                case RTPC_TYPE_LIST_FLOATS: {
                    auto& values = prop->GetValue<std::vector<float>>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::Text(RuntimeContainerProperty::GetTypeName(prop->GetType()).c_str());
                    for (auto& value : values) {
                        ImGui::InputFloat(".", &value);
                    }
                    break;
                }

                case RTPC_TYPE_LIST_BYTES: {
                    auto& values = prop->GetValue<std::vector<uint8_t>>();
                    ImGui::Text(prop->GetName().c_str());
                    ImGui::Text(RuntimeContainerProperty::GetTypeName(prop->GetType()).c_str());
                    // for (auto& value : values) {
                    // ImGui::InputInt(".", &value);
                    //}
                    break;
                }

                case RTPC_TYPE_OBJECT_ID: {
                    auto& value = prop->GetValue<std::pair<uint32_t, uint32_t>>();
                    if (ImGui::InputScalarN(prop->GetName().c_str(), ImGuiDataType_U32, (void*)&value.first, 2)) {
                        prop->SetValue(std::make_pair(value.first, value.second));
                    }

                    break;
                }

                case RTPC_TYPE_EVENTS: {
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

void RuntimeContainer::Load(const std::filesystem::path&                           filename,
                            std::function<void(std::shared_ptr<RuntimeContainer>)> callback)
{
    FileLoader::Get()->ReadFile(filename, [&, filename, callback](bool success, FileBuffer data) {
        if (success) {
            auto result = FileLoader::Get()->ParseRuntimeContainer(filename, data);
            callback(result);
        } else {
            LOG_ERROR("Failed to read runtime container \"{}\"", filename.string());
        }
    });
}
