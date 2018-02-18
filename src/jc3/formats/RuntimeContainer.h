#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <any>

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
    uint32_t m_NameHash = 0x0;
    PropertyType m_Type = RTPC_TYPE_UNASSIGNED;
    std::any m_Value;

public:
    RuntimeContainerProperty(uint32_t name_hash, uint8_t type);
    virtual ~RuntimeContainerProperty() = default;

    void SetValue(const std::any& anything) { m_Value = anything; }
    const std::any& GetValue() { return m_Value; }

    uint32_t GetNameHash() const { return m_NameHash; }
    PropertyType GetType() const { return m_Type; }

#if 0
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
#endif
};

class RuntimeContainer
{
private:
    uint32_t m_NameHash = 0x0;
    std::vector<RuntimeContainerProperty*> m_Properties;
    std::vector<RuntimeContainer*> m_Containers;

public:
    RuntimeContainer(uint32_t name_hash);
    virtual ~RuntimeContainer();

    void AddProperty(RuntimeContainerProperty* prop) { m_Properties.emplace_back(prop); };
    void AddContainer(RuntimeContainer* cont) { m_Containers.emplace_back(cont); };

    uint32_t GetNameHash() const { return m_NameHash; }

    RuntimeContainerProperty* GetProperty(uint32_t name_hash);
    RuntimeContainer* GetContainer(uint32_t name_hash);

    const std::vector<RuntimeContainerProperty*>& GetProperties() { return m_Properties; }
    const std::vector<RuntimeContainer*>& GetContainers() { return m_Containers; }
};