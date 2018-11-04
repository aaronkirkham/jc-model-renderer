#include <Window.h>
#include <fnv1.h>
#include <graphics/DDSTextureLoader.h>
#include <graphics/Renderer.h>
#include <graphics/TextureManager.h>
#include <jc3/FileLoader.h>

#include <graphics/imgui/fonts/fontawesome5_icons.h>

#include <graphics/UI.h>

TextureManager::TextureManager()
{
    m_MissingTexture = std::make_unique<Texture>("missing-texture.dds");
#ifdef DEBUG
    m_MissingTexture->LoadFromFile("../assets/missing-texture.dds");
#else
    m_MissingTexture->LoadFromFile("assets/missing-texture.dds");
#endif

    Renderer::Get()->Events().PostRender.connect([&](RenderContext_t* context) {
        for (auto it = m_PreviewTextures.begin(); it != m_PreviewTextures.end();) {
            bool       open    = true;
            const auto texture = (*it);

            std::stringstream ss;
            ss << ICON_FA_FILE_IMAGE << "  " << texture->GetPath().filename();

            // maintain aspect ratio
            ImGui::SetNextWindowSizeConstraints({128, 128}, {FLT_MAX, FLT_MAX}, [](ImGuiSizeCallbackData* data) {
                data->DesiredSize = {std::max(data->DesiredSize.x, data->DesiredSize.y),
                                     std::max(data->DesiredSize.x, data->DesiredSize.y)};
            });

            // draw window
            ImGui::SetNextWindowSize({512, 512}, ImGuiCond_Appearing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            if (ImGui::Begin(
                    ss.str().c_str(), &open,
                    (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking))) {
                ImGui::PopStyleVar();

                const auto  titlebar_size = ImGui::GetCursorPosY();
                const auto& w_size        = ImGui::GetWindowSize();

                ImGui::Image(texture->GetSRV(), {w_size.x, w_size.y - titlebar_size});

                // render context menus so we can save/export from this window
                UI::Get()->RenderContextMenu(texture->GetPath(), 0, CTX_TEXTURE);

                ImGui::End();
            } else
                ImGui::PopStyleVar();

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
    m_MissingTexture = nullptr;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const fs::path& filename, uint8_t flags)
{
    auto name = filename.filename().stem().string();
    auto key  = fnv_1_32::hash(name.c_str(), name.length());

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
            if (success && HasTexture(filename)) {
                m_Textures[key]->LoadFromBuffer(&data);
            }
        });

        return m_Textures[key];
    }

    return nullptr;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const fs::path& filename, FileBuffer* buffer, uint8_t flags)
{
    auto name = filename.filename().stem().string();
    auto key  = fnv_1_32::hash(name.c_str(), name.length());

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

bool TextureManager::HasTexture(const fs::path& filename)
{
    auto name = filename.filename().stem().string();
    auto key  = fnv_1_32::hash(name.c_str(), name.length());

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
    auto name = texture->GetPath().filename().stem().string();
    auto key  = fnv_1_32::hash(name.c_str(), name.length());

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
    DDS_PIXELFORMAT pixelFormat;
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
            pixelFormat.fourCC = 0x30315844; // DX10
            break;
        }
    }

    return pixelFormat;
}
