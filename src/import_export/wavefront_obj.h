#pragma once

#include "iimportexporter.h"

#include "../version.h"
#include "../window.h"

#include "../jc3/file_loader.h"
#include "../jc3/formats/render_block_model.h"
#include "../jc3/renderblocks/irenderblock.h"

namespace import_export
{

class Wavefront_Obj : public IImportExporter
{
  private:
    uint32_t m_ExporterFlags = 0;

  public:
    enum WavefrontExporterFlags {
        WavefrontExporterFlags_None           = 0,
        WavefrontExporterFlags_MergeBlocks    = 1 << 0,
        WavefrontExporterFlags_ExportTextures = 1 << 1,
    };

    Wavefront_Obj()          = default;
    virtual ~Wavefront_Obj() = default;

    ImportExportType GetType() override final
    {
        return IE_TYPE_BOTH;
    }

    const char* GetName() override final
    {
        return "Wavefront";
    }

    std::vector<const char*> GetImportExtension() override final
    {
        return {".rbm"};
    }

    const char* GetExportExtension() override final
    {
        return ".obj";
    }

    bool HasSettingsUI() override final
    {
        return true;
    }

    bool DrawSettingsUI() override final
    {
        const auto& _text_colour = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);

        ImGui::CheckboxFlags("Merge Render Blocks", &m_ExporterFlags, WavefrontExporterFlags_MergeBlocks);
        ImGui::TextColored(_text_colour, "Merge models with multiple Render Blocks into a single file.");

        ImGui::Separator();

        ImGui::CheckboxFlags("Export Textures", &m_ExporterFlags, WavefrontExporterFlags_ExportTextures);
        ImGui::TextColored(_text_colour, "Export model textures as DDS");

        return ImGui::Button("Export");
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        LOG_INFO("Importing \"{}\"", filename.string());

        if (!std::filesystem::exists(filename)) {
            return callback(false, 0);
        }

        std::ifstream stream(filename);
        if (stream.fail()) {
            return callback(false, 0);
        }

        vertices_t vertices;
        uint16s_t  indices;

        std::vector<glm::vec3> _temp_vertices;
        std::vector<glm::vec2> _temp_uvs;
        std::vector<glm::vec3> _temp_normals;

        while (!stream.eof()) {
            std::string line;
            std::getline(stream, line);

            if (line[0] == '#') {
                continue;
            }

            const auto& type = line.substr(0, line.find_first_of(' '));

            // vertex
            if (type == "v") {
                float x, y, z;
                auto  r = sscanf(line.c_str(), "v %f %f %f", &x, &y, &z);
                assert(r == 3);

                _temp_vertices.emplace_back(glm::vec3{x, y, z});
            }
            // uv
            else if (type == "vt") {
                float x, y;
                auto  r = sscanf(line.c_str(), "vt %f %f", &x, &y);
                assert(r == 2);

                _temp_uvs.emplace_back(glm::vec2{x, y});
            }
            // normal
            else if (type == "vn") {
                float x, y, z;
                auto  r = sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z);
                assert(r == 3);

                _temp_normals.emplace_back(glm::vec3{x, y, z});
            }
            // face
            else if (type == "f") {
                int32_t vertex_idx[3], uv_idx[3], normal_idx[3];
                auto    r =
                    sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &vertex_idx[0], &uv_idx[0], &normal_idx[0],
                           &vertex_idx[1], &uv_idx[1], &normal_idx[1], &vertex_idx[2], &uv_idx[2], &normal_idx[2]);
                assert(r == 9);

