#pragma once

#include <algorithm>
#include <vector>

#include "iimportexporter.h"
#include "../singleton.h"

class ImportExportManager : public Singleton<ImportExportManager>
{
  private:
    std::vector<IImportExporter*> m_Items;

  public:
    ImportExportManager()          = default;
    virtual ~ImportExportManager() = default;

    void Register(IImportExporter* ie)
    {
        m_Items.emplace_back(ie);
    }

    std::vector<IImportExporter*> GetImporters(const std::string& extension)
    {
        std::vector<IImportExporter*> result;
        std::copy_if(m_Items.begin(), m_Items.end(), std::back_inserter(result), [&](IImportExporter* item) {
            if (item->GetType() == IE_TYPE_IMPORTER || item->GetType() == IE_TYPE_BOTH) {
                const auto& extensions = item->GetImportExtension();
                if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                    return true;
                }
            }

            return false;
        });

        return result;
    }

    std::vector<IImportExporter*> GetExporters(const std::string& extension)
    {
        std::vector<IImportExporter*> result;
        std::copy_if(m_Items.begin(), m_Items.end(), std::back_inserter(result), [&](IImportExporter* item) {
            if (item->GetType() == IE_TYPE_EXPORTER || item->GetType() == IE_TYPE_BOTH) {
                const auto& extensions = item->GetImportExtension();
                if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                    return true;
                }
            }

            return false;
        });

        return result;
    }
};
