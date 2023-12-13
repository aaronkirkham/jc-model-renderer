#include "pch.h"

#include "renderblockcharacterskin.h"

#include "app/app.h"

#include "game/format.h"
#include "game/game.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/shader.h"
#include "render/texture.h"

#include <AvaFormatLib/util/byte_array_buffer.h>
#include <AvaFormatLib/util/byte_vector_stream.h>
#include <d3d11.h>

namespace jcmr::game::justcause3
{
struct RenderBlockCharacterSkinImpl final : RenderBlockCharacterSkin {
    static inline constexpr i32 VERSION         = 6;
    static inline constexpr i32 VertexStrides[] = {0x18, 0x1C, 0x20, 0x20, 0x24, 0x28};

    // vertex strides
    static inline constexpr u8 Packed4Bones1UV  = 0;
    static inline constexpr u8 Packed4Bones2UVs = 1;
    static inline constexpr u8 Packed4Bones3UVs = 2;
    static inline constexpr u8 Packed8Bones1UV  = 3;
    static inline constexpr u8 Packed8Bones2UVs = 4;
    static inline constexpr u8 Packed8Bones3UVs = 5;

    // vertex shaders for each given stride
    static inline const char* VertexShaders[] = {
        "characterskin",  "characterskin2uvs",  "characterskin3uvs",
        "characterskin8", "characterskin82uvs", "characterskin83uvs",
    };

#pragma pack(push, 1)
    struct Attributes {
        uint32_t flags;
        float    packed_scale;
        vec2     detail_tiling_factor_uv_or_eye_gloss;
        vec2     blood_tiling_factor_uv_or_eye_reflect;
        float    diffuse_roughness;
        float    diffuse_wrap;
        float    dirt_factor;
        vec4     camera;
        float    translucency_scale;
    };
#pragma pack(pop)

    static_assert(sizeof(Attributes) == 0x38, "Attributes alignment is wrong!");

    enum {
        DISABLE_BACKFACE_CULLING = 0x1,
        USE_WRINKLE_MAP          = 0x2,
        EIGHT_BONES              = 0x4,
        USE_FEATURE_MAP          = 0x10,
        ALPHA_MASK               = 0x20,
    };

    // TODO : justcause3 common stuff, maybe ava-format-lib RenderBlockModel helpers?
#pragma pack(push, 1)
    struct CSkinBatch {
        i16* m_BatchLoopup;
        i32  m_BatchSize;
        i32  m_Size;
        i32  m_Offset;
        char _alignment[4];
    };

    struct CDeformTable {
        u16 m_Table[256];
        u8  m_Size;
    };

    static_assert(sizeof(CSkinBatch) == 0x18, "CSkinBatch alignment is wrong!");
    static_assert(sizeof(CDeformTable) == 0x201, "CDeformTable alignment is wrong!");
#pragma pack(pop)

  public:
    RenderBlockCharacterSkinImpl(App& app)
        : RenderBlockCharacterSkin(app)
    {
    }

    ~RenderBlockCharacterSkinImpl()
    {
        //
        destroy_skin_batches();

        m_app.get_renderer().destroy_buffer(m_vertex_shader_constants);
        m_app.get_renderer().destroy_buffer(m_pixel_shader_constants[0]);
        m_app.get_renderer().destroy_buffer(m_pixel_shader_constants[1]);
    }

