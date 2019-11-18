#pragma once

#include "factory.h"

#include <any>
#include <filesystem>
#include <fstream>

#include "../vendor/ava-format-lib/include/runtime_property_container.h"

namespace RTPC
{
enum VariantFlags : uint32_t {
    E_VARIANT_IS_HASH = 0x1,
    E_VARIANT_IS_GUID = 0x2,
};

class Variant : public ava::RuntimePropertyContainer::RtpcContainerVariant
{
  public:
    std::string m_Name;
    std::any    m_Value;
    uint32_t    m_Flags = 0;

  public:
    Variant(const ava::RuntimePropertyContainer::RtpcContainerVariant& variant);
    virtual ~Variant() = default;

    template <typename T> T Value() const
    {
        return std::any_cast<T>(m_Value);
    }
};

class Container : public ava::RuntimePropertyContainer::RtpcContainer
{
  public:
    std::string                             m_Name;
    std::vector<std::unique_ptr<Container>> m_Containers;
    std::vector<std::unique_ptr<Variant>>   m_Variants;

  public:
    Container(const ava::RuntimePropertyContainer::RtpcContainer& container);
    virtual ~Container() = default;

    void UpdateDisplayName();

    Container* GetContainer(const uint32_t name_hash, bool recursive = true);
    Variant*   GetVariant(const uint32_t name_hash, bool recursive = true);
};
}; // namespace RTPC

class RuntimeContainer : public Factory<RuntimeContainer>
{
    using ParseCallback_t = std::function<void(bool success)>;
    using LoadCallback_t  = std::function<void(bool success, std::shared_ptr<RuntimeContainer> container)>;

  private:
    std::filesystem::path            m_Filename = "";
    std::vector<uint8_t>             m_Buffer;
    std::unique_ptr<RTPC::Container> m_Root;

  public:
    RuntimeContainer(const std::filesystem::path& filename)
        : m_Filename(filename)
    {
    }
    virtual ~RuntimeContainer() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    void Parse(const std::vector<uint8_t>& buffer, ParseCallback_t callback = nullptr);

    static void ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                 bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);

    static void Load(const std::filesystem::path& filename, LoadCallback_t callback);

    void DrawUI(RTPC::Container* container = nullptr, int32_t index = 0, uint8_t depth = 0);

    RTPC::Container* GetContainer(const uint32_t name_hash, bool recursive = true)
    {
        assert(m_Root);
        return m_Root->GetContainer(name_hash, recursive);
    }

    RTPC::Variant* GetVariant(const uint32_t name_hash, bool recursive = true)
    {
        assert(m_Root);
        return m_Root->GetVariant(name_hash, recursive);
    }

    const std::filesystem::path& GetFilePath()
    {
        return m_Filename;
    }
};
