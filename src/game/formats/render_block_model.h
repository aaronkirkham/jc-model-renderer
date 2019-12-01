#pragma once

#include "factory.h"

#include "graphics/types.h"

#include <filesystem>

class AvalancheArchive;
class RuntimeContainer;
class AvalancheDataFormat;
class IRenderBlock;

class RenderBlockModel : public Factory<RenderBlockModel>
{
    using ParseCallback_t = std::function<void(bool)>;

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

    static void ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                 bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);
    static void ContextMenuUI(const std::filesystem::path& filename);

    static void Load(const std::filesystem::path& filename);
    static void LoadFromRuntimeContainer(const std::filesystem::path& filename, std::shared_ptr<RuntimeContainer> rc);

    bool ParseLOD(const std::vector<uint8_t>& buffer);
    bool ParseRBM(const std::vector<uint8_t>& buffer, bool add_to_render_list = true);

    void UpdateBoundingBoxScale();
    void RecalculateBoundingBox();

    inline static bool                     LoadingFromRuntimeContainer = false;
    inline static std::vector<std::string> SuppressedWarnings;

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

    const BoundingBox& GetBoundingBox()
    {
        return m_BoundingBox;
    }

    AvalancheArchive* GetParentArchive()
    {
        return m_ParentArchive.get();
    }
};
