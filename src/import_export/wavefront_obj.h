#pragma once

#include "iimportexporter.h"

class IRenderBlock;
namespace ImportExport
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
        return ImportExportType_Both;
    }

    const char* GetImportName() override final
    {
        return "Avalanche Model Format";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        return {".rbm", ".modelc"};
    }

    const char* GetExportName() override final
    {
        return "Wavefront";
    }

    const char* GetExportExtension() override final
    {
        return ".obj";
    }

    bool HasSettingsUI() override final
    {
        return true;
    }

    bool DrawSettingsUI() override final;

    std::map<std::string, std::string> ImportMaterials(const std::filesystem::path& filename)
    {
        if (!std::filesystem::exists(filename)) {
            return {};
        }

        std::ifstream stream(filename);
        if (stream.fail()) {
            return {};
        }

        std::map<std::string, std::string> materials;

        std::string line;
        std::string current_mtl;
        while (std::getline(stream, line)) {
            const auto& type = line.substr(0, line.find_first_of(' '));
            if (type == "newmtl") {
                current_mtl = line.substr(7, line.length());
            } else if (type == "map_Kd") {
                auto diffuse = filename;
                diffuse.replace_filename(line.substr(7, line.length()));
                materials[current_mtl] = diffuse.string();
            }
        }

        return materials;
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final;

    void WriteModelFile(const std::filesystem::path& filename, const std::filesystem::path& path,
                        const std::vector<IRenderBlock*>& render_blocks);

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final;
};
}; // namespace ImportExport
