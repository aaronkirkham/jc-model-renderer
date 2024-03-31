#include <AvaFormatLib.h>
#include <spdlog/spdlog.h>

#include "avalanche_model_format.h"

#include "graphics/renderer.h"
#include "graphics/texture_manager.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"
#include "game/jc4/renderblocks/irenderblock.h"
#include "game/render_block_factory.h"

std::recursive_mutex                                      Factory<AvalancheModelFormat>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheModelFormat>> Factory<AvalancheModelFormat>::Instances;

AvalancheModelFormat::AvalancheModelFormat(const std::filesystem::path& filename)
    : m_Filename(filename)
{
    // find the parent archive
    std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
    for (const auto& archive : AvalancheArchive::Instances) {
        if (archive.second->HasFile(filename)) {
            m_ParentArchive = archive.second;
            break;
        }
    }
}

AvalancheModelFormat::~AvalancheModelFormat()
{
    // remove from renderlist
    Renderer::Get()->RemoveFromRenderList(m_RenderBlocks);

    for (auto& render_block : m_RenderBlocks) {
        SAFE_DELETE(render_block);
    }

    SAFE_DELETE(m_AmfModel);
    SAFE_DELETE(m_AmfMeshHeader);
    SAFE_DELETE(m_LowMeshBuffers);
    SAFE_DELETE(m_HighMeshBuffers);
    SAFE_DELETE(m_ModelAdf);
    SAFE_DELETE(m_MeshAdf);
    SAFE_DELETE(m_HRMeshAdf);
}

void AvalancheModelFormat::ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                            bool external)
{
    if (!AvalancheModelFormat::exists(filename.string())) {
        if (const auto amf = AvalancheModelFormat::make(filename)) {
            amf->Parse(data, [](bool) {});
        }
    }
}

bool AvalancheModelFormat::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    SPDLOG_ERROR("NOT IMPLEMENTED!");
    return false;
}

void AvalancheModelFormat::ContextMenuUI(const std::filesystem::path& filename) {}

void AvalancheModelFormat::Load(const std::filesystem::path& filename)
{
    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, std::vector<uint8_t> data) {
        if (success) {
            AvalancheModelFormat::ReadFileCallback(filename, std::move(data), false);
        } else {
            SPDLOG_INFO("Failed to read model \"{}\"", filename.generic_string());
        }
    });
}

void AvalancheModelFormat::Parse(const std::vector<uint8_t>& data, ParseCallback_t callback)
{
    // TODO: we ideally want to be able to open all 3 types. (.modelc, .meshc, .hrmeshc)
    // using 1 format. right now we're just assuming the type is modelc.

    // NOTE:
    //	.modelc		= contains all model information (textures, mesh files...)
    //	.meshc		= contains all mesh information, and low resolution buffers
    //	.hrmeshc	= only high resolution buffers

    using namespace ava::AvalancheDataFormat;

    std::thread([&, data, callback] {
        ava::AvalancheModelFormat::ParseModelc(data, &m_ModelAdf, &m_AmfModel);

        // create render blocks
        for (uint32_t i = 0; i < m_AmfModel->m_Materials.m_Count; ++i) {
            auto& material = m_AmfModel->m_Materials.m_Data[i];

            const auto name = m_ModelAdf->HashLookup(material.m_Name);
            // const auto render_block_id = m_ModelAdf->HashLookup(material.m_RenderBlockId);

            // const auto render_block = RenderBlockFactory::CreateRenderBlock(render_block_id, true);
            const auto render_block = RenderBlockFactory::CreateRenderBlock(material.m_RenderBlockId, true);
            assert(render_block != nullptr);

            // load textures
            render_block->AllocateTextureSlots(material.m_Textures.m_Count);
            for (uint32_t x = 0; x < material.m_Textures.m_Count; ++x) {
                auto texture = TextureManager::Get()->GetTexture(m_ModelAdf->HashLookup(material.m_Textures.m_Data[x]));
                render_block->SetTexture(x, texture);
            }

            // load constants
            ((jc4::IRenderBlock*)render_block)->m_Name = name;
            ((jc4::IRenderBlock*)render_block)->Load(&material.m_Attributes);

            m_RenderBlocks.emplace_back(std::move(render_block));
        }

        const auto mesh_filename = m_ModelAdf->HashLookup(m_AmfModel->m_Mesh);
        FileLoader::Get()->ReadFile(
            mesh_filename, [&, callback, mesh_filename](bool success, std::vector<uint8_t> data) {
                if (success) {
                    ava::AvalancheModelFormat::ParseMeshc(data, &m_MeshAdf, &m_AmfMeshHeader, &m_LowMeshBuffers);

                    m_BoundingBox = {m_AmfMeshHeader->m_BoundingBox.m_Min, m_AmfMeshHeader->m_BoundingBox.m_Max};

                    // get the high LOD mesh name (remove the leading "intermediate" path)
                    auto hrmesh_filename = std::string(m_MeshAdf->HashLookup(m_AmfMeshHeader->m_HighLodPath));
                    if (hrmesh_filename.find_first_of("intermediate/") == 0) {
                        hrmesh_filename = hrmesh_filename.erase(0, 13);
                    }

                    // read the high resolution mesh file
                    FileLoader::Get()->ReadFile(
                        hrmesh_filename, [&, callback, hrmesh_filename](bool success, std::vector<uint8_t> data) {
                            if (success) {
                                SPDLOG_INFO("Will use high resolution mesh! ({})", hrmesh_filename);

                                ava::AvalancheModelFormat::ParseHrmeshc(data, &m_HRMeshAdf, &m_HighMeshBuffers);
                            } else {
                                SPDLOG_INFO("No high resolution mesh available. Using low resolution instead. ({})",
                                            hrmesh_filename);
                            }

                            return callback(ParseMeshBuffers());
                        });
                }
            });
    }).detach();
}

