#include "pch.h"

#include "justcause3.h"

#include "app/app.h"
#include "app/directory_list.h"
#include "app/profile.h"
#include "app/settings.h"
#include "app/utils.h"

#include "game/games/justcause3/renderblocks/renderblockcharacter.h"
#include "game/games/justcause3/renderblocks/renderblockcharacterskin.h"
#include "game/games/justcause3/renderblocks/renderblockgeneralmkiii.h"
#include "game/render_block.h"
#include "game/resource_manager.h"

#include "render/dds_texture_loader.h"
#include "render/renderer.h"
#include "render/shader.h"
#include "render/texture.h"

#include <AvaFormatLib/legacy/string_lookup.h>
#include <AvaFormatLib/util/byte_vector_stream.h>

namespace jcmr::game
{
static constexpr i32 INTERNAL_RESOURCE_JUSTCAUSE3_DICTIONARY = 128;

struct JustCause3Impl final : JustCause3 {
    JustCause3Impl(App& app)
        : m_app(app)
    {
        m_directory = m_app.get_settings().get<const char*>("jc3_path");

        m_resource_manager = ResourceManager::create(m_app);
        m_resource_manager->set_base_path(m_directory);
        m_resource_manager->set_flags(ResourceManager::E_FLAG_LEGACY_ARCHIVE_TABLE);
        m_resource_manager->load_dictionary(INTERNAL_RESOURCE_JUSTCAUSE3_DICTIONARY);

        load_shader_bundle();
        init_shader_constants();
    }

    ~JustCause3Impl()
    {
        // destroy shader constants
        m_app.get_renderer().destroy_buffer(m_vertex_global_constants);
        m_app.get_renderer().destroy_buffer(m_pixel_global_constants);

        // free shader bundle
        if (m_shader_library) {
            std::free(m_shader_library);
            delete m_shader_adf;
        }

        ResourceManager::destroy(m_resource_manager);
    }

    void load_shader_bundle()
    {
        ByteArray buffer;
        if (!m_resource_manager->read_from_disk("Shaders_F.shader_bundle", &buffer)) {
            DEBUG_BREAK();
            return;
        }

        AVA_FL_ENSURE(ava::ShaderBundle::Parse(buffer, &m_shader_adf, &m_shader_library));
        LOG_INFO("JustCause3 : shader bundle \"{}\" ({}) loaded!", m_shader_library->m_Name,
                 m_shader_library->m_BuildTime);
    }

    void init_shader_constants()
    {
        auto& renderer = m_app.get_renderer();

        m_vertex_global_constants = renderer.create_constant_buffer(VertexGlobalConstants, "VertexGlobalConstants");
        memset(&VertexGlobalConstants, 0, sizeof(VertexGlobalConstants));

        m_pixel_global_constants = renderer.create_constant_buffer(FragmentGlobalConstants, "FragmentGlobalConstants");
        memset(&FragmentGlobalConstants, 0, sizeof(FragmentGlobalConstants));
    }

    IRenderBlock* create_render_block(u32 typehash) override
    {
        using namespace jcmr::game::justcause3;
        using namespace ava::RenderBlockModel;

        switch (typehash) {
            case RB_CHARACTER: return RenderBlockCharacter::create(m_app);
            case RB_CHARACTERSKIN: return RenderBlockCharacterSkin::create(m_app);
            case RB_GENERALMKIII: return RenderBlockGeneralMkIII::create(m_app);
        }

        return nullptr;
    }

    std::shared_ptr<Texture> create_texture(const std::string& filename) override
    {
        // use cached texture if it's available
        auto& renderer = m_app.get_renderer();
        if (auto texture = renderer.get_texture(filename); texture) {
            return texture;
        }

        ByteArray buffer;
        if (!m_resource_manager->read(filename, &buffer)) {
            LOG_ERROR("JustCause3 : create_texture - failed to load texture \"{}\"", filename);
            return nullptr;
        }

        // attempt to read source file
        auto      source_filename = utils::replace(filename, ".ddsc", ".hmddsc");
        ByteArray source_buffer;
        if (m_resource_manager->read(source_filename, &source_buffer)) {
            LOG_INFO("JustCause3 : create_texture - loaded source texture \"{}\"", source_filename);
        }

        ava::AvalancheTexture::TextureEntry entry{};
        ByteArray                           texture_buffer;
        AVA_FL_ENSURE(ava::AvalancheTexture::ReadBestEntry(buffer, &entry, &texture_buffer, source_buffer), nullptr);

        LOG_INFO("JustCause3 : create_texture - using best texture {}x{}", entry.m_Width, entry.m_Height);

        DDS_HEADER header{};
        header.size        = sizeof(DDS_HEADER);
        header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
        header.width       = entry.m_Width;
        header.height      = entry.m_Height;
        header.depth       = entry.m_Depth;
        header.mipMapCount = 1;
        header.ddspf       = Renderer::get_pixel_format((DXGI_FORMAT)entry.m_Format);
        header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

        const bool is_dxt10 = (header.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));

        ByteArray dds_buffer(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + (is_dxt10 ? sizeof(DDS_HEADER_DXT10) : 0)
                             + texture_buffer.size());
        ava::utils::ByteVectorStream stream(&dds_buffer);
        stream.setp(0);

        stream.write(DDS_MAGIC);
        stream.write(header);

        if (is_dxt10) {
            DDS_HEADER_DXT10 dxt10_header{};
            dxt10_header.dxgiFormat        = (DXGI_FORMAT)entry.m_Format;
            dxt10_header.resourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
            dxt10_header.arraySize         = 1;
            stream.write(dxt10_header);
        }

        stream.write(texture_buffer);
        return renderer.create_texture(filename, std::move(dds_buffer));
    }

