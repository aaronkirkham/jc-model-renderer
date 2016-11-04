#include <MainWindow.h>
#include <engine/renderblocks/RenderBlockGeneralJC3.h>
#include <engine/renderblocks/RenderBlockCharacter.h>
#include <engine/renderblocks/RenderBlockCarPaintMM.h>
#include <engine/renderblocks/RenderBlockBuildingJC3.h>
#include <engine/renderblocks/RenderBlockLandmark.h>

RenderBlockFactory::RenderBlockFactory()
{
    //0x45DBC85F - 2DTex1
    //0x8663465 - 2DTex2
    //0x474ECF38 - 3DText
    //0x67BCF1D0 - AOBox
    //0xCF84413C - AddFogVolume
    //0x90D7C43 - AtmosphericScattering
    //0x898323B2 - Bark
    //0xA5D24CCD - BavariumShield
    //0x1B5E8315 - Beam
    //0x389265A9 - Billboard
    //0x416C4035 - Box
    //0x91571CF0 - Bullet
    //0xDB948BF1 - CarLight
    //0x483304D6 - CarPaintMM
    //0x626F5E3B - CharacterSkin
    //0x3449988B - CirrusClouds
    //0xA399123E - Clouds
    //0xDF9E9916 - DecalDeformable
    //0x2BAFDD6C - DecalSimple
    //0x8A42C0C1 - DecalSkinned
    //0x4684BC - DecalSkinnedDestruction
    //0x4598D520 - DeferredLighting
    //0xF9EF9DE4 - DepthDownSample
    //0x7287FA5F - EnvironmentReflection
    //0x6FF3659 - FXMeshFire
    //0xD885FCDB - Flag
    //0x1637FB2A - FogGradient
    //0xB0E85383 - FogVolume
    //0x3C8DE6D3 - Foilage
    //0x7E8F98CE - Font
    //0xAE7E6231 - FullScreenVideo
    //0x91C2D583 - GIOnly
    //0xA7583B2B - General
    //0x8F2335EC - GeneralMaskedJC3
    //0x2CEC5AD5 - GeneralMkIII
    //0x65D9B5B2 - Halo
    //0xE13A427D - LRCloudsCompose
    //0x6B535F86 - LRParticleCompose
    //0xC7021EE3 - Layered
    //0xD2033C51 - LightGlow
    //0xDB48F55A - Lights
    //0xC07B6866 - Line
    //0xC2457E44 - MaterialTune
    //0xBAE64FF8 - MeshParticle
    //0x2A44553C - Occluder
    //0xBD231E4 - Occlusion
    //0x5B13AA49 - Open
    //0xDAF5CDB0 - OutlineEffectBlur
    //0x58DE8D87 - PostEffects
    //0x4894ECD - Prop
    //0x7450659E - RainOccluder
    //0xA981F55F - Reflection
    //0x566DCE92 - RoadJunction
    //0xC2C03635 - SSAO
    //0x4FE3AE77 - SSDecal
    //0x3EB17238 - SceneCapture
    //0xDA1EB637 - ScreenSpaceRelection
    //0xB65AC9D7 - ScreenSpaceSubSurfaceSkin
    //0x9D1EE307 - Single
    //0x4D3A7D2F - Skidmarks
    //0xC24EFB87 - SkyBox
    //0x64076188 - SkyGradient
    //0x2D95C25E - SkyModel
    //0xB308E2F4 - SoftClouds
    //0x359F6B2C - SphericalHarmonicProbe
    //0xEDABAD - SplineRoad
    //0x33E6BCA3 - SpotLightCone
    //0x815DF732 - TerrainPatch
    //0x858E7014 - Trail
    //0xEB96E782 - Triangle
    //0x69DE065B - UnderwaterFogGradient
    //0xD89EF9 - VegIntReCenter
    //0x6D87DBFC - VegInteractionVolume
    //0x77B4C335 - VegSampling
    //0x20DA8E9D - VegSystemPostDraw
    //0x1AF1F8AC - VegUpdate
    //0xF99C72A1 - WaterDisplacementOverride
    //0x90FE086C - WaterMask
    //0x910EDC80 - WaterReflectionPlane
    //0x89215D85 - Weather
    //0x5B2003F6 - Window
}

IRenderBlock* RenderBlockFactory::GetRenderBlock(uint32_t typeHash)
{
    if (typeHash == 0x9D6E332A)
        return new RenderBlockCharacter;
    else if (typeHash == 0x2EE0F4A9)
        return new RenderBlockGeneralJC3;
    else if (typeHash == 0x483304D6)
        return new RenderBlockCarPaintMM;
    else if (typeHash == 0x35BF53D5)
        return new RenderBlockBuildingJC3;
    else if (typeHash == 0x3B630E6D)
        return new RenderBlockLandmark;

    return nullptr;
}
