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

    const std::vector<IImportExporter*>& GetItems() { return m_Items; }

    std::vector<IImportExporter*> GetImporters()
    {
        decltype(GetImporters()) result;
        std::copy_if(m_Items.begin(), m_Items.end(), std::back_inserter(result), [](IImportExporter* ie) {
            return (ie->GetType() == IE_TYPE_IMPORTER || ie->GetType() == IE_TYPE_BOTH);
        });

        return result;
    }

    std::vector<IImportExporter*> GetExporters()
    {
        decltype(GetExporters()) result;
        std::copy_if(m_Items.begin(), m_Items.end(), std::back_inserter(result), [](IImportExporter* ie) {
            return (ie->GetType() == IE_TYPE_EXPORTER || ie->GetType() == IE_TYPE_BOTH);
        });

        return result;
    }
};