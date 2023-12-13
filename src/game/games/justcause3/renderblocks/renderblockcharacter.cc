#include "pch.h"

#include "renderblockcharacter.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/shader.h"
#include "render/texture.h"

#include <AvaFormatLib/util/byte_array_buffer.h>
#include <AvaFormatLib/util/byte_vector_stream.h>
#include <d3d11.h>

namespace jcmr::game::justcause3
{
struct RenderBlockCharacterImpl final : RenderBlockCharacter {
    static inline constexpr i32 VERSION         = 9;
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
        "character", "character2uvs", "character3uvs", "character8", "character82uvs", "character83uvs",
    };

#pragma pack(push, 1)
    struct Attributes {
        u32   flags;
        float packed_scale;
        vec2  detail_tiling_factor_uv_or_eye_gloss;
        vec2  blood_tiling_factor_uv_or_eye_reflect;
        float eye_gloss_shadow_intensity;
        float specular_gloss;
        float transmission;
        float specular_fresnel;
        float diffuse_roughness;
        float diffuse_wrap;
        float night_visibility;
        float dirt_factor;
        float emissive;
        vec4  anisotropic;
        vec4  camera;
        vec4  ring;
    };
#pragma pack(pop)

    static_assert(sizeof(Attributes) == 0x6C, "RenderBlockCharacter::Attributes alignment is wrong!");

    enum {
        DISABLE_BACKFACE_CULLING   = 0x1,
        EIGHT_BONES                = 0x2,
        ALPHA_MASK                 = 0x4,
        TRANSPARENCY_ALPHABLENDING = 0x8,
        USE_FEATURE_MAP            = 0x10,
        USE_WRINKLE_MAP            = 0x20,
        USE_CAMERA_LIGHTING        = 0x40,
        USE_EYE_REFLECTION         = 0x80,
    };

    enum CharacterAttributeFlags {
        GEAR      = 0x0,
        EYES      = 0x1000,
        HAIR      = 0x2000,
        BODY_PART = 0x3000,
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
    RenderBlockCharacterImpl(App& app)
        : RenderBlockCharacter(app)
    {
    }

    ~RenderBlockCharacterImpl()
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

            LOG_INFO("RenderBlockCharacter : skin_batch {}, size={}, offset={}, batch_size={}", i, batch.m_Size,
                     batch.m_Offset, batch.m_BatchSize);

            m_skin_batches.emplace_back(std::move(batch));
        }

        LOG_INFO("RenderBlockCharacter : read {} skin batches.", m_skin_batches.size());
    }

    void write_skin_batches(ByteArray* buffer)
    {
        ava::utils::ByteVectorStream stream(buffer);

        u32 count = m_skin_batches.size();
        stream.write(count);

        for (auto& batch : m_skin_batches) {
            stream.write(batch.m_Size);
            stream.write(batch.m_Offset);
            stream.write(batch.m_BatchSize);

            // write batch lookup table
            if (batch.m_BatchSize > 0) {
                stream.write((char*)batch.m_BatchLoopup, (sizeof(i16) * batch.m_BatchSize));
            }
        }
    }

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
        LOG_INFO("RenderBlockCharacter : read...");

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
        const auto stride = (3 * ((flags >> 1) & 1) + ((flags >> 4) & 1) + ((flags >> 5) & 1));
        LOG_INFO("vertex stride={}", VertexStrides[stride]);

        // read buffers
        m_vertex_buffer = read_buffer(stream, Buffer::BUFFER_TYPE_VERTEX, VertexStrides[stride]);
        read_skin_batches(stream);
        m_index_buffer = read_buffer(stream, Buffer::BUFFER_TYPE_INDEX, sizeof(u16));

