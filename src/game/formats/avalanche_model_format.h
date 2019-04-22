#pragma once

#include <filesystem>
#include <string>

#include "../../factory.h"
#include "../../graphics/types.h"

class AvalancheArchive;
class AvalancheDataFormat;

class AdfInstanceMemberInfo;
class AvalancheModelFormat;
class IRenderBlock;
class AMFMesh
{
  private:
    AdfInstanceMemberInfo* m_Info          = nullptr;
    AvalancheModelFormat*  m_Parent        = nullptr;
    std::string            m_Name          = "";
    std::string            m_RenderBlockId = "";
    IRenderBlock*          m_RenderBlock   = nullptr;

  public:
    AMFMesh(AdfInstanceMemberInfo* info, AvalancheModelFormat* parent);
    virtual ~AMFMesh();

    void LoadBuffers(AdfInstanceMemberInfo* info, FileBuffer* vertices, FileBuffer* indices);

    const std::string& GetName()
    {
        return m_Name;
    }

    IRenderBlock* GetRenderBlock()
    {
        return m_RenderBlock;
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
};
