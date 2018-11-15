#pragma once

#include "IWidget.h"
#include <json.hpp>

class AvalancheArchive;
class Widget_FileExplorer : public IWidget
{
  public:
    Widget_FileExplorer();

    virtual void Render(RenderContext_t*) override;

    static void RenderTreeView(nlohmann::json* tree, AvalancheArchive* current_archive = nullptr,
                               std::string acc_filepath = "");
};
