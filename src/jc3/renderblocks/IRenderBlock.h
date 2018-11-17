#pragma once

#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <graphics/TextureManager.h>
#include <graphics/Types.h>
#include <graphics/UI.h>
#include <graphics/imgui/fonts/fontawesome5_icons.h>
#include <graphics/imgui/imgui_bitfield.h>
#include <graphics/imgui/imgui_disabled.h>
#include <jc3/Types.h>

class IRenderBlock
{
  protected:
    bool  m_Visible       = true;
    float m_ScaleModifier = 1.0f;

    VertexBuffer_t*                       m_VertexBuffer      = nullptr;
    IndexBuffer_t*                        m_IndexBuffer       = nullptr;
    std::shared_ptr<VertexShader_t>       m_VertexShader      = nullptr;
    std::shared_ptr<PixelShader_t>        m_PixelShader       = nullptr;
    VertexDeclaration_t*                  m_VertexDeclaration = nullptr;
    SamplerState_t*                       m_SamplerState      = nullptr;
    std::vector<std::shared_ptr<Texture>> m_Textures;
    float                                 m_MaterialParams[4] = {0};

  public:
    IRenderBlock() = default;
    virtual ~IRenderBlock()
    {
        m_VertexShader = nullptr;
        m_PixelShader  = nullptr;

        Renderer::Get()->DestroyBuffer(m_VertexBuffer);
        Renderer::Get()->DestroyBuffer(m_IndexBuffer);
        Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
        Renderer::Get()->DestroySamplerState(m_SamplerState);
    }

    virtual const char* GetTypeName()       = 0;
    virtual uint32_t    GetTypeHash() const = 0;

    virtual void SetVisible(bool visible)
    {
        m_Visible = visible;
    }

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

    virtual const std::vector<std::shared_ptr<Texture>>& GetTextures()
    {
        return m_Textures;
    }

    virtual bool IsOpaque() = 0;

    virtual void Create()                  = 0;
    virtual void Read(std::istream& file)  = 0;
    virtual void Write(std::ostream& file) = 0;

    virtual void SetData(floats_t* vertices, uint16s_t* indices, floats_t* uvs) = 0;
    virtual std::tuple<floats_t, uint16s_t, floats_t> GetData()                 = 0;

    virtual void Setup(RenderContext_t* context)
    {
        if (!m_Visible)
            return;

        assert(m_VertexShader);
        assert(m_PixelShader);
        assert(m_VertexDeclaration);
        assert(m_VertexBuffer);

        // enable the vertex and pixel shaders
        context->m_DeviceContext->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
        context->m_DeviceContext->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

        // set the input layout
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);

        // set the vertex buffer
        context->m_Renderer->SetVertexStream(m_VertexBuffer, 0);

        // set the topology
        context->m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    virtual void Draw(RenderContext_t* context)
    {
        if (!m_Visible)
            return;

        if (m_IndexBuffer) {
            context->m_Renderer->DrawIndexed(0, m_IndexBuffer->m_ElementCount, m_IndexBuffer);
        } else {
            context->m_Renderer->Draw(0, (m_VertexBuffer->m_ElementCount / 3));
        }
    }

    inline void BindTexture(int32_t texture_index, int32_t slot, SamplerState_t* sampler = nullptr)
    {
        const auto& texture = m_Textures[texture_index];
        if (texture && texture->IsLoaded()) {
            texture->Use(slot, sampler);
        }
    }

    inline void BindTexture(int32_t texture_index, SamplerState_t* sampler = nullptr)
    {
        return BindTexture(texture_index, texture_index, sampler);
    }

    IBuffer_t* ReadBuffer(std::istream& stream, BufferType type, uint32_t stride)
    {
        assert(stride);

        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        auto buffer = std::make_unique<char[]>(count * stride);
        stream.read((char*)buffer.get(), (count * stride));

        return Renderer::Get()->CreateBuffer(buffer.get(), count, stride, type, "IRenderBlock Buffer");
    }

