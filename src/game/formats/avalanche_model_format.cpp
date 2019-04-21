#include "avalanche_model_format.h"
#include "avalanche_archive.h"
#include "avalanche_data_format.h"

#include "../file_loader.h"
#include "../render_block_factory.h"
#include "../renderblocks/irenderblock.h"

#include "../../graphics/renderer.h"
#include "../../window.h"

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

void AvalancheModelFormat::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data,
                                            bool external)
{
    auto amf = AvalancheModelFormat::make(filename);
    if (amf) {
        amf->Parse(data, [](bool) {});
    }
}

bool AvalancheModelFormat::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    LOG_ERROR("NOT IMPLEMENTED!");
    return false;
}

void AvalancheModelFormat::ContextMenuUI(const std::filesystem::path& filename) {}

void AvalancheModelFormat::Load(const std::filesystem::path& filename)
{
    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
        if (success) {
            AvalancheModelFormat::ReadFileCallback(filename, std::move(data), false);
        } else {
            LOG_INFO("Failed to read model \"{}\"", filename.generic_string());
        }
    });
}

void AvalancheModelFormat::Parse(const FileBuffer& data, ParseCallback_t callback)
{
    // TODO: we ideally want to be able to open all 3 types. (.modelc, .meshc, .hrmeshc)
    // using 1 format. right now we're just assuming the type is modelc.

    // NOTE:
    //	.modelc		= contains all model information (textures, mesh files...)
    //	.meshc		= contains all mesh information, and low resolution buffers
    //	.hrmeshc	= only high resolution buffers

    m_ModelAdf = AvalancheDataFormat::make(m_Filename);
    if (!m_ModelAdf->Parse(data)) {
        LOG_ERROR("Failed to parse modelc ADF! ({})", m_Filename.generic_string());
        return callback(false);
    }

    const auto instance  = m_ModelAdf->GetMember("instance"); // TODO: not all models have this name.
    const auto materials = m_ModelAdf->GetMember(instance, "Materials");

    for (const auto& member : materials->m_Members) {
        auto mesh = std::make_unique<AMFMesh>(member.get(), this);
        m_Meshes.emplace_back(std::move(mesh));
    }

    // read the mesh file
    const auto& meshc_filename = instance->m_Members[0]->m_StringData;
    LOG_INFO("Loading model mesh: \"{}\"", meshc_filename);
    FileLoader::Get()->ReadFile(meshc_filename, [&, callback, meshc_filename](bool success, FileBuffer data) {
        if (success) {
            m_MeshAdf = AvalancheDataFormat::make(meshc_filename);
            if (!m_MeshAdf->Parse(data)) {
                LOG_ERROR("Failed to parse meshc ADF! ({})", meshc_filename);
                return callback(false);
            }

            // get the high LOD mesh name (remove the leading "intermediate" path)
            auto hrmesh_filename = m_MeshAdf->GetMember("HighLodPath")->m_StringData;
            if (hrmesh_filename.find_first_of("intermediate/") == 0) {
                hrmesh_filename = hrmesh_filename.erase(0, 13);
            }

            // read the high resolution mesh file
            FileLoader::Get()->ReadFile(hrmesh_filename, [&, callback, hrmesh_filename](bool success, FileBuffer data) {
                if (success) {
                    m_HighResMeshAdf = AvalancheDataFormat::make(hrmesh_filename);
                    if (!m_HighResMeshAdf->Parse(data)) {
                        LOG_ERROR("Failed to parse hrmeshc ADF! ({})", hrmesh_filename);
                        return callback(false);
                    }

                    LOG_INFO("Will use high resolution mesh! ({})", hrmesh_filename);
                } else {
                    LOG_INFO("No high resolution mesh available. Using low resolution instead. ({})", hrmesh_filename);
                }

                return callback(ParseMeshBuffers());
            });
        }
    });
}

