#pragma once

#include <unordered_map>

#include "../game/formats/avalanche_data_format.h"
#include "../singleton.h"

struct VertexShader_t;
struct PixelShader_t;
class ShaderManager : public Singleton<ShaderManager>
{
  private:
    std::unordered_map<uint32_t, std::shared_ptr<VertexShader_t>> m_VertexShaders;
    std::unordered_map<uint32_t, std::shared_ptr<PixelShader_t>>  m_PixelShaders;

    std::unique_ptr<AvalancheDataFormat> m_ShaderBundle = nullptr;

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