#if 0
        LOG_INFO(" - flags={}", m_attributes.flags);
        LOG_INFO(" - packed_scale={}", m_attributes.packed_scale);
        LOG_INFO(" - detail_tiling_factor_uv_or_eye_gloss={}, {}", m_attributes.detail_tiling_factor_uv_or_eye_gloss.x,
                 m_attributes.detail_tiling_factor_uv_or_eye_gloss.y);
        LOG_INFO(" - decal_blend_factors_or_eye_reflect={}, {}", m_attributes.decal_blend_factors_or_eye_reflect.x,
                 m_attributes.decal_blend_factors_or_eye_reflect.y);
        LOG_INFO(" - eye_gloss_shadow_intensity={}", m_attributes.eye_gloss_shadow_intensity);
        LOG_INFO(" - specular_gloss={}", m_attributes.specular_gloss);
        LOG_INFO(" - transmission={}", m_attributes.transmission);
        LOG_INFO(" - specular_fresnel={}", m_attributes.specular_fresnel);
        LOG_INFO(" - diffuse_roughness={}", m_attributes.diffuse_roughness);
        LOG_INFO(" - diffuse_wrap={}", m_attributes.diffuse_wrap);
        LOG_INFO(" - night_visibility={}", m_attributes.night_visibility);
        LOG_INFO(" - dirt_factor={}", m_attributes.dirt_factor);
        LOG_INFO(" - emissive={}", m_attributes.emissive);
        LOG_INFO(" - anisotropic={}, {}, {}, {}", m_attributes.anisotropic.x, m_attributes.anisotropic.y,
                 m_attributes.anisotropic.z, m_attributes.anisotropic.w);
        LOG_INFO(" - camera={}, {}, {}, {}", m_attributes.camera.x, m_attributes.camera.y, m_attributes.camera.z,
                 m_attributes.camera.w);
        LOG_INFO(" - ring={}, {}, {}, {}", m_attributes.ring.x, m_attributes.ring.y, m_attributes.ring.z,
                 m_attributes.ring.w);