    // TODO : this should be moved into IGame probably. There're no differences between JC3/JC4.
    //        if we support any other avalanche game which happens to use a different ShaderBundle struct,
    //        we can just override on a game-specific level?
    std::shared_ptr<Shader> create_shader(const std::string& filename, u8 shader_type) override
    {
        // use cached shader if it's available
        auto& renderer = m_app.get_renderer();
        // FIXME : this won't work because we append the extension to the filename.
        if (auto shader = renderer.get_shader(filename); shader) {
            return shader;
        }

        ASSERT(m_shader_adf != nullptr);
        ASSERT(m_shader_library != nullptr);

        // use the correct shader bundle for the shader type
        ava::SAdfArray<ava::ShaderBundle::SShader>* shaders = nullptr;
        if (shader_type == Shader::E_SHADER_TYPE_VERTEX) {
            shaders = &m_shader_library->m_VertexShaders;
        } else if (shader_type == Shader::E_SHADER_TYPE_PIXEL) {
            shaders = &m_shader_library->m_FragmentShaders;
        }

        auto iter =
            std::find_if(shaders->begin(), shaders->end(),
                         [filename](const ava::ShaderBundle::SShader& shader) { return shader.m_Name == filename; });
        if (iter == shaders->end()) {
            LOG_ERROR("JustCause3 : can't find shader \"{}\"", filename);
            return nullptr;
        }

        ByteArray buffer((*iter).m_BinaryData.begin(), (*iter).m_BinaryData.end());
        return renderer.create_shader(filename, shader_type, std::move(buffer));
    }

    void setup_render_constants(RenderContext& context) override
    {
        auto& renderer = m_app.get_renderer();

        VertexGlobalConstants.ViewProjectionMatrix  = context.view_proj_matrix;
        VertexGlobalConstants.LightDiffuseColour    = vec4(1, 0, 0, 1);
        VertexGlobalConstants.LightDirection        = vec4(-1, -1, -1, 0);
        VertexGlobalConstants.ViewProjectionMatrix2 = context.view_proj_matrix;
        VertexGlobalConstants.ViewProjectionMatrix3 = context.view_proj_matrix;

        FragmentGlobalConstants.LightSpecularColour = glm::vec4(0, 1, 0, 1);
        FragmentGlobalConstants.LightDiffuseColour  = VertexGlobalConstants.LightDiffuseColour;
        FragmentGlobalConstants.LightDirection      = VertexGlobalConstants.LightDirection;
        FragmentGlobalConstants.LightDirection2     = VertexGlobalConstants.LightDirection;

        renderer.set_vertex_shader_constants(m_vertex_global_constants, 0, VertexGlobalConstants);
        renderer.set_pixel_shader_constants(m_pixel_global_constants, 0, FragmentGlobalConstants);
    }

    const std::filesystem::path& get_directory() const override { return m_directory; }
    ResourceManager*             get_resource_manager() override { return m_resource_manager; }

  private:
    struct {
        mat4 ViewProjectionMatrix;  // 0 - 4
        vec4 CameraPosition;        // 4 - 5
        vec4 _unknown[2];           // 5 - 7
        vec4 LightDiffuseColour;    // 7 - 8
        vec4 LightDirection;        // 8 - 9
        vec4 _unknown2[20];         // 9 - 29
        mat4 ViewProjectionMatrix2; // 29 - 33
        mat4 ViewProjectionMatrix3; // 33 - 37
        vec4 _unknown3[12];         // 37 - 49
    } VertexGlobalConstants;

    struct {
        glm::vec4 _unknown;            // 0 - 1
        glm::vec4 LightSpecularColour; // 1 - 2
        glm::vec4 LightDiffuseColour;  // 2 - 3
        glm::vec4 LightDirection;      // 3 - 4
        glm::vec4 _unknown2;           // 4 - 5
        glm::vec4 CameraPosition;      // 5 - 6
        glm::vec4 _unknown3[25];       // 6 - 31
        glm::vec4 LightDirection2;     // 31 - 32
        glm::vec4 _unknown4[63];       // 32 - 95
    } FragmentGlobalConstants;

    // struct {
    //     vec4  PointLights[768];
    //     vec4  SpotLights[1024];
    //     vec4  Projections[512];
    //     vec4  SpotLightShadowFades[8];
    //     mat4  WorldToLPVTransform;
    //     mat4  WorldToLPVUnsnappedTransform;
    //     mat4  WorldToLPVTransform2;
    //     mat4  WorldToLPVUnsnappedTransform2;
    //     vec4  GroundFillFlux_1234;
    //     vec4  GroundFillFlux_5678;
    //     vec4  GroundFillFlux_9_Color;
    //     vec3  AOLightInfluence;
    //     float SkyAmbientSaturation;
    //     float NearCascadeDampeningFactor;
    //     float EmissiveScale;
    //     int   bEnableGlobalIllumination;
    //     int   padding0;
    // } LightingFrameConsts;

    // static_assert(sizeof(LightingFrameConsts) == 37328);

  private:
    App&                               m_app;
    std::filesystem::path              m_directory;
    ResourceManager*                   m_resource_manager        = nullptr;
    ava::AvalancheDataFormat::ADF*     m_shader_adf              = nullptr;
    ava::ShaderBundle::SShaderLibrary* m_shader_library          = nullptr;
    Buffer*                            m_vertex_global_constants = nullptr;
    Buffer*                            m_pixel_global_constants  = nullptr;
};

JustCause3* JustCause3::create(App& app)
{
    return new JustCause3Impl(app);
}

void JustCause3::destroy(JustCause3* inst)
{
    delete inst;
}
} // namespace jcmr::game