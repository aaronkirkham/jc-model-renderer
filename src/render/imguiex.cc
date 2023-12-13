#include "pch.h"

#include "app/os.h"
#include "imguiex.h"

#include <imgui_internal.h>

namespace ImGuiEx
{
using namespace ImGui;

ImVec2 ImVec2_Add(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2((lhs.x + rhs.x), (lhs.y + rhs.y));
}

ImVec2 ImVec2_Sub(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2((lhs.x - rhs.x), (lhs.y - rhs.y));
}

void Label(const char* fmt, ...)
{
    ImGuiContext& g      = *GImGui;
    ImGuiWindow*  window = g.CurrentWindow;

    const float f_width = GetContentRegionAvail().x;

    va_list args;
    va_start(args, fmt);
    const char *text_begin, *text_end;
    ImFormatStringToTempBufferV(&text_begin, &text_end, fmt, args);
    va_end(args);

    ImVec2 text_size = CalcTextSize(text_begin, text_end);

    ImRect text_bb;
    text_bb.Min = GetCursorScreenPos();
    text_bb.Min.x += g.Style.ItemSpacing.x;
    text_bb.Max = {text_bb.Min.x + f_width, text_bb.Min.y + text_size.y};

    SetCursorScreenPos(text_bb.Min);

    AlignTextToFramePadding();
    text_bb.Min.y += window->DC.CurrLineTextBaseOffset;
    text_bb.Max.y += window->DC.CurrLineTextBaseOffset;

    ItemSize(text_bb);
    if (ItemAdd(text_bb, window->GetID(text_begin))) {
        RenderTextEllipsis(GetWindowDrawList(), text_bb.Min, text_bb.Max, text_bb.Max.x, text_bb.Max.x, text_begin,
                           text_end, &text_size);

        if (text_bb.GetWidth() < text_size.x && IsItemHovered()) {
            SetTooltip(text_begin);
        }
    }
}

void SpinningIconLabel(const char* icon, const char* fmt, ...)
{
    ImGuiContext& g      = *GImGui;
    ImGuiWindow*  window = g.CurrentWindow;

    auto draw_list_buffer_size = window->DrawList->VtxBuffer.Size;

    // icon
    ImGui::Text(icon);

    // calculate center of rotation
    ImVec2 rotation_center;
    {
        ImVec2 bounds_u{FLT_MAX, FLT_MAX};
        ImVec2 bounds_l{-FLT_MAX, -FLT_MAX};

        for (int i = draw_list_buffer_size; i < window->DrawList->VtxBuffer.Size; ++i) {
            bounds_u = ImMin(bounds_u, window->DrawList->VtxBuffer[i].pos);
            bounds_l = ImMax(bounds_l, window->DrawList->VtxBuffer[i].pos);
        }

        rotation_center = ImVec2{(bounds_l.x + bounds_u.x) / 2, (bounds_l.y + bounds_u.y) / 2};
    }

    // spinning icon
    {
        float rotation_amount = (-0.005f * jcmr::os::get_tick_count());
        float sin_a           = std::sinf(rotation_amount);
        float cos_a           = std::cosf(rotation_amount);

        auto center = ImVec2_Sub(ImRotate(rotation_center, sin_a, cos_a), rotation_center);

        for (int i = draw_list_buffer_size; i < window->DrawList->VtxBuffer.Size; ++i) {
            window->DrawList->VtxBuffer[i].pos =
                ImVec2_Sub(ImRotate(window->DrawList->VtxBuffer[i].pos, sin_a, cos_a), center);
        }
    }

    va_list args;
    va_start(args, fmt);
    const char *text_begin, *text_end;
    ImFormatStringToTempBufferV(&text_begin, &text_end, fmt, args);
    va_end(args);

    ImGui::SameLine();
    ImGui::TextEx(text_begin, text_end);
}

bool TextureButton(const char* desc_id, ImTextureID tex_id, InputTextureFlags flags, ImVec2 size = ImVec2(0, 0))
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ImGuiContext& g  = *GImGui;
    const ImGuiID id = window->GetID(desc_id);

    float default_size = GetFrameHeight();
    if (size.x == 0.0f) size.x = default_size;
    if (size.y == 0.0f) size.y = default_size;

