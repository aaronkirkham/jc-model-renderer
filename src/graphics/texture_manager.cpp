#include "texture_manager.h"
#include "../fnv1.h"
#include "../jc3/file_loader.h"
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
                UI::Get()->RenderContextMenu((*it)->GetFileName(), 0, CTX_TEXTURE);
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
    if (flags & CREATE_IF_NOT_EXISTS) {
        m_Textures[key] = std::make_shared<Texture>(filename);
        // load the texture
        FileLoader::Get()->ReadTexture(filename, [&, key, filename](bool success, FileBuffer data) {
#ifdef DEBUG
            if (!success) {
                DEBUG_LOG("TextureManager::GetTexture - Failed to read texture. (" << filename << ")");
            } else if (!HasTexture(filename)) {
                DEBUG_LOG("TextureManager::GetTexture - Read texture, but doesn't exists in TextureManager.");
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
        if (flags & IS_UI_RENDERABLE) {
            m_PreviewTextures.emplace_back(it->second);
        }

        return it->second;
    }

    // do we want to create the texture?
    if (flags & CREATE_IF_NOT_EXISTS) {
        m_Textures[key] = std::make_shared<Texture>(filename);
        if (!m_Textures[key]->LoadFromBuffer(buffer)) {
            DEBUG_LOG("TextureManager::GetTexture - Failed to load texture from buffer!");
        }

        if (flags & IS_UI_RENDERABLE) {
            m_PreviewTextures.emplace_back(m_Textures[key]);
        }

        return m_Textures[key];
    }

    return nullptr;
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
        DEBUG_LOG("TextureManager::Flush - Deleted " << _flush_count << " unused textures.");
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