bool AvalancheModelFormat::ParseMeshBuffers()
{
    if (!m_AmfModel || !m_AmfMeshHeader) {
        SPDLOG_ERROR("Can't parse mesh buffers without loading model/mesh/hrmeshc first.");
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    using namespace ava::AvalancheDataFormat;
    using namespace ava::AvalancheModelFormat;

    // TODO: something better for this.
    std::vector<SAmfBuffer*> vertex_buffers;
    std::vector<SAmfBuffer*> index_buffers;
    {
        static const auto CopyBuffer = [](SAdfArray<SAmfBuffer>& buffers, std::vector<SAmfBuffer*>* to) {
            for (uint32_t i = 0; i < buffers.m_Count; ++i) {
                to->emplace_back(&buffers.m_Data[i]);
            }
        };

        CopyBuffer(m_LowMeshBuffers->m_VertexBuffers, &vertex_buffers);
        CopyBuffer(m_HighMeshBuffers->m_VertexBuffers, &vertex_buffers);
        CopyBuffer(m_LowMeshBuffers->m_IndexBuffers, &index_buffers);
        CopyBuffer(m_HighMeshBuffers->m_IndexBuffers, &index_buffers);
    }

    // get the highest resolution LOD available
    const auto& lod_group = m_AmfMeshHeader->m_LodGroups.m_Data[m_AmfMeshHeader->m_LodGroups.m_Count - 1];
    for (uint32_t i = 0; i < lod_group.m_Meshes.m_Count; ++i) {
        const auto& mesh = lod_group.m_Meshes.m_Data[i];

        const auto mesh_type_id = m_MeshAdf->HashLookup(mesh.m_MeshTypeId);
        const auto sub_mesh_id  = m_MeshAdf->HashLookup(mesh.m_SubMeshes.m_Data[0].m_SubMeshId);

#ifdef DEBUG
        SPDLOG_INFO("Mesh: {} ({})", sub_mesh_id, mesh_type_id);
        SPDLOG_INFO(" - VertexBufferIndex: {} ({})", mesh.m_VertexBufferIndices.m_Data[0],
                    (m_LowMeshBuffers->m_VertexBuffers.m_Count + m_HighMeshBuffers->m_VertexBuffers.m_Count));
        SPDLOG_INFO(" - IndexBufferIndex: {} ({})", mesh.m_IndexBufferIndex,
                    (m_LowMeshBuffers->m_IndexBuffers.m_Count + m_HighMeshBuffers->m_IndexBuffers.m_Count));
#pragma warning(disable : 4018)
        assert(mesh.m_VertexBufferIndices.m_Data[0]
               < (m_LowMeshBuffers->m_VertexBuffers.m_Count + m_HighMeshBuffers->m_VertexBuffers.m_Count));
        assert(mesh.m_IndexBufferIndex
               < (m_LowMeshBuffers->m_IndexBuffers.m_Count + m_HighMeshBuffers->m_IndexBuffers.m_Count));
#endif

        // TODO: this is bad. sheldon has 2 hair meshes with the same name!
        const auto it = std::find_if(m_RenderBlocks.begin(), m_RenderBlocks.end(), [&](const IRenderBlock* value) {
            return !strcmp(((jc4::IRenderBlock*)value)->m_Name, sub_mesh_id);
        });
        if (it == m_RenderBlocks.end()) {
            continue;
        }

        const auto render_block = (*it);

        auto amf_vertex_buffer = vertex_buffers.at(mesh.m_VertexBufferIndices.m_Data[0]);
        auto amf_index_buffer  = index_buffers.at(mesh.m_IndexBufferIndex);

        // create the vertex buffer
        auto vertex_buffer = Renderer::Get()->CreateBuffer(
            &amf_vertex_buffer->m_Data.m_Data[mesh.m_VertexStreamOffsets.m_Data[0]], mesh.m_VertexCount,
            mesh.m_VertexStreamStrides.m_Data[0], VERTEX_BUFFER, "SAmfBuffer vertex buffer");

        // create the index buffer
        auto index_buffer =
            Renderer::Get()->CreateBuffer(&amf_index_buffer->m_Data.m_Data[mesh.m_IndexBufferOffset], mesh.m_IndexCount,
                                          mesh.m_IndexBufferStride, INDEX_BUFFER, "SAmfBuffer index buffer");

#if 0
        static auto UsageToStr = [&](const jc4::EAmfUsage usage) {
            using namespace jc4;

            switch (usage) {
                case EAmfUsage::AmfUsage_Unspecified:
                    return "AmfUsage_Unspecified";

                case EAmfUsage::AmfUsage_Position:
                    return "AmfUsage_Position";

                case EAmfUsage::AmfUsage_TextureCoordinate:
                    return "AmfUsage_TextureCoordinate";

                case EAmfUsage::AmfUsage_Normal:
                    return "AmfUsage_Normal";

                case EAmfUsage::AmfUsage_Tangent:
                    return "AmfUsage_Tangent";

                case EAmfUsage::AmfUsage_BiTangent:
                    return "AmfUsage_BiTangent";

                case EAmfUsage::AmfUsage_TangentSpace:
                    return "AmfUsage_TangentSpace";

                case EAmfUsage::AmfUsage_BoneIndex:
                    return "AmfUsage_BoneIndex";

                case EAmfUsage::AmfUsage_BoneWeight:
                    return "AmfUsage_BoneWeight";

                case EAmfUsage::AmfUsage_Color:
                    return "AmfUsage_Color";

                case EAmfUsage::AmfUsage_WireRadius:
                    return "AmfUsage_WireRadius";
            }

            return "Unknown Usage";
        };

        // TODO: it would be ideal if we could use these to build the render block vertex declaration

        for (uint32_t i = 0; i < mesh.m_StreamAttributes.m_Count; ++i) {
            const auto& attr = mesh.m_StreamAttributes.m_Data[i];

            const auto usage_name = UsageToStr(attr.m_Usage);
            const auto format     = static_cast<uint32_t>(attr.m_Format);

            const auto f  = *(float*)&attr.m_PackingData;
            const auto f2 = *(float*)&attr.m_PackingData[4];

            SPDLOG_INFO("#{} - Name={}, Format={}, Index={}, Offset={}, Stride={}, PackingData=({}, {})", i, usage_name,
                        format, attr.m_StreamIndex, attr.m_StreamOffset, attr.m_StreamStride, f, f2);
        }
#endif

        // init the render block
        ((jc4::IRenderBlock*)render_block)->Create(mesh_type_id, vertex_buffer, index_buffer);
        Renderer::Get()->AddToRenderList(render_block);
    }

    return true;
}
