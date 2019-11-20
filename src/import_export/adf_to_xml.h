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
#if 0
    template <typename T> inline void WriteScalarToPayload(T value, const char* payload, uint32_t payload_offset)
    {
        std::memcpy((char*)payload + payload_offset, &value, sizeof(T));
    }

    void WriteScalarToPayloadFromString(const jc::AvalancheDataFormat::Type* type, const char* payload,
                                        uint32_t payload_offset, const std::string& data)
    {
        using namespace jc::AvalancheDataFormat;

        // clang-format off
        switch (type->m_ScalarType) {
        case ScalarType::Signed:
            switch (type->m_Size) {
                case sizeof(int8_t):	WriteScalarToPayload<int8_t>(std::stoi(data), payload, payload_offset); break;
                case sizeof(int16_t):	WriteScalarToPayload<int16_t>(std::stoi(data), payload, payload_offset); break;
                case sizeof(int32_t):	WriteScalarToPayload<int32_t>(std::stol(data), payload, payload_offset); break;
                case sizeof(int64_t):	WriteScalarToPayload<int64_t>(std::stoll(data), payload, payload_offset); break;
            }
            break;
        case ScalarType::Unsigned:
            switch (type->m_Size) {
                case sizeof(uint8_t):	WriteScalarToPayload<uint8_t>(std::stoi(data), payload, payload_offset); break;
                case sizeof(uint16_t):	WriteScalarToPayload<uint16_t>(std::stoi(data), payload, payload_offset); break;
                case sizeof(uint32_t):	WriteScalarToPayload<uint32_t>(std::stoul(data), payload, payload_offset); break;
                case sizeof(uint64_t):	WriteScalarToPayload<uint64_t>(std::stoull(data), payload, payload_offset); break;
            }
            break;
        case ScalarType::Float:
            switch (type->m_Size) {
                case sizeof(float):		WriteScalarToPayload(std::stof(data), payload, payload_offset); break;
                case sizeof(double):	WriteScalarToPayload(std::stod(data), payload, payload_offset); break;
            }
            break;
        }
        // clang-format on
    }
#endif

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
