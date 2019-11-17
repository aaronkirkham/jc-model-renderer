#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct WindowAttributes {
    union {
        struct {
            float SpecGloss;
            float SpecFresnel;
            float DiffuseRougness;
            float TintPower;
            float MinAlpha;
            float UVScale;
        };

        float data[6];
    };

    glm::vec3 _unknown;
    uint32_t  flags;
};

static_assert(sizeof(WindowAttributes) == 0x28, "WindowAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t WINDOW_VERSION = 1;

struct Window {
    uint8_t          version;
    WindowAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockWindow : public IRenderBlock
{
  private:
    enum {
        ENABLE_BACKFACE_CULLING = 0x1,
        SIMPLE                  = 0x2,
    };

    struct cbInstanceConsts {
        glm::mat4 World;
        glm::mat4 WorldViewProjection;
    } m_cbInstanceConsts;

    struct cbMaterialConsts {
        float     SpecGloss;
        float     SpecFresnel;
        float     DiffuseRougness;
        float     TintPower;
        float     MinAlpha;
        float     UVScale;
        glm::vec2 unused_;                 // [unused]
        float     DamageMaskWeight = 1.0f; // [unused]
        float     DamageRampWeight = 0.5f; // [unused]
        float     Alpha            = 1.0f;
    } m_cbMaterialConsts;

    jc::RenderBlocks::Window m_Block;
    ConstantBuffer_t*        m_VertexShaderConstants   = nullptr;
    ConstantBuffer_t*        m_FragmentShaderConstants = nullptr;

    SamplerState_t* _test = nullptr;

  public:
    RenderBlockWindow() = default;
    virtual ~RenderBlockWindow();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockWindow";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return false;
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::WINDOW_VERSION) {
            __debugbreak();
        }

        // read materials
        ReadMaterials(stream);

        // read vertex buffer
        m_VertexBuffer = ReadVertexBuffer<UnpackedVertexPosition2UV>(stream);

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;
    virtual void Draw(RenderContext_t* context) override final;

    virtual void DrawContextMenu() override final;
    virtual void DrawUI() override final;

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPosition2UV>();
        vertices.reserve(vb.size());

        for (const auto& vertex : vb) {
            vertex_t v;
            v.pos    = {vertex.x, vertex.y, vertex.z};
            v.uv     = {vertex.u0, vertex.v0};
            v.normal = unpack_normal(vertex.n);
            vertices.emplace_back(std::move(v));
        }

        return {vertices, indices};
    }
};
} // namespace jc3
