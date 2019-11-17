#pragma once

#include "factory.h"

#include <any>
#include <filesystem>
#include <fstream>

#include "../vendor/ava-format-lib/include/runtime_property_container.h"

class RuntimeContainerProperty
{
  private:
    std::string                                 m_Name     = "";
    uint32_t                                    m_NameHash = 0x0;
    ava::RuntimePropertyContainer::EVariantType m_Type     = ava::RuntimePropertyContainer::T_VARIANT_UNASSIGNED;
    std::any                                    m_Value;

  public:
    RuntimeContainerProperty(uint32_t name_hash, ava::RuntimePropertyContainer::EVariantType type);
    virtual ~RuntimeContainerProperty() = default;

    void SetValue(const std::any& anything)
    {
        m_Value = anything;
    }

    const std::any& GetValue()
    {
        return m_Value;
    }

    template <typename T> T GetValue() const
    {
        return std::any_cast<T>(m_Value);
    }

    const std::string& GetName() const
    {
        return m_Name;
    }

    uint32_t GetNameHash() const
    {
        return m_NameHash;
    }

    ava::RuntimePropertyContainer::EVariantType GetType() const
    {
        return m_Type;
    }

    static std::string GetTypeName(ava::RuntimePropertyContainer::EVariantType type);
};

class RuntimeContainer : public Factory<RuntimeContainer>
{
  private:
    std::filesystem::path                  m_Filename = "";
    std::string                            m_Name     = "";
    uint32_t                               m_NameHash = 0x0;
    std::vector<RuntimeContainerProperty*> m_Properties;
    std::vector<RuntimeContainer*>         m_Containers;

  public:
    RuntimeContainer(uint32_t name_hash, const std::filesystem::path& filename = "");
    virtual ~RuntimeContainer();

    static void Load(const std::filesystem::path&                           filename,
                     std::function<void(std::shared_ptr<RuntimeContainer>)> callback);

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    void GenerateBetterNames();

    static void ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                 bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);

    void        DrawUI(int32_t index = 0, uint8_t depth = 0);
    static void ContextMenuUI(const std::filesystem::path& filename);

    void AddProperty(RuntimeContainerProperty* prop)
    {
        m_Properties.emplace_back(prop);
    }

    void AddContainer(RuntimeContainer* cont)
    {
        m_Containers.emplace_back(cont);
    }

    void SetFileName(const std::filesystem::path& filename)
    {
        m_Filename = filename;
    }

    const std::filesystem::path& GetFileName() const
    {
        return m_Filename;
    }

    const std::string& GetName() const
    {
        return m_Name;
    }

    uint32_t GetNameHash() const
    {
        return m_NameHash;
    }

    RuntimeContainerProperty* GetProperty(uint32_t name_hash, bool include_children = true);
    RuntimeContainerProperty* GetProperty(const std::string& name, bool include_children = true);

    RuntimeContainer* GetContainer(uint32_t name_hash, bool include_children = true);
    RuntimeContainer* GetContainer(const std::string& name, bool include_children = true);

    std::vector<RuntimeContainer*> GetAllContainers(const std::string& class_name);

    const std::vector<RuntimeContainerProperty*>& GetProperties()
    {
        return m_Properties;
    }

    std::vector<RuntimeContainerProperty*> GetSortedProperties();

    const std::vector<RuntimeContainer*>& GetContainers()
    {
        return m_Containers;
    }
};
