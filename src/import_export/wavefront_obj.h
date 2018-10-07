#pragma once

#include "IImportExporter.h"

namespace import_export
{

class Wavefront_Obj : public IImportExporter
{
  public:
    Wavefront_Obj()          = default;
    virtual ~Wavefront_Obj() = default;

    ImportExportType GetType() override final
    {
        return IE_TYPE_EXPORTER;
    }
    const char* GetName() override final
    {
        return "Wavefront";
    }
    std::vector<const char*> GetInputExtensions() override final
    {
        return {".rbm"};
    }
    const char* GetOutputExtension() override final
    {
        return ".obj";
    }

    void WriteModelFile(const fs::path& path, ::RenderBlockModel* model)
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
            out_stream << "# " << block->GetTypeName() << std::endl;

            // vertices
            const auto& vertices = block->GetVertices();
            for (auto i = 0; i < vertices.size(); i += 3) {
                out_stream << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << std::endl;
            }

            // uvs
            const auto& uvs = block->GetUVs();
            for (auto i = 0; i < uvs.size(); i += 2) {
                out_stream << "vt " << uvs[i] << " " << uvs[i + 1] << std::endl;
            }

            // normals
            // TODO

            // faces
            const auto& indices = block->GetIndices();
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
            // for (const auto& texture : block->GetTextures()) {
            //    auto& filename = texture->GetPath().stem();

            //    out_stream << "newmtl " << filename.string() << std::endl;
            //    out_stream << "map_Kd " << filename.string() << ".dds" << std::endl;
            //    out_stream << "usemtl " << filename.string() << std::endl << std::endl;
            //}

            //out_stream << "newmtl racing_arrows_dif" << std::endl
            //           << "map_Kd racing_arrows_dif.dds" << std::endl
            //           << "usemtl racing_arrows_dif" << std::endl;
        }
    }

    void Export(const fs::path& filename, const fs::path& to, ImportExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem();
        path += ".obj";

        DEBUG_LOG("Wavefront_Obj::Export - Exporting model to '" << path << "'...");

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
