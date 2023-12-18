#include "pch.h"

#include "runtime_container.h"

#include "app/app.h"

#include "game/game.h"
#include "game/name_hash_lookup.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/ui.h"

#include <imgui.h>
#include <sstream>

namespace jcmr::game::format
{
// static const char* variant_type_to_string(const ava::RuntimePropertyContainer::EVariantType type)
// {
//     using namespace ava::RuntimePropertyContainer;

//     switch (type) {
//         case T_VARIANT_UNASSIGNED: return "Unassigned";
//         case T_VARIANT_INTEGER: return "int";
//         case T_VARIANT_FLOAT: return "float";
//         case T_VARIANT_STRING: return "string";
//         case T_VARIANT_VEC2: return "vec2";
//         case T_VARIANT_VEC3: return "vec3";
//         case T_VARIANT_VEC4: return "vec4";
//         case T_VARIANT__DO_NOT_USE_1: return "T_VARIANT__DO_NOT_USE_1";
//         case T_VARIANT_MAT4x4: return "mat4x4";
//         case T_VARIANT_VEC_INTS: return "int[]";
//         case T_VARIANT_VEC_FLOATS: return "float[]";
//         case T_VARIANT_VEC_BYTES: return "byte[]";
//         case T_VARIANT__DO_NOT_USE_2: return "T_VARIANT__DO_NOT_USE_2";
//         case T_VARIANT_OBJECTID: return "ObjectID";
//         case T_VARIANT_VEC_EVENTS: return "ObjectID[]";
//     }

//     return "";
// }

static const char* variant_data_type_to_string(u32 datatype)
{
    if (datatype == RuntimeContainer::VARIANT_DATATYPE_NAMEHASH)
        return " (namehash)";
    else if (datatype == RuntimeContainer::VARIANT_DATATYPE_GUID)
        return " (guid)";
    else if (datatype == RuntimeContainer::VARIANT_DATATYPE_COLOUR)
        return " (colour)";

    return "";
}

struct RuntimeContainerWrapper {
    RuntimeContainerWrapper() = delete;
    explicit RuntimeContainerWrapper(const ava::RuntimePropertyContainer::Container& container)
        : m_container(std::move(container))
    {
        update_container_display_names(m_container);
    }

    void render() { render(m_container); }

