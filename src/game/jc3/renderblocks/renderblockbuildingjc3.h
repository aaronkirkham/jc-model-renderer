#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct BuildingJC3Attributes {
    char                         pad[0x68];
    jc::Vertex::SPackedAttribute packed;
    char                         pad2[0x1C];
};

namespace jc::RenderBlocks
{
struct BuildingJC3 {
    uint8_t               version;
    BuildingJC3Attributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockBuildingJC3 : public IRenderBlock
{
  private:
    jc::RenderBlocks::BuildingJC3 m_Block;
    VertexBuffer_t*               m_VertexBufferData = nullptr;

  public:
    RenderBlockBuildingJC3() = default;
    virtual ~RenderBlockBuildingJC3();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockBuildingJC3";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return true;
        // return ~(m_Block.attributes.flags >> 1) & 1;
    }

    virtual void Create() override final
    {
#if 0
        // load shaders
        m_VertexShader = GET_VERTEX_SHADER(general_jc3);
        m_PixelShader = GET_PIXEL_SHADER(general_jc3);

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R16G16B16A16_SINT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockBuildingJC3");

        // create the constant buffer
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_Constants, "RenderBlockBuildingJC3");
#endif
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block header
        stream.read((char*)&m_Block, sizeof(m_Block));

        // set vertex shader constants
        // m_Constants.m_Scale = m_Block.attributes.packed.scale;
        // m_Constants.m_UVExtent = m_Block.attributes.packed.uv0Extent;

        // read the materials
        ReadMaterials(stream);

#if 0
        // read the vertex buffers
        if (m_Block.attributes.packed.format == 1) {
            std::vector<PackedVertexPosition> vertices;
            ReadVertexBuffer<PackedVertexPosition>(stream, &m_VertexBuffer);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(unpack(vertex.x) * m_Block.attributes.packed.scale);
                m_Vertices.emplace_back(unpack(vertex.y) * m_Block.attributes.packed.scale);
                m_Vertices.emplace_back(unpack(vertex.z) * m_Block.attributes.packed.scale);
            }

            std::vector<GeneralShortPacked> vertices_data;
            ReadVertexBuffer<GeneralShortPacked>(stream, &m_VertexBufferData);

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(unpack(data.u0) * m_Block.attributes.packed.uv0Extent.x);
                m_UVs.emplace_back(unpack(data.v0) * m_Block.attributes.packed.uv0Extent.y);
            }
        } else {
            // ReadVertexBuffer<Unpacked>(stream, &m_Vertices);
            __debugbreak();
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
#endif
    }

    virtual void Write(std::ostream& stream) override final
    {
        //
    }

    virtual void Setup(RenderContext_t* context) override final;
	// Draw

    virtual void DrawContextMenu() override final {}
    virtual void DrawUI() override final {}

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
