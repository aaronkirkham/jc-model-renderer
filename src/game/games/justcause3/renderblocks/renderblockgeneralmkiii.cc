#include "pch.h"

#include "renderblockgeneralmkiii.h"

#include "app/app.h"

#include "game/format.h"
#include "game/game.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/texture.h"

#include <AvaFormatLib/util/byte_array_buffer.h>

namespace jcmr::game::justcause3
{
struct RenderBlockGeneralMkIIIImpl final : RenderBlockGeneralMkIII {
    static inline constexpr i32 VERSION = 5;

#pragma pack(push, 1)
    struct Attributes {
        float                 depth_bias;
        float                 hardware_depth_bias;
        float                 hardware_slope_bias;
        float                 min_time_of_day_emissive;
        float                 start_fade_out_distance_emissive_sq;
        float                 gi_emission_modulator;
        float                 ripple_angle;
        float                 ripple_speed;
        float                 ripple_magnitude;
        ava::SPackedAttribute packed;
        float                 wire_radius;
        u32                   flags;
        vec3                  bounding_box_min;
        vec3                  bounding_box_max;
        u32                   pristine_index_count;
    };

    static_assert(sizeof(Attributes) == 0x68, "RenderBlockGeneralMkIII::Attributes alignment is wrong!");
#pragma pack(pop)

  public:
    RenderBlockGeneralMkIIIImpl(App& app)
        : RenderBlockGeneralMkIII(app)
    {
    }

    void read(const ByteArray& buffer) override
    {
        //
        LOG_INFO("RenderBlockGeneralMkIII : read...");

        byte_array_buffer buf(buffer);
        std::istream      stream(&buf);

        // ensure version is correct
        u8 version;
        stream.read((char*)&version, sizeof(u8));
        ASSERT(version == VERSION);

        // read attributes
        stream.read((char*)&m_attributes, sizeof(Attributes));

        // read material constants
        stream.read((char*)&MaterialConsts2, sizeof(MaterialConsts2));

        // read textures
        read_textures(stream);

        // read buffers
        // TODO
    }

    void write(ByteArray* buffer) override
    {
        //
    }
    void draw_ui() override
    {
        //
        IMGUIEX_TABLE_COL("Depth Bias", ImGui::DragFloat("##depth_bias", &m_attributes.depth_bias));
        IMGUIEX_TABLE_COL("Hardware Depth Bias",
                          ImGui::DragFloat("##hardware_depth_bias", &m_attributes.hardware_depth_bias));
        IMGUIEX_TABLE_COL("Hardware Slope Bias",
                          ImGui::DragFloat("##hardware_slope_bias", &m_attributes.hardware_slope_bias));
        IMGUIEX_TABLE_COL("Min Time of Day Emissive",
                          ImGui::DragFloat("##min_time_of_day_emissive", &m_attributes.min_time_of_day_emissive));
        IMGUIEX_TABLE_COL("Start Fade Out Distance Emissive Sq",
                          ImGui::DragFloat("##start_fade_out_distance_emissive_sq",
                                           &m_attributes.start_fade_out_distance_emissive_sq));
        IMGUIEX_TABLE_COL("GI Emission Modulator",
                          ImGui::DragFloat("##gi_emission_modulator", &m_attributes.gi_emission_modulator));
        IMGUIEX_TABLE_COL("Ripple Angle", ImGui::DragFloat("##ripple_angle", &m_attributes.ripple_angle));
        IMGUIEX_TABLE_COL("Ripple Speed", ImGui::DragFloat("##ripple_speed", &m_attributes.ripple_speed));
        IMGUIEX_TABLE_COL("Ripple Magnitude", ImGui::DragFloat("##ripple_magnitude", &m_attributes.ripple_magnitude));
        // packed
        IMGUIEX_TABLE_COL("Wire Radius", ImGui::DragFloat("##wire_radius", &m_attributes.wire_radius));
        // pristine_index_count;

        static const char* texture_names[] = {
            "Albedo Map", "Gloss Map", "Metallic Map", "Normal Map",
            // (flags & USE_OVERLAY || flags & USE_LAYERED)
            "Layered Mask Map", "Layered Height Map",
            // (flags & USE_LAYERED)
            "Layered Albedo Map", "Layered Gloss Map", "Layered Metallic Map", "Layered Normal Map", "UNUSED",
            // (flags & USE_DECAL)
            "Decal Albedo Map", "Decal Gloss Map", "Decal Metallic Map", "Decal Normal Map",
            // (flags & USE_DAMAGE)
            "Damage Albedo Map", "Damage Gloss Map", "Damage Mask Map", "Damage Metallic Map", "Damage Normal Map"};

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

        return false;
    }

