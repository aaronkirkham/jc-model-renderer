#include "render_block_model.h"
#include "avalanche_archive.h"
#include "runtime_container.h"

#include "../file_loader.h"
#include "../render_block_factory.h"

#include "../jc3/renderblocks/irenderblock.h"

#include "../../graphics/camera.h"
#include "../../graphics/renderer.h"
#include "../../graphics/ui.h"
#include "../../window.h"

extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;

std::recursive_mutex                                  Factory<RenderBlockModel>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RenderBlockModel>> Factory<RenderBlockModel>::Instances;

RenderBlockModel::RenderBlockModel(const std::filesystem::path& filename)
    : m_Filename(filename)
{
    // find the parent archive
    // TODO: better way.
    std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
    for (const auto& archive : AvalancheArchive::Instances) {
        if (archive.second->HasFile(filename)) {
            m_ParentArchive = archive.second;
            break;
        }
    }
}

RenderBlockModel::~RenderBlockModel()
{
    // remove from renderlist
    Renderer::Get()->RemoveFromRenderList(m_RenderBlocks);

    // delete the render blocks
    for (auto& render_block : m_RenderBlocks) {
        SAFE_DELETE(render_block);
    }
}

bool RenderBlockModel::ParseLOD(const FileBuffer& data)
{
    std::istringstream stream(std::string{(char*)data.data(), data.size()});
    std::string        line;

    while (std::getline(stream, line)) {
        LOG_INFO(line);
    }

    return true;
}

bool RenderBlockModel::ParseRBM(const FileBuffer& data, bool add_to_render_list)
{
    std::istringstream stream(std::string{(char*)data.data(), data.size()});

    bool parse_success = true;

    // read the model header
    jc::RBMHeader header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp((char*)&header.m_Magic, "RBMDL", 5) != 0) {
        LOG_ERROR("Invalid header magic.");
        parse_success = false;
        goto end;
    }

    LOG_INFO("RBM v{}.{}.{}. NumberOfBlocks={}, Flags={}", header.m_VersionMajor, header.m_VersionMinor,
             header.m_VersionRevision, header.m_NumberOfBlocks, header.m_Flags);

    // ensure we can read the file version (TODO: when JC4 is released, check this)
    if (header.m_VersionMajor != 1 && header.m_VersionMinor != 16) {
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    m_RenderBlocks.reserve(header.m_NumberOfBlocks);

    // store the bounding box
    m_BoundingBox.m_Min = header.m_BoundingBoxMin;
    m_BoundingBox.m_Max = header.m_BoundingBoxMax;

    // focus on the bounding box
    if (RenderBlockModel::Instances.size() == 1) {
        Camera::Get()->FocusOn(this);
    }

    // use file batches
    if (!LoadingFromRuntimeContainer) {
        FileLoader::UseBatches = true;
    }

    // read all the blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; ++i) {
        uint32_t hash;
        stream.read((char*)&hash, sizeof(uint32_t));

        const auto render_block = (jc3::IRenderBlock*)RenderBlockFactory::CreateRenderBlock(hash);
        if (render_block) {
            // render_block->SetParent(this);
            render_block->Read(stream);
            render_block->Create();

            uint32_t checksum;
            stream.read((char*)&checksum, sizeof(uint32_t));

            // did we read the block correctly?
            if (checksum != RBM_END_OF_BLOCK) {
                LOG_ERROR("Failed to read render block \"{}\" (checksum mismatch)",
                          RenderBlockFactory::GetRenderBlockName(hash));
                parse_success = false;
                break;
            }

            m_RenderBlocks.emplace_back(render_block);
        } else {
            LOG_ERROR("Unknown render block \"{}\" (0x{:x})", RenderBlockFactory::GetRenderBlockName(hash), hash);

            if (LoadingFromRuntimeContainer) {
                std::stringstream error;
                error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase
                      << std::setw(4) << std::hex << hash << ") is not yet supported.\n";

                if (std::find(SuppressedWarnings.begin(), SuppressedWarnings.end(), error.str())
                    == SuppressedWarnings.end()) {
                    SuppressedWarnings.emplace_back(error.str());
                }
            } else {
                std::stringstream error;
                error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase
                      << std::setw(4) << std::hex << hash << ") is not yet supported.\n\n";
                error << "A model might still be rendered, but some parts of it may be missing.";

                Window::Get()->ShowMessageBox(error.str(), MB_ICONINFORMATION | MB_OK);
            }

            // if we haven't parsed any other blocks yet
            // we'll never see anything rendered, let the user know something is wrong
            if (m_RenderBlocks.size() == 0) {
                parse_success = false;
            }

            break;
        }
    }

