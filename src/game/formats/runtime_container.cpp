#include <AvaFormatLib.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <spdlog/spdlog.h>

#include "../vendor/ava-format-lib/include/util/byte_array_buffer.h"

#include <queue>
#include <sstream>

#include <glm/glm.hpp>

#include "runtime_container.h"
#include "util.h"

#include "game/file_loader.h"
#include "game/formats/render_block_model.h"
#include "game/name_hash_lookup.h"
#include "game/types.h"

extern bool g_IsJC4Mode;

std::recursive_mutex                                  Factory<RuntimeContainer>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RuntimeContainer>> Factory<RuntimeContainer>::Instances;

void RuntimeContainer::Parse(const std::vector<uint8_t>& buffer, ParseCallback_t callback)
{
    if (buffer.empty()) {
        if (callback) {
            callback(false);
        }
        return;
    }

    using namespace ava::RuntimePropertyContainer;

    std::thread([&, buffer, callback] {
        byte_array_buffer buf(buffer);
        std::istream      stream(&buf);

        // @TODO: most of this should be inside of AvaFormatLib.

        // read header
        RtpcHeader header{};
        stream.read((char*)&header, sizeof(RtpcHeader));
        if (header.m_Magic != RTPC_MAGIC) {
#ifdef _DEBUG
            __debugbreak();
#endif
            if (callback) {
                callback(false);
            }
            return;
        }

        SPDLOG_INFO("RTPC v{}", header.m_Version);

        // read the root container
        RtpcContainer root_container{};
        stream.read((char*)&root_container, sizeof(RtpcContainer));

        m_Root = std::make_unique<RTPC::Container>(std::move(root_container));

        std::queue<RTPC::Container*> container_queue;
        container_queue.push(m_Root.get());

        while (!container_queue.empty()) {
            const auto container = container_queue.front();

            // read all the container variants
            for (decltype(container->m_NumVariants) i = 0; i < container->m_NumVariants; ++i) {
                stream.seekg(container->m_DataOffset + (i * sizeof(RtpcContainerVariant)));

                RtpcContainerVariant variant{};
                stream.read((char*)&variant, sizeof(RtpcContainerVariant));

                // seek to the variant data offset
                if (variant.m_Type != T_VARIANT_INTEGER && variant.m_Type != T_VARIANT_FLOAT) {
                    stream.seekg(variant.m_DataOffset);
                }

                auto variant_wrapper = std::make_unique<RTPC::Variant>(variant);
                switch (variant.m_Type) {
                    case T_VARIANT_INTEGER: {
                        variant_wrapper->m_Value = *(int32_t*)&variant.m_DataOffset;
                        break;
                    }

                    case T_VARIANT_FLOAT: {
                        variant_wrapper->m_Value = *(float*)&variant.m_DataOffset;
                        break;
                    }

                    case T_VARIANT_STRING: {
                        std::string value;
                        std::getline(stream, value, '\0');
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC2: {
                        glm::vec2 value{};
                        stream.read((char*)&value, sizeof(glm::vec2));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC3: {
                        glm::vec3 value{};
                        stream.read((char*)&value, sizeof(glm::vec3));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC4: {
                        glm::vec4 value{};
                        stream.read((char*)&value, sizeof(glm::vec4));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_MAT4x4: {
                        glm::mat4x4 value{};
                        stream.read((char*)&value, sizeof(glm::mat4x4));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC_INTS: {
                        int32_t count;
                        stream.read((char*)&count, sizeof(int32_t));

                        std::vector<int32_t> value(count);
                        stream.read((char*)value.data(), (count * sizeof(int32_t)));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC_FLOATS: {
                        int32_t count;
                        stream.read((char*)&count, sizeof(int32_t));

                        std::vector<float> value(count);
                        stream.read((char*)value.data(), (count * sizeof(float)));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC_BYTES: {
                        int32_t count;
                        stream.read((char*)&count, sizeof(int32_t));

                        std::vector<uint8_t> value(count);
                        stream.read((char*)value.data(), (count * sizeof(uint8_t)));

                        if (variant_wrapper->m_Flags & RTPC::E_VARIANT_IS_GUID) {
                            assert(value.size() == sizeof(GUID)); // vector size should always be 16 here!
                            variant_wrapper->m_Value = *(GUID*)value.data();
                        } else {
#ifdef _DEBUG
                            // PROBABY A GUID. TAKE NOTE OF THE NAME AND ADD THE HASH IN THE VARIANT CONSTRUCTOR!
                            if (value.size() == 16) {
                                __debugbreak();
                            }
#endif
                            variant_wrapper->m_Value = value;
                        }
                        break;
                    }

                    case T_VARIANT_OBJECTID: {
                        jc::SObjectID value{};
                        stream.read((char*)&value, sizeof(jc::SObjectID));
                        variant_wrapper->m_Value = value;
                        break;
                    }

                    case T_VARIANT_VEC_EVENTS: {
                        int32_t count = 0;
                        stream.read((char*)&count, sizeof(int32_t));

                        std::vector<jc::SObjectID> value(count);
                        stream.read((char*)value.data(), (count * sizeof(jc::SObjectID)));
                        variant_wrapper->m_Value = value;
                        break;
                    }
                }

                container->m_Variants.push_back(std::move(variant_wrapper));
            }

            const uint32_t pos = container->m_DataOffset + (container->m_NumVariants * sizeof(RtpcContainerVariant));
            stream.seekg(ava::math::align(pos));

            // read all sub containers
            for (decltype(container->m_NumContainers) i = 0; i < container->m_NumContainers; ++i) {
                RtpcContainer sub_container{};
                stream.read((char*)&sub_container, sizeof(RtpcContainer));

                auto sub_container_wrapper = std::make_unique<RTPC::Container>(std::move(sub_container));
                container_queue.push(sub_container_wrapper.get());

                container->m_Containers.push_back(std::move(sub_container_wrapper));
            }

            container_queue.pop();
        }

        // update all container display names
        m_Root->UpdateDisplayName();

        if (callback) {
            callback(true);
        }
    }).detach();
}

void RuntimeContainer::ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                        bool external)
{
    if (!RuntimeContainer::exists(filename.string())) {
        auto rtpc = RuntimeContainer::make(filename);
        rtpc->Parse(data);
    }
}

bool RuntimeContainer::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
#if 0
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
#endif
    return false;
}

void RuntimeContainer::Load(const std::filesystem::path& filename, LoadCallback_t callback)
{
    if (RuntimeContainer::exists(filename.string())) {
        callback(true, RuntimeContainer::get(filename.string()));
    } else {
        FileLoader::Get()->ReadFile(filename, [&, filename, callback](bool success, std::vector<uint8_t> buffer) {
            if (success) {
                auto rtpc = RuntimeContainer::make(filename);
                rtpc->Parse(buffer, [&, rtpc, callback](bool success) { callback(success, rtpc); });
            } else {
                SPDLOG_ERROR("Failed to read RuntimeContainer \"{}\"", filename.string());
            }
        });
    }
}

void RuntimeContainer::DrawUI(RTPC::Container* container, int32_t index, uint8_t depth)
{
    using namespace ava::RuntimePropertyContainer;
    using namespace RTPC;

    /*if (!container)
        container = m_Root.get();*/

    // @TODO: there can also be variants in the root node!
    //		  we should change this a little or those items will not be drawn!

    if (!container) {
        // draw root container children
        if (m_Root) {
            for (size_t i = 0; i < m_Root->m_Containers.size(); ++i) {
                DrawUI(m_Root->m_Containers[i].get(), i, (depth + 1));
            }
        }
        return;
    }

    std::string title = container->m_Name + "##" + std::to_string(index);
    if (ImGui::TreeNode(title.c_str())) {
        // draw variants
        for (const auto& variant : container->m_Variants) {
            switch (variant->m_Type) {
                case T_VARIANT_INTEGER: {
                    auto value = variant->Value<int32_t>();
                    if (ImGui::InputInt(variant->m_Name.c_str(), &value, 0, 0,
                                        (variant->m_Flags & E_VARIANT_IS_HASH) ? ImGuiInputTextFlags_CharsHexadecimal
                                                                               : ImGuiInputTextFlags_CharsDecimal)) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_FLOAT: {
                    auto value = variant->Value<float>();
                    if (ImGui::InputFloat(variant->m_Name.c_str(), &value)) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_STRING: {
                    auto& value = variant->Value<std::string>();
                    if (ImGui::InputText(variant->m_Name.c_str(), &value)) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_VEC2: {
                    auto& value = variant->Value<glm::vec2>();
                    if (ImGui::InputFloat2(variant->m_Name.c_str(), glm::value_ptr(value))) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_VEC3: {
                    auto& value = variant->Value<glm::vec3>();
                    if (ImGui::InputFloat3(variant->m_Name.c_str(), glm::value_ptr(value))) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_VEC4: {
                    auto& value = variant->Value<glm::vec4>();
                    if (ImGui::InputFloat4(variant->m_Name.c_str(), glm::value_ptr(value))) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_MAT4x4: {
                    auto& value = variant->Value<glm::mat4x4>();
                    for (uint8_t i = 0; i < 4; ++i) {
                        auto title = util::format("%s [%d]", variant->m_Name.c_str(), i);
                        ImGui::InputFloat4(title.c_str(), glm::value_ptr(value[i]));
                    }
                    break;
                }

                case T_VARIANT_VEC_INTS: {
                    auto& value = variant->Value<std::vector<int32_t>>();
                    for (size_t i = 0; i < value.size(); ++i) {
                        auto title = util::format("%s [%d]", variant->m_Name.c_str(), i);
                        if (ImGui::InputScalarN(title.c_str(), ImGuiDataType_S32, (void*)&value[i], 1, nullptr, nullptr,
                                                "%08X")) {
                            variant->m_Value = value;
                        }
                    }
                    break;
                }

                case T_VARIANT_VEC_FLOATS: {
                    auto& value = variant->Value<std::vector<float>>();
                    for (size_t i = 0; i < value.size(); ++i) {
                        auto title = util::format("%s [%d]", variant->m_Name.c_str(), i);
                        if (ImGui::InputScalarN(title.c_str(), ImGuiDataType_Float, (void*)&value[i], 1)) {
                            variant->m_Value = value;
                        }
                    }
                    break;
                }

                case T_VARIANT_VEC_BYTES: {
                    // special case for GUIDs
                    if (variant->m_Flags & E_VARIANT_IS_GUID) {
                        auto& value       = variant->Value<GUID>();
                        auto  guid_string = util::GUID_to_string(value);
                        if (ImGui::InputText(variant->m_Name.c_str(), &guid_string)) {
                            variant->m_Value = util::GUID_from_string(guid_string, value);
                        }
                    } else {
                        auto& value = variant->Value<std::vector<uint8_t>>();
                        for (size_t i = 0; i < value.size(); ++i) {
                            auto title = util::format("%s [%d] (bytes)", variant->m_Name.c_str(), i);
                            if (ImGui::InputScalarN(title.c_str(), ImGuiDataType_U8, (void*)&value[i], 1, nullptr,
                                                    nullptr, "%02X", ImGuiInputTextFlags_CharsHexadecimal)) {
                                variant->m_Value = value;
                            }
                        }
                    }
                    break;
                }

                case T_VARIANT_OBJECTID: {
                    auto& value = variant->Value<jc::SObjectID>();
                    if (ImGui::InputScalarN(variant->m_Name.c_str(), ImGuiDataType_U16, (void*)&value, 4, nullptr,
                                            nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
                        variant->m_Value = value;
                    }
                    break;
                }

                case T_VARIANT_VEC_EVENTS: {
                    auto& value = variant->Value<std::vector<jc::SObjectID>>();
                    for (size_t i = 0; i < value.size(); ++i) {
                        auto title = util::format("%s [%d]", variant->m_Name.c_str(), i);
                        if (ImGui::InputScalarN(title.c_str(), ImGuiDataType_U16, (void*)&value[i], 4, nullptr, nullptr,
                                                "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
                            variant->m_Value = value;
                        }
                    }
                    break;
                }
            }
        }

        // draw child containers
        for (size_t i = 0; i < container->m_Containers.size(); ++i) {
            DrawUI(container->m_Containers[i].get(), i, (depth + 1));
        }

        ImGui::TreePop();
    }
}

RTPC::Container::Container(const ava::RuntimePropertyContainer::RtpcContainer& container)
{
    m_Name          = NameHashLookup::GetName(container.m_Key);
    m_Key           = container.m_Key;
    m_DataOffset    = container.m_DataOffset;
    m_NumVariants   = container.m_NumVariants;
    m_NumContainers = container.m_NumContainers;
}

void RTPC::Container::UpdateDisplayName()
{
    const auto name       = GetVariant("name"_hash_little, false);
    const auto class_name = GetVariant("_class"_hash_little, false);
    const auto class_hash = GetVariant("_class_hash"_hash_little, false);

    std::stringstream ss;
    bool              has_name = (name != nullptr);

    // append the real name
    if (has_name) {
        ss << name->Value<std::string>();
    }

    // append the class name
    if (class_name || class_hash) {
        ss << (has_name ? " (" : "");

        if (class_name) {
            ss << class_name->Value<std::string>();
        } else if (class_hash) {
            ss << NameHashLookup::GetName(class_hash->Value<int32_t>());
        }

        ss << (has_name ? ")" : "");
    }

    if (m_Name.empty()) {
        m_Name = ss.str();
    } else {
        m_Name = ss.str() + " (" + m_Name + ")";
    }

    for (const auto& container : m_Containers) {
        container->UpdateDisplayName();
    }
}

RTPC::Container* RTPC::Container::GetContainer(const uint32_t name_hash, bool recursive)
{
    for (const auto& container : m_Containers) {
        if (container->m_Key == name_hash) {
            return container.get();
        }

        // search child containers
        if (recursive) {
            if (auto child = container->GetContainer(name_hash)) {
                return child;
            }
        }
    }

    return nullptr;
}

RTPC::Variant* RTPC::Container::GetVariant(const uint32_t name_hash, bool recursive)
{
    for (const auto& variant : m_Variants) {
        if (variant->m_Key == name_hash) {
            return variant.get();
        }
    }

    // search child containers
    if (recursive) {
        for (const auto& container : m_Containers) {
            if (auto variant = container->GetVariant(name_hash)) {
                return variant;
            }
        }
    }

    return nullptr;
}

RTPC::Variant::Variant(const ava::RuntimePropertyContainer::RtpcContainerVariant& variant)
{
    m_Name       = NameHashLookup::GetName(variant.m_Key);
    m_Key        = variant.m_Key;
    m_DataOffset = variant.m_DataOffset;
    m_Type       = variant.m_Type;

    if (m_Name.empty()) {
        m_Name = util::format("Unknown (%08x)", m_Key);
    }

    // common variant names that contain hashes
    static std::array hash_keys{
        "ragdoll"_hash_little,
        "skeleton"_hash_little,
        "model"_hash_little,
        "filepath"_hash_little,
    };

    static std::array guid_keys{
        "Sound"_hash_little,
        "FMODEvent"_hash_little,
    };

#define InKeys(keys) std::find(keys.begin(), keys.end(), m_Key) != keys.end()

    if (InKeys(hash_keys) || m_Name.rfind("hash") != std::string::npos) {
        m_Flags |= E_VARIANT_IS_HASH;
    } else if (InKeys(guid_keys)) {
        m_Flags |= E_VARIANT_IS_GUID;
    }
}

#if 0
// @TODO: move inside AvaFormatLib
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
#endif
