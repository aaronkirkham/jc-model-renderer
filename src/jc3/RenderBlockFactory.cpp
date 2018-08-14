#include <jc3/RenderBlockFactory.h>
#include <jc3/renderblocks/RenderBlockGeneralJC3.h>
#include <jc3/renderblocks/RenderBlockCharacter.h>
#include <jc3/renderblocks/RenderBlockCharacterSkin.h>
#include <jc3/renderblocks/RenderBlockBuildingJC3.h>
#include <jc3/renderblocks/RenderBlockGeneralMkIII.h>
#include <jc3/renderblocks/RenderBlockCarPaintMM.h>
#include <jc3/renderblocks/RenderBlockWindow.h>

IRenderBlock* RenderBlockFactory::CreateRenderBlock(const uint32_t type)
{
    switch (type) {
    case RB_GENERALJC3: return new RenderBlockGeneralJC3;
    case RB_CHARACTER: return new RenderBlockCharacter;
    case RB_CHARACTERSKIN: return new RenderBlockCharacterSkin;
    case RB_BUILDINGJC3: return new RenderBlockBuildingJC3;
    case RB_GENERALMKIII: return new RenderBlockGeneralMkIII;
    case RB_CARPAINTMM: return new RenderBlockCarPaintMM;
    case RB_WINDOW: return new RenderBlockWindow;
    }
    
    return nullptr;
}

const char* RenderBlockFactory::GetRenderBlockName(const uint32_t type)
{
    switch (type) {
    case RB_2DTEX1: return "2DTex1";
    case RB_2DTEX2: return "2DTex2";
    case RB_3DTEXT: return "3DText";
    case RB_AOBOX: return "AOBox";
    case RB_ADDFOGVOLUME: return "AddFogVolume";
    case RB_ATMOSPHERICSCATTERING: return "AtmosphericScattering";
    case RB_BARK: return "Bark";
    case RB_BAVARIUMSHIELD: return "BavariumShield";
    case RB_BEAM: return "Beam";
    case RB_BILLBOARD: return "Billboard";
    case RB_BOX: return "Box";
    case RB_BUILDINGJC3: return "BuildingJC3";
    case RB_BULLET: return "Bullet";
    case RB_CARLIGHT: return "CarLight";
    case RB_CARPAINTMM: return "CarPaintMM";
    case RB_CHARACTER: return "Character";
    case RB_CHARACTERSKIN: return "CharacterSkin";
    case RB_CIRRUSCLOUDS: return "CirrusClouds";
    case RB_CLOUDS: return "Clouds";
    case RB_DECALDEFORMABLE: return "DecalDeformable";
    case RB_DECALSIMPLE: return "DecalSimple";
    case RB_DECALSKINNED: return "DecalSkinned";
    case RB_DECALSKINNEDDESTRUCTION: return "DecalSkinnedDestruction";
    case RB_DEFERREDLIGHTING: return "DeferredLighting";
    case RB_DEPTHDOWNSAMPLE: return "DepthDownSample";
    case RB_ENVIRONMENTREFLECTION: return "EnvironmentReflection";
    case RB_FXMESHFIRE: return "FXMeshFire";
    case RB_FLAG: return "Flag";
    case RB_FOGGRADIENT: return "FogGradient";
    case RB_FOGVOLUME: return "FogVolume";
    case RB_FOILAGE: return "Foilage";
    case RB_FONT: return "Font";
    case RB_FULLSCREENVIDEO: return "FullScreenVideo";
    case RB_GIONLY: return "GIOnly";
    case RB_GENERAL: return "General";
    case RB_GENERALJC3: return "GeneralJC3";
    case RB_GENERALMASKEDJC3: return "GeneralMaskedJC3";
    case RB_GENERALMKIII: return "GeneralMkIII";
    case RB_HALO: return "Halo";
    case RB_LRCLOUDSCOMPOSE: return "LRCloudsCompose";
    case RB_PARTICLECOMPOSE: return "LRParticleCompose";
    case RB_LAYERED: return "Layered";
    case RB_LIGHTGLOW: return "LightGlow";
    case RB_LIGHTS: return "Lights";
    case RB_LINE: return "Line";
    case RB_MATERIALTUNE: return "MaterialTune";
    case RB_MESHPARTICLE: return "MeshParticle";
    case RB_OCCLUDER: return "Occluder";
    case RB_OCCULSION: return "Occulsion";
    case RB_OPEN: return "Open";
    case RB_OUTLINEEFFECTBLUR: return "OutlineEffectBlur";
    case RB_POSTEFFECTS: return "PostEffects";
    case RB_PROP: return "Prop";
    case RB_RAINOCCLUDER: return "RainOccluder";
    case RB_REFLECTION: return "Reflection";
    case RB_ROADJUNCTION: return "RoadJunction";
    case RB_SSAO: return "SSAO";
    case RB_SSDECAL: return "SSDecal";
    case RB_SCENECAPTURE: return "SceneCapture";
    case RB_SCREENSPACEREFLECTION: return "ScreenSpaceReflection";
    case RB_SCREENSPACESUBSURFACESKIN: return "ScreenSpaceSubSurfaceSkin";
    case RB_SINGLE: return "Single";
    case RB_SKIDMARKS: return "Skidmarks";
    case RB_SKYBOX: return "SkyBox";
    case RB_SKYGRADIENT: return "SkyGradient";
    case RB_SKYMODEL: return "SkyModel";
    case RB_SOFTCLOUDS: return "SoftClouds";
    case RB_SPHERICALHARMONICPROBE: return "SphericalHarmonicProbe";
    case RB_SPLINEROAD: return "SplineRoad";
    case RB_SPOTLIGHTCONE: return "SpotLightCone";
    case RB_TERRAINPATCH: return "TerrainPatch";
    case RB_TRAIL: return "Trail";
    case RB_TRIANGLE: return "Triangle";
    case RB_UNDERWATERFOGGRADIENT: return "UnderwaterFogGradient";
    case RB_VEGINTRECENTER: return "VegIntReCenter";
    case RB_VEGINTERACTIONVOLUME: return "VegInteractionVolume";
    case RB_VEGSAMPLING: return "VegSampling";
    case RB_VEGSYSTEMPOSTDRAW: return "VegSystemPostDraw";
    case RB_VEGUPDATE: return "VegUpdate";
    case RB_WATERDISPLACEMENTOVERRIDE: return "WaterDisplacementOverride";
    case RB_WATERMARK: return "WaterMask";
    case RB_WATERREFLECTIONPLANE: return "WaterReflectionPlane";
    case RB_WEATHER: return "Weather";
    case RB_WINDOW: return "Window";
    }

    return "Unknown";
}