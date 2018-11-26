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
  public:
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

    void WriteModelFile(const std::filesystem::path& path, ::RenderBlockModel* model)
    {
        assert(model);

        std::ofstream out_stream(path, std::ios::binary);
        assert(!out_stream.fail());
        out_stream << "# File generated by " << VER_PRODUCTNAME_STR << " v" << VER_PRODUCT_VERSION_STR << std::endl;
        out_stream << "# https://github.com/aaronkirkham/jc3-rbm-renderer" << std::endl << std::endl;

        // output all render blocks
        // TODO: split each render block into it's own file. each block should also write either a JSON file or binary
        // file containing block shader settings
        for (const auto& block : model->GetRenderBlocks()) {
            out_stream << "g " << block->GetTypeName() << std::endl;

            const auto& [vertices, indices, uvs] = block->GetData();

            // vertices
            for (auto i = 0; i < vertices.size(); i += 3) {
                out_stream << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << std::endl;
            }

            // uvs
            for (auto i = 0; i < uvs.size(); i += 2) {
                out_stream << "vt " << uvs[i] << " " << uvs[i + 1] << std::endl;
            }

            // normals
            // TODO

            // faces
            for (auto i = 0; i < indices.size(); i += 3) {
                const auto f1 = indices[i] + 1;
                const auto f2 = indices[i + 1] + 1;
                const auto f3 = indices[i + 2] + 1;

                // f v[/t/n]
                out_stream << "f " << f1 << "/" << f1;
                out_stream << " " << f2 << "/" << f2;
                out_stream << " " << f3 << "/" << f3 << std::endl;
            }

            // materials
            const auto& textures = block->GetTextures();

            // only diffuse texture for now
            out_stream << "newmtl " << textures[0]->GetFileName().stem() << std::endl;
            out_stream << "map_Kd " << textures[0]->GetFileName().stem() << ".dds" << std::endl;
            out_stream << "usemtl " << textures[0]->GetFileName().stem() << std::endl;

            out_stream << std::endl;
        }
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem();
        path += GetExportExtension();

        LOG_INFO("Exporting model to \"{}\"", path.string());

        auto model = ::RenderBlockModel::get(filename.string());
        if (model) {
            WriteModelFile(path, model.get());
            callback(true);
        } else {
            FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, FileBuffer data) {
                if (success) {
                    auto model = std::make_unique<::RenderBlockModel>(filename);
                    if (model->Parse(data, false)) {
                        WriteModelFile(path, model.get());
                    }
                }

                callback(success);
            });
        }
    }
};

}; // namespace import_export