    void render(ava::RuntimePropertyContainer::Container& container)
    {
        using namespace ava::RuntimePropertyContainer;

        ImGui::PushID((void*)&container);

        u32 flags = ImGuiTreeNodeFlags_SpanAvailWidth;

        // open the root tree by default
        static auto s_root_container_hash = "root"_hash_little;
        if (container.m_NameHash == s_root_container_hash) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        auto title = get_container_display_name(&container);
        if (ImGui::TreeNodeEx(title.c_str(), flags)) {
            // draw variants
            if (ImGuiEx::BeginWidgetTableLayout()) {
                for (auto& variant : container.m_Variants) {
                    // skip unassigned variants
                    if (variant.m_Type == T_VARIANT_UNASSIGNED) continue;

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    auto& [label, is_unknown, datatype] = get_variant_data(variant.m_NameHash);

                    if (is_unknown) ImGui::PushStyleColor(ImGuiCol_Text, {0.53f, 0.53f, 0.53f, 1.0f});
                    ImGuiEx::Label("%s", label.c_str());
                    if (is_unknown) ImGui::PopStyleColor();

                    // if the current variant is unknown, left mouse click will copy the namehash to the clipboard
                    // if (is_unknown && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // }

                    // tooltip
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s%s - %s", label.c_str(), variant_data_type_to_string(datatype),
                                          VariantTypeToString(variant.m_Type));
                    }

                    ImGui::TableNextColumn();

                    ImGui::PushID((void*)&variant);
                    ImGui::SetNextItemWidth(-1);

                    switch (variant.m_Type) {
                        case T_VARIANT_INTEGER: {
                            auto value = variant.as<i32>();
                            if (ImGui::InputInt("", &value, 0, 0,
                                                (datatype == RuntimeContainer::VARIANT_DATATYPE_NAMEHASH)
                                                    ? ImGuiInputTextFlags_CharsHexadecimal
                                                    : ImGuiInputTextFlags_CharsDecimal)) {
                                variant.m_Value = value;
                            }

                            break;
                        }

                        case T_VARIANT_FLOAT: {
                            auto value = variant.as<float>();
                            if (ImGui::InputFloat("", &value)) {
                                variant.m_Value = value;
                            }

                            break;
                        }

                        case T_VARIANT_STRING: {
                            auto value = variant.as<std::string>();

                            char buf[1024] = {0};
                            strcpy_s(buf, value.c_str());
                            if (ImGui::InputText("", buf, lengthOf(buf))) {
                                variant.m_Value = std::string(buf);
                            }

                            break;
                        }

                        case T_VARIANT_VEC2: {
                            auto value = variant.as<std::array<float, 2>>();
                            if (ImGui::InputFloat2("", value.data())) {
                                variant.m_Value = value;
                            }

                            break;
                        }

                        case T_VARIANT_VEC3: {
                            // TODO : colours seem to be acting weird? might be something todo with how imgui is
                            // treating
                            //        them as we're getting float values 0-255, imgui is expecting 0-1?
                            auto value = variant.as<std::array<float, 3>>();
                            if ((datatype == RuntimeContainer::VARIANT_DATATYPE_COLOUR)
                                    ? ImGuiEx::ColorButtonPicker3("", value.data())
                                    : ImGui::InputFloat3("", value.data())) {
                                variant.m_Value = value;
                            }

                            break;
                        }

                        case T_VARIANT_VEC4: {
                            // TODO : colours seem to be acting weird? might be something todo with how imgui is
                            // treating
                            //        them as we're getting float values 0-255, imgui is expecting 0-1?
                            auto value = variant.as<std::array<float, 4>>();
                            if ((datatype == RuntimeContainer::VARIANT_DATATYPE_COLOUR)
                                    ? ImGuiEx::ColorButtonPicker4("", value.data())
                                    : ImGui::InputFloat4("", value.data())) {
                                variant.m_Value = value;
                            }

                            break;
                        }

                        case T_VARIANT_MAT4x4: {
                            auto value = variant.as<std::array<float, 16>>();

                            vec3 scale, translation, skew;
                            vec4 perspective;
                            quat orientation;

                            glm::decompose(glm::make_mat4(value.data()), scale, orientation, translation, skew,
                                           perspective);

                            ImGui::InputFloat3("Translation##translation", glm::value_ptr(translation));

                            ImGui::SetNextItemWidth(-1);
                            glm::vec3 euler = glm::eulerAngles(orientation) * glm::pi<float>() / 180.f;
                            ImGui::InputFloat3("Rotation##rotation", glm::value_ptr(euler));

                            ImGui::SetNextItemWidth(-1);
                            ImGui::InputFloat3("Scale##scale", glm::value_ptr(scale));

                            // ImGui::PushItemWidth(-1);
                            // ImGui::InputFloat4("##m1", value.data());
                            // ImGui::InputFloat4("##m2", &value.data()[4]);
                            // ImGui::InputFloat4("##m3", &value.data()[8]);
                            // ImGui::InputFloat4("##m4", &value.data()[12]);
                            break;
                        }

                        case T_VARIANT_VEC_INTS: {
                            auto& value = variant.as<std::vector<i32>>();
                            ImGui::Text("vec_ints (%d)", value.size());
                            break;
                        }

                        case T_VARIANT_VEC_FLOATS: {
                            auto& value = variant.as<std::vector<float>>();
                            ImGui::Text("vec_floats (%d)", value.size());
                            break;
                        }

                        case T_VARIANT_VEC_BYTES: {
                            auto& value = variant.as<std::vector<u8>>();
                            ImGui::Text("vec_bytes (%d)", value.size());
                            break;
                        }

                        case T_VARIANT_OBJECTID: {
                            auto value = variant.as<ava::SObjectID>().to_uint64();
                            if (ImGui::InputScalar("", ImGuiDataType_U64, (void*)&value, nullptr, nullptr, "%016llX",
                                                   ImGuiInputTextFlags_CharsHexadecimal)) {
                                variant.m_Value = ava::SObjectID(value);
                            }

                            break;
                        }

                        case T_VARIANT_VEC_EVENTS: {
                            auto& value = variant.as<std::vector<ava::SObjectID>>();
                            ImGui::Text("vec_events (%d)", value.size());
                            for (auto i = 0; i < value.size(); ++i) {

                                // auto val = value[i].to_uint64();
                                // if (ImGui::InputScalar("", ImGuiDataType_U64, (void*)&val, nullptr, nullptr,
                                // "%016llX",
                                //                        ImGuiInputTextFlags_CharsHexadecimal)) {
                                //     value[i]        = ava::SObjectID(val);
                                //     variant.m_Value = value;
                                // }

                                // ImGui::TableNextRow();
                                // ImGui::TableNextColumn();
                                // ImGui::TableNextColumn();
                                // ImGui::SetNextItemWidth(-1);
                            }

                            break;
                        }
                    }

                    ImGui::PopID();
                }
            }

