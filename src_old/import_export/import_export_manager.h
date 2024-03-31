#pragma once

#include <algorithm>
#include <vector>

#include "../singleton.h"

class IImportExporter;
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

    // Get a list of importers for a given extension
    std::vector<IImportExporter*> GetImportersFor(const std::string& extension)
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

    // Get a list of importers from a given exported extension
    std::vector<IImportExporter*> GetImportersFrom(const std::string& extension, bool check_drag_drop_status = false)
    {
        std::vector<IImportExporter*> result;
        for (const auto& item : m_Items) {
            if (item->GetType() == ImportExportType_Importer || item->GetType() == ImportExportType_Both) {
                if (check_drag_drop_status && item->IsDragDropImportable() && extension == item->GetExportExtension()) {
                    result.emplace_back(item.get());
                } else if (!check_drag_drop_status && extension == item->GetExportExtension()) {
                    result.emplace_back(item.get());
                }
            }
        }

        return result;
    }

    // Get a list of exporters for a given extension
    std::vector<IImportExporter*> GetExportersFor(const std::string& extension)
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
