#pragma once

#include "IImportExporter.h"

namespace import_export
{

class Wavefront_Obj : public IImportExporter
{
public:
    Wavefront_Obj() = default;
    virtual ~Wavefront_Obj() = default;

    ImportExportType GetType() override final { return IE_TYPE_EXPORTER; }
    const char* GetName() override final { return "Wavefront"; }
    std::vector<const char*> GetInputExtensions() override final { return { ".rbm" }; }
    const char* GetOutputExtension() override final { return ".obj"; }

    void Export(const fs::path& filename, const std::any& input, const fs::path& to, ImportExportFinishedCallback callback) override final
    {
        DEBUG_LOG("Wavefront_Obj::Export");

        auto& buffer = std::any_cast<FileBuffer>(input);

        const auto rbm = new RenderBlockModel(filename);
        if (rbm->Parse(buffer)) {
            const auto& blocks = rbm->GetRenderBlocks();

            std::ofstream out_stream("C:/Users/aaron/Desktop/obj/wavefront_exporter.obj", std::ios::binary);
            out_stream << "# File generated by jc3-rbm-renderer" << std::endl << std::endl;

            for (const auto& block : rbm->GetRenderBlocks()) {
                out_stream << "# " << block->GetTypeName() << std::endl;

                // vertices
                const auto& vertices = block->GetVertices();
                for (auto i = 0; i < vertices.size(); i += 3) {
                    out_stream << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << std::endl;
                }

                out_stream << std::endl;

                // uvs
                const auto& uvs = block->GetUVs();
                for (auto i = 0; i < uvs.size(); i += 2) {
                    out_stream << "vt " << uvs[i] << " " << uvs[i + 1] << std::endl;
                }

                out_stream << std::endl;

                // faces
                const auto& indices = block->GetIndices();
                for (auto i = 0; i < indices.size(); i += 3) {

                    //auto uv_x = uvs[indices[i]];
                    //auto uv_y = uvs[indices[i + 1]];

                    auto uv_x = 50;
                    auto uv_y = 51;

                    out_stream << "f ";
                    out_stream << indices[i] + 1 << "/" << uv_x << " ";
                    out_stream << indices[i + 1] + 1 << "/" << uv_y << " ";
                    out_stream << indices[i + 2] + 1 << "/" << "1";
                    out_stream << std::endl;
                }

                out_stream << std::endl;

                // materials
                for (const auto& texture : block->GetTextures()) {
                    auto& filename = texture->GetPath().stem();

                    out_stream << "newmtl " << filename.string() << std::endl;
                    out_stream << "map_Kd " << filename.string() << ".dds" << std::endl;
                    out_stream << "usemtl " << filename.string() << std::endl << std::endl;

                    // TODO: also export the textures
                }
            }

            out_stream.close();
        }

        //delete rbm;

        if (callback) {
            callback();
        }
    }
};

};