bool AvalancheModelFormat::ParseMeshBuffers()
{
    if (!m_ModelAdf || !m_MeshAdf) {
        LOG_ERROR("Can't parse mesh buffers without loading model/mesh first.");
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    std::vector<FileBuffer> vertex_buffers;
    std::vector<FileBuffer> index_buffers;

    static const auto CopyBuffer = [](AvalancheDataFormat* adf, const char* name, std::vector<FileBuffer>* to) {
        if (adf) {
            if (const auto member = adf->GetMember(name)) {
                for (const auto& buffer : member->m_Members) {
                    to->emplace_back(buffer->m_Members[0]->m_Data);
                }
            }
        }
    };

    // copy the low & high resolution buffers into a single buffer
    CopyBuffer(m_MeshAdf.get(), "VertexBuffers", &vertex_buffers);
    CopyBuffer(m_HighResMeshAdf.get(), "VertexBuffers", &vertex_buffers);
    CopyBuffer(m_MeshAdf.get(), "IndexBuffers", &index_buffers);
    CopyBuffer(m_HighResMeshAdf.get(), "IndexBuffers", &index_buffers);

    // get the highest resolution LOD available
    const auto lod_groups = m_MeshAdf->GetMember("LodGroups");
    const auto meshes     = m_MeshAdf->GetMember(lod_groups->m_Members.back().get(), "Meshes");
    for (const auto& mesh : meshes->m_Members) {
        const auto mesh_type_id = m_MeshAdf->GetMember(mesh.get(), "MeshTypeId")->m_StringData;
        const auto sub_mesh_id  = m_MeshAdf->GetMember(mesh.get(), "SubMeshId")->m_StringData;

        LOG_INFO("Mesh: {} ({})", sub_mesh_id, mesh_type_id);

        // TODO: this is bad. sheldon has 2 hair meshes with the same name!
        const auto find_it = std::find_if(m_Meshes.begin(), m_Meshes.end(), [&](const std::unique_ptr<AMFMesh>& value) {
            return value->GetName() == sub_mesh_id;
        });

        if (find_it != m_Meshes.end()) {
            /*
            // vertex buffer
            const auto vertex_count         = m_MeshAdf->GetMember(mesh.get(), "VertexCount")->GetData<uint32_t>();
            const auto vertex_buffer_index  = m_MeshAdf->GetMember(mesh.get(), "VertexBufferIndices")->m_Data[0];
            const auto vertex_buffer_stride = m_MeshAdf->GetMember(mesh.get(), "VertexStreamStrides")->m_Data[0];
            const auto vertex_buffer_offset =
                m_MeshAdf->GetMember(mesh.get(), "VertexStreamOffsets")->GetData<uint32_t>();

            LOG_INFO(" - VertexCount={}, VertexBufferIndices={}, VertexStreamStrides={}, VertexStreamOffsets={}",
                     vertex_count, vertex_buffer_index, vertex_buffer_stride, vertex_buffer_offset);

            const auto& vertex_buffer = vertex_buffers.at(vertex_buffer_index);

            // copy the vertex buffer
            std::vector<uint8_t> vertices;
            std::copy(vertex_buffer.begin() + vertex_buffer_offset,
                      vertex_buffer.begin() + vertex_buffer_offset + (vertex_count * vertex_buffer_stride),
                      std::back_inserter(vertices));

            // index buffer
            const auto index_count         = m_MeshAdf->GetMember(mesh.get(), "IndexCount")->GetData<uint32_t>();
            const auto index_buffer_index  = m_MeshAdf->GetMember(mesh.get(), "IndexBufferIndex")->m_Data[0];
            const auto index_buffer_stride = m_MeshAdf->GetMember(mesh.get(), "IndexBufferStride")->m_Data[0];
            const auto index_buffer_offset = m_MeshAdf->GetMember(mesh.get(), "IndexBufferOffset")->m_Data[0];

            LOG_INFO(" - IndexCount={}, IndexBufferIndex={}, IndexBufferStride={}, IndexBufferOffset={}", index_count,
                     index_buffer_index, index_buffer_stride, index_buffer_offset);

            const auto& index_buffer = CastBuffer<uint16_t>(&index_buffers.at(index_buffer_index));

            // copy the index buffer
            std::vector<uint16_t> indices;
            std::copy(index_buffer.begin() + index_buffer_offset,
                      index_buffer.begin() + index_buffer_offset + (index_count), std::back_inserter(indices));
            */
        }
    }

    return true;
}

AMFMesh::AMFMesh(AdfInstanceMemberInfo* info, AvalancheModelFormat* parent)
    : m_Info(info)
    , m_Parent(parent)
{
    auto model_adf  = parent->GetModelAdf();
    m_Name          = model_adf->GetMember(info, "Name")->m_StringData;
    m_RenderBlockId = model_adf->GetMember(info, "RenderBlockId")->m_StringData;

    LOG_INFO("{} ({})", m_Name, m_RenderBlockId);

    m_RenderBlock = RenderBlockFactory::CreateRenderBlock(m_RenderBlockId);
    assert(m_RenderBlock != nullptr);

    // load textures
    const auto textures = model_adf->GetMember(info, "Textures");
    m_RenderBlock->AllocateTextureSlots(textures->m_Members.size());
    for (auto i = 0; i < textures->m_Members.size(); ++i) {
        auto texture = TextureManager::Get()->GetTexture(textures->m_Members[i]->m_StringData);
        m_RenderBlock->SetTexture(i, texture);
    }
}

AMFMesh::~AMFMesh()
{
    Renderer::Get()->RemoveFromRenderList(m_RenderBlock);
    // SAFE_DELETE(m_RenderBlock);
}