end:
    if (parse_success) {}

    // if we are not loading from a runtime container, run the batches now
    if (!LoadingFromRuntimeContainer) {
        // run file batches
        FileLoader::UseBatches = false;
        FileLoader::Get()->RunFileBatches();
    }

    // add to renderlist
    if (add_to_render_list) {
        Renderer::Get()->AddToRenderList(m_RenderBlocks);
    }

    return parse_success;
}

#if 0
#include "../renderblocks/renderblockgeneral.h"

void RenderBlockModel::ParseAMF(const FileBuffer& data, ParseCallback_t callback, bool add_to_render_list)
{
    auto model_adf = AvalancheDataFormat::make(m_Filename);
    if (!model_adf->Parse(data)) {
        LOG_ERROR("Failed to parse modelc ADF!");
        return callback(false);
    }

    auto        inst      = model_adf->GetMember("instance");
    const auto& mesh_name = inst->m_Members[0]->m_StringData;

    // MODELC	= contains all model information (textures, mesh files...)
    // MESHC	= contains all mesh information
    // HRMESHC	= only high resolution BUFFERS

    auto  materials       = model_adf->GetMember(inst, "Materials");
    auto& render_block_id = model_adf->GetMember(materials, "RenderBlockId")->m_StringData;

    LOG_INFO("Creating render block \"{}\". Reading mesh: \"{}\"", render_block_id, mesh_name);

    const auto render_block = RenderBlockFactory::CreateRenderBlock(render_block_id);
    if (render_block) {
        render_block->SetParent(this);
    }

    // read the meshc
    FileLoader::Get()->ReadFile(mesh_name, [&, callback, mesh_name, render_block](bool success, FileBuffer data) {
        if (success) {
            auto mesh_adf = AvalancheDataFormat::make(mesh_name);
            if (!mesh_adf->Parse(data)) {
                LOG_ERROR("Failed to parse meshc ADF");
                return callback(false);
            }

            const auto& lod_groups = mesh_adf->GetMember("LodGroups");

            // get the high LOD mesh name (remove the leading intermediate path)
            auto hrmesh_name = mesh_adf->GetMember("HighLodPath")->m_StringData;
            if (hrmesh_name.find_first_of("intermediate/") == 0) {
                hrmesh_name = hrmesh_name.erase(0, 13);
            }

            // read the hrmeshc
            FileLoader::Get()->ReadFile(hrmesh_name, [&, callback, render_block, mesh_adf, hrmesh_name,
                                                      lod_groups](bool success, FileBuffer hrmesh_data) {
                AdfInstanceMemberInfo* hr_vertex_buffers = nullptr;
                AdfInstanceMemberInfo* hr_index_buffers  = nullptr;

                if (success) {
                    LOG_INFO("Using high resolution mesh! ({})", hrmesh_name);

                    auto hrmesh_adf = AvalancheDataFormat::make(hrmesh_name);
                    if (!hrmesh_adf->Parse(hrmesh_data)) {
                        LOG_ERROR("Failed to parse hrmeshc ADF");
                        return callback(false);
                    }

                    hr_vertex_buffers = hrmesh_adf->GetMember("VertexBuffers");
                    hr_index_buffers  = hrmesh_adf->GetMember("IndexBuffers");
                } else {
                    LOG_INFO("No high resolution mesh available. Using low resolution mesh. ({})", hrmesh_name);
                }

                const auto result =
                    ParseAMFMeshBuffers(mesh_adf.get(), lod_groups, hr_vertex_buffers, hr_index_buffers);
                callback(result);
            });

            /*
                // multiple vertex buffers
                FileBuffer vert_data;
                std::copy(vb_data.begin() + _offsets[0], vb_data.begin() + _offsets[1],
               std::back_inserter(vert_data)); FileBuffer uv_data; std::copy(vb_data.begin() + _offsets[1],
               vb_data.end(), std::back_inserter(uv_data));

                ((RenderBlockGeneral*)render_block)->CreateBuffers(&vert_data, &uv_data, &ib_data);

                m_RenderBlocks.emplace_back(render_block);
                // Renderer::Get()->AddToRenderList(render_block);
            */
        }

        // callback(success);
    });
}

