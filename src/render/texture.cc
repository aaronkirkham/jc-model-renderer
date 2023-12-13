#include "pch.h"

#include "texture.h"

#include "render/dds_texture_loader.h"
#include "render/renderer.h"

namespace jcmr
{
struct TextureImpl final : Texture {
    TextureImpl(const std::string& filename, Renderer& renderer)
        : m_filename(filename)
        , m_renderer(renderer)
    {
    }

    ~TextureImpl()
    {
        if (m_srv) m_srv->Release();
        if (m_texture) m_texture->Release();
    }

    bool load(const ByteArray& buffer) override
    {
        auto& context = m_renderer.get_context();

        auto result =
            DirectX::CreateDDSTextureFromMemoryEx(context.device, buffer.data(), buffer.size(), 0, D3D11_USAGE_DEFAULT,
                                                  D3D11_BIND_SHADER_RESOURCE, 0, 0, false, &m_texture, &m_srv);
        if (FAILED(result)) {
            LOG_ERROR("Texture : failed to load texture. (GetLastError={})", GetLastError());
            return false;
        }

        return true;
    }

    const std::string&        get_filename() const override { return m_filename; }
    ID3D11Resource*           get_texture() const override { return m_texture; }
    ID3D11ShaderResourceView* get_srv() const override { return m_srv; }

  private:
    Renderer&                 m_renderer;
    std::string               m_filename;
    ID3D11Resource*           m_texture = nullptr;
    ID3D11ShaderResourceView* m_srv     = nullptr;
};

Texture* Texture::create(const std::string& filename, Renderer& renderer)
{
    return new TextureImpl(filename, renderer);
}

void Texture::destroy(Texture* instance)
{
    delete instance;
}
} // namespace jcmr