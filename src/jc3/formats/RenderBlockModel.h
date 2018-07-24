#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#include <mutex>
#include <unordered_map>

class RenderBlockModel
{
private:
    std::recursive_mutex m_RenderBlocksMutex;
    std::vector<IRenderBlock*> m_RenderBlocks;

    fs::path m_Filename = "";
    std::string m_ReadBlocksError = "";

    glm::vec3 m_Position = { 0, 0, 0 };
    glm::vec3 m_Rotation = { 0, 0, 0 };
    glm::vec3 m_Scale = { 1, 1, 1 };
    glm::mat4x4 m_WorldMatrix;

    glm::vec3 m_BoundingBoxMin;
    glm::vec3 m_BoundingBoxMax;

public:
    //RenderBlockModel(const fs::path& file);
    RenderBlockModel(const fs::path& filename);
    virtual ~RenderBlockModel();

    static void FileReadCallback(const fs::path& filename, const FileBuffer& data);

    //bool ParseRenderBlockModel(std::istream& stream);
    bool Parse(const FileBuffer& data);

    void Draw(RenderContext_t* context);

    const std::vector<IRenderBlock*>& GetRenderBlocks() { return m_RenderBlocks; }

    std::string GetFileName() { return m_Filename.filename().string(); }
    std::string GetFilePath() { return m_Filename.parent_path().string(); }
    const fs::path& GetPath() { return m_Filename; }

    void SetPosition(const glm::vec3& position) { m_Position = position; }
    const glm::vec3& GetPosition() const { return m_Position; }

    void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }
    const glm::vec3& GetRotation() const { return m_Rotation; }

    void SetScale(const glm::vec3& scale) { m_Scale = scale; }
    const glm::vec3& GetScale() const { return m_Scale; }

    std::tuple<glm::vec3, glm::vec3> GetBoundingBox() { return { m_BoundingBoxMin, m_BoundingBoxMax }; }

    const std::string& GetError() const { return m_ReadBlocksError; }
};