bool RenderBlockModel::ParseAMFMeshBuffers(AvalancheDataFormat* mesh_adf, AdfInstanceMemberInfo* lod_groups,
                                           AdfInstanceMemberInfo* hr_vertex_buffers,
                                           AdfInstanceMemberInfo* hr_index_buffers)
{
    if (!mesh_adf || !lod_groups) {
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    const auto& lr_vertex_buffers = mesh_adf->GetMember("VertexBuffers");
    const auto& lr_index_buffers  = mesh_adf->GetMember("IndexBuffers");

    std::vector<FileBuffer> vertex_buffers;
    std::vector<FileBuffer> index_buffers;

    // TODO: clean up.
    {
        // the buffer indexes require the low resolution buffers to be first, followed by the high resolution buffers

        for (const auto& v : lr_vertex_buffers->m_Members) {
            vertex_buffers.emplace_back(v->m_Members[0]->m_Data);
        }

        if (hr_vertex_buffers) {
            for (const auto& v : hr_vertex_buffers->m_Members) {
                vertex_buffers.emplace_back(v->m_Members[0]->m_Data);
            }
        }

        for (const auto& i : lr_index_buffers->m_Members) {
            index_buffers.emplace_back(i->m_Members[0]->m_Data);
        }

        if (hr_index_buffers) {
            for (const auto& i : hr_index_buffers->m_Members) {
                index_buffers.emplace_back(i->m_Members[0]->m_Data);
            }
        }
    }

    // write output file
    std::ofstream stream("C:/users/aaron/desktop/sheldon_body_002.obj");
    uint32_t      _num_vertices = 0;

    const auto& meshes = mesh_adf->GetMember(lod_groups->m_Members.back().get(), "Meshes");
    for (const auto& mesh : meshes->m_Members) {
        const auto  type    = mesh_adf->GetMember(mesh.get(), "MeshTypeId")->m_StringData;
        const auto& mesh_id = mesh_adf->GetMember(mesh.get(), "SubMeshId")->m_StringData;

        LOG_INFO("Mesh: {}", mesh_id);
        LOG_INFO(" - Type={}", type);

        // vertex buffer
        const auto vertex_count         = mesh_adf->GetMember(mesh.get(), "VertexCount")->GetData<uint32_t>();
        const auto vertex_buffer_index  = mesh_adf->GetMember(mesh.get(), "VertexBufferIndices")->m_Data[0];
        const auto vertex_buffer_stride = mesh_adf->GetMember(mesh.get(), "VertexStreamStrides")->m_Data[0];
        const auto vertex_buffer_offset = mesh_adf->GetMember(mesh.get(), "VertexStreamOffsets")->GetData<uint32_t>();

        LOG_INFO(" - VertexCount={}, VertexBufferIndices={}, VertexStreamStrides={}, VertexStreamOffsets={}",
                 vertex_count, vertex_buffer_index, vertex_buffer_stride, vertex_buffer_offset);

        const auto& vertex_buffer = vertex_buffers.at(vertex_buffer_index);

        // copy the vertex buffer
        std::vector<uint8_t> vertices;
        std::copy(vertex_buffer.begin() + vertex_buffer_offset,
                  vertex_buffer.begin() + vertex_buffer_offset + (vertex_count * vertex_buffer_stride),
                  std::back_inserter(vertices));

        auto& packed_vertices = CastBuffer<jc::Vertex::RenderBlockCharacter::Packed4Bones1UV>(&vertices);
        for (const auto& vertex : packed_vertices) {
            float x = jc::Vertex::unpack(vertex.x);
            float y = jc::Vertex::unpack(vertex.y);
            float z = jc::Vertex::unpack(vertex.z);
            stream << "v " << x << " " << y << " " << z << std::endl;
        }

        // index buffer
        const auto index_count         = mesh_adf->GetMember(mesh.get(), "IndexCount")->GetData<uint32_t>();
        const auto index_buffer_index  = mesh_adf->GetMember(mesh.get(), "IndexBufferIndex")->m_Data[0];
        const auto index_buffer_stride = mesh_adf->GetMember(mesh.get(), "IndexBufferStride")->m_Data[0];
        const auto index_buffer_offset = mesh_adf->GetMember(mesh.get(), "IndexBufferOffset")->m_Data[0];

        LOG_INFO(" - IndexCount={}, IndexBufferIndex={}, IndexBufferStride={}, IndexBufferOffset={}", index_count,
                 index_buffer_index, index_buffer_stride, index_buffer_offset);

        const auto& index_buffer = CastBuffer<uint16_t>(&index_buffers.at(index_buffer_index));

        // copy the index buffer
        std::vector<uint16_t> indices;
        std::copy(index_buffer.begin() + index_buffer_offset,
                  index_buffer.begin() + index_buffer_offset + (index_count), std::back_inserter(indices));

        // write
        for (auto i = 0; i < indices.size(); i += 3) {
            int32_t index[3] = {indices[i] + 1, indices[i + 1] + 1, indices[i + 2] + 1};

            index[0] += _num_vertices;
            index[1] += _num_vertices;
            index[2] += _num_vertices;

            stream << "f " << index[0] << " " << index[1] << " " << index[2] << std::endl;
        }

        //
        _num_vertices += packed_vertices.size();
    }

    stream.close();

    return true;
}
#endif

void RenderBlockModel::DrawGizmos()
{
    auto largest_scale = 0.0f;
    /*for (auto& render_block : m_RenderBlocks) {
        if (render_block->IsVisible()) {
            const auto scale = render_block->GetScale();

            // keep the biggest scale
            if (scale > largest_scale) {
                largest_scale = scale;
            }
        }
    }*/

    // hack to fix GetCenter
    m_BoundingBox.SetScale(largest_scale);

    // draw filename label
    if (g_ShowModelLabels) {
        static auto white = glm::vec4{1, 1, 1, 0.6};
        UI::Get()->DrawText(m_Filename.filename().string(), m_BoundingBox.GetCenter(), white, true);
    }

    // draw bounding boxes
    if (g_DrawBoundingBoxes) {
        // auto mouse_pos = Input::Get()->GetMouseWorldPosition();

        // Ray   ray(mouse_pos, {0, 0, 1});
        // float distance = 0.0f;
        // auto  intersects = m_BoundingBox.Intersect(ray, &distance);

        static auto red = glm::vec4{1, 0, 0, 1};
        // static auto green = glm::vec4{0, 1, 0, 1};

        UI::Get()->DrawBoundingBox(m_BoundingBox, red);

        // DebugRenderer::Get()->DrawBBox(m_BoundingBox.GetMin(), m_BoundingBox.GetMax(), intersects ? green : red);
    }
}

void RenderBlockModel::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    auto rbm = RenderBlockModel::make(filename);
    assert(rbm);

    if (filename.extension() == ".lod") {
        rbm->ParseLOD(data);
    } else if (filename.extension() == ".rbm") {
        rbm->ParseRBM(data);
    }
}

