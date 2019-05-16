#pragma once

#include <filesystem>
#include <string>

#include "../../factory.h"
#include "../../graphics/types.h"

#include "../types.h"

class AvalancheArchive;
class AvalancheDataFormat;
class AvalancheModelFormat;
class IRenderBlock;

#pragma pack(push, 8)
struct SAmfMaterial {
    uint32_t           m_Name;
    uint32_t           m_RenderBlockId;
    SAdfDeferredPtr    m_Attributes;
    AdfArray<uint32_t> m_Textures;
};

struct SAmfModel {
    uint32_t               m_Mesh;
    AdfArray<uint8_t>      m_LodSlots;
    uint32_t               m_MemoryTag;
    float                  m_LodFactor;
    AdfArray<SAmfMaterial> m_Materials;
};

static_assert(sizeof(SAmfMaterial) == 0x28, "SAmfMaterial alignment is wrong!");
static_assert(sizeof(SAmfModel) == 0x30, "SAmfModel alignment is wrong!");
#pragma pack(pop)

class AMFMesh
{
  private:
    AvalancheModelFormat* m_Parent        = nullptr;
    std::string           m_Name          = "";
    std::string           m_RenderBlockId = "";
    IRenderBlock*         m_RenderBlock   = nullptr;
    BoundingBox           m_BoundingBox;

  public:
    AMFMesh(AvalancheModelFormat* parent);
    virtual ~AMFMesh();

    void InitBuffers(FileBuffer* vertices, FileBuffer* indices);

    const std::string& GetName()
    {
        return m_Name;
    }

    IRenderBlock* GetRenderBlock()
    {
        return m_RenderBlock;
    }

    void SetBoundingBox(const BoundingBox& box)
    {
        m_BoundingBox = box;
    }

    BoundingBox* GetBoundingBox()
    {
        return &m_BoundingBox;
    }
};

class AvalancheModelFormat : public Factory<AvalancheModelFormat>
{
    using ParseCallback_t = std::function<void(bool)>;

  private:
    std::filesystem::path                m_Filename = "";
    std::shared_ptr<AvalancheArchive>    m_ParentArchive;
    std::shared_ptr<AvalancheDataFormat> m_ModelAdf;
    std::shared_ptr<AvalancheDataFormat> m_MeshAdf;
    std::shared_ptr<AvalancheDataFormat> m_HighResMeshAdf;

    std::vector<std::unique_ptr<AMFMesh>> m_Meshes;
    BoundingBox                           m_BoundingBox;

  public:
    AvalancheModelFormat(const std::filesystem::path& filename);
    virtual ~AvalancheModelFormat() = default;

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

    const std::vector<std::unique_ptr<AMFMesh>>& GetMeshes()
    {
        return m_Meshes;
    }

    std::vector<IRenderBlock*> GetRenderBlocks()
    {
        std::vector<IRenderBlock*> result;
        result.reserve(m_Meshes.size());

        for (const auto& mesh : m_Meshes) {
            result.emplace_back(mesh->GetRenderBlock());
        }

        return result;
    }

    BoundingBox* GetBoundingBox()
    {
        return &m_BoundingBox;
    }
};
