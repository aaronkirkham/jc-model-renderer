#pragma once

#include <cstdint>

class IRenderBlock;
class RenderBlockFactory
{
  public:
    RenderBlockFactory()          = default;
    virtual ~RenderBlockFactory() = default;

    static IRenderBlock* CreateRenderBlock(const uint32_t type);
    static const char*   GetRenderBlockName(const uint32_t type);

    // render block type hashes
    static constexpr auto RB_2DTEX1                    = 0x45DBC85F;
    static constexpr auto RB_2DTEX2                    = 0x8663465;
    static constexpr auto RB_3DTEXT                    = 0x474ECF38;
    static constexpr auto RB_AOBOX                     = 0x67BCF1D0;
    static constexpr auto RB_ADDFOGVOLUME              = 0xCF84413C;
    static constexpr auto RB_ATMOSPHERICSCATTERING     = 0x90D7C43;
    static constexpr auto RB_BARK                      = 0x898323B2;
    static constexpr auto RB_BAVARIUMSHIELD            = 0xA5D24CCD;
    static constexpr auto RB_BEAM                      = 0x1B5E8315;
    static constexpr auto RB_BILLBOARD                 = 0x389265A9;
    static constexpr auto RB_BOX                       = 0x416C4035;
    static constexpr auto RB_BUILDINGJC3               = 0x35BF53D5;
    static constexpr auto RB_BULLET                    = 0x91571CF0;
    static constexpr auto RB_CARLIGHT                  = 0xDB948BF1;
    static constexpr auto RB_CARPAINTMM                = 0x483304D6;
    static constexpr auto RB_CHARACTER                 = 0x9D6E332A;
    static constexpr auto RB_CHARACTERSKIN             = 0x626F5E3B;
    static constexpr auto RB_CIRRUSCLOUDS              = 0x3449988B;
    static constexpr auto RB_CLOUDS                    = 0xA399123E;
    static constexpr auto RB_DECALDEFORMABLE           = 0xDF9E9916;
    static constexpr auto RB_DECALSIMPLE               = 0x2BAFDD6C;
    static constexpr auto RB_DECALSKINNED              = 0x8A42C0C1;
    static constexpr auto RB_DECALSKINNEDDESTRUCTION   = 0x4684BC;
    static constexpr auto RB_DEFERREDLIGHTING          = 0x4598D520;
    static constexpr auto RB_DEPTHDOWNSAMPLE           = 0xF9EF9DE4;
    static constexpr auto RB_ENVIRONMENTREFLECTION     = 0x7287FA5F;
    static constexpr auto RB_FXMESHFIRE                = 0x6FF3659;
    static constexpr auto RB_FLAG                      = 0xD885FCDB;
    static constexpr auto RB_FOGGRADIENT               = 0x1637FB2A;
    static constexpr auto RB_FOGVOLUME                 = 0xB0E85383;
    static constexpr auto RB_FOILAGE                   = 0x3C8DE6D3;
    static constexpr auto RB_FONT                      = 0x7E8F98CE;
    static constexpr auto RB_FULLSCREENVIDEO           = 0xAE7E6231;
    static constexpr auto RB_GIONLY                    = 0x91C2D583;
    static constexpr auto RB_GENERAL                   = 0xA7583B2B;
    static constexpr auto RB_GENERALJC3                = 0x2EE0F4A9;
    static constexpr auto RB_GENERALMASKEDJC3          = 0x8F2335EC;
    static constexpr auto RB_GENERALMKIII              = 0x2CEC5AD5;
    static constexpr auto RB_HALO                      = 0x65D9B5B2;
    static constexpr auto RB_LANDMARK                  = 0x3B630E6D;
    static constexpr auto RB_LAYERED                   = 0xC7021EE3;
    static constexpr auto RB_LIGHTGLOW                 = 0xD2033C51;
    static constexpr auto RB_LIGHTS                    = 0xDB48F55A;
    static constexpr auto RB_LINE                      = 0xC07B6866;
    static constexpr auto RB_LRCLOUDSCOMPOSE           = 0xE13A427D;
    static constexpr auto RB_MATERIALTUNE              = 0xC2457E44;
    static constexpr auto RB_MESHPARTICLE              = 0xBAE64FF8;
    static constexpr auto RB_OCCLUDER                  = 0x2A44553C;
    static constexpr auto RB_OCCULSION                 = 0xBD231E4;
    static constexpr auto RB_OPEN                      = 0x5B13AA49;
    static constexpr auto RB_OUTLINEEFFECTBLUR         = 0xDAF5CDB0;
    static constexpr auto RB_PARTICLECOMPOSE           = 0x6B535F86;
    static constexpr auto RB_POSTEFFECTS               = 0x58DE8D87;
    static constexpr auto RB_PROP                      = 0x4894ECD;
    static constexpr auto RB_RAINOCCLUDER              = 0x7450659E;
    static constexpr auto RB_REFLECTION                = 0xA981F55F;
    static constexpr auto RB_ROADJUNCTION              = 0x566DCE92;
    static constexpr auto RB_SSAO                      = 0xC2C03635;
    static constexpr auto RB_SSDECAL                   = 0x4FE3AE77;
    static constexpr auto RB_SCENECAPTURE              = 0x3EB17238;
    static constexpr auto RB_SCREENSPACEREFLECTION     = 0xDA1EB637;
    static constexpr auto RB_SCREENSPACESUBSURFACESKIN = 0xB65AC9D7;
    static constexpr auto RB_SINGLE                    = 0x9D1EE307;
    static constexpr auto RB_SKIDMARKS                 = 0x4D3A7D2F;
    static constexpr auto RB_SKYBOX                    = 0xC24EFB87;
    static constexpr auto RB_SKYGRADIENT               = 0x64076188;
    static constexpr auto RB_SKYMODEL                  = 0x2D95C25E;
    static constexpr auto RB_SOFTCLOUDS                = 0xB308E2F4;
    static constexpr auto RB_SPHERICALHARMONICPROBE    = 0x359F6B2C;
    static constexpr auto RB_SPLINEROAD                = 0xEDABAD;
    static constexpr auto RB_SPOTLIGHTCONE             = 0x33E6BCA3;
    static constexpr auto RB_TERRAINPATCH              = 0x815DF732;
    static constexpr auto RB_TRAIL                     = 0x858E7014;
    static constexpr auto RB_TRIANGLE                  = 0xEB96E782;
    static constexpr auto RB_UNDERWATERFOGGRADIENT     = 0x69DE065B;
    static constexpr auto RB_VEGINTRECENTER            = 0xD89EF9;
    static constexpr auto RB_VEGINTERACTIONVOLUME      = 0x6D87DBFC;
    static constexpr auto RB_VEGSAMPLING               = 0x77B4C335;
    static constexpr auto RB_VEGSYSTEMPOSTDRAW         = 0x20DA8E9D;
    static constexpr auto RB_VEGUPDATE                 = 0x1AF1F8AC;
    static constexpr auto RB_WATERDISPLACEMENTOVERRIDE = 0xF99C72A1;
    static constexpr auto RB_WATERMARK                 = 0x90FE086C;
    static constexpr auto RB_WATERREFLECTIONPLANE      = 0x910EDC80;
    static constexpr auto RB_WEATHER                   = 0x89215D85;
    static constexpr auto RB_WINDOW                    = 0x5B2003F6;
};
