#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>
#include <shaders/compiled/shaders.hpp>

#define GET_VERTEX_SHADER(shader) ShaderManager::Get()->GetVertexShader(#shader, hlsl_##shader::vs_main, sizeof(hlsl_##shader::vs_main));
#define GET_PIXEL_SHADER(shader) ShaderManager::Get()->GetPixelShader(#shader, hlsl_##shader::ps_main, sizeof(hlsl_##shader::ps_main));

class ShaderManager : public Singleton<ShaderManager>
{
private:
    std::unordered_map<uint32_t, std::shared_ptr<VertexShader_t>> m_VertexShaders;
    std::unordered_map<uint32_t, std::shared_ptr<PixelShader_t>> m_PixelShaders;

public:
    ShaderManager() = default;
    virtual ~ShaderManager() = default;

    std::shared_ptr<VertexShader_t> GetVertexShader(const std::string& file);
    std::shared_ptr<VertexShader_t> GetVertexShader(const std::string& name, const void* buffer, uint64_t size);

    std::shared_ptr<PixelShader_t> GetPixelShader(const std::string& file);
    std::shared_ptr<PixelShader_t> GetPixelShader(const std::string& name, const void* buffer, uint64_t size);
};