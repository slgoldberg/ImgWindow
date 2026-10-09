// ============================================================================
// ImgWindow Contrib Widget: ClickableLink
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Ergonomic link affordance helpers for Dear ImGui widgets:
// Eliminates repetitive boilerplate by automatically assigning the hand cursor
// (ImGuiMouseCursor_Hand) to buttons, checkboxes, text links, and selectables
// when hovered.
// ============================================================================

#pragma once

#ifndef IMGUI_DISABLE_EXTRA_CLICKABLE_LINK
#ifndef IMGUI_EXTRA_CLICKABLE_LINK_H
#define IMGUI_EXTRA_CLICKABLE_LINK_H

#include "imgui.h"
#include <string>

namespace ImGui {

    // -------------------------------------------------------------------------
    // 1. Core Affordance Helper
    // -------------------------------------------------------------------------
    // Call immediately after drawing any interactive element to change the
    // mouse cursor to ImGuiMouseCursor_Hand when hovered.
    inline void ShowLinkCursorOnHover()
    {
        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }

    // -------------------------------------------------------------------------
    // 2. Buttons with Hand Cursor
    // -------------------------------------------------------------------------

    inline bool ButtonLink(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Button(label, size);
        ShowLinkCursorOnHover();
        return result;
    }

    inline bool ButtonLink(const std::string& label, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        return ButtonLink(label.c_str(), size);
    }

    inline bool SmallButtonLink(const char* label)
    {
        bool result = SmallButton(label);
        ShowLinkCursorOnHover();
        return result;
    }

    // -------------------------------------------------------------------------
    // 3. Clickable Text Links
    // -------------------------------------------------------------------------

    inline bool TextLink(const char* label, const ImVec4* col_override = nullptr)
    {
        ImVec4 link_col = col_override ? *col_override : GetStyle().Colors[ImGuiCol_HeaderActive];
        if (link_col.w <= 0.0f) link_col = GetStyle().Colors[ImGuiCol_ButtonActive];

        PushStyleColor(ImGuiCol_Text, link_col);
        TextUnformatted(label);
        PopStyleColor();

        ShowLinkCursorOnHover();
        return IsItemClicked();
    }

    inline bool TextLink(const std::string& label, const ImVec4* col_override = nullptr)
    {
        return TextLink(label.c_str(), col_override);
    }

    // -------------------------------------------------------------------------
    // 4. Input & Selection Controls with Hand Cursor
    // -------------------------------------------------------------------------

    inline bool CheckboxLink(const char* label, bool* v)
    {
        bool result = Checkbox(label, v);
        ShowLinkCursorOnHover();
        return result;
    }

    inline bool RadioButtonLink(const char* label, bool active)
    {
        bool result = RadioButton(label, active);
        ShowLinkCursorOnHover();
        return result;
    }

    inline bool RadioButtonLink(const char* label, int* v, int v_button)
    {
        bool result = RadioButton(label, v, v_button);
        ShowLinkCursorOnHover();
        return result;
    }

    inline bool SelectableLink(const char* label,
                               bool selected = false,
                               ImGuiSelectableFlags flags = 0,
                               const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Selectable(label, selected, flags, size);
        ShowLinkCursorOnHover();
        return result;
    }

    inline bool SelectableLink(const char* label,
                               bool* p_selected,
                               ImGuiSelectableFlags flags = 0,
                               const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Selectable(label, p_selected, flags, size);
        ShowLinkCursorOnHover();
        return result;
    }

} // namespace ImGui

#endif // IMGUI_EXTRA_CLICKABLE_LINK_H
#endif // IMGUI_DISABLE_EXTRA_CLICKABLE_LINK
