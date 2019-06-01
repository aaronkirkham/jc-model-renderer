#pragma once

#include <filesystem>
#include <string>

#include "../../factory.h"
#include "../../graphics/types.h"

#include "../jc4/types.h"
#include "../types.h"

class AvalancheArchive;
class AvalancheDataFormat;
class AvalancheModelFormat;
class IRenderBlock;

#pragma pack(push, 8)
struct SAmfBoundingBox {
    float m_Min[3];
    float m_Max[3];
};

static_assert(sizeof(SAmfBoundingBox) == 0x18, "SAmfBoundingBox alignment is wrong!");

struct SAmfMaterial {
    uint32_t            m_Name;
    uint32_t            m_RenderBlockId;
    SAdfDeferredPtr     m_Attributes;
    SAdfArray<uint32_t> m_Textures;
};

struct SAmfModel {
    uint32_t                m_Mesh;
    SAdfArray<uint8_t>      m_LodSlots;
    uint32_t                m_MemoryTag;
    float                   m_LodFactor;
    SAdfArray<SAmfMaterial> m_Materials;
};

static_assert(sizeof(SAmfMaterial) == 0x28, "SAmfMaterial alignment is wrong!");
static_assert(sizeof(SAmfModel) == 0x30, "SAmfModel alignment is wrong!");

struct SAmfSubMesh {
    uint32_t        m_SubMeshId;
    uint32_t        m_IndexCount;
    uint32_t        m_IndexStreamOffset;
    SAmfBoundingBox m_BoundingBox;
};

struct SAmfStreamAttribute {
    jc4::EAmfUsage  m_Usage;
    jc4::EAmfFormat m_Format;
    uint8_t         m_StreamIndex;
    uint8_t         m_StreamOffset;
    uint8_t         m_StreamStride;
    uint8_t         m_PackingData[8];
};

struct SAmfMesh {
    uint32_t                       m_MeshTypeId;
    uint32_t                       m_IndexCount;
    uint32_t                       m_VertexCount;
    int8_t                         m_IndexBufferIndex;
    int8_t                         m_IndexBufferStride;
    uint32_t                       m_IndexBufferOffset;
    SAdfArray<int8_t>              m_VertexBufferIndices;
    SAdfArray<int8_t>              m_VertexStreamStrides;
    SAdfArray<int32_t>             m_VertexStreamOffsets;
    float                          m_TextureDensities[3];
    SAdfDeferredPtr                m_MeshProperties;
    SAdfArray<int16_t>             m_BoneIndexLookup;
    SAdfArray<SAmfSubMesh>         m_SubMeshes;
    SAdfArray<SAmfStreamAttribute> m_StreamAttributes;
};

static_assert(sizeof(SAmfSubMesh) == 0x24, "SAmfSubMesh alignment is wrong!");
static_assert(sizeof(SAmfStreamAttribute) == 0x14, "SAmfStreamAttribute alignment is wrong!");
static_assert(sizeof(SAmfMesh) == 0x98, "SAmfMesh alignment is wrong!");

struct SAmfLodGroup {
    uint32_t            m_LODIndex;
    SAdfArray<SAmfMesh> m_Meshes;
};

struct SAmfMeshHeader {
    SAmfBoundingBox         m_BoundingBox;
    uint32_t                m_MemoryTag;
    SAdfArray<SAmfLodGroup> m_LodGroups;
    uint32_t                m_HighLodPath;
};

static_assert(sizeof(SAmfLodGroup) == 0x18, "SAmfLodGroup alignment is wrong!");
static_assert(sizeof(SAmfMeshHeader) == 0x38, "SAmfMeshHeader alignment is wrong!");

struct SAmfBuffer {
    SAdfArray<int8_t> m_Data;
    int8_t            m_CreateSRV : 1;
};

struct SAmfMeshBuffers {
    uint32_t              m_MemoryTag;
    SAdfArray<SAmfBuffer> m_IndexBuffers;
    SAdfArray<SAmfBuffer> m_VertexBuffers;
};

static_assert(sizeof(SAmfBuffer) == 0x18, "SAmfBuffer alignment is wrong!");
static_assert(sizeof(SAmfMeshBuffers) == 0x28, "SAmfMeshBuffers alignment is wrong!");
#pragma pack(pop)

class AvalancheModelFormat : public Factory<AvalancheModelFormat>
{
    using ParseCallback_t = std::function<void(bool)>;

  private:
    std::filesystem::path                m_Filename = "";
    std::shared_ptr<AvalancheArchive>    m_ParentArchive;
    std::shared_ptr<AvalancheDataFormat> m_ModelAdf;
    std::shared_ptr<AvalancheDataFormat> m_MeshAdf;
    std::shared_ptr<AvalancheDataFormat> m_HighResMeshAdf;

    SAmfModel*       m_AmfModel        = nullptr;
    SAmfMeshHeader*  m_AmfMeshHeader   = nullptr;
    SAmfMeshBuffers* m_LowMeshBuffers  = nullptr;
    SAmfMeshBuffers* m_HighMeshBuffers = nullptr;

    std::vector<IRenderBlock*> m_RenderBlocks;
    BoundingBox                m_BoundingBox;

  public:
    AvalancheModelFormat(const std::filesystem::path& filename);
    virtual ~AvalancheModelFormat();

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    static void ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);
    static void ContextMenuUI(const std::filesystem::path& filename);

    static void Load(const std::filesystem::path& filename);

    void Parse(const FileBuffer& data, ParseCallback_t callback);
    bool ParseMeshBuffers();

    std::string GetFileName()
    {
        return m_Filename.filename().string();
    }

    std::string GetFilePath()
    {
        return m_Filename.parent_path().string();
    }

    const std::filesystem::path& GetPath()
    {
        return m_Filename;
    }

    AvalancheDataFormat* GetModelAdf()
    {
        return m_ModelAdf.get();
    }

    AvalancheDataFormat* GetMeshAdf()
    {
        return m_MeshAdf.get();
    }

    AvalancheDataFormat* GetHighResMeshAdf()
    {
        return m_HighResMeshAdf.get();
    }

    const std::vector<IRenderBlock*>& GetRenderBlocks()
    {
        return m_RenderBlocks;
    }

    BoundingBox* GetBoundingBox()
    {
        return &m_BoundingBox;
    }
};