    const ImRect bb(window->DC.CursorPos, ImVec2_Add(window->DC.CursorPos, size));
    ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
    if (!ItemAdd(bb, id)) {
        return false;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    float  rounding = g.Style.FrameRounding;
    ImRect bb_inner = bb;
    bb_inner.Expand(-0.75f);
    window->DrawList->AddImageRounded(tex_id, bb_inner.Min, bb_inner.Max, ImVec2(0, 0), ImVec2(1, 1),
                                      GetColorU32(ImVec4(1, 1, 1, 1)), rounding);

    if (g.Style.FrameBorderSize > 0.0f) {
        RenderFrameBorder(bb.Min, bb.Max, rounding);
    }

    // if (!(flags & InputTextureFlags_NoTooltip) && hovered) {
    //     TextureTooltip(desc_id, tex_id, v);
    // }

    return pressed;
}

bool InputTexture(const char* label, char* buf, size_t buf_size, ImTextureID tex_id, InputTextureFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ImGuiContext& g     = *GImGui;
    ImGuiStyle&   style = g.Style;

    float       square_sz         = GetFrameHeight();
    float       w_full            = CalcItemWidth();
    float       w_button          = (square_sz + style.ItemInnerSpacing.x);
    float       w_input           = (w_full - w_button);
    const char* label_display_end = FindRenderedTextEnd(label);

    BeginGroup();
    PushID(label);

    ImVec2 pos             = window->DC.CursorPos;
    float  inputs_offset_x = 0.0f;
    window->DC.CursorPos.x = (pos.x + inputs_offset_x);

    // text input
    SetNextItemWidth(w_input);
    bool changed = InputText("##input_text", buf, buf_size);

    // texture button
    float button_offset_x = (w_input + style.ItemInnerSpacing.x);
    window->DC.CursorPos  = ImVec2(pos.x + button_offset_x, pos.y);

    if (TextureButton(label, tex_id, flags)) {
        OpenPopup("picker");
        SetNextWindowPos(ImVec2_Add(g.LastItemData.Rect.GetBL(), ImVec2(-1, style.ItemInnerSpacing.y)));
    }

    if (BeginPopup("picker")) {
        if (label != label_display_end) {
            TextEx(label, label_display_end);
            Spacing();
        }

        Image(tex_id, {square_sz * 10.0f, square_sz * 10.0f});
        EndPopup();
    }

    // label
    if (label != label_display_end) {
        float text_offset_x    = (w_full + style.ItemInnerSpacing.x);
        window->DC.CursorPos.x = text_offset_x;
        TextEx(label, label_display_end);
    }

    PopID();
    EndGroup();

    return changed;
}

bool ColorButtonPicker3(const char* label, float col[4], ImGuiColorEditFlags flags)
{
    return ColorButtonPicker4(label, col, flags | ImGuiColorEditFlags_NoAlpha);
}

bool ColorButtonPicker4(const char* label, float col[4], ImGuiColorEditFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ImGuiContext&     g     = *GImGui;
    const ImGuiStyle& style = g.Style;

    const float square_sz         = GetFrameHeight();
    const float w_button          = GetContentRegionAvail().x;
    const char* label_display_end = FindRenderedTextEnd(label);

    bool       value_changed = false;
    const bool alpha         = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
    const int  components    = alpha ? 4 : 3;

    BeginGroup();
    PushID(label);

    const ImGuiColorEditFlags flags_untouched = flags;

    const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
    if (ColorButton(label, col_v4, flags, {w_button, 0})) {
        // Store current color and open a picker
        g.ColorPickerRef = col_v4;
        OpenPopup("picker");

        auto bottom_left = g.LastItemData.Rect.GetBL();
        SetNextWindowPos({bottom_left.x + -1, bottom_left.y + style.ItemSpacing.y});
    }

    ImGuiWindow* picker_active_window = nullptr;
    if (BeginPopup("picker")) {
        picker_active_window = g.CurrentWindow;
        if (label != label_display_end) {
            TextEx(label, label_display_end);
            Spacing();
        }

        ImGuiColorEditFlags picker_flags_to_forward =
            ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_
            | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
        ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward)
                                           | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel
                                           | ImGuiColorEditFlags_AlphaPreviewHalf;

        SetNextItemWidth(square_sz * 12.0f);
        value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
        EndPopup();
    }

    PopID();
    EndGroup();

    // Drag and Drop Target
    // NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
    const bool can_drag_drop =
        (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop);
    if (can_drag_drop && BeginDragDropTarget()) {
        bool accepted_drag_drop = false;
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F)) {
            memcpy((float*)col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
            value_changed = accepted_drag_drop = true;
        }

        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F)) {
            memcpy((float*)col, payload->Data, sizeof(float) * components);
            value_changed = accepted_drag_drop = true;
        }

        EndDragDropTarget();
    }

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window) {
        g.LastItemData.ID = g.ActiveId;
    }

    if (value_changed) {
        MarkItemEdited(g.LastItemData.ID);
    }

    return value_changed;
}

bool BeginWidgetTableLayout(float label_column_width)
{
    if (!ImGui::BeginTable("##widget_table", 2, ImGuiTableFlags_NoSavedSettings)) {
        return false;
    }

    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed,
                            (ImGui::GetContentRegionAvail().x * label_column_width));
    ImGui::TableSetupColumn("Widget");
    return true;
}

void EndWidgetTableLayout()
{
    ImGui::EndTable();
}

void WidgetTableLayoutColumn(const char* label, std::function<void()> callback)
{
    // label
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGuiEx::Label(label);

    // widget
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::PushID(label);
    {
        callback();
    }
    ImGui::PopID();
}
} // namespace ImGuiEx