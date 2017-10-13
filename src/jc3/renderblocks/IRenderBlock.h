#pragma once

#include <jc3/Types.h>
#include <graphics/ShaderManager.h>
#include <graphics/TextureManager.h>
#include <graphics/Types.h>
#include <graphics/Renderer.h>

class IRenderBlock
{
protected:
    struct ModelConstants
    {
        float scale = 1.0f;
        glm::vec2 uvExtent = { 0, 0 };
        BOOL hasNormalMap = FALSE;
    } m_Constants;

    VertexBuffer_t* m_Vertices = nullptr;
    IndexBuffer_t* m_Indices = nullptr;
    ConstantBuffer_t* m_ConstantBuffer = nullptr;
    std::shared_ptr<VertexShader_t> m_VertexShader = nullptr;
    std::shared_ptr<PixelShader_t> m_PixelShader = nullptr;

    std::vector<fs::path> m_Materials;
    std::vector<std::shared_ptr<Texture>> m_Textures;
    std::vector<JustCause3::CSkinBatch> m_SkinBatches;

public:
    IRenderBlock()
    {
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_Constants);
    }

    virtual ~IRenderBlock()
    {
        OutputDebugStringA("~IRenderBlock\n");

        m_VertexShader = nullptr;
        m_PixelShader = nullptr;

        Renderer::Get()->DestroyBuffer(m_Vertices);
        Renderer::Get()->DestroyBuffer(m_Indices);
        Renderer::Get()->DestroyBuffer(m_ConstantBuffer);
    }

    virtual const char* GetTypeName() = 0;

    virtual void Create() = 0;
    virtual void Read(fs::path& filename, std::istream& file) = 0;
    virtual void Setup(RenderContext_t* context) = 0;

    virtual void Draw(RenderContext_t* context)
    {
        Renderer::Get()->SetVertexShaderConstants(m_ConstantBuffer, 2, m_Constants);
        Renderer::Get()->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);
    }

    virtual VertexBuffer_t* GetVertexBuffer() { return m_Vertices; }
    virtual IndexBuffer_t* GetIndexBuffer() { return m_Indices; }

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

        std::vector<uint16_t> indices;
        indices.resize(count);
        stream.read((char *)indices.data(), (count * sizeof(uint16_t)));

        *outBuffer = Renderer::Get()->CreateIndexBuffer(indices.data(), count);
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
            auto texture = TextureManager::Get()->GetTexture(filename.parent_path() / "textures" / fs::path(buffer).filename());
            if (texture && (texture->IsLoaded() || (!texture->IsLoaded() && texture->LoadFromFile()))) {
                m_Textures.emplace_back(texture);
            }

            delete[] buffer;
        }

        uint32_t unknown[4];
        stream.read((char *)&unknown, sizeof(unknown));

        // flush the texture cache
        TextureManager::Get()->Flush();
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
};