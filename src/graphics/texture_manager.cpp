#include "texture_manager.h"
#include "../fnv1.h"
#include "../game/file_loader.h"
#include "dds_texture_loader.h"
#include "imgui/fonts/fontawesome5_icons.h"
#include "renderer.h"
#include "texture.h"
#include "ui.h"
#include "window.h"

#include <imgui.h>

TextureManager::TextureManager()
{
    Renderer::Get()->Events().PostRender.connect([&](RenderContext_t* context) {
        for (auto it = m_PreviewTextures.begin(); it != m_PreviewTextures.end();) {
            bool open = true;

            std::string title = ICON_FA_FILE_IMAGE "  " + (*it)->GetFileName().filename().string();

            // maintain aspect ratio
            /*ImGui::SetNextWindowSizeConstraints({128, 128}, {FLT_MAX, FLT_MAX}, [](ImGuiSizeCallbackData* data) {
                data->DesiredSize = {std::max(data->DesiredSize.x, data->DesiredSize.y),
                                     std::max(data->DesiredSize.x, data->DesiredSize.y)};
            });*/

            ImGui::SetNextWindowSize({512, 512}, ImGuiCond_Appearing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoCollapse);
            ImGui::PopStyleVar();
            {
                auto width =
                    static_cast<int32_t>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
                auto height =
                    static_cast<int32_t>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

                width -= (width % 2 != 0) ? 1 : 0;
                height -= (height % 2 != 0) ? 1 : 0;

                ImGui::Image((*it)->GetSRV(), ImVec2((float)width, (float)height));

                // render context menus so we can save/export from this window
                UI::Get()->RenderContextMenu((*it)->GetFileName(), 0, ContextMenuFlags_Texture);
            }
            ImGui::End();

            // close button pressed
            if (!open) {
                it = m_PreviewTextures.erase(it);
                Flush();
                continue;
            }

            ++it;
        }
    });
}

void TextureManager::Shutdown()
{
    Empty();
    m_PreviewTextures.clear();
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::filesystem::path& filename, uint8_t flags)
{
    const auto& name = filename.string();
    const auto  key  = fnv_1_32::hash(name.c_str(), name.length());

    if (std::find(m_LastUsedTextures.begin(), m_LastUsedTextures.end(), key) == m_LastUsedTextures.end()) {
        m_LastUsedTextures.emplace_back(key);
    }

    // if we have already cached this texture, use that
    auto it = m_Textures.find(key);
    if (it != m_Textures.end()) {
        return it->second;
    }

    // do we want to create the texture?
    if (flags & TextureCreateFlags_CreateIfNotExists) {
        m_Textures[key] = std::make_shared<Texture>(filename);
        // load the texture
        FileLoader::Get()->ReadTexture(filename, [&, key, filename](bool success, FileBuffer data) {
#ifdef DEBUG
            if (!success) {
                SPDLOG_ERROR("Failed to read texture \"{}\"", filename.string());
            } else if (!HasTexture(filename)) {
                SPDLOG_WARN("Read texture, but it no longer exists in TextureManager cache. (probably deleted before "
                            "loading finished)");
            }
#endif

            if (success && HasTexture(filename)) {
                m_Textures[key]->LoadFromBuffer(&data);
            }
        });

        return m_Textures[key];
    }

    return nullptr;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::filesystem::path& filename, FileBuffer* buffer,
                                                    uint8_t flags)
{
    const auto& name = filename.generic_string();
    const auto  key  = fnv_1_32::hash(name.c_str(), name.length());

    if (std::find(m_LastUsedTextures.begin(), m_LastUsedTextures.end(), key) == m_LastUsedTextures.end()) {
        m_LastUsedTextures.emplace_back(key);
    }

    // if we have already cached this texture, use that
    auto it = m_Textures.find(key);
    if (it != m_Textures.end()) {
        if ((flags & TextureCreateFlags_IsUIRenderable) && !IsPreviewingTexture(it->second)) {
            m_PreviewTextures.emplace_back(it->second);
        }

        return it->second;
    }

    // do we want to create the texture?
    if (flags & TextureCreateFlags_CreateIfNotExists) {
        m_Textures[key] = std::make_shared<Texture>(filename);
        if (!m_Textures[key]->LoadFromBuffer(buffer)) {
            SPDLOG_ERROR("Failed to load texture from buffer!");
        }

        if ((flags & TextureCreateFlags_IsUIRenderable) && !IsPreviewingTexture(m_Textures[key])) {
            m_PreviewTextures.emplace_back(m_Textures[key]);
        }

        return m_Textures[key];
    }

    return nullptr;
}

