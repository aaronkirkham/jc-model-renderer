#pragma once

#include <singleton.h>
#include <import_export/IImportExporter.h>
#include <vector>

class ImportExportManager : public Singleton<ImportExportManager>
{
private:
    std::vector<IImportExporter*> m_Items;

public:
    ImportExportManager() = default;
    virtual ~ImportExportManager() = default;

    void Register(IImportExporter* ie)
    {
        m_Items.emplace_back(ie);
    }

    std::vector<IImportExporter*> GetExportersForExtension(const std::string& extension)
    {
        std::vector<IImportExporter*> result;
        std::copy_if(m_Items.begin(), m_Items.end(), std::back_inserter(result), [&](IImportExporter* item) {
            if (item->GetType() == IE_TYPE_EXPORTER || item->GetType() == IE_TYPE_BOTH) {
                const auto& extensions = item->GetInputExtensions();
                if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                    return true;
                }
            }

            return false;
        });

        return result;
    }
};