  private:
    struct {
        float NormalStrength; // [unused]
        float Reflectivity_1; // [unused]
        float Roughness_1;
        float DiffuseWrap_1;
        float Emissive_1;
        float Transmission_1;
        float ClearCoat_1;
        float Roughness_2;               // [unused]
        float DiffuseWrap_2;             // [unused]
        float Emissive_2;                // [unused]
        float Transmission_2;            // [unused]
        float Reflectivity_2;            // [unused]
        float ClearCoat_2;               // [unused]
        float Roughness_3;               // [unused]
        float DiffuseWrap_3;             // [unused]
        float Emissive_3;                // [unused]
        float Transmission_3;            // [unused]
        float Reflectivity_3;            // [unused]
        float ClearCoat_3;               // [unused]
        float Roughness_4;               // [unused]
        float DiffuseWrap_4;             // [unused]
        float Emissive_4;                // [unused]
        float Transmission_4;            // [unused]
        float Reflectivity_4;            // [unused]
        float ClearCoat_4;               // [unused]
        float LayeredHeightMapUVScale;   // [unused]
        float LayeredUVScale;            // [unused]
        float LayeredHeight1Influence;   // [unused]
        float LayeredHeight2Influence;   // [unused]
        float LayeredHeightMapInfluence; // [unused]
        float LayeredMaskInfluence;      // [unused]
        float LayeredShift;              // [unused]
        float LayeredRoughness;          // [unused]
        float LayeredDiffuseWrap;        // [unused]
        float LayeredEmissive;           // [unused]
        float LayeredTransmission;       // [unused]
        float LayeredReflectivity;       // [unused]
        float LayeredClearCoat;          // [unused]
        float DecalBlend;                // [unused]
        float DecalBlendNormal;          // [unused]
        float DecalReflectivity;         // [unused]
        float DecalRoughness;            // [unused]
        float DecalDiffuseWrap;          // [unused]
        float DecalEmissive;             // [unused]
        float DecalTransmission;         // [unused]
        float DecalClearCoat;            // [unused]
        float OverlayHeightInfluence;    // [unused]
        float OverlayHeightMapInfluence; // [unused]
        float OverlayMaskInfluence;      // [unused]
        float OverlayShift;              // [unused]
        float OverlayColorR;             // [unused]
        float OverlayColorG;             // [unused]
        float OverlayColorB;             // [unused]
        float OverlayBrightness;         // [unused]
        float OverlayGloss;              // [unused]
        float OverlayMetallic;           // [unused]
        float OverlayReflectivity;       // [unused]
        float OverlayRoughness;          // [unused]
        float OverlayDiffuseWrap;        // [unused]
        float OverlayEmissive;           // [unused]
        float OverlayTransmission;       // [unused]
        float OverlayClearCoat;          // [unused]
        float DamageReflectivity;        // [unused]
        float DamageRoughness;           // [unused]
        float DamageDiffuseWrap;         // [unused]
        float DamageEmissive;            // [unused]
        float DamageTransmission;        // [unused]
        float DamageHeightInfluence;     // [unused]
        float DamageMaskInfluence;       // [unused]
        float DamageClearCoat;           // [unused]
    } MaterialConsts2;

  private:
    Attributes m_attributes;
};

RenderBlockGeneralMkIII* RenderBlockGeneralMkIII::create(App& app)
{
    return new RenderBlockGeneralMkIIIImpl(app);
}

void RenderBlockGeneralMkIII::destroy(RenderBlockGeneralMkIII* instance)
{
    delete instance;
}
} // namespace jcmr::game::justcause3