    template <typename T> IBuffer_t* ReadVertexBuffer(std::istream& stream)
    {
        return ReadBuffer(stream, VERTEX_BUFFER, sizeof(T));
    }

    IBuffer_t* ReadIndexBuffer(std::istream& stream)
    {
        return ReadBuffer(stream, INDEX_BUFFER, sizeof(uint16_t));
    }

    void WriteBuffer(std::ostream& stream, IBuffer_t* buffer)
    {
        stream.write((char*)&buffer->m_ElementCount, sizeof(buffer->m_ElementCount));
        stream.write((char*)buffer->m_Data.data(), buffer->m_Data.size());
    }

    void ReadMaterials(std::istream& stream)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        m_Textures.resize(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t length;
            stream.read((char*)&length, sizeof(length));

            if (length == 0) {
                continue;
            }

            auto filename = std::make_unique<char[]>(length + 1);
            stream.read(filename.get(), length);
            filename[length] = '\0';

            // load the material
            auto texture = TextureManager::Get()->GetTexture(filename.get());
            if (texture) {
                m_Textures[i] = std::move(texture);
            }
        }

        stream.read((char*)&m_MaterialParams, sizeof(m_MaterialParams));
    }

    void WriteMaterials(std::ostream& stream)
    {
        auto count = static_cast<uint32_t>(m_Textures.size());
        stream.write((char*)&count, sizeof(count));

        for (uint32_t i = 0; i < count; ++i) {
            const auto& filename = m_Textures[i]->GetPath().generic_string();
            const auto  length   = static_cast<uint32_t>(filename.length());

            stream.write((char*)&length, sizeof(length));
            stream.write(filename.c_str(), length);
        }

        stream.write((char*)&m_MaterialParams, sizeof(m_MaterialParams));
    }

    void ReadSkinBatch(std::istream& stream, std::vector<JustCause3::CSkinBatch>* skin_batches)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));
        skin_batches->reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            JustCause3::CSkinBatch batch;

            stream.read((char*)&batch.m_Size, sizeof(batch.m_Size));
            stream.read((char*)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.read((char*)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            // allocate the batch lookup
            if (batch.m_BatchSize != 0) {
                batch.m_BatchLookup = new int16_t[batch.m_BatchSize];
                stream.read((char*)batch.m_BatchLookup, sizeof(int16_t) * batch.m_BatchSize);
            }

            skin_batches->emplace_back(std::move(batch));
        }
    }

    void WriteSkinBatch(std::ostream& stream, std::vector<JustCause3::CSkinBatch>* skin_batches)
    {
        uint32_t count = skin_batches->size();
        stream.write((char*)&count, sizeof(count));

        for (const auto& batch : *skin_batches) {
            stream.write((char*)&batch.m_Size, sizeof(batch.m_Size));
            stream.write((char*)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.write((char*)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            if (batch.m_BatchSize != 0) {
                stream.write((char*)batch.m_BatchLookup, sizeof(int16_t) * batch.m_BatchSize);
            }
        }
    }

    void ReadDeformTable(std::istream& stream, JustCause3::CDeformTable* deform_table)
    {
        uint32_t deformTable[256];
        stream.read((char*)&deformTable, sizeof(deformTable));

        for (uint32_t i = 0; i < 256; ++i) {
            const auto data = deformTable[i];

            deform_table->table[i] = data;
            if (data != -1 && deform_table->size < i) {
                deform_table->size = i;
            }
        }
    }

    void WriteDeformTable(std::ostream& stream, JustCause3::CDeformTable* deform_table)
    {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t data = deform_table->table[i];
            stream.write((char*)&data, sizeof(data));
        }
    }

    void DrawSkinBatches(RenderContext_t* context, const std::vector<JustCause3::CSkinBatch>& skin_batches)
    {
        if (!m_Visible)
            return;

        for (const auto& batch : skin_batches) {
            context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
        }
    }

    virtual void DrawUI() = 0;
};
