#include <jc3/formats/RuntimeContainer.h>
#include <Window.h>
#include <jc3/FileLoader.h>
#include <jc3/hashlittle.h>
#include <glm.hpp>
#include <jc3/formats/RenderBlockModel.h>

std::recursive_mutex Factory<RuntimeContainer>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RuntimeContainer>> Factory<RuntimeContainer>::Instances;

RuntimeContainerProperty::RuntimeContainerProperty(uint32_t name_hash, uint8_t type)
    : m_NameHash(name_hash),
    m_Type(static_cast<PropertyType>(type))
{
    //DEBUG_LOG("RuntimeContainerProperty::RuntimeContainerProperty");
    m_Name = NameHashLookup::GetName(name_hash);

    if (m_Name.empty()) {
        std::stringstream ss;
        ss << "Unknown (" << std::hex << std::setw(4) << name_hash << ")";
        m_Name = ss.str();
    }
}

RuntimeContainer::RuntimeContainer(uint32_t name_hash)
    : m_NameHash(name_hash)
{
    //DEBUG_LOG("RuntimeContainer::RuntimeContainer");
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

void RuntimeContainer::GenerateNamesIfNeeded()
{
    if (m_Name.empty()) {
        std::stringstream ss;
        const auto _class = GetProperty("_class");
        const auto _name = GetProperty("name");

        if (_class) {
            ss << _class->GetValue<std::string>();

            if (_name) ss << " (";
        }

        if (_name) {
            ss << _name->GetValue<std::string>();

            if (_class) ss << ")";
        }

        m_Name = ss.str();
    }

    for (const auto& container : m_Containers) {
        container->GenerateNamesIfNeeded();
    }
}

RuntimeContainerProperty* RuntimeContainer::GetProperty(uint32_t name_hash)
{
    // find the property
    for (auto& prop : m_Properties) {
        if (prop->GetNameHash() == name_hash) {
            return prop;
        }
    }

    // try find the property in child containers
    for (auto& container : m_Containers) {
        auto prop = container->GetProperty(name_hash);
        if (prop) {
            return prop;
        }
    }

    return nullptr;
}

RuntimeContainerProperty* RuntimeContainer::GetProperty(const std::string& name)
{
    return GetProperty(hashlittle(name.c_str()));
}

RuntimeContainer* RuntimeContainer::GetContainer(uint32_t name_hash)
{
    // find the container
    for (auto& container : m_Containers) {
        if (container->GetNameHash() == name_hash) {
            return container;
        }

        // if the container has a child we're looking for, use that
        auto child_container = container->GetContainer(name_hash);
        if (child_container) {
            return child_container;
        }
    }

    return nullptr;
}

RuntimeContainer* RuntimeContainer::GetContainer(const std::string& name)
{
    return GetContainer(hashlittle(name.c_str()));
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

void RuntimeContainer::FileReadCallback(const fs::path& filename, const FileBuffer& data)
{
    DEBUG_LOG("RuntimeContainer::FileReadCallback");
    DEBUG_LOG(filename);

    const auto rtpc = FileLoader::Get()->ParseRuntimeContainer(data);
    if (rtpc) {
        rtpc->SetFileName(filename);
    }
    else {
        DEBUG_LOG("failed to read RTPC!");
    }
}

void RuntimeContainer::DrawUI(uint8_t depth)
{
    if (ImGui::TreeNode(m_Name.c_str())) {
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
                auto value = std::any_cast<std::string>(prop->GetValue());
                std::vector<char> buf(value.begin(), value.end());
                if (ImGui::InputText(prop->GetName().c_str(), buf.data(), value.size())) {

                }
                break;
            }

            case RTPC_TYPE_VEC2: {
                auto value = std::any_cast<glm::vec2>(prop->GetValue());
                if (ImGui::InputFloat2(prop->GetName().c_str(), &value.x)) {
                    prop->SetValue(glm::vec2{ value.x, value.y });
                }
                break;
            }

            case RTPC_TYPE_VEC3: {
                auto value = std::any_cast<glm::vec3>(prop->GetValue());
                if (ImGui::InputFloat3(prop->GetName().c_str(), &value.x)) {
                    prop->SetValue(glm::vec3{ value.x, value.y, value.z });
                }
                break;
            }

            case RTPC_TYPE_VEC4: {
                auto value = std::any_cast<glm::vec4>(prop->GetValue());
                if (ImGui::InputFloat4(prop->GetName().c_str(), &value.x)) {
                    prop->SetValue(glm::vec4{ value.x, value.y, value.z, value.w });
                }
                break;
            }

            case RTPC_TYPE_MAT4X4: {
                auto value = prop->GetValue<glm::mat4>();
                ImGui::Text(prop->GetName().c_str());
                ImGui::InputFloat4("m0", &value[0][0]);
                ImGui::InputFloat4("m1", &value[1][0]);
                ImGui::InputFloat4("m2", &value[2][0]);
                ImGui::InputFloat4("m3", &value[3][0]);
                break;
            }
            }
        }

        // draw children
        if (m_Containers.size() > 0) {
            auto d = ++depth;
            for (const auto& child : m_Containers) {
                child->DrawUI(d);
            }
        }

        ImGui::TreePop();
    }
}

void RuntimeContainer::ContextMenuUI(const fs::path& filename)
{
    if (ImGui::Selectable("Load models")) {
        DEBUG_LOG("loading...");

        auto rtpc = FileLoader::Get()->GetRuntimeContainer(filename);

        if (!rtpc) {
        }

        if (rtpc) {
            auto parts = rtpc->GetContainer("CParts");
            assert(parts);

            auto file = filename.parent_path().string();
            file = file.replace(file.find("editor\\entities"), strlen("editor\\entities"), "models");

            for (const auto& container : parts->GetContainers()) {
                auto child_name = container->GetProperty("name");
                if (child_name) {
                    auto filename = std::any_cast<std::string>(child_name->GetValue());
                    filename += "_lod1.rbm";

                    fs::path modelname = file;
                    modelname /= filename;

                    DEBUG_LOG(modelname);
                    RenderBlockModel::LoadModel(modelname);
                }

                for (const auto& child : container->GetContainers()) {
                    auto child_name = child->GetProperty("name");
                    auto filename = std::any_cast<std::string>(child_name->GetValue());
                    filename += "_lod1.rbm";

                    fs::path modelname = file;
                    modelname /= filename;

                    DEBUG_LOG(modelname);
                    RenderBlockModel::LoadModel(modelname);
                }
            }
        }
    }
}