                for (auto i = 0; i < 3; ++i) {
                    vertex_t _vert;
                    _vert.pos = _temp_vertices[vertex_idx[i] - 1];
                    _vert.uv  = _temp_uvs[uv_idx[i] - 1];

                    // insert unique values into the vertices and regenerate the index buffer
                    const auto it = std::find(vertices.begin(), vertices.end(), _vert);
                    if (it == vertices.end()) {
                        vertices.emplace_back(std::move(_vert));

                        const auto index = static_cast<uint16_t>(vertices.size() - 1);
                        indices.emplace_back(index);
                    } else {
                        const auto index = static_cast<uint16_t>(it - vertices.begin());
                        indices.emplace_back(index);
                    }
                }
            }
        }

        callback(true, std::make_tuple(vertices, indices));
    }

    void WriteModelFile(const std::filesystem::path& filename, const std::filesystem::path& path,
                        ::RenderBlockModel* model)
    {
        std::filesystem::path       mtl_path      = path / (filename.stem().string() + ".mtl");
        const auto&                 render_blocks = model->GetRenderBlocks();
        std::map<uint32_t, int32_t> _seen;
        uint32_t                    _num_vertices = 0;

        // set the merge flag if we only have 1 render block
        if (render_blocks.size() == 1) {
            m_ExporterFlags |= WavefrontExporterFlags_MergeBlocks;
        }

        // export textures
        if (m_ExporterFlags & WavefrontExporterFlags_ExportTextures) {
            std::ofstream            mtl_stream(mtl_path);
            std::vector<std::string> _materials;

            for (const auto& block : render_blocks) {
                const auto& textures                 = block->GetTextures();
                const auto& diffuse_texture          = textures[0];
                const auto& diffuse_texture_filename = diffuse_texture->GetFileName().stem();

                // only write unique materials
                if (std::find(_materials.begin(), _materials.end(), diffuse_texture_filename.string())
                    == _materials.end()) {
                    // write the block material
                    mtl_stream << "newmtl " << diffuse_texture_filename.string() << std::endl;
                    mtl_stream << "Kd 1.0 1.0 1.0" << std::endl;
                    mtl_stream << "Ks 0.0 0.0 0.0" << std::endl;
                    mtl_stream << "map_Kd " << diffuse_texture_filename.string() << ".dds" << std::endl << std::endl;

                    // export the texture
                    std::filesystem::path texture_path = path / diffuse_texture_filename;
                    WriteBufferToFile(texture_path.string() + ".dds", diffuse_texture->GetBuffer());

                    //
                    _materials.emplace_back(diffuse_texture_filename.string());
                }
            }
        }

        std::ofstream out_stream;

        // merging blocks, create a single file
        if (m_ExporterFlags & WavefrontExporterFlags_MergeBlocks) {
            std::filesystem::path block_path = path / (filename.stem().string() + GetExportExtension());
            out_stream.open(block_path, std::ios::binary);
            out_stream << "# File generated by " << VER_PRODUCTNAME_STR << " v" << VER_PRODUCT_VERSION_STR << std::endl;
            out_stream << "# https://github.com/aaronkirkham/jc3-rbm-renderer" << std::endl << std::endl;

            // link the material library
            if (m_ExporterFlags & WavefrontExporterFlags_ExportTextures) {
                out_stream << "mtllib " << mtl_path.filename().string() << std::endl << std::endl;
            }
        }

        // export the render blocks
        for (const auto& block : render_blocks) {
            const auto& textures = block->GetTextures();
            std::string block_name =
                std::string(block->GetTypeName()) + "." + std::to_string(_seen[block->GetTypeHash()]);

            // not merging blocks, create a new file
            if (!(m_ExporterFlags & WavefrontExporterFlags_MergeBlocks)) {
                std::filesystem::path block_path = path / (block_name + GetExportExtension());
                out_stream.close();
                out_stream.open(block_path, std::ios::binary);
                out_stream << "# File generated by " << VER_PRODUCTNAME_STR << " v" << VER_PRODUCT_VERSION_STR
                           << std::endl;
                out_stream << "# https://github.com/aaronkirkham/jc3-rbm-renderer" << std::endl << std::endl;

                // link the material library
                if (m_ExporterFlags & WavefrontExporterFlags_ExportTextures) {
                    out_stream << "mtllib " << mtl_path.filename().string() << std::endl;
                    out_stream << "usemtl " << textures[0]->GetFileName().stem().string() << std::endl << std::endl;
                }
            }

            // write the object
            if (m_ExporterFlags & WavefrontExporterFlags_MergeBlocks) {
                out_stream << "o " << block_name << std::endl << std::endl;

                if (m_ExporterFlags & WavefrontExporterFlags_ExportTextures) {
                    out_stream << "usemtl " << textures[0]->GetFileName().stem().string() << std::endl << std::endl;
                }
            }

            auto [vertices, indices] = block->GetData();
            for (auto& vertex : vertices) {
                out_stream << "v " << vertex.pos.x << " " << vertex.pos.y << " " << vertex.pos.z << std::endl;
                out_stream << "vt " << vertex.uv.x << " " << vertex.uv.y << std::endl;
            }

            for (auto i = 0; i < indices.size(); i += 3) {
                int32_t vertex_indices[3] = {indices[i] + 1, indices[i + 1] + 1, indices[i + 2] + 1};

                // if we are merging blocks, add the total vertices/uvs the the current index
                if (m_ExporterFlags & WavefrontExporterFlags_MergeBlocks) {
                    vertex_indices[0] += _num_vertices;
                    vertex_indices[1] += _num_vertices;
                    vertex_indices[2] += _num_vertices;
                }

                // f v[/t/n]
                out_stream << "f " << vertex_indices[0] << "/" << vertex_indices[0];
                out_stream << " " << vertex_indices[1] << "/" << vertex_indices[1];
                out_stream << " " << vertex_indices[2] << "/" << vertex_indices[2] << std::endl;
            }

            _num_vertices += vertices.size();
            _seen[block->GetTypeHash()]++;
        }

        // reset the export flags
        m_ExporterFlags = 0;
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem();

        // create the directory if we need to
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }

        auto model = ::RenderBlockModel::get(filename.string());
        if (model) {
            WriteModelFile(filename, path, model.get());
            callback(true);
        } else {
            FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, FileBuffer data) {
                if (success) {
                    auto model = std::make_unique<::RenderBlockModel>(filename);
                    if (model->Parse(data, false)) {
                        WriteModelFile(filename, path, model.get());
                    }
                }

                callback(success);
            });
        }
    }
};

}; // namespace import_export