            ImGuiEx::EndWidgetTableLayout();

            // draw child containers
            for (auto& child : container.m_Containers) {
                render(child);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    const std::string& get_container_display_name(ava::RuntimePropertyContainer::Container* container)
    {
        auto iter = m_container_names.find(container);
        if (iter != m_container_names.end()) {
            return (*iter).second;
        }

        // cache the name from the lookup table
        m_container_names.insert({container, find_in_namehash_lookup_table(container->m_NameHash)});
        return m_container_names[container];
    }

    const RuntimeContainer::VariantData& get_variant_data(u32 namehash)
    {
        auto iter = m_variant_names.find(namehash);
        return (*iter).second;
    }

    const ava::RuntimePropertyContainer::Container& get_container() const { return m_container; }

  private:
    void update_container_display_names(ava::RuntimePropertyContainer::Container& container)
    {
        auto& name       = container.GetVariant("name"_hash_little, false);
        auto& class_name = container.GetVariant("_class"_hash_little, false);
        auto& class_hash = container.GetVariant("_class_hash"_hash_little, false);

        std::stringstream display_name;

        // append class_name or class_hash
        if (class_name.valid()) {
            display_name << class_name.as<std::string>() << " - ";
        } else if (class_hash.valid()) {
            display_name << find_in_namehash_lookup_table(class_hash.as<i32>()) << " - ";
        }

        // add real name
        if (name.valid()) {
            display_name << name.as<std::string>();
        } else {
            display_name << find_in_namehash_lookup_table(container.m_NameHash);
        }

        // fallback to the class_hash or container namehash if the final string is still empty
        if (display_name.str().empty()) {
            if (class_hash.valid()) {
                display_name << fmt::format("Class 0x{:X}", class_hash.as<i32>());
            } else {
                display_name << fmt::format("0x{:X}", container.m_NameHash);
            }
        }

        m_container_names.insert({&container, display_name.str()});

        for (auto& variant : container.m_Variants) {
            update_variant_display_names(variant);
        }

        for (auto& child_container : container.m_Containers) {
            update_container_display_names(child_container);
        }
    }

    // known variants that contain hashes
    static inline std::array s_hash_keys{
        "ragdoll"_hash_little,  "skeleton"_hash_little,   "model"_hash_little,
        "filepath"_hash_little, "layer_name"_hash_little, "EffectUsage"_hash_little,
    };

    // known variants that contain guids
    static inline std::array s_guid_keys{
        "Sound"_hash_little,
        "FMODEvent"_hash_little,
    };

    // known variants that contain colours
    static inline std::array s_colour_keys{
        "diffuse"_hash_little,
    };

#define IS_KNOWN_KEY(array) std::find(array.begin(), array.end(), variant.m_NameHash) != array.end()

    void update_variant_display_names(ava::RuntimePropertyContainer::Variant& variant)
    {
        if (m_variant_names.find(variant.m_NameHash) != m_variant_names.end()) {
            return;
        }

        std::string name       = find_in_namehash_lookup_table(variant.m_NameHash);
        bool        is_unknown = false;
        u32         datatype   = RuntimeContainer::VARIANT_DATATYPE_COUNT;

        if (name.empty()) {
            name       = fmt::format("0x{:X}", variant.m_NameHash);
            is_unknown = true;
        }

        static auto string_contains = [](const std::string_view& haystack, const std::string_view& needle) -> bool {
            auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                                  [](char ch1, char ch2) { return toupper(ch1) == toupper(ch2); });
            return (it != haystack.end());
        };

        if (IS_KNOWN_KEY(s_hash_keys) || string_contains(name, "hash")) {
            datatype = RuntimeContainer::VARIANT_DATATYPE_NAMEHASH;
        } else if (IS_KNOWN_KEY(s_guid_keys)) {
            datatype = RuntimeContainer::VARIANT_DATATYPE_GUID;
        } else if (IS_KNOWN_KEY(s_colour_keys) || string_contains(name, "color")) {
            datatype = RuntimeContainer::VARIANT_DATATYPE_COLOUR;
        }

        m_variant_names.insert({variant.m_NameHash, std::make_tuple(name, is_unknown, datatype)});
    }