bool TextureManager::IsPreviewingTexture(std::shared_ptr<Texture> texture)
{
    const auto it =
        std::find_if(m_PreviewTextures.begin(), m_PreviewTextures.end(),
                     [&](const std::shared_ptr<Texture> item) { return texture->GetHash() == item->GetHash(); });
    return it != m_PreviewTextures.end();
}

bool TextureManager::HasTexture(const std::filesystem::path& filename)
{
    const auto& name = filename.string();
    const auto  key  = fnv_1_32::hash(name.c_str(), name.length());

    return (m_Textures.find(key) != m_Textures.end());
}

void TextureManager::Flush()
{
#ifdef DEBUG
    uint32_t _flush_count = 0;
#endif

    for (auto it = m_LastUsedTextures.begin(); it != m_LastUsedTextures.end();) {
        auto texture = *it;

        auto texture_it = m_Textures.find(texture);
        if (texture_it != m_Textures.end() && texture_it->second.use_count() == 1) {
#ifdef DEBUG
            ++_flush_count;
#endif

            m_Textures.erase(texture);
            it = m_LastUsedTextures.erase(it);
        } else {
            ++it;
        }
    }

#ifdef DEBUG
    if (_flush_count > 0) {
        SPDLOG_INFO("Deleted {} unused textures", _flush_count);
    }
#endif
}

void TextureManager::Delete(std::shared_ptr<Texture> texture)
{
    const auto& name = texture->GetFileName().string();
    const auto  key  = fnv_1_32::hash(name.c_str(), name.length());

    m_LastUsedTextures.erase(std::remove(m_LastUsedTextures.begin(), m_LastUsedTextures.end(), key),
                             m_LastUsedTextures.end());
    m_Textures.erase(key);
}

void TextureManager::Empty()
{
    m_Textures.clear();
    m_LastUsedTextures.clear();
}

void TextureManager::PreviewTexture(std::shared_ptr<Texture> texture)
{
    if (std::find(m_PreviewTextures.begin(), m_PreviewTextures.end(), texture) == m_PreviewTextures.end()) {
        m_PreviewTextures.emplace_back(texture);
    }
}

