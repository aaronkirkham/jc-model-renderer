#pragma once

#include "iimportexporter.h"

#include "../version.h"
#include "../window.h"

#include "../jc3/file_loader.h"
#include "../jc3/formats/render_block_model.h"
#include "../jc3/renderblocks/irenderblock.h"

template <typename T> static size_t RemoveDuplicates(std::vector<T>& vec)
{
    std::vector<T> seen;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                             [&seen](const T& value) {
                                 if (std::find(seen.begin(), seen.end(), value) != seen.end()) {
                                     return true;
                                 }

                                 seen.emplace_back(value);
                                 return false;
                             }),
              vec.end());

    return vec.size();
}

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
        ImGui::CheckboxFlags("Merge Render Blocks", &m_ExporterFlags, WavefrontExporterFlags_MergeBlocks);
        ImGui::CheckboxFlags("Export Textures", &m_ExporterFlags, WavefrontExporterFlags_ExportTextures);

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

        floats_t  _out_vertices;
        floats_t  _out_uvs;
        floats_t  _out_normals;
        uint16s_t _out_indices;

        std::vector<int> vertex_indices;
        std::vector<int> uv_indices;
        std::vector<int> normal_indices;

        std::vector<glm::vec2> _tmp_uvs;
        std::vector<glm::vec2> _uvs;
        std::vector<glm::vec3> _normals;

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

                _out_vertices.emplace_back(x);
                _out_vertices.emplace_back(y);
                _out_vertices.emplace_back(z);
            }
            // uv
            else if (type == "vt") {
                float x, y;
                auto  r = sscanf(line.c_str(), "vt %f %f", &x, &y);
                assert(r == 2);

                _uvs.emplace_back(glm::vec2{x, y});
            }
            // normal
            else if (type == "vn") {
                float x, y, z;
                auto  r = sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z);
                assert(r == 3);

                _normals.emplace_back(glm::vec3{x, y, z});
            }
            // face
            else if (type == "f") {
                int  v_index[3], uv_index[3], n_index[3];
                auto r = sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &v_index[0], &uv_index[0], &n_index[0],
                                &v_index[1], &uv_index[1], &n_index[1], &v_index[2], &uv_index[2], &n_index[2]);
                assert(r == 9);

                vertex_indices.emplace_back(v_index[0] - 1);
                vertex_indices.emplace_back(v_index[1] - 1);
                vertex_indices.emplace_back(v_index[2] - 1);

                uv_indices.emplace_back(uv_index[0] - 1);
                uv_indices.emplace_back(uv_index[1] - 1);
                uv_indices.emplace_back(uv_index[2] - 1);

                normal_indices.emplace_back(n_index[0] - 1);
                normal_indices.emplace_back(n_index[1] - 1);
                normal_indices.emplace_back(n_index[2] - 1);

                // indices
                _out_indices.emplace_back(v_index[0] - 1);
                _out_indices.emplace_back(v_index[1] - 1);
                _out_indices.emplace_back(v_index[2] - 1);
            }
        }

        for (int i = 0; i < vertex_indices.size(); ++i) {
            // auto vi = vertex_indices[i];
            auto ui = uv_indices[i];
            auto ni = normal_indices[i];

            _tmp_uvs.emplace_back(_uvs[ui]);
        }

        RemoveDuplicates(_tmp_uvs);
        for (const auto& v : _tmp_uvs) {
            _out_uvs.emplace_back(v.x);
            _out_uvs.emplace_back(v.y);
        }

        __debugbreak();

        callback(true, std::make_tuple(_out_vertices, _out_uvs, _out_normals, _out_indices));
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

            const auto& [vertices, indices, uvs] = block->GetData();

            // vertices
            for (auto i = 0; i < vertices.size(); i += 3) {
                out_stream << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << std::endl;
            }

            // uvs
            for (auto i = 0; i < uvs.size(); i += 2) {
                out_stream << "vt " << uvs[i] << " " << uvs[i + 1] << std::endl;
            }

            // faces
            for (auto i = 0; i < indices.size(); i += 3) {
                auto f1 = indices[i] + 1;
                auto f2 = indices[i + 1] + 1;
                auto f3 = indices[i + 2] + 1;

				if (m_ExporterFlags & WavefrontExporterFlags_MergeBlocks) {
                    f1 += _num_vertices;
                    f2 += _num_vertices;
                    f3 += _num_vertices;
				}

                // f v[/t/n]
                out_stream << "f " << f1 << "/" << f1;
                out_stream << " " << f2 << "/" << f2;
                out_stream << " " << f3 << "/" << f3 << std::endl;
            }

			//
            _num_vertices += vertices.size() / 3;

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
