#pragma once

#include "iimportexporter.h"

class IRenderBlock;
namespace ImportExport
{
using materials_map_t = std::map<std::string, std::vector<std::pair<std::string, std::filesystem::path>>>;

class Wavefront_Obj : public IImportExporter
{
  private:
    uint32_t m_ExporterFlags = 0;

    materials_map_t ImportMaterials(const std::filesystem::path& filename);

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

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final;

    void WriteModelFile(const std::filesystem::path& filename, const std::filesystem::path& path,
                        const std::vector<IRenderBlock*>& render_blocks);

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final;
}; // namespace ImportExport
}; // namespace ImportExport
