#include <engine/formats/RenderBlockModel.h>
#include <engine/RenderBlockFactory.h>
#include <Window.h>
#include <engine/RenderBlockFactory.h>
#include <graphics/Renderer.h>
#include <graphics/Camera.h>
#include <graphics/Renderer2D.h>

// TODO: need to add some render things like the worldViewProj buffer
// so we can render multiple render blocks at the same position/rotation

RenderBlockModel::RenderBlockModel(const fs::path& file)
    : m_File(file)
{
    // make sure this is an rbm file
    if (file.extension() != ".rbm") {
        throw std::invalid_argument("RenderBlockModel type only supports .rbm files!");
    }

    // try and open the file
    std::ifstream fh(m_File, std::ios::binary);
    if (fh.fail()) {
        throw std::runtime_error("Failed to read RenderBlockModel file!");
    }

    // read the model header
    JustCause3::RBM header;
    fh.read((char *)&header, sizeof(header));

    Window::Get()->DebugString("RBMDL v%d.%d.%d", header.m_VersionMajor, header.m_VersionMinor, header.m_VersionRevision);
    Window::Get()->DebugString(" - m_NumberOfBlocks = %d", header.m_NumberOfBlocks);

    m_RenderBlocks.reserve(header.m_NumberOfBlocks);

    // store the bounding box
    m_BoundingBoxMin = header.m_BoundingBoxMin;
    m_BoundingBoxMax = header.m_BoundingBoxMax;

    // read all the render blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; i++)
    {
        uint32_t hash;
        fh.read((char *)&hash, sizeof(hash));

        auto renderBlock = RenderBlockFactory::CreateRenderBlock(hash);
        if (renderBlock) {
            renderBlock->Create();
            renderBlock->Read(m_File, fh);

            uint32_t checksum;
            fh.read((char *)&checksum, sizeof(checksum));

            // did we read the block correctly?
            if (checksum != 0x89ABCDEF) {
                throw std::runtime_error("Failed to read render block.");
            }

            m_RenderBlocks.emplace_back(renderBlock);

            Window::Get()->DebugString("Finished reading render block...\n\n");
        }
        else {
            Window::Get()->DebugString("[ERROR] Unknown Render Block hash 0x%p!", hash);
            break;
        }
    }

    Window::Get()->DebugString("ReadNativeFormat complete!\n");
    fh.close();

    // create the mesh constant buffer
    MeshConstants constants;
    m_MeshConstants = Renderer::Get()->CreateConstantBuffer(constants);
}

RenderBlockModel::~RenderBlockModel()
{
    for (auto& renderBlock : m_RenderBlocks) {
        delete renderBlock;
    }

    Renderer::Get()->DestroyBuffer(m_MeshConstants);
}

void RenderBlockModel::Draw(RenderContext_t* context)
{
    m_Rotation.z += 0.015f;

    // calculate the world matrix
    {
        auto roll = glm::rotate({}, m_Rotation.x, glm::vec3{ 0.0f, 0.0f, 1.0f });
        auto pitch = glm::rotate({}, m_Rotation.y, glm::vec3{ 1.0f, 0.0f, 0.0f });
        auto yaw = glm::rotate({}, m_Rotation.z, glm::vec3{ 0.0f, 1.0f, 0.0f });

        m_WorldMatrix = glm::translate({}, m_Position) * glm::scale({}, m_Scale) * (roll * pitch * yaw);
    }

    // update mesh matrix
    {
        auto worldView = m_WorldMatrix * Camera::Get()->GetViewMatrix();

        MeshConstants constants;
        constants.worldMatrix = m_WorldMatrix;
        constants.worldViewInverseTranspose = glm::transpose(glm::inverse(worldView));
        constants.colour = glm::vec4{ 1, 1, 1, 1 };

        Renderer::Get()->SetVertexShaderConstants(m_MeshConstants, 1, constants);
    }

    for (auto& renderBlock : m_RenderBlocks) {
        Renderer::Get()->SetDefaultRenderStates();

        renderBlock->Setup(context);
        renderBlock->Draw(context);
    }

#if 0
    glm::vec3 screen;
    if (Camera::Get()->WorldToScreen(m_Position, &screen)) {
        float size = 100.0f;
        screen -= (size * 0.5f);

        Renderer2D::Get()->DrawRect({ screen.x, screen.y }, { size, size }, { 1, 0, 0, 1 });
    }
#endif
}