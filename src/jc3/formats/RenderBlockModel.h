#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>
#include <mutex>
#include <memory>
#include <Factory.h>

class AvalancheArchive;
class RuntimeContainer;
class RenderBlockModel : public Factory<RenderBlockModel>
{
private:
    fs::path m_Filename = "";

    std::recursive_mutex m_RenderBlocksMutex;
    std::vector<IRenderBlock*> m_RenderBlocks;

    std::shared_ptr<AvalancheArchive> m_ParentArchive;

    glm::vec3 m_BoundingBoxMin;
    glm::vec3 m_BoundingBoxMax;

public:
    RenderBlockModel(const fs::path& filename);
    virtual ~RenderBlockModel();

    virtual std::string GetFactoryKey() const { return m_Filename.string(); }

    static void FileReadCallback(const fs::path& filename, const FileBuffer& data);
    static void Load(const fs::path& filename);
    static void LoadFromRuntimeContainer(const fs::path& filename, std::shared_ptr<RuntimeContainer> rc);

    bool Parse(const FileBuffer& data);
    inline static bool LoadingFromRC = false;
    inline static std::vector<std::string> SuppressedWarnings;

    void Draw(RenderContext_t* context);

    const std::vector<IRenderBlock*>& GetRenderBlocks() { return m_RenderBlocks; }

    std::string GetFileName() { return m_Filename.filename().string(); }
    std::string GetFilePath() { return m_Filename.parent_path().string(); }
    const fs::path& GetPath() { return m_Filename; }

    std::tuple<glm::vec3, glm::vec3> GetBoundingBox() { return { m_BoundingBoxMin, m_BoundingBoxMax }; }

    AvalancheArchive* GetParentArchive() { return m_ParentArchive.get(); }
};