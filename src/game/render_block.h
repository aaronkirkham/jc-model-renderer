#ifndef JCMR_GAMES_RENDER_BLOCK_H_HEADER_GUARD
#define JCMR_GAMES_RENDER_BLOCK_H_HEADER_GUARD

#include "platform.h"

struct ID3D11SamplerState;

namespace jcmr
{
struct App;
struct ResourceManager;
struct Texture;
struct Shader;
struct RenderContext;
struct Buffer;

using Sampler = ID3D11SamplerState;

namespace game
{
    struct IRenderBlock {
        IRenderBlock(App& app)
            : m_app(app)
        {
        }

        virtual ~IRenderBlock();

        void set_resource_manager(ResourceManager* resource_manager) { m_resource_manager = resource_manager; }

        virtual const char* get_type_name() const = 0;
        virtual u32         get_type_hash() const = 0;

        virtual void draw_ui() {}

        virtual bool setup(RenderContext& context);
        virtual void draw(RenderContext& context);

        // virtual bool bind_texture(i32 index, i32 slot) = 0;

      protected:
        App&                                  m_app;
        ResourceManager*                      m_resource_manager = nullptr;
        std::shared_ptr<Shader>               m_vertex_shader;
        std::shared_ptr<Shader>               m_pixel_shader;
        std::vector<std::shared_ptr<Texture>> m_textures;
        float    m_material_params[4] = {0}; // [0] = density, [1] = unknown, [2] = unknown, [3] = unknown
        Buffer*  m_vertex_buffer      = nullptr;
        Buffer*  m_index_buffer       = nullptr;
        Sampler* m_sampler            = nullptr;
    };
} // namespace game
} // namespace jcmr

#endif // JCMR_GAMES_RENDER_BLOCK_H_HEADER_GUARD