DDS_PIXELFORMAT TextureManager::GetPixelFormat(DXGI_FORMAT format)
{
    DDS_PIXELFORMAT pixelFormat{};
    pixelFormat.size = sizeof(DDS_PIXELFORMAT);

    switch (format) {
        case DXGI_FORMAT_BC1_UNORM: {
            pixelFormat.flags       = DDS_FOURCC;
            pixelFormat.RGBBitCount = 0;
            pixelFormat.RBitMask    = 0;
            pixelFormat.GBitMask    = 0;
            pixelFormat.BBitMask    = 0;
            pixelFormat.ABitMask    = 0;
            pixelFormat.fourCC      = 0x31545844; // DXT1
            break;
        }

        case DXGI_FORMAT_BC2_UNORM: {
            pixelFormat.flags       = DDS_FOURCC;
            pixelFormat.RGBBitCount = 0;
            pixelFormat.RBitMask    = 0;
            pixelFormat.GBitMask    = 0;
            pixelFormat.BBitMask    = 0;
            pixelFormat.ABitMask    = 0;
            pixelFormat.fourCC      = 0x33545844; // DXT3
            break;
        }

        case DXGI_FORMAT_BC3_UNORM: {
            pixelFormat.flags       = DDS_FOURCC;
            pixelFormat.RGBBitCount = 0;
            pixelFormat.RBitMask    = 0;
            pixelFormat.GBitMask    = 0;
            pixelFormat.BBitMask    = 0;
            pixelFormat.ABitMask    = 0;
            pixelFormat.fourCC      = 0x35545844; // DXT5
            break;
        }

        case DXGI_FORMAT_B8G8R8A8_UNORM: {
            pixelFormat.flags       = (DDS_RGB | DDS_ALPHA);
            pixelFormat.RGBBitCount = 32; // A8B8G8R8
            pixelFormat.RBitMask    = 0x00FF0000;
            pixelFormat.GBitMask    = 0x0000FF00;
            pixelFormat.BBitMask    = 0x000000FF;
            pixelFormat.ABitMask    = 0xFF000000;
            pixelFormat.fourCC      = 0;
            break;
        }

        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC7_UNORM: {
            pixelFormat.flags  = DDS_FOURCC;
            pixelFormat.fourCC = 0x30315844; // DX10
            break;
        }
    }

    return pixelFormat;
}

