#pragma once

#include <Factory.h>
#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>
#include <memory>
#include <mutex>

static constexpr auto RBM_END_OF_BLOCK = 0x89ABCDEF;

class AvalancheArchive;
class RuntimeContainer;
class RenderBlockModel : public Factory<RenderBlockModel>
{
  private:
    fs::path                          m_Filename = "";
    std::vector<IRenderBlock*>        m_RenderBlocks;
    std::shared_ptr<AvalancheArchive> m_ParentArchive;
    BoundingBox                       m_BoundingBox;

  public:
    RenderBlockModel(const fs::path& filename);
    virtual ~RenderBlockModel();

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    static void ReadFileCallback(const fs::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const fs::path& filename, const fs::path& directory);
    static void ContextMenuUI(const fs::path& filename);

    static void Load(const fs::path& filename);
    static void LoadFromRuntimeContainer(const fs::path& filename, std::shared_ptr<RuntimeContainer> rc);

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

    const fs::path& GetPath()
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
