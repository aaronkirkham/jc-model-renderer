#pragma once

#include <unordered_map>

#include "singleton.h"

#include "../vendor/ava-format-lib/include/avalanche_data_format.h"

#pragma pack(push, 8)
struct SShader {
    uint32_t                                     m_NameHash;
    const char*                                  m_Name;
    uint32_t                                     m_DataHash;
    ava::AvalancheDataFormat::SAdfArray<uint8_t> m_BinaryData;
};

struct SShaderLibrary {
    const char*                                  m_Name;
    const char*                                  m_BuildTime;
    ava::AvalancheDataFormat::SAdfArray<SShader> m_VertexShaders;
    ava::AvalancheDataFormat::SAdfArray<SShader> m_FragmentShaders;
    ava::AvalancheDataFormat::SAdfArray<SShader> m_ComputeShaders;
    ava::AvalancheDataFormat::SAdfArray<SShader> m_GeometryShaders;
    ava::AvalancheDataFormat::SAdfArray<SShader> m_HullShaders;
    ava::AvalancheDataFormat::SAdfArray<SShader> m_DomainShaders;
};

static_assert(sizeof(SShader) == 0x28, "SShader alignment is wrong!");
static_assert(sizeof(SShaderLibrary) == 0x70, "SShaderLibrary alignment is wrong!");
#pragma pack(pop)

struct VertexShader_t;
struct PixelShader_t;
class ShaderManager : public Singleton<ShaderManager>
{
  private:
    std::unordered_map<uint32_t, std::shared_ptr<VertexShader_t>> m_VertexShaders;
    std::unordered_map<uint32_t, std::shared_ptr<PixelShader_t>>  m_PixelShaders;

    std::shared_ptr<ava::AvalancheDataFormat::ADF> m_ShaderBundle  = nullptr;
    SShaderLibrary*                                m_ShaderLibrary = nullptr;

  public:
    ShaderManager()          = default;
    virtual ~ShaderManager() = default;

    void Init();
    void Shutdown();
    void Empty();

    std::shared_ptr<VertexShader_t> GetVertexShader(const std::string& name);
    std::shared_ptr<PixelShader_t>  GetPixelShader(const std::string& name);

    std::size_t GetCacheSize()
    {
        return m_VertexShaders.size() + m_PixelShaders.size();
    }
};