const char* TextureManager::GetFormatString(DXGI_FORMAT format)
{
    switch (format) {
        case DXGI_FORMAT_UNKNOWN:
            return "DXGI_FORMAT_UNKNOWN";
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return "DXGI_FORMAT_R32G32B32A32_FLOAT";
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return "DXGI_FORMAT_R32G32B32A32_UINT";
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return "DXGI_FORMAT_R32G32B32A32_SINT";
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return "DXGI_FORMAT_R32G32B32_TYPELESS";
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return "DXGI_FORMAT_R32G32B32_FLOAT";
        case DXGI_FORMAT_R32G32B32_UINT:
            return "DXGI_FORMAT_R32G32B32_UINT";
        case DXGI_FORMAT_R32G32B32_SINT:
            return "DXGI_FORMAT_R32G32B32_SINT";
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return "DXGI_FORMAT_R16G16B16A16_FLOAT";
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return "DXGI_FORMAT_R16G16B16A16_UNORM";
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return "DXGI_FORMAT_R16G16B16A16_UINT";
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return "DXGI_FORMAT_R16G16B16A16_SNORM";
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return "DXGI_FORMAT_R16G16B16A16_SINT";
        case DXGI_FORMAT_R32G32_TYPELESS:
            return "DXGI_FORMAT_R32G32_TYPELESS";
        case DXGI_FORMAT_R32G32_FLOAT:
            return "DXGI_FORMAT_R32G32_FLOAT";
        case DXGI_FORMAT_R32G32_UINT:
            return "DXGI_FORMAT_R32G32_UINT";
        case DXGI_FORMAT_R32G32_SINT:
            return "DXGI_FORMAT_R32G32_SINT";
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return "DXGI_FORMAT_R32G8X24_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return "DXGI_FORMAT_R10G10B10A2_UNORM";
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return "DXGI_FORMAT_R10G10B10A2_UINT";
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return "DXGI_FORMAT_R11G11B10_FLOAT";
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return "DXGI_FORMAT_R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return "DXGI_FORMAT_R8G8B8A8_UINT";
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return "DXGI_FORMAT_R8G8B8A8_SNORM";
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return "DXGI_FORMAT_R8G8B8A8_SINT";
        case DXGI_FORMAT_R16G16_TYPELESS:
            return "DXGI_FORMAT_R16G16_TYPELESS";
        case DXGI_FORMAT_R16G16_FLOAT:
            return "DXGI_FORMAT_R16G16_FLOAT";
        case DXGI_FORMAT_R16G16_UNORM:
            return "DXGI_FORMAT_R16G16_UNORM";
        case DXGI_FORMAT_R16G16_UINT:
            return "DXGI_FORMAT_R16G16_UINT";
        case DXGI_FORMAT_R16G16_SNORM:
            return "DXGI_FORMAT_R16G16_SNORM";
        case DXGI_FORMAT_R16G16_SINT:
            return "DXGI_FORMAT_R16G16_SINT";
        case DXGI_FORMAT_R32_TYPELESS:
            return "DXGI_FORMAT_R32_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT:
            return "DXGI_FORMAT_D32_FLOAT";
        case DXGI_FORMAT_R32_FLOAT:
            return "DXGI_FORMAT_R32_FLOAT";
        case DXGI_FORMAT_R32_UINT:
            return "DXGI_FORMAT_R32_UINT";
        case DXGI_FORMAT_R32_SINT:
            return "DXGI_FORMAT_R32_SINT";
        case DXGI_FORMAT_R24G8_TYPELESS:
            return "DXGI_FORMAT_R24G8_TYPELESS";
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return "DXGI_FORMAT_D24_UNORM_S8_UINT";
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
        case DXGI_FORMAT_R8G8_TYPELESS:
            return "DXGI_FORMAT_R8G8_TYPELESS";
        case DXGI_FORMAT_R8G8_UNORM:
            return "DXGI_FORMAT_R8G8_UNORM";
        case DXGI_FORMAT_R8G8_UINT:
            return "DXGI_FORMAT_R8G8_UINT";
        case DXGI_FORMAT_R8G8_SNORM:
            return "DXGI_FORMAT_R8G8_SNORM";
        case DXGI_FORMAT_R8G8_SINT:
            return "DXGI_FORMAT_R8G8_SINT";
        case DXGI_FORMAT_R16_TYPELESS:
            return "DXGI_FORMAT_R16_TYPELESS";
        case DXGI_FORMAT_R16_FLOAT:
            return "DXGI_FORMAT_R16_FLOAT";
        case DXGI_FORMAT_D16_UNORM:
            return "DXGI_FORMAT_D16_UNORM";
        case DXGI_FORMAT_R16_UNORM:
            return "DXGI_FORMAT_R16_UNORM";
        case DXGI_FORMAT_R16_UINT:
            return "DXGI_FORMAT_R16_UINT";
        case DXGI_FORMAT_R16_SNORM:
            return "DXGI_FORMAT_R16_SNORM";
        case DXGI_FORMAT_R16_SINT:
            return "DXGI_FORMAT_R16_SINT";
        case DXGI_FORMAT_R8_TYPELESS:
            return "DXGI_FORMAT_R8_TYPELESS";
        case DXGI_FORMAT_R8_UNORM:
            return "DXGI_FORMAT_R8_UNORM";
        case DXGI_FORMAT_R8_UINT:
            return "DXGI_FORMAT_R8_UINT";
        case DXGI_FORMAT_R8_SNORM:
            return "DXGI_FORMAT_R8_SNORM";
        case DXGI_FORMAT_R8_SINT:
            return "DXGI_FORMAT_R8_SINT";
        case DXGI_FORMAT_A8_UNORM:
            return "DXGI_FORMAT_A8_UNORM";
        case DXGI_FORMAT_R1_UNORM:
            return "DXGI_FORMAT_R1_UNORM";
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
            return "DXGI_FORMAT_R8G8_B8G8_UNORM";
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            return "DXGI_FORMAT_G8R8_G8B8_UNORM";
        case DXGI_FORMAT_BC1_TYPELESS:
            return "DXGI_FORMAT_BC1_TYPELESS";
        case DXGI_FORMAT_BC1_UNORM:
            return "DXGI_FORMAT_BC1_UNORM";
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return "DXGI_FORMAT_BC1_UNORM_SRGB";
        case DXGI_FORMAT_BC2_TYPELESS:
            return "DXGI_FORMAT_BC2_TYPELESS";
        case DXGI_FORMAT_BC2_UNORM:
            return "DXGI_FORMAT_BC2_UNORM";
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return "DXGI_FORMAT_BC2_UNORM_SRGB";
        case DXGI_FORMAT_BC3_TYPELESS:
            return "DXGI_FORMAT_BC3_TYPELESS";
        case DXGI_FORMAT_BC3_UNORM:
            return "DXGI_FORMAT_BC3_UNORM";
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return "DXGI_FORMAT_BC3_UNORM_SRGB";
        case DXGI_FORMAT_BC4_TYPELESS:
            return "DXGI_FORMAT_BC4_TYPELESS";
        case DXGI_FORMAT_BC4_UNORM:
            return "DXGI_FORMAT_BC4_UNORM";
        case DXGI_FORMAT_BC4_SNORM:
            return "DXGI_FORMAT_BC4_SNORM";
        case DXGI_FORMAT_BC5_TYPELESS:
            return "DXGI_FORMAT_BC5_TYPELESS";
        case DXGI_FORMAT_BC5_UNORM:
            return "DXGI_FORMAT_BC5_UNORM";
        case DXGI_FORMAT_BC5_SNORM:
            return "DXGI_FORMAT_BC5_SNORM";
        case DXGI_FORMAT_B5G6R5_UNORM:
            return "DXGI_FORMAT_B5G6R5_UNORM";
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return "DXGI_FORMAT_B5G5R5A1_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return "DXGI_FORMAT_B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return "DXGI_FORMAT_B8G8R8X8_UNORM";
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
        case DXGI_FORMAT_BC6H_TYPELESS:
            return "DXGI_FORMAT_BC6H_TYPELESS";
        case DXGI_FORMAT_BC6H_UF16:
            return "DXGI_FORMAT_BC6H_UF16";
        case DXGI_FORMAT_BC6H_SF16:
            return "DXGI_FORMAT_BC6H_SF16";
        case DXGI_FORMAT_BC7_TYPELESS:
            return "DXGI_FORMAT_BC7_TYPELESS";
        case DXGI_FORMAT_BC7_UNORM:
            return "DXGI_FORMAT_BC7_UNORM";
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return "DXGI_FORMAT_BC7_UNORM_SRGB";
        case DXGI_FORMAT_AYUV:
            return "DXGI_FORMAT_AYUV";
        case DXGI_FORMAT_Y410:
            return "DXGI_FORMAT_Y410";
        case DXGI_FORMAT_Y416:
            return "DXGI_FORMAT_Y416";
        case DXGI_FORMAT_NV12:
            return "DXGI_FORMAT_NV12";
        case DXGI_FORMAT_P010:
            return "DXGI_FORMAT_P010";
        case DXGI_FORMAT_P016:
            return "DXGI_FORMAT_P016";
        case DXGI_FORMAT_420_OPAQUE:
            return "DXGI_FORMAT_420_OPAQUE";
        case DXGI_FORMAT_YUY2:
            return "DXGI_FORMAT_YUY2";
        case DXGI_FORMAT_Y210:
            return "DXGI_FORMAT_Y210";
        case DXGI_FORMAT_Y216:
            return "DXGI_FORMAT_Y216";
        case DXGI_FORMAT_NV11:
            return "DXGI_FORMAT_NV11";
        case DXGI_FORMAT_AI44:
            return "DXGI_FORMAT_AI44";
        case DXGI_FORMAT_IA44:
            return "DXGI_FORMAT_IA44";
        case DXGI_FORMAT_P8:
            return "DXGI_FORMAT_P8";
        case DXGI_FORMAT_A8P8:
            return "DXGI_FORMAT_A8P8";
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return "DXGI_FORMAT_B4G4R4A4_UNORM";
        case DXGI_FORMAT_FORCE_UINT:
            return "DXGI_FORMAT_FORCE_UINT";
        default:
            return "DXGI_ERROR_FORMAT";
    }
}
