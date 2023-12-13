#include "pch.h"

#include "shader.h"

#include "render/renderer.h"

#include <d3d11.h>

namespace jcmr
{
struct ShaderImpl final : Shader {
    ShaderImpl(const std::string& filename, ShaderType shader_type, Renderer& renderer)
        : m_filename(filename)
        , m_shader_type(shader_type)
        , m_renderer(renderer)
    {
    }

    ~ShaderImpl()
    {
        if (m_input_layout) m_input_layout->Release();

        // TODO : we should only store a single pointer, maybe in std::any?
        //        don't like the idea of managing 2 separate classes for Vertex/Pixel shaders.
        //        and if we support other shader types in the future, we'll have a bunch of pointers!
        if (m_vertex_shader) m_vertex_shader->Release();
        if (m_pixel_shader) m_pixel_shader->Release();
    }

    bool load(const ByteArray& buffer) override
    {
        auto& context = m_renderer.get_context();

        // vertex shader
        if (m_shader_type == E_SHADER_TYPE_VERTEX) {
            auto hr = context.device->CreateVertexShader(buffer.data(), buffer.size(), nullptr, &m_vertex_shader);
            if (FAILED(hr)) {
                LOG_ERROR("Shader : failed to load vertex shader \"{}\"", m_filename);
                return false;
            }

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(m_vertex_shader, m_filename.c_str());
#endif

        }
        // pixel shader
        else if (m_shader_type == E_SHADER_TYPE_PIXEL) {
            auto hr = context.device->CreatePixelShader(buffer.data(), buffer.size(), nullptr, &m_pixel_shader);
            if (FAILED(hr)) {
                LOG_ERROR("Shader : failed to load pixel shader \"{}\"", m_filename);
                return false;
            }

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(m_pixel_shader, m_filename.c_str());
#endif
        }

        m_buffer = buffer;
        return true;
    }

    void set_vertex_layout(std::initializer_list<VertexLayoutDesc> layout_desc, const char* debug_name) override
    {
        ASSERT(m_shader_type == E_SHADER_TYPE_VERTEX);
        std::vector<D3D11_INPUT_ELEMENT_DESC> descs;
        descs.reserve(layout_desc.size());

        // VertexLayoutDesc to D3D11_INPUT_ELEMENT_DESC
        for (auto desc : layout_desc) {
            D3D11_INPUT_ELEMENT_DESC _desc{};
            _desc.SemanticName         = VertexLayoutDesc::usage_strings[desc.usage];
            _desc.SemanticIndex        = desc.index;
            _desc.Format               = desc.format;
            _desc.InputSlot            = desc.slot;
            _desc.AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
            _desc.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
            _desc.InstanceDataStepRate = 0;
            descs.emplace_back(std::move(_desc));
        }

        auto& context = m_renderer.get_context();
        auto  hr      = context.device->CreateInputLayout(descs.data(), descs.size(), m_buffer.data(), m_buffer.size(),
                                                          &m_input_layout);
        ASSERT(SUCCEEDED(hr));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        if (debug_name) D3D_SET_OBJECT_NAME_A(m_input_layout, debug_name);
#endif
    }

    ID3D11VertexShader* get_vertex_shader() override { return m_vertex_shader; }
    ID3D11PixelShader*  get_pixel_shader() override { return m_pixel_shader; }
    ID3D11InputLayout*  get_input_layout() override { return m_input_layout; }

  private:
    Renderer&           m_renderer;
    std::string         m_filename;
    ShaderType          m_shader_type;
    ByteArray           m_buffer;
    ID3D11VertexShader* m_vertex_shader = nullptr;
    ID3D11PixelShader*  m_pixel_shader  = nullptr;
    ID3D11InputLayout*  m_input_layout  = nullptr;
};

Shader* Shader::create(const std::string& filename, ShaderType shader_type, Renderer& renderer)
{
    return new ShaderImpl(filename, shader_type, renderer);
}

void Shader::destroy(Shader* instance)
{
    delete instance;
}
} // namespace jcmr