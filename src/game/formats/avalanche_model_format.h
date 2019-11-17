#pragma once

#include "factory.h"

#include <filesystem>
#include <string>

namespace ava
{
namespace AvalancheDataFormat
{
    class ADF;
};
namespace AvalancheModelFormat
{
    struct SAmfModel;
    struct SAmfMeshHeader;
    struct SAmfMeshBuffers;
}; // namespace AvalancheModelFormat
}; // namespace ava

class AvalancheArchive;
class IRenderBlock;
class AvalancheModelFormat : public Factory<AvalancheModelFormat>
{
    using ParseCallback_t = std::function<void(bool)>;

  private:
    std::filesystem::path                          m_Filename = "";
    std::shared_ptr<AvalancheArchive>              m_ParentArchive;
    std::shared_ptr<ava::AvalancheDataFormat::ADF> m_ModelAdf;
    std::shared_ptr<ava::AvalancheDataFormat::ADF> m_MeshAdf;
    std::shared_ptr<ava::AvalancheDataFormat::ADF> m_HighResMeshAdf;

    ava::AvalancheModelFormat::SAmfModel*       m_AmfModel        = nullptr;
    ava::AvalancheModelFormat::SAmfMeshHeader*  m_AmfMeshHeader   = nullptr;
    ava::AvalancheModelFormat::SAmfMeshBuffers* m_LowMeshBuffers  = nullptr;
    ava::AvalancheModelFormat::SAmfMeshBuffers* m_HighMeshBuffers = nullptr;

    std::vector<IRenderBlock*> m_RenderBlocks;
    // BoundingBox                m_BoundingBox;

  public:
    AvalancheModelFormat(const std::filesystem::path& filename);
    virtual ~AvalancheModelFormat();

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    static void ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                 bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);
    static void ContextMenuUI(const std::filesystem::path& filename);

    static void Load(const std::filesystem::path& filename);

    void Parse(const std::vector<uint8_t>& data, ParseCallback_t callback);
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

    ava::AvalancheDataFormat::ADF* GetModelAdf()
    {
        return m_ModelAdf.get();
    }

    ava::AvalancheDataFormat::ADF* GetMeshAdf()
    {
        return m_MeshAdf.get();
    }

    ava::AvalancheDataFormat::ADF* GetHighResMeshAdf()
    {
        return m_HighResMeshAdf.get();
    }

    const std::vector<IRenderBlock*>& GetRenderBlocks()
    {
        return m_RenderBlocks;
    }

    /*BoundingBox* GetBoundingBox()
    {
        return &m_BoundingBox;
    }*/
};
