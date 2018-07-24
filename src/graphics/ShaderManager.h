#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>

class ShaderManager : public Singleton<ShaderManager>
{
private:
    std::unordered_map<uint32_t, std::shared_ptr<VertexShader_t>> m_VertexShaders;
    std::unordered_map<uint32_t, std::shared_ptr<PixelShader_t>> m_PixelShaders;

public:
    ShaderManager() = default;
    virtual ~ShaderManager() = default;

    void Shutdown();

    std::shared_ptr<VertexShader_t> GetVertexShader(const std::string& name);
    std::shared_ptr<PixelShader_t> GetPixelShader(const std::string& name);
};