#pragma once

#include <filesystem>

#include "../../factory.h"
#include "../../graphics/types.h"

static constexpr auto RBM_END_OF_BLOCK = 0x89ABCDEF;

class AvalancheArchive;
class RuntimeContainer;
class IRenderBlock;

class RenderBlockModel : public Factory<RenderBlockModel>
{
  private:
    std::filesystem::path             m_Filename = "";
    std::vector<IRenderBlock*>        m_RenderBlocks;
    std::shared_ptr<AvalancheArchive> m_ParentArchive;
    BoundingBox                       m_BoundingBox;

  public:
    RenderBlockModel(const std::filesystem::path& filename);
    virtual ~RenderBlockModel();

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    static void ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);
    static void ContextMenuUI(const std::filesystem::path& filename);

    static void Load(const std::filesystem::path& filename);
    static void LoadFromRuntimeContainer(const std::filesystem::path& filename, std::shared_ptr<RuntimeContainer> rc);

    bool                                   Parse(const FileBuffer& data, bool add_to_render_list = true);
    inline static bool                     LoadingFromRC = false;
    inline static std::vector<std::string> SuppressedWarnings;

    void DrawGizmos();

    std::vector<IRenderBlock*>& GetRenderBlocks()
    {
        return m_RenderBlocks;
    }

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

    BoundingBox* GetBoundingBox()
    {
        return &m_BoundingBox;
    }

    AvalancheArchive* GetParentArchive()
    {
        return m_ParentArchive.get();
    }
};
