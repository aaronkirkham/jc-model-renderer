#pragma once

#include <jc3/Types.h>
#include <graphics/ShaderManager.h>
#include <graphics/TextureManager.h>
#include <graphics/Types.h>
#include <graphics/Renderer.h>

class IRenderBlock
{
protected:
    VertexBuffer_t* m_VertexBuffer = nullptr;
    IndexBuffer_t* m_IndexBuffer = nullptr;
    std::shared_ptr<VertexShader_t> m_VertexShader = nullptr;
    std::shared_ptr<PixelShader_t> m_PixelShader = nullptr;
    VertexDeclaration_t* m_VertexDeclaration = nullptr;
    SamplerState_t* m_SamplerState = nullptr;

    std::vector<fs::path> m_Materials;
    std::vector<std::shared_ptr<Texture>> m_Textures;
    std::vector<JustCause3::CSkinBatch> m_SkinBatches;

    // used by the exporter stuff, vertices are unpacked by the renders blocks
    std::vector<float> m_Vertices;
    std::vector<uint16_t> m_Indices;
    std::vector<float> m_UVs;

public:
    IRenderBlock() = default;
    virtual ~IRenderBlock()
    {
        OutputDebugStringA("~IRenderBlock\n");

        m_VertexShader = nullptr;
        m_PixelShader = nullptr;

        Renderer::Get()->DestroyBuffer(m_VertexBuffer);
        Renderer::Get()->DestroyBuffer(m_IndexBuffer);
        Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
        Renderer::Get()->DestroySamplerState(m_SamplerState);
    }

    virtual const char* GetTypeName() = 0;

    virtual VertexBuffer_t* GetVertexBuffer() { return m_VertexBuffer; }
    virtual IndexBuffer_t* GetIndexBuffer() { return m_IndexBuffer; }
    virtual const std::vector<std::shared_ptr<Texture>>& GetTextures() { return m_Textures; }
    virtual const std::vector<float>& GetVertices() { return m_Vertices; }
    virtual const std::vector<uint16_t>& GetIndices() { return m_Indices; }
    virtual const std::vector<float>& GetUVs() { return m_UVs; }

    virtual void Create() = 0;
    virtual void Read(std::istream& file) = 0;

    virtual void Setup(RenderContext_t* context)
    {
        assert(m_VertexShader);
        assert(m_PixelShader);
        assert(m_VertexDeclaration);

        // enable the vertex and pixel shaders
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);
        context->m_DeviceContext->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
        context->m_DeviceContext->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

        // enable textures
        for (uint32_t i = 0; i < m_Textures.size(); ++i) {
            const auto& texture = m_Textures[i];
            if (texture && texture->IsLoaded()) {
                texture->Use(i);
            }
        }

        // set the vertex buffer
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer->m_Buffer, &m_VertexBuffer->m_ElementStride, &offset);

        // set the sampler state
        if (m_SamplerState) {
            context->m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState->m_SamplerState);
        }

        // set the topology
        context->m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    virtual void Draw(RenderContext_t* context)
    {
        if (m_IndexBuffer) {
            Renderer::Get()->DrawIndexed(0, m_IndexBuffer->m_ElementCount, m_IndexBuffer);
        }
        else {
            Renderer::Get()->Draw(0, m_VertexBuffer->m_ElementCount / 3);
        }
    }

    template <typename T>
    void ReadVertexBuffer(std::istream& stream, VertexBuffer_t** outBuffer, std::vector<T>* outVertices = nullptr)
    {
        auto stride = static_cast<uint32_t>(sizeof(T));

        uint32_t count;
        stream.read((char *)&count, sizeof(count));

        outVertices->resize(count);
        stream.read((char *)outVertices->data(), (count * stride));

        *outBuffer = Renderer::Get()->CreateVertexBuffer(outVertices->data(), count, stride, D3D11_USAGE_DEFAULT, "IRenderBlock Vertex Buffer");
    }

    void ReadIndexBuffer(std::istream& stream, IndexBuffer_t** outBuffer)
    {
        uint32_t count;
        stream.read((char *)&count, sizeof(count));

        m_Indices.resize(count);
        stream.read((char *)m_Indices.data(), (count * sizeof(uint16_t)));

        *outBuffer = Renderer::Get()->CreateIndexBuffer(m_Indices.data(), count, D3D11_USAGE_DEFAULT, "IRenderBlock Index Buffer");
    }

    void ReadMaterials(std::istream& stream)
    {
        uint32_t count;
        stream.read((char *)&count, sizeof(count));

        m_Materials.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t length;
            stream.read((char *)&length, sizeof(length));

            if (length == 0) {
                continue;
            }

            auto filename = std::unique_ptr<char[]>(new char[length + 1]);
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
        stream.read((char *)&unknown, sizeof(unknown));
    }

    void ReadSkinBatch(std::istream& stream)
    {
        uint32_t count;
        stream.read((char *)&count, sizeof(count));
        m_SkinBatches.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            JustCause3::CSkinBatch batch;

            stream.read((char *)&batch.m_Size, sizeof(batch.m_Size));
            stream.read((char *)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.read((char *)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            for (int32_t n = 0; n < batch.m_BatchSize; ++n) {
                int16_t unknown;
                stream.read((char *)&unknown, sizeof(unknown));
            }

            m_SkinBatches.emplace_back(batch);
        }
    }

    void DrawSkinBatches(RenderContext_t* context)
    {
        for (auto& batch : m_SkinBatches) {
            Renderer::Get()->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
        }
    }

    virtual void DrawUI() = 0;
};