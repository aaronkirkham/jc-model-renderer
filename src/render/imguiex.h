#ifndef JCMR_RENDER_IMGUIEX_H_HEADER_GUARD
#define JCMR_RENDER_IMGUIEX_H_HEADER_GUARD

#include "platform.h"
#include <imgui.h>

#define IMGUIEX_TABLE_COL(label, widget) ImGuiEx::WidgetTableLayoutColumn(label, [&] { widget; });

namespace ImGuiEx
{
typedef int InputTextureFlags;
enum InputTextureFlags_ {
    InputTextureFlags_None      = 0,
    InputTextureFlags_NoTooltip = (1 << 0),
};

void Label(const char* fmt, ...);
void SpinningIconLabel(const char* icon, const char* fmt, ...);

bool InputTexture(const char* label, char* buf, size_t buf_size, ImTextureID tex_id, InputTextureFlags flags = 0);
bool ColorButtonPicker3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
bool ColorButtonPicker4(const char* label, float col[4], ImGuiColorEditFlags flags = 0);

bool BeginWidgetTableLayout(float label_column_width = 0.3f);
void EndWidgetTableLayout();
void WidgetTableLayoutColumn(const char* label, std::function<void()> callback);
} // namespace ImGuiEx

#endif // JCMR_RENDER_IMGUIEX_H_HEADER_GUARD
