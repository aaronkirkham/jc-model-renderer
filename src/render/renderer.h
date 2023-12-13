#ifndef JCMR_RENDER_RENDERER_H_HEADER_GUARD
#define JCMR_RENDER_RENDERER_H_HEADER_GUARD

#include "app/os.h"

#include <dxgiformat.h>

struct ID3D11Buffer;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct ID3D11SamplerState;
struct DDS_PIXELFORMAT;
struct D3D11_SAMPLER_DESC;
enum D3D11_BLEND : int;
enum D3D11_BLEND_OP : int;
enum D3D11_FILL_MODE : int;
enum D3D11_CULL_MODE : int;

namespace jcmr
{
struct App;
struct Renderer;
struct Texture;
struct Shader;
struct UI;
struct Camera;

namespace game
{
    struct IRenderBlock;
}

struct RenderContext {
    float                dt             = 0.0f;
    Renderer*            renderer       = nullptr;
    ID3D11Device*        device         = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    mat4                 view_matrix;
    mat4                 proj_matrix;
    mat4                 view_proj_matrix;
    bool                 blend_state_is_dirty      = true;
    bool                 rasterizer_state_is_dirty = true;
    bool                 alpha_blend_enabled       = true;
    bool                 alpha_test_enabled        = true;
    D3D11_BLEND          blend_source_colour       = (D3D11_BLEND)5;     // D3D11_BLEND_SRC_ALPHA;
    D3D11_BLEND          blend_source_alpha        = (D3D11_BLEND)2;     // D3D11_BLEND_ONE;
    D3D11_BLEND          blend_dest_colour         = (D3D11_BLEND)6;     // D3D11_BLEND_INV_SRC_ALPHA;
    D3D11_BLEND          blend_dest_alpha          = (D3D11_BLEND)1;     // D3D11_BLEND_ZERO;
    D3D11_BLEND_OP       blend_colour_eq           = (D3D11_BLEND_OP)1;  // D3D11_BLEND_OP_ADD;
    D3D11_BLEND_OP       blend_alpha_eq            = (D3D11_BLEND_OP)1;  // D3D11_BLEND_OP_ADD;
    D3D11_CULL_MODE      cull_mode                 = (D3D11_CULL_MODE)3; // D3D11_CULL_BACK;
    D3D11_FILL_MODE      fill_mode                 = (D3D11_FILL_MODE)3; // D3D11_FILL_SOLID;
};

using Sampler = ID3D11SamplerState;

struct Buffer {
    enum Type {
        BUFFER_TYPE_VERTEX = 1, // D3D11_BIND_VERTEX_BUFFER
        BUFFER_TYPE_INDEX  = 2, // D3D11_BIND_INDEX_BUFFER
    };

    // TODO : don't copy??
    template <typename T> std::vector<T> as()
    {
        return {reinterpret_cast<T*>(data.data()), reinterpret_cast<T*>(data.data() + data.size())};
    };

    ID3D11Buffer* ptr = nullptr;
    ByteArray     data;
    u32           count  = 0;
    u32           stride = 0;
};

struct Renderer {
    struct InitRendererArgs {
        os::WindowHandle window_handle;
        u32              width;
        u32              height;
    };

    static Renderer* create(App& app);
    static void      destroy(Renderer* renderer);

    static DDS_PIXELFORMAT get_pixel_format(DXGI_FORMAT dxgi_format);

    virtual ~Renderer() = default;

    virtual bool init(const InitRendererArgs& init_renderer_args) = 0;
    virtual void shutdown()                                       = 0;

    virtual void resize(u32 width, u32 height) = 0;

    virtual std::shared_ptr<Texture> create_texture(const std::string& filename, const ByteArray& buffer) = 0;
    virtual std::shared_ptr<Texture> get_texture(const std::string& filename)                             = 0;

    virtual std::shared_ptr<Shader> create_shader(const std::string& filename, u8 shader_type,
                                                  const ByteArray& buffer)  = 0;
    virtual std::shared_ptr<Shader> get_shader(const std::string& filename) = 0;

    virtual ID3D11Texture2D* create_texture(u32 width, u32 height, u32 format, u32 bind_flags,
                                            const char* debug_name = "") = 0;

    virtual Sampler* create_sampler(const D3D11_SAMPLER_DESC& desc, const char* debug_name) = 0;

    virtual Buffer* create_buffer(const void* data, u32 count, u32 stride, u32 type,
                                  const char* debug_name = nullptr)                                           = 0;
    virtual Buffer* create_constant_buffer(const void* data, u32 vec4count, const char* debug_name = nullptr) = 0;
    virtual bool    map_buffer(Buffer* buffer, const void* data, u64 size)                                    = 0;
    virtual void    destroy_buffer(Buffer* buffer)                                                            = 0;

    virtual void set_vertex_shader_constants(Buffer* buffer, u32 slot, const void* data, u64 size) = 0;
    virtual void set_pixel_shader_constants(Buffer* buffer, u32 slot, const void* data, u64 size)  = 0;

    template <typename T> Buffer* create_constant_buffer(T& data, const char* debug_name = nullptr)
    {
        return create_constant_buffer(&data, (sizeof(T) / 16), debug_name);
    }

    template <typename T> void set_vertex_shader_constants(Buffer* buffer, u32 slot, T& data)
    {
        return set_vertex_shader_constants(buffer, slot, &data, sizeof(T));
    }

    template <typename T> void set_pixel_shader_constants(Buffer* buffer, u32 slot, T& data)
    {
        return set_pixel_shader_constants(buffer, slot, &data, sizeof(T));
    }

    virtual void set_vertex_stream(Buffer* buffer, i32 slot, u32 offset = 0)         = 0;
    virtual void set_texture(Texture* texture, i32 slot, Sampler* sampler = nullptr) = 0;
    virtual void set_sampler(Sampler* sampler, i32 slot)                             = 0;
    virtual void set_depth_enabled(bool state)                                       = 0;

    virtual void set_cull_mode(D3D11_CULL_MODE mode) = 0;
    virtual void set_fill_mode(D3D11_FILL_MODE mode) = 0;

    virtual void draw(u32 offset, u32 count)                              = 0;
    virtual void draw_indexed(u32 start_index, u32 count)                 = 0;
    virtual void draw_indexed(Buffer* buffer, u32 start_index = 0)        = 0;
    virtual void draw_indexed(Buffer* buffer, u32 start_index, u32 count) = 0;

    virtual void add_to_renderlist(const std::vector<game::IRenderBlock*>& render_blocks)      = 0;
    virtual void remove_from_renderlist(const std::vector<game::IRenderBlock*>& render_blocks) = 0;

    virtual void frame() = 0;

    virtual const RenderContext& get_context() const = 0;
    virtual UI&                  get_ui()            = 0;
    virtual Camera&              get_camera()        = 0;
};
} // namespace jcmr

#endif // JCMR_RENDER_RENDERER_H_HEADER_GUARD
