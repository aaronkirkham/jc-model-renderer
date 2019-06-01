#pragma once

#include <unordered_map>

#include "../game/formats/avalanche_data_format.h"
#include "../singleton.h"

#pragma pack(push, 8)
struct SShader {
    uint32_t           m_NameHash;
    const char*        m_Name;
    uint32_t           m_DataHash;
    SAdfArray<uint8_t> m_BinaryData;
};

struct SShaderLibrary {
    const char*        m_Name;
    const char*        m_BuildTime;
    SAdfArray<SShader> m_VertexShaders;
    SAdfArray<SShader> m_FragmentShaders;
    SAdfArray<SShader> m_ComputeShaders;
    SAdfArray<SShader> m_GeometryShaders;
    SAdfArray<SShader> m_HullShaders;
    SAdfArray<SShader> m_DomainShaders;
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

    std::shared_ptr<AvalancheDataFormat> m_ShaderBundle  = nullptr;
    SShaderLibrary*                      m_ShaderLibrary = nullptr;

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