#endif

        create_shaders(flags, stride);
        create_shader_params();
    }

    void write(ByteArray* buffer) override
    {
        ava::utils::ByteVectorStream stream(buffer);

        // write version
        stream.write((char*)&VERSION, sizeof(u8));

        // write attributes
        stream.write(m_attributes);

        // write materials
        write_textures(buffer);

        // write buffers
        write_buffer(m_vertex_buffer, buffer);
        write_skin_batches(buffer);
        write_buffer(m_index_buffer, buffer);
    }

    void draw_ui() override
    {
        // TODO : Eye Gloss/reflect should be switched based on their flags.

        IMGUIEX_TABLE_COL("Scale", ImGui::DragFloat("##packed_scale", &m_attributes.packed_scale));
        IMGUIEX_TABLE_COL("Detail Tiling Factor UV",
                          ImGui::DragFloat2("##detail_tiling_factor_uv_or_eye_gloss",
                                            glm::value_ptr(m_attributes.detail_tiling_factor_uv_or_eye_gloss)));
        IMGUIEX_TABLE_COL("Blood Tiling Factor UV",
                          ImGui::DragFloat2("##decal_blend_factors_or_eye_reflect",
                                            glm::value_ptr(m_attributes.blood_tiling_factor_uv_or_eye_reflect)));
        IMGUIEX_TABLE_COL("Eye Gloss Shadow Intensity",
                          ImGui::DragFloat("##eye_gloss_shadow_intensity", &m_attributes.eye_gloss_shadow_intensity));
        IMGUIEX_TABLE_COL("Specular Gloss", ImGui::DragFloat("##specular_gloss", &m_attributes.specular_gloss));
        IMGUIEX_TABLE_COL("Transmission", ImGui::DragFloat("##transmission", &m_attributes.transmission));
        IMGUIEX_TABLE_COL("Specular Fresnel", ImGui::DragFloat("##specular_fresnel", &m_attributes.specular_fresnel));
        IMGUIEX_TABLE_COL("Diffuse Roughness",
                          ImGui::DragFloat("##diffuse_roughness", &m_attributes.diffuse_roughness));
        IMGUIEX_TABLE_COL("Diffuse Wrap", ImGui::DragFloat("##diffuse_wrap", &m_attributes.diffuse_wrap));
        IMGUIEX_TABLE_COL("Night Visibility", ImGui::DragFloat("##night_visibility", &m_attributes.night_visibility));
        IMGUIEX_TABLE_COL("Dirt Factor", ImGui::DragFloat("##dirt_factor", &m_attributes.dirt_factor));
        IMGUIEX_TABLE_COL("Emissive", ImGui::DragFloat("##emissive", &m_attributes.emissive));
        IMGUIEX_TABLE_COL("Diffuse Wrap", ImGui::DragFloat("##diffuse_wrap", &m_attributes.diffuse_wrap));

        // LOG_INFO(" - anisotropic={}, {}, {}, {}", m_attributes.anisotropic.x, m_attributes.anisotropic.y,
        //          m_attributes.anisotropic.z, m_attributes.anisotropic.w);
        // LOG_INFO(" - camera={}, {}, {}, {}", m_attributes.camera.x, m_attributes.camera.y, m_attributes.camera.z,
        //          m_attributes.camera.w);
        // LOG_INFO(" - ring={}, {}, {}, {}", m_attributes.ring.x, m_attributes.ring.y, m_attributes.ring.z,
        //          m_attributes.ring.w);

        static const char* texture_names[] = {
            "Diffuse Map", "Normal Map", "Properties Map", "Detail Diffuse Map", "Detail Normal Map", "Feature Map",
            "Wrinkle Map", "UNUSED",     "Camera Map",     "Metallic Map",       "Reflection Map",
        };

        for (u32 i = 0; i < m_textures.size(); ++i) {
            auto texture = m_textures[i];
            if (texture) {
                char buf[512] = {0};
                strcpy_s(buf, texture->get_filename().c_str());

                ImGui::PushID(texture_names[i]);
                IMGUIEX_TABLE_COL(texture_names[i],
                                  ImGuiEx::InputTexture("##tex", buf, lengthOf(buf), texture->get_srv()));
                ImGui::PopID();
            }
        }
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
        if ((m_attributes.flags & BODY_PART) == EYES) {
            cbMaterialConsts.DetailTilingFactorUVOrEyeGloss  = vec2(600, 1);
            cbMaterialConsts.BloodTilingFactorUVOrEyeReflect = vec2(1, 0);
        } else {
            cbMaterialConsts.DetailTilingFactorUVOrEyeGloss  = m_attributes.detail_tiling_factor_uv_or_eye_gloss;
            cbMaterialConsts.BloodTilingFactorUVOrEyeReflect = m_attributes.blood_tiling_factor_uv_or_eye_reflect;
        }
        cbMaterialConsts.EyeGlossShadowIntensity = m_attributes.eye_gloss_shadow_intensity;
        cbMaterialConsts.SpecularGloss           = m_attributes.specular_gloss;
        cbMaterialConsts.Transmission            = m_attributes.transmission;
        cbMaterialConsts.SpecularFresnel         = m_attributes.specular_fresnel;
        cbMaterialConsts.DiffuseWrap = ((m_attributes.flags & BODY_PART) == HAIR ? m_attributes.diffuse_wrap : 0.0f);
        cbMaterialConsts.DiffuseRoughness = m_attributes.diffuse_roughness;
        cbMaterialConsts.DirtFactor       = m_attributes.dirt_factor;
        cbMaterialConsts.Emissive         = m_attributes.emissive;
        context.renderer->set_pixel_shader_constants(m_pixel_shader_constants[1], 2, cbMaterialConsts);

        // set the culling mode
        context.renderer->set_cull_mode((!(m_attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                           : D3D11_CULL_NONE);

        // set blend mode
        if (m_attributes.flags & ALPHA_MASK) {
            // context.renderer->set_alpha_test_enabled(true);
            // context.renderer->set_blending_enabled(false);
        } else {
            // context.renderer->set_alpha_test_enabled(false);
        }

        // bind textures
        context.renderer->set_texture(m_textures[0].get(), 0, m_sampler);

        switch (m_attributes.flags & BODY_PART) {
            case GEAR: {
                context.renderer->set_texture(m_textures[1].get(), 1, m_sampler);
                context.renderer->set_texture(m_textures[2].get(), 2, m_sampler);
                context.renderer->set_texture(m_textures[3].get(), 3, m_sampler);
                context.renderer->set_texture(m_textures[4].get(), 4, m_sampler);
                context.renderer->set_texture(m_textures[9].get(), 9, m_sampler);

                if (m_attributes.flags & USE_FEATURE_MAP) {
                    context.renderer->set_texture(m_textures[5].get(), 5, m_sampler);
                } else {
                    context.renderer->set_texture(nullptr, 5, nullptr);
                }

                if (m_attributes.flags & USE_WRINKLE_MAP) {
                    context.renderer->set_texture(m_textures[7].get(), 6, m_sampler);
                } else {
                    context.renderer->set_texture(nullptr, 6, nullptr);
                }

                if (m_attributes.flags & USE_CAMERA_LIGHTING) {
                    context.renderer->set_texture(m_textures[8].get(), 8, m_sampler);
                } else {
                    context.renderer->set_texture(nullptr, 8, nullptr);
                }

                if (m_attributes.flags & TRANSPARENCY_ALPHABLENDING) {
                    // context.renderer->set_blending_enabled(true);
                    // context.renderer->set_blending_function(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE,
                    //                                         D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
                } else {
                    // context.renderer->set_blending_enabled(false);
                }

                break;
            }

            case EYES: {
                if (m_attributes.flags & USE_EYE_REFLECTION) {
                    context.renderer->set_texture(m_textures[10].get(), 11, m_sampler);
                } else {
                    context.renderer->set_texture(nullptr, 11, nullptr);
                }

                // context.renderer->set_blending_enabled(true);
                // context.renderer->set_blending_function(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA,
                //                                         D3D11_BLEND_ONE);
                break;
            }

            case HAIR: {
                context.renderer->set_texture(m_textures[1].get(), 1, m_sampler);
                context.renderer->set_texture(m_textures[2].get(), 2, m_sampler);

                if (m_attributes.flags & TRANSPARENCY_ALPHABLENDING) {
                    // context.renderer->set_blending_enabled(true);
                    // context.renderer->set_blending_function(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE,
                    //                                         D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
                } else {
                    // context.renderer->set_blending_enabled(false);
                }

                break;
            }
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
        m_pixel_shader = m_app.get_game().create_shader((flags & BODY_PART) != HAIR ? "character" : "characterhair_msk",
                                                        Shader::E_SHADER_TYPE_PIXEL);

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
        m_vertex_shader_constants   = renderer.create_constant_buffer(cbLocalConsts, "Character cbLocalConsts");
        m_pixel_shader_constants[0] = renderer.create_constant_buffer(cbInstanceConsts, "Character cbInstanceConsts");
        m_pixel_shader_constants[1] = renderer.create_constant_buffer(cbMaterialConsts, "Character cbMaterialConsts");

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

        m_sampler = renderer.create_sampler(desc, "Character Sampler");

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
        float _unknown              = 0.0f;
        float _unknown2             = 0.0f;
        float EnableHairDebug       = 0.0f;
        float EnableCameraHairLight = 0.0f;
        vec4  DiffuseColour         = vec4(0, 0, 0, 1);
        float _unknown3             = 0.0f;
        float _unknown4             = 0.0f;
        float _unknown5             = 0.0f;
        float SnowFactor            = 0.0f;
    } cbInstanceConsts;

    struct {
        vec2  DetailTilingFactorUVOrEyeGloss;
        vec2  BloodTilingFactorUVOrEyeReflect;
        float EyeGlossShadowIntensity;
        float SpecularGloss;
        float Transmission;
        float SpecularFresnel;
        float DiffuseWrap;
        float DiffuseRoughness;
        float DirtFactor;
        float Emissive;
        vec4  unknown[7];
    } cbMaterialConsts;

    static_assert(sizeof(cbLocalConsts) == 0xDB0);
    static_assert(sizeof(cbInstanceConsts) == 0x30);
    static_assert(sizeof(cbMaterialConsts) == 0xA0);

  private:
    Attributes              m_attributes;
    Buffer*                 m_vertex_shader_constants = nullptr;
    std::array<Buffer*, 2>  m_pixel_shader_constants  = {nullptr};
    std::vector<CSkinBatch> m_skin_batches;
};

RenderBlockCharacter* RenderBlockCharacter::create(App& app)
{
    return new RenderBlockCharacterImpl(app);
}

void RenderBlockCharacter::destroy(RenderBlockCharacter* instance)
{
    delete instance;
}
} // namespace jcmr::game::justcause3