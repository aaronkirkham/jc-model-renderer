#pragma once

#include <StdInc.h>
#include <any>
#include <Factory.h>

enum PropertyType : uint8_t
{
    RTPC_TYPE_UNASSIGNED = 0,
    RTPC_TYPE_INTEGER,
    RTPC_TYPE_FLOAT,
    RTPC_TYPE_STRING,
    RTPC_TYPE_VEC2,
    RTPC_TYPE_VEC3,
    RTPC_TYPE_VEC4,
    RTPC_TYPE_MAT4X4 = 8,
    RTPC_TYPE_LIST_INTEGERS,
    RTPC_TYPE_LIST_FLOATS,
    RTPC_TYPE_LIST_BYTES,
    RTPC_TYPE_OBJECT_ID = 13,
    RTPC_TYPE_EVENTS,
    RTPC_TYPE_TOTAL,
};

class RuntimeContainerProperty
{
private:
    std::string m_Name = "";
    uint32_t m_NameHash = 0x0;
    PropertyType m_Type = RTPC_TYPE_UNASSIGNED;
    std::any m_Value;

public:
    RuntimeContainerProperty(uint32_t name_hash, uint8_t type);
    virtual ~RuntimeContainerProperty() = default;

    void SetValue(const std::any& anything) { m_Value = anything; }
    const std::any& GetValue() { return m_Value; }

    template <typename T>
    T GetValue() const { return std::any_cast<T>(m_Value); }

    const std::string& GetName() const { return m_Name; }
    uint32_t GetNameHash() const { return m_NameHash; }
    PropertyType GetType() const { return m_Type; }

    static std::string GetTypeName(uint8_t type) {
        switch (type) {
        case RTPC_TYPE_UNASSIGNED: return "unassigned";
        case RTPC_TYPE_INTEGER: return "integer";
        case RTPC_TYPE_FLOAT: return "float";
        case RTPC_TYPE_STRING: return "string";
        case RTPC_TYPE_VEC2: return "vec2";
        case RTPC_TYPE_VEC3: return "vec3";
        case RTPC_TYPE_VEC4: return "vec4";
        case RTPC_TYPE_MAT4X4: return "mat4x4";
        case RTPC_TYPE_LIST_INTEGERS: return "vec_int";
        case RTPC_TYPE_LIST_FLOATS: return "vec_float";
        case RTPC_TYPE_LIST_BYTES: return "vec_byte";
        case RTPC_TYPE_OBJECT_ID: return "object_id";
        case RTPC_TYPE_EVENTS: return "vec_events";
        }

        return "unknown";
    }
};

class RuntimeContainer : public Factory<RuntimeContainer>
{
private:
    fs::path m_Filename = "";
    std::string m_Name = "";
    uint32_t m_NameHash = 0x0;
    std::vector<RuntimeContainerProperty*> m_Properties;
    std::vector<RuntimeContainer*> m_Containers;

public:
    RuntimeContainer(uint32_t name_hash, const fs::path& filename = "");
    virtual ~RuntimeContainer();

    static void Load(const fs::path& filename, std::function<void(std::shared_ptr<RuntimeContainer>)> callback);

    virtual std::string GetFactoryKey() const { return m_Filename.string(); }

    void GenerateBetterNames();

    void AddProperty(RuntimeContainerProperty* prop) { m_Properties.emplace_back(prop); }
    void AddContainer(RuntimeContainer* cont) { m_Containers.emplace_back(cont); };

    static void ReadFileCallback(const fs::path& filename, const FileBuffer& data);
    void DrawUI(uint8_t depth = 0);
    static void ContextMenuUI(const fs::path& filename);

    void SetFileName(const fs::path& filename) { m_Filename = filename; }
    const fs::path& GetFileName() const { return m_Filename; }
    const std::string& GetName() const { return m_Name; }
    uint32_t GetNameHash() const { return m_NameHash; }

    RuntimeContainerProperty* GetProperty(uint32_t name_hash, bool include_children = true);
    RuntimeContainerProperty* GetProperty(const std::string& name, bool include_children = true);

    RuntimeContainer* GetContainer(uint32_t name_hash, bool include_children = true);
    RuntimeContainer* GetContainer(const std::string& name, bool include_children = true);

    std::vector<RuntimeContainer*> GetAllContainers(const std::string& class_name);

    const std::vector<RuntimeContainerProperty*>& GetProperties() { return m_Properties; }
    std::vector<RuntimeContainerProperty*> GetSortedProperties();
    const std::vector<RuntimeContainer*>& GetContainers() { return m_Containers; }
};