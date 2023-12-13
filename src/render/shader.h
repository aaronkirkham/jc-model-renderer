#ifndef JCMR_RENDER_SHADER_H_HEADER_GUARD
#define JCMR_RENDER_SHADER_H_HEADER_GUARD

#include "platform.h"

#include <dxgiformat.h>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

namespace jcmr
{
struct Renderer;

struct Shader {
    enum ShaderType : u8 {
        E_SHADER_TYPE_VERTEX = 0,
        E_SHADER_TYPE_PIXEL  = 1,
    };

    static inline const char* shader_extensions[] = {
        ".vb",
        ".fb",
    };

    enum UsageLayout : u8 {
        E_USAGE_POSITION = 0,
        E_USAGE_TEXCOORD = 1,
    };

    struct VertexLayoutDesc {
        static inline const char* usage_strings[] = {
            "POSITION",
            "TEXCOORD",
        };

        UsageLayout usage;
        u32         index;
        DXGI_FORMAT format;
        u32         slot = 0;
    };

    static Shader* create(const std::string& filename, ShaderType shader_type, Renderer& renderer);
    static void    destroy(Shader* instance);

    virtual ~Shader() = default;

    virtual bool load(const ByteArray& buffer) = 0;

    virtual void set_vertex_layout(std::initializer_list<VertexLayoutDesc> layout_desc,
                                   const char*                             debug_name = nullptr) = 0;

    virtual ID3D11VertexShader* get_vertex_shader() = 0;
    virtual ID3D11PixelShader*  get_pixel_shader()  = 0;
    virtual ID3D11InputLayout*  get_input_layout()  = 0;
};
} // namespace jcmr

#endif // JCMR_RENDER_SHADER_H_HEADER_GUARD