bool RenderBlockModel::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    const auto rbm = RenderBlockModel::get(filename.string());
    if (rbm) {
        std::ofstream stream(path, std::ios::binary);
        if (stream.fail()) {
            return false;
        }

        const auto& render_blocks = rbm->GetRenderBlocks();

        // generate the rbm header
        jc::RBMHeader header;
        header.m_BoundingBoxMin = rbm->GetBoundingBox()->GetMin();
        header.m_BoundingBoxMax = rbm->GetBoundingBox()->GetMax();
        header.m_NumberOfBlocks = render_blocks.size();
        header.m_Flags          = 8;

        // write the header
        stream.write((char*)&header, sizeof(header));

        for (const auto& render_block : render_blocks) {
            // write the block type hash
            const auto& type_hash = render_block->GetTypeHash();
            stream.write((char*)&type_hash, sizeof(type_hash));

            // write the block data
            ((jc3::IRenderBlock*)render_block)->Write(stream);

            // write the end of block checksum
            stream.write((char*)&RBM_END_OF_BLOCK, sizeof(RBM_END_OF_BLOCK));
        }

        stream.close();
        return true;
    }

    return false;
}

void RenderBlockModel::ContextMenuUI(const std::filesystem::path& filename)
{
    const auto rbm = get(filename.string());

    if (ImGui::BeginMenu("Add Render Block")) {
        // show available render blocks
        for (const auto& block_name : RenderBlockFactory::GetValidRenderBlocks()) {
            if (ImGui::MenuItem(block_name)) {
                const auto render_block = RenderBlockFactory::CreateRenderBlock(block_name);
                assert(render_block);
                rbm->GetRenderBlocks().emplace_back(render_block);
            }
        }

        ImGui::EndMenu();
    }
}

void RenderBlockModel::Load(const std::filesystem::path& filename)
{
    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
        if (success) {
            RenderBlockModel::ReadFileCallback(filename, std::move(data), false);
        } else {
            LOG_ERROR("Failed to read model \"{}\"", filename.string());
        }
    });
}

void RenderBlockModel::LoadFromRuntimeContainer(const std::filesystem::path&      filename,
                                                std::shared_ptr<RuntimeContainer> rc)
{
    assert(rc);

    auto path = filename.parent_path().string();
    path      = path.replace(path.find("editor/entities"), strlen("editor/entities"), "models");

    std::thread([&, rc, path] {
        FileLoader::UseBatches                        = true;
        RenderBlockModel::LoadingFromRuntimeContainer = true;

        for (const auto& container : rc->GetAllContainers("CPartProp")) {
            auto name     = container->GetProperty("name");
            auto filename = std::any_cast<std::string>(name->GetValue());
            filename += "_lod1.rbm";

            std::filesystem::path modelname = path;
            modelname /= filename;

            RenderBlockModel::Load(modelname.generic_string());
        }

        RenderBlockModel::LoadingFromRuntimeContainer = false;
        FileLoader::UseBatches                        = false;
        FileLoader::Get()->RunFileBatches();

        std::stringstream error;
        for (const auto& warning : RenderBlockModel::SuppressedWarnings) {
            error << warning;
        }

        error << "\nA model might still be rendered, but some parts of it may be missing.";
        Window::Get()->ShowMessageBox(error.str(), MB_ICONINFORMATION | MB_OK);
    }).detach();
}
