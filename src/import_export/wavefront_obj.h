#pragma once

#include "IImportExporter.h"

class Wavefront_Obj : public IImportExporter
{
public:
    Wavefront_Obj() = default;
    virtual ~Wavefront_Obj() = default;

    ImportExportType GetType() override final { return IE_TYPE_EXPORTER; }
    const char* GetName() override final { return "Wavefront"; }
    const char* GetExtension() override final { return ".obj"; }

    void Export(RenderBlockModel* rbm) override final
    {
        DEBUG_LOG("Wavefront_Obj::Export");

        const auto block = rbm->GetRenderBlocks()[0];

        std::ofstream stream("C:/Users/aaron/Desktop/wavefront_exporter.obj", std::ios::binary);
        stream << "# File generated by jc3-rbm-renderer" << std::endl << std::endl;

        // read all the block vertices
        /*auto& vertices = block->GetVertices();
        for (auto i = 0; i < vertices.size(); i += 2) {
            auto x = JustCause3::Vertex::unpack(vertices[i]);
            auto y = JustCause3::Vertex::unpack(vertices[i + 1]);
            auto z = JustCause3::Vertex::unpack(vertices[i + 2]);

            stream << "v " << x << " " << y << " " << z << std::endl;
        }*/

        stream.close();
    }
};