#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct LandmarkAttributes {
    float                        depthBias;
    float                        specularGloss;
    float                        reflectivity;
    float                        emmissive;
    float                        diffuseWrap;
    float                        specularFresnel;
    glm::vec4                    diffuseModulator;
    float                        backLight;
    float                        _unknown2;
    float                        startFadeOutDistanceEmissiveSq;
    jc::Vertex::SPackedAttribute packed;
    uint32_t                     flags;
};
namespace jc::RenderBlocks
{
static constexpr uint8_t LANDMARK_VERSION = 7;

struct Landmark {
    uint8_t            version;
    LandmarkAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockLandmark : public IRenderBlock
{
  private:
    struct cbVertexInstanceConsts {
        glm::mat4 worldViewProjection;
        glm::mat4 world;
        uint32_t  unknown[4] = {0xFFFFFFFF};
        glm::vec4 colour;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        float     DepthBias;
        float     EmissiveStartFadeDistSq;
        glm::vec2 UVScale;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        float     SpecularGloss;
        float     Reflectivity;
        float     Emissive;
        float     SpecularFresnel;
        glm::vec3 DiffuseModulator;
        float     DiffuseWrap;
        float     BackLight;
    } m_cbFragmentMaterialConsts;

    struct cbFragmentInstanceConsts {
        glm::vec4 DebugColor;
    } m_cbFragmentInstanceConsts;

    jc::RenderBlocks::Landmark       m_Block;
    VertexBuffer_t*                  m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = {nullptr};

  public:
    RenderBlockLandmark() = default;
    virtual ~RenderBlockLandmark();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockLandmark";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return true;
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::LANDMARK_VERSION) {
            __debugbreak();
        }

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffer
        m_VertexBuffer     = ReadVertexBuffer<PackedVertexPosition>(stream);
        m_VertexBufferData = ReadVertexBuffer<PackedTexWithSomething>(stream);

        // read the index buffer
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
        WriteBuffer(stream, m_VertexBufferData);

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;

    virtual void DrawContextMenu() override final;
    virtual void DrawUI() override final;

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        return {};
    }
};
} // namespace jc3