    void read_skin_batches(std::istream& stream)
    {
        u32 count;
        stream.read((char*)&count, sizeof(u32));
        m_skin_batches.reserve(count);

        for (u32 i = 0; i < count; ++i) {
            CSkinBatch batch{};
            stream.read((char*)&batch.m_Size, sizeof(batch.m_Size));
            stream.read((char*)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.read((char*)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            // read batch lookup table
            if (batch.m_BatchSize > 0) {
                batch.m_BatchLoopup = new i16[batch.m_BatchSize];
                stream.read((char*)batch.m_BatchLoopup, (sizeof(i16) * batch.m_BatchSize));
            }

            LOG_INFO("RenderBlockCharacterSkin : skin_batch {}, size={}, offset={}, batch_size={}", i, batch.m_Size,
                     batch.m_Offset, batch.m_BatchSize);

            m_skin_batches.emplace_back(std::move(batch));
        }

        LOG_INFO("RenderBlockCharacterSkin : read {} skin batches.", m_skin_batches.size());
    }

    // void write_skin_batches(ByteArray* buffer)
    // {
    //     ava::utils::ByteVectorStream stream(buffer);

    //     u32 count = m_skin_batches.size();
    //     stream.write(count);

    //     for (auto& batch : m_skin_batches) {
    //         stream.write(batch.m_Size);
    //         stream.write(batch.m_Offset);
    //         stream.write(batch.m_BatchSize);

    //         // write batch lookup table
    //         if (batch.m_BatchSize > 0) {
    //             stream.write((char*)batch.m_BatchLoopup, (sizeof(i16) * batch.m_BatchSize));
    //         }
    //     }
    // }

    void destroy_skin_batches()
    {
        for (auto& batch : m_skin_batches) {
            if (batch.m_BatchLoopup) {
                delete[] batch.m_BatchLoopup;
            }
        }
    }

    void read(const ByteArray& buffer) override
    {
        //
        LOG_INFO("RenderBlockCharacterSkin : read...");

        byte_array_buffer buf(buffer);
        std::istream      stream(&buf);

        // ensure version is correct
        u8 version;
        stream.read((char*)&version, sizeof(u8));
        ASSERT(version == VERSION);

        // read attributes
        stream.read((char*)&m_attributes, sizeof(Attributes));

        // read textures
        read_textures(stream);

        // get the vertex stride
        const auto flags  = m_attributes.flags;
        const auto stride = (3 * ((flags >> 2) & 1) + ((flags >> 1) & 1) + ((flags >> 4) & 1));
        LOG_INFO("vertex stride={}", VertexStrides[stride]);

        // read buffers
        m_vertex_buffer = read_buffer(stream, Buffer::BUFFER_TYPE_VERTEX, VertexStrides[stride]);
        read_skin_batches(stream);
        m_index_buffer = read_buffer(stream, Buffer::BUFFER_TYPE_INDEX, sizeof(u16));

        create_shaders(flags, stride);
        create_shader_params();
    }

    void write(ByteArray* buffer) override
    {
        //
    }

    void draw_ui() override
    {
        //
        IMGUIEX_TABLE_COL("CharacterSkin UI :-)", ImGui::Text("..."));
    }

    bool setup(RenderContext& context) override
    {
        if (!IRenderBlock::setup(context)) {
            return false;
        }

        static mat4 world(1);

        // update vertex shader constants
        cbLocalConsts.World               = world;
        cbLocalConsts.WorldViewProjection = (world * context.view_proj_matrix);
        cbLocalConsts.Scale               = vec4(m_attributes.packed_scale);
        context.renderer->set_vertex_shader_constants(m_vertex_shader_constants, 1, cbLocalConsts);

        // update pixel shader constants (instance)
        context.renderer->set_pixel_shader_constants(m_pixel_shader_constants[0], 1, cbInstanceConsts);

        // update pixel shader constants (material)
        cbMaterialConsts.DetailTilingFactorUVOrEyeGloss  = m_attributes.detail_tiling_factor_uv_or_eye_gloss;
        cbMaterialConsts.BloodTilingFactorUVOrEyeReflect = m_attributes.blood_tiling_factor_uv_or_eye_reflect;
        context.renderer->set_pixel_shader_constants(m_pixel_shader_constants[1], 2, cbMaterialConsts);

        // set the culling mode
        context.renderer->set_cull_mode((!(m_attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                           : D3D11_CULL_NONE);

        // set blend mode
        // if (flags & ALPHA_MASK) {
        //     context->m_Renderer->SetBlendingEnabled(false);
        //     context->m_Renderer->SetAlphaTestEnabled(false);
        // }

        // context->m_Renderer->SetBlendingEnabled(true);
        // context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA,
        //                                      D3D11_BLEND_ONE);

        // bind textures
        context.renderer->set_texture(m_textures[0].get(), 0, m_sampler);
        context.renderer->set_texture(m_textures[1].get(), 1, m_sampler);
        context.renderer->set_texture(m_textures[2].get(), 2, m_sampler);
        context.renderer->set_texture(m_textures[3].get(), 3, m_sampler);
        context.renderer->set_texture(m_textures[4].get(), 4, m_sampler);

        if (m_attributes.flags & USE_FEATURE_MAP) {
            context.renderer->set_texture(m_textures[7].get(), 5, m_sampler);
        } else {
            context.renderer->set_texture(nullptr, 5, nullptr);
        }

        if (m_attributes.flags & USE_WRINKLE_MAP) {
            context.renderer->set_texture(m_textures[8].get(), 6, m_sampler);
        } else {
            context.renderer->set_texture(nullptr, 6, nullptr);
        }

        return true;
    }

    void draw(RenderContext& context) override
    {
        for (const auto& batch : m_skin_batches) {
            context.renderer->draw_indexed(m_index_buffer, batch.m_Offset, batch.m_Size);
        }
    }

  private:
    void create_shaders(u32 flags, u32 stride)
    {
        m_vertex_shader = m_app.get_game().create_shader(VertexShaders[stride], Shader::E_SHADER_TYPE_VERTEX);
        m_pixel_shader  = m_app.get_game().create_shader("characterskin", Shader::E_SHADER_TYPE_PIXEL);

        if (stride == Packed4Bones1UV) {
            m_vertex_shader->set_vertex_layout({
                {Shader::E_USAGE_POSITION, 0, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 1, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 4, DXGI_FORMAT_R16G16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 6, DXGI_FORMAT_R8G8B8A8_UNORM},
            });
        } else if (stride == Packed4Bones2UVs) {
            m_vertex_shader->set_vertex_layout({
                {Shader::E_USAGE_POSITION, 0, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 1, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 4, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 6, DXGI_FORMAT_R8G8B8A8_UNORM},
            });
        } else if (stride == Packed4Bones3UVs) {
            m_vertex_shader->set_vertex_layout({
                {Shader::E_USAGE_POSITION, 0, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 1, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 4, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 5, DXGI_FORMAT_R16G16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 6, DXGI_FORMAT_R8G8B8A8_UNORM},
            });
        } else if (stride == Packed8Bones1UV) {
            m_vertex_shader->set_vertex_layout({
                {Shader::E_USAGE_POSITION, 0, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 1, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 2, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 3, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 4, DXGI_FORMAT_R16G16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 6, DXGI_FORMAT_R8G8B8A8_UNORM},
            });
        } else if (stride == Packed8Bones2UVs) {
            m_vertex_shader->set_vertex_layout({
                {Shader::E_USAGE_POSITION, 0, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 1, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 2, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 3, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 4, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 6, DXGI_FORMAT_R8G8B8A8_UNORM},
            });
        } else if (stride == Packed8Bones3UVs) {
            m_vertex_shader->set_vertex_layout({
                {Shader::E_USAGE_POSITION, 0, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 1, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 2, DXGI_FORMAT_R8G8B8A8_UNORM},
                {Shader::E_USAGE_TEXCOORD, 3, DXGI_FORMAT_R8G8B8A8_UINT},
                {Shader::E_USAGE_TEXCOORD, 4, DXGI_FORMAT_R16G16B16A16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 5, DXGI_FORMAT_R16G16_SNORM},
                {Shader::E_USAGE_TEXCOORD, 6, DXGI_FORMAT_R8G8B8A8_UNORM},
            });
        }
    }

    void create_shader_params()
    {
        auto& renderer = m_app.get_renderer();

        // reset constants
        std::memset(&cbMaterialConsts, 0, sizeof(cbMaterialConsts));
        for (u32 i = 0; i < lengthOf(cbLocalConsts.MatrixPalette); ++i) {
            cbLocalConsts.MatrixPalette[i] = mat3x4(1);
        }

        // create constant buffers
        m_vertex_shader_constants = renderer.create_constant_buffer(cbLocalConsts, "CharacterSkin cbLocalConsts");
        m_pixel_shader_constants[0] =
            renderer.create_constant_buffer(cbInstanceConsts, "CharacterSkin cbInstanceConsts");
        m_pixel_shader_constants[1] =
            renderer.create_constant_buffer(cbMaterialConsts, "CharacterSkin cbMaterialConsts");

        // create sampler state
        D3D11_SAMPLER_DESC desc{};
        desc.Filter         = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias     = 0.0f;
        desc.MaxAnisotropy  = 8;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD         = 0.0f;
        desc.MaxLOD         = 13.0f;

        m_sampler = renderer.create_sampler(desc, "CharacterSkin Sampler");

        // TODO : normal map sampler state too!
    }

  private:
    struct {
        mat4   World;
        mat4   WorldViewProjection;
        vec4   Scale;
        mat3x4 MatrixPalette[70];
    } cbLocalConsts;

    struct {
        float _unknown  = 0.0f;
        float _unknown2 = 0.0f;
        float _unknown3 = 0.0f;
        float _unknown4 = 0.0f;
    } cbInstanceConsts;

    struct {
        vec2 DetailTilingFactorUVOrEyeGloss;
        vec2 BloodTilingFactorUVOrEyeReflect;
        vec4 _unknown_;
        vec4 _unknown[3];
    } cbMaterialConsts;

    static_assert(sizeof(cbLocalConsts) == 0xDB0);
    static_assert(sizeof(cbInstanceConsts) == 0x10);
    static_assert(sizeof(cbMaterialConsts) == 0x50);

  private:
    Attributes              m_attributes;
    Buffer*                 m_vertex_shader_constants = nullptr;
    std::array<Buffer*, 2>  m_pixel_shader_constants  = {nullptr};
    std::vector<CSkinBatch> m_skin_batches;
};

RenderBlockCharacterSkin* RenderBlockCharacterSkin::create(App& app)
{
    return new RenderBlockCharacterSkinImpl(app);
}

void RenderBlockCharacterSkin::destroy(RenderBlockCharacterSkin* instance)
{
    delete instance;
}
} // namespace jcmr::game::justcause3