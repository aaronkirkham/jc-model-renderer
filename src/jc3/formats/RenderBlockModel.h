#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#include <mutex>

class RenderBlockModel
{
private:
    std::vector<IRenderBlock*> m_RenderBlocks;
    fs::path m_File = "";

    glm::vec3 m_Position = { 0, 0, 0 };
    glm::vec3 m_Rotation = { 0, 0, 0 };
    glm::vec3 m_Scale = { 1, 1, 1 };
    glm::mat4x4 m_WorldMatrix;

    glm::vec3 m_BoundingBoxMin;
    glm::vec3 m_BoundingBoxMax;

    ConstantBuffer_t* m_MeshConstants = nullptr;

    void ParseRenderBlockModel(std::istream& stream);

public:
    struct MeshConstants
    {
        glm::mat4 worldMatrix;
        glm::mat4 worldViewInverseTranspose;
        glm::vec4 colour;
    };

    RenderBlockModel(const fs::path& file);
    RenderBlockModel(const fs::path& filename, const std::vector<uint8_t>& buffer);
    virtual ~RenderBlockModel();

    void Draw(RenderContext_t* context);

    const std::vector<IRenderBlock*>& GetRenderBlocks() { return m_RenderBlocks; }

    const std::string& GetFileName() { return m_File.filename().string(); }
    const std::string& GetFilePath() { return m_File.parent_path().string(); }
    const fs::path& GetPath() { return m_File; }

    void SetPosition(const glm::vec3& position) { m_Position = position; }
    const glm::vec3& GetPosition() const { return m_Position; }

    void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }
    const glm::vec3& GetRotation() const { return m_Rotation; }

    void SetScale(const glm::vec3& scale) { m_Scale = scale; }
    const glm::vec3& GetScale() const { return m_Scale; }
};