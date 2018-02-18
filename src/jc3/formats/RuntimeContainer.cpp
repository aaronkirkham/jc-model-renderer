#include <jc3/formats/RuntimeContainer.h>
#include <Window.h>

RuntimeContainerProperty::RuntimeContainerProperty(uint32_t name_hash, uint8_t type)
    : m_NameHash(name_hash),
    m_Type(static_cast<PropertyType>(type))
{
    //DEBUG_LOG("RuntimeContainerProperty::RuntimeContainerProperty");
}

RuntimeContainer::RuntimeContainer(uint32_t name_hash)
    : m_NameHash(name_hash)
{
    //DEBUG_LOG("RuntimeContainer::RuntimeContainer");
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

RuntimeContainerProperty* RuntimeContainer::GetProperty(uint32_t name_hash)
{
    // find the property
    for (auto& prop : m_Properties) {
        if (prop->GetNameHash() == name_hash) {
            return prop;
        }

        // try find the property in child containers
        for (auto& container : m_Containers) {
            auto child_prop = container->GetProperty(name_hash);
            if (child_prop) {
                return child_prop;
            }
        }
    }

    return nullptr;
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