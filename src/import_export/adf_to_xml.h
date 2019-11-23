#pragma once

#include "iimportexporter.h"

#if 0
#include <bitset>

#include "iimportexporter.h"

#include "../util.h"
#include "../window.h"

#include "../game/file_loader.h"
#include "../game/formats/avalanche_data_format.h"

#include <tinyxml2.h>
#endif

namespace ImportExport
{
class ADF2XML : public IImportExporter
{
  private:
    // @NOTE: helper class just so we can access protected ADF class members!
    class ADF2XMLHelper : public ava::AvalancheDataFormat::ADF
    {
      public:
        ADF2XMLHelper()  = default;
        ~ADF2XMLHelper() = default;

        void PushType(ava::AvalancheDataFormat::AdfType* type)
        {
            m_Types.push_back(type);
        }
    };

  public:
    ADF2XML()          = default;
    virtual ~ADF2XML() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Both;
    }

    const char* GetImportName() override final
    {
        return "Avalanche Data Format";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        // clang-format off
        static std::vector<std::string> extensions = {
            ".atunec", ".ctunec", ".etunec", ".ftunec", ".gtunec", ".mtunec", ".wtunec", ".cmtunec",
            ".effc", ".blo_adf", ".flo_adf", ".epe_adf", ".vmodc", ".environc", ".aisystunec",
            ".weaponsc", ".asynccc", ".vfxsettingsc", ".vocals_settingsc", ".vocalsc",
            ".streampatch", ".roadgraphc", ".shudc", ".revolutionc", ".worldaudioinfo_xmlc",
            ".onlinec", ".statisticsc", ".gadfc", ".gdc", ".terrainsystemc", ".vegetationinfo",
            ".graphc", ".nhf", ".encountersc", ".collectionc", ".objectivesc", ".physicstunec",
            ".vfxc", ".reactionc", ".modelc", ".meshc", ".hrmeshc"
        };
        // clang-format on

        return extensions;
    }

    const char* GetExportName() override final
    {
        return "XML";
    }

    const char* GetExportExtension() override final
    {
        return ".xml";
    }

    bool IsDragDropImportable() override final
    {
        return true;
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final;
    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final;
};
}; // namespace ImportExport
