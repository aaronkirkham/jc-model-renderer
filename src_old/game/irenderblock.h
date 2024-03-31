#pragma once

#include "graphics/types.h"

#include "game/types.h"

#include <array>
#include <memory>
#include <vector>

class Texture;
class RenderBlockModel;
class IRenderBlock
{
  protected:
    RenderBlockModel*                     m_Owner             = nullptr;
    bool                                  m_Visible           = true;
    float                                 m_ScaleModifier     = 1.0f;
    VertexBuffer_t*                       m_VertexBuffer      = nullptr;
    IndexBuffer_t*                        m_IndexBuffer       = nullptr;
    std::shared_ptr<VertexShader_t>       m_VertexShader      = nullptr;
    std::shared_ptr<PixelShader_t>        m_PixelShader       = nullptr;
    VertexDeclaration_t*                  m_VertexDeclaration = nullptr;
    SamplerState_t*                       m_SamplerState      = nullptr;
    std::vector<std::shared_ptr<Texture>> m_Textures;

  public:
    using rb_textures_t = std::vector<std::pair<std::string, std::shared_ptr<Texture>>>;

    IRenderBlock() = default;
    virtual ~IRenderBlock();

    virtual const char* GetTypeName()       = 0;
    virtual uint32_t    GetTypeHash() const = 0;

    virtual bool IsOpaque() = 0;

    virtual void Setup(RenderContext_t* context);
    virtual void Draw(RenderContext_t* context);

    virtual void DrawContextMenu() = 0;
    virtual void DrawUI()          = 0;

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) = 0;
    virtual std::tuple<vertices_t, uint16s_t> GetData()                                    = 0;

    virtual rb_textures_t GetTextures() = 0;

    void BindTexture(int32_t texture_index, int32_t slot, SamplerState_t* sampler = nullptr);
    void BindTexture(int32_t texture_index, SamplerState_t* sampler = nullptr)
    {
        return BindTexture(texture_index, texture_index, sampler);
    }

    inline void AllocateTextureSlots(int32_t count)
    {
        m_Textures.resize(count);
    }

    inline void SetTexture(int32_t texture_index, std::shared_ptr<Texture> texture)
    {
        m_Textures[texture_index] = texture;
    }

    void DrawUI_Texture(const std::string& title, uint32_t texture_index);

    virtual bool IsVisible() const
    {
        return m_Visible;
    }

    virtual float GetScale() const
    {
        return m_ScaleModifier;
    }

    virtual VertexBuffer_t* GetVertexBuffer()
    {
        return m_VertexBuffer;
    }

    virtual IndexBuffer_t* GetIndexBuffer()
    {
        return m_IndexBuffer;
    }

    void SetOwner(RenderBlockModel* owner)
    {
        m_Owner = owner;
    }

    RenderBlockModel* GetOwner()
    {
        return m_Owner;
    }
};
