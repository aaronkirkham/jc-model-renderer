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

    std::vector<uint16_t> m_Indices;

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
    //virtual const std::vector<uint8_t>& GetVertices() = 0;
    virtual const std::vector<uint16_t>& GetIndices() { return m_Indices; }

    virtual void Create() = 0;
    virtual void Read(fs::path& filename, std::istream& file) = 0;

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
            auto texture = m_Textures[i];
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
    void ReadVertexBuffer(std::istream& stream, VertexBuffer_t** outBuffer)
    {
        auto stride = static_cast<uint32_t>(sizeof(T));

        uint32_t count;
        stream.read((char *)&count, sizeof(count));

        std::vector<T> vertices;
        vertices.resize(count);
        stream.read((char *)vertices.data(), (count * stride));

        *outBuffer = Renderer::Get()->CreateVertexBuffer(vertices.data(), count, stride);
    }

    void ReadIndexBuffer(std::istream& stream, IndexBuffer_t** outBuffer)
    {
        uint32_t count;
        stream.read((char *)&count, sizeof(count));

        m_Indices.resize(count);
        stream.read((char *)m_Indices.data(), (count * sizeof(uint16_t)));

        *outBuffer = Renderer::Get()->CreateIndexBuffer(m_Indices.data(), count);
    }

    void ReadMaterials(fs::path& filename, std::istream& stream)
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

            auto buffer = new char[length + 1];
            stream.read(buffer, length);
            buffer[length] = '\0';

            m_Materials.emplace_back(buffer);

            // load the material
            auto texture = TextureManager::Get()->GetTexture(buffer);
            if (texture) {
                m_Textures.emplace_back(texture);
            }

            delete[] buffer;
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

            stream.read((char *)&batch.size, sizeof(batch.size));
            stream.read((char *)&batch.offset, sizeof(batch.offset));
            stream.read((char *)&batch.batchSize, sizeof(batch.batchSize));

            for (int32_t n = 0; n < batch.batchSize; ++n) {
                int16_t unknown;
                stream.read((char *)&unknown, sizeof(unknown));
            }

            m_SkinBatches.emplace_back(batch);
        }
    }

    void DrawSkinBatches(RenderContext_t* context)
    {
        for (auto& batch : m_SkinBatches) {
            Renderer::Get()->DrawIndexed(batch.offset, batch.size, m_IndexBuffer);
        }
    }

    virtual void DrawUI() = 0;
};