  private:
    ava::RuntimePropertyContainer::Container                                   m_container;
    std::unordered_map<ava::RuntimePropertyContainer::Container*, std::string> m_container_names;
    std::unordered_map<u32, RuntimeContainer::VariantData>                     m_variant_names;
};

struct RuntimeContainerImpl final : RuntimeContainer {
    RuntimeContainerImpl(App& app)
        : m_app(app)
    {
        app.get_ui().on_render([this](RenderContext& context) {
            std::string _runtime_container_to_unload;
            for (auto& container : m_containers) {
                bool open  = true;
                auto title = fmt::format("{} {}##rtpc_editor", get_filetype_icon(), container.first);

                auto dock_id = m_app.get_ui().get_dockspace_id(UI::E_DOCKSPACE_RIGHT);
                ImGui::SetNextWindowDockID(dock_id, ImGuiCond_Appearing); // TODO: ImGuiCond_FirstUseEver

                if (ImGui::Begin(title.c_str(), &open, (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))) {
                    // menu bar
                    if (ImGui::BeginMenuBar()) {
                        if (ImGui::BeginMenu("File")) {
                            // save file
                            if (ImGui::Selectable(ICON_FA_SAVE " Save to...")) {
                                std::filesystem::path path;
                                if (os::get_open_folder(&path, os::FileDialogParams{})) {
                                    m_app.save_file(this, container.first, path);
                                }
                            }

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }

                    container.second.render();
                }

                if (!open) {
                    _runtime_container_to_unload = container.first;
                }

                ImGui::End();
            }

            if (!_runtime_container_to_unload.empty()) {
                unload(_runtime_container_to_unload);
                _runtime_container_to_unload.clear();
            }
        });
    }

    bool load(const std::string& filename) override
    {
        LOG_INFO("RuntimeContainer : loading \"{}\"...", filename);

        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            return false;
        }

        ava::RuntimePropertyContainer::Container container{};
        AVA_FL_ENSURE(ava::RuntimePropertyContainer::Parse(buffer, &container), false);

        m_containers.insert({filename, RuntimeContainerWrapper(std::move(container))});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_containers.find(filename);
        if (iter == m_containers.end()) return;
        m_containers.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        auto iter = m_containers.find(filename);
        if (iter == m_containers.end()) return false;

        auto& runtime_container = (*iter).second.get_container();
        AVA_FL_ENSURE(ava::RuntimePropertyContainer::Write(runtime_container, out_buffer), false);
        return true;
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_containers.find(filename) != m_containers.end();
    }

  public:
    App&                                                     m_app;
    std::unordered_map<std::string, RuntimeContainerWrapper> m_containers;
};

RuntimeContainer* RuntimeContainer::create(App& app)
{
    return new RuntimeContainerImpl(app);
}

void RuntimeContainer::destroy(RuntimeContainer* instance)
{
    delete instance;
}
} // namespace jcmr::game::format