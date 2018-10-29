#pragma once

#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <graphics/TextureManager.h>
#include <graphics/Types.h>
#include <graphics/imgui/imgui_bitfield.h>
#include <jc3/Types.h>

class IRenderBlock
{
  protected:
    bool  m_Visible       = true;
    float m_ScaleModifier = 1.0f;

    VertexBuffer_t*                 m_VertexBuffer      = nullptr;
    IndexBuffer_t*                  m_IndexBuffer       = nullptr;
    std::shared_ptr<VertexShader_t> m_VertexShader      = nullptr;
    std::shared_ptr<PixelShader_t>  m_PixelShader       = nullptr;
    VertexDeclaration_t*            m_VertexDeclaration = nullptr;
    SamplerState_t*                 m_SamplerState      = nullptr;

    std::vector<fs::path>                 m_Materials;
    std::vector<std::shared_ptr<Texture>> m_Textures;
    std::vector<JustCause3::CSkinBatch>   m_SkinBatches;

    // used by the exporter stuff, vertices are unpacked by the renders blocks
    std::vector<float>    m_Vertices;
    std::vector<uint16_t> m_Indices;
    std::vector<float>    m_UVs;

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

        // delete the batch lookup
        for (auto& batch : m_SkinBatches) {
            SAFE_DELETE(batch.m_BatchLookup);
        }
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
    virtual const std::vector<float>& GetVertices()
    {
        return m_Vertices;
    }
    virtual const std::vector<uint16_t>& GetIndices()
    {
        return m_Indices;
    }
    virtual const std::vector<float>& GetUVs()
    {
        return m_UVs;
    }

    virtual bool IsOpaque() = 0;

    virtual void Create()                  = 0;
    virtual void Read(std::istream& file)  = 0;
    virtual void Write(std::ostream& file) = 0;

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

        // enable textures
        for (uint32_t i = 0; i < m_Textures.size(); ++i) {
            const auto& texture = m_Textures[i];
            if (texture && texture->IsLoaded()) {
                texture->Use(i);
            }
        }

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

    template <typename T>
    void ReadVertexBuffer(std::istream& stream, VertexBuffer_t** outBuffer, std::vector<T>* outVertices = nullptr)
    {
        auto stride = static_cast<uint32_t>(sizeof(T));

        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        outVertices->resize(count);
        stream.read((char*)outVertices->data(), (count * stride));

        *outBuffer = Renderer::Get()->CreateVertexBuffer(outVertices->data(), count, stride, D3D11_USAGE_DEFAULT,
                                                         "IRenderBlock Vertex Buffer");
    }

    void ReadVertexBuffer(std::istream& stream, VertexBuffer_t** outBuffer, uint32_t stride)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        auto buffer = std::make_unique<char[]>(count * stride);
        stream.read((char*)buffer.get(), (count * stride));

        *outBuffer = Renderer::Get()->CreateVertexBuffer(buffer.get(), count, stride, D3D11_USAGE_DEFAULT,
                                                         "IRenderBlock Vertex Buffer");
    }

    void WriteVertexBuffer(std::ostream& stream, VertexBuffer_t* buffer)
    {
        stream.write((char*)&buffer->m_ElementCount, sizeof(buffer->m_ElementCount));
        stream.write((char*)buffer->m_Data.data(), buffer->m_Data.size());
    }

    void ReadIndexBuffer(std::istream& stream, IndexBuffer_t** outBuffer)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        m_Indices.resize(count);
        stream.read((char*)m_Indices.data(), (count * sizeof(uint16_t)));

        *outBuffer = Renderer::Get()->CreateIndexBuffer(m_Indices.data(), count, D3D11_USAGE_DEFAULT,
                                                        "IRenderBlock Index Buffer");
    }

    void WriteIndexBuffer(std::ostream& stream)
    {
        auto count = static_cast<uint32_t>(m_Indices.size());
        stream.write((char*)&count, sizeof(count));
        stream.write((char*)m_Indices.data(), (count * sizeof(uint16_t)));
    }

    void WriteIndexBuffer(std::ostream& stream, IndexBuffer_t* buffer)
    {
        stream.write((char*)&buffer->m_ElementCount, sizeof(buffer->m_ElementCount));
        stream.write((char*)buffer->m_Data.data(), (buffer->m_ElementCount * buffer->m_ElementStride));
    }

    void ReadMaterials(std::istream& stream)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        m_Materials.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t length;
            stream.read((char*)&length, sizeof(length));

            if (length == 0) {
                continue;
            }

            auto filename = std::make_unique<char[]>(length + 1);
            stream.read(filename.get(), length);
            filename[length] = '\0';

            m_Materials.emplace_back(filename.get());

            // load the material
            auto& texture = TextureManager::Get()->GetTexture(filename.get());
            if (texture) {
                m_Textures.emplace_back(texture);
            }
        }

        uint32_t unknown[4];
        stream.read((char*)&unknown, sizeof(unknown));
    }

    void WriteMaterials(std::ostream& stream)
    {
        auto count = static_cast<uint32_t>(m_Materials.size());
        stream.write((char*)&count, sizeof(count));

        for (const auto& material : m_Materials) {
            auto str = material.string();
            std::replace(str.begin(), str.end(), '\\', '/');

            auto length = static_cast<uint32_t>(str.length());
            stream.write((char*)&length, sizeof(length));
            stream.write(str.c_str(), length);
        }

        // TODO: find out what all this is. probably something important...
        uint32_t unknown[4] = {0};
        stream.write((char*)&unknown, sizeof(unknown));
    }

    void ReadSkinBatch(std::istream& stream)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));
        m_SkinBatches.reserve(count);

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

            m_SkinBatches.emplace_back(batch);
        }
    }

    void WriteSkinBatch(std::ostream& stream)
    {
        uint32_t count = m_SkinBatches.size();
        stream.write((char*)&count, sizeof(count));

        for (const auto& batch : m_SkinBatches) {
            stream.write((char*)&batch.m_Size, sizeof(batch.m_Size));
            stream.write((char*)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.write((char*)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            if (batch.m_BatchSize != 0) {
                stream.write((char*)&batch.m_BatchLookup, sizeof(int16_t) * batch.m_BatchSize);
                __debugbreak();
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

    void DrawSkinBatches(RenderContext_t* context)
    {
        if (!m_Visible)
            return;

        for (const auto& batch : m_SkinBatches) {
            context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
        }
    }

    virtual void DrawUI() = 0;
};
