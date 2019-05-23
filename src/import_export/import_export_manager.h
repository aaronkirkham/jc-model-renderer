#pragma once

#include <algorithm>
#include <vector>

#include "../singleton.h"
#include "iimportexporter.h"

class ImportExportManager : public Singleton<ImportExportManager>
{
  private:
    std::vector<std::unique_ptr<IImportExporter>> m_Items;

  public:
    ImportExportManager()          = default;
    virtual ~ImportExportManager() = default;

    template <typename T, class... Args> void Register(Args&&... args)
    {
        m_Items.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    std::vector<IImportExporter*> GetImporters(const std::string& extension)
    {
        std::vector<IImportExporter*> result;
        for (const auto& item : m_Items) {
            if (item->GetType() == ImportExportType_Importer || item->GetType() == ImportExportType_Both) {
                const auto& extensions = item->GetImportExtension();
                if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                    result.emplace_back(item.get());
                }
            }
        }

        return result;
    }

    std::vector<IImportExporter*> GetExporters(const std::string& extension)
    {
        std::vector<IImportExporter*> result;
        for (const auto& item : m_Items) {
            if (item->GetType() == ImportExportType_Exporter || item->GetType() == ImportExportType_Both) {
                const auto& extensions = item->GetImportExtension();
                if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                    result.emplace_back(item.get());
                }
            }
        }

        return result;
    }
};
