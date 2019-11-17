#pragma once

#include <d3d11.h>
#include <filesystem>

struct SamplerState_t;
class Texture
{
  private:
    ID3D11Resource*           m_Texture  = nullptr;
    ID3D11ShaderResourceView* m_SRV      = nullptr;
    std::filesystem::path     m_Filename = "";
    uint32_t                  m_NameHash = 0;
    D3D11_TEXTURE2D_DESC      m_Desc     = {};
    std::vector<uint8_t>      m_Buffer;

  public:
    Texture(const std::filesystem::path& filename);
    Texture(const std::filesystem::path& filename, std::vector<uint8_t>* buffer);
    virtual ~Texture();

    bool LoadFromBuffer(std::vector<uint8_t>* buffer);
    bool LoadFromFile(const std::filesystem::path& filename);

    void Use(uint32_t slot, SamplerState_t* sampler = nullptr);
    void UseVS(uint32_t slot);

    void                         SetFileName(const std::filesystem::path& filename);
    const std::filesystem::path& GetFileName() const;

    bool                        IsLoaded() const;
    const uint32_t              GetHash() const;
    const std::vector<uint8_t>* GetBuffer() const;

    ID3D11Resource*           GetResource() const;
    ID3D11ShaderResourceView* GetSRV() const;
    D3D11_TEXTURE2D_DESC*     GetDesc();
};
