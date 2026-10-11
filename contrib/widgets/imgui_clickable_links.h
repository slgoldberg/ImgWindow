// ============================================================================
// ImGui Contrib Widgets: Clickable Links & Item Cursor (imgui_clickable_links.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Ergonomic link affordance helpers for Dear ImGui widgets:
// Eliminates repetitive boilerplate by automatically attaching the hand cursor
// (ImGuiMouseCursor_Hand) to buttons, checkboxes, text links, and selectables
// when hovered.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_CLICKABLE_LINKS) && \
    !defined(CONTRIB_DISABLE_CLICKABLE_LINK) && \
    !defined(IMGUI_CONTRIB_DISABLE_CLICKABLE_LINKS) && \
    !defined(IMGUI_CONTRIB_DISABLE_CLICKABLE_LINK) && \
    !defined(IMGUI_DISABLE_CONTRIB_CLICKABLE_LINKS) && \
    !defined(IMGUI_DISABLE_CONTRIB_CLICKABLE_LINK) && \
    !defined(IMGUI_DISABLE_CLICKABLE_LINKS) && \
    !defined(IMGUI_DISABLE_CLICKABLE_LINK) && \
    !defined(IMGUI_DISABLE_EXTRA_CLICKABLE_LINKS) && \
    !defined(IMGUI_DISABLE_EXTRA_CLICKABLE_LINK)
#ifndef IMGUI_CLICKABLE_LINKS_H
#define IMGUI_CLICKABLE_LINKS_H
#define IMGUI_EXTRA_CLICKABLE_LINK_H

#include "imgui.h"
#include <string>

namespace ImGui {

    // -------------------------------------------------------------------------
    // 1. Core Item Cursor Chaser (AttachItemCursor)
    // -------------------------------------------------------------------------
    // Call immediately after drawing any interactive element (Item) to attach
    // a hover cursor (defaults to ImGuiMouseCursor_Hand when no cursor is passed).
    inline IMGUI_API void AttachItemCursor(ImGuiMouseCursor cursor = ImGuiMouseCursor_Hand)
    {
        if (IsItemHovered()) {
            SetMouseCursor(cursor);
        }
    }

    // Aliases for discoverability & backward compatibility:
    inline IMGUI_API void ShowItemCursor(ImGuiMouseCursor cursor = ImGuiMouseCursor_Hand)
    {
        AttachItemCursor(cursor);
    }

    inline IMGUI_API void SetItemCursor(ImGuiMouseCursor cursor = ImGuiMouseCursor_Hand)
    {
        AttachItemCursor(cursor);
    }

    inline IMGUI_API void SetItemHoverCursor(ImGuiMouseCursor cursor = ImGuiMouseCursor_Hand)
    {
        AttachItemCursor(cursor);
    }

    inline IMGUI_API void ShowItemCursorOnHover(ImGuiMouseCursor cursor = ImGuiMouseCursor_Hand)
    {
        AttachItemCursor(cursor);
    }

    inline IMGUI_API void ShowCursorOnHover(ImGuiMouseCursor cursor = ImGuiMouseCursor_Hand)
    {
        AttachItemCursor(cursor);
    }

    inline IMGUI_API void ShowLinkCursorOnHover()
    {
        AttachItemCursor(ImGuiMouseCursor_Hand);
    }

    // -------------------------------------------------------------------------
    // 2. Buttons with Hand Cursor
    // -------------------------------------------------------------------------

    inline IMGUI_API bool ButtonLink(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Button(label, size);
        AttachItemCursor();
        return result;
    }

    inline IMGUI_API bool ButtonLink(const std::string& label, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        return ButtonLink(label.c_str(), size);
    }

    inline IMGUI_API bool SmallButtonLink(const char* label)
    {
        bool result = SmallButton(label);
        AttachItemCursor();
        return result;
    }

    inline IMGUI_API bool SmallButtonLink(const std::string& label)
    {
        return SmallButtonLink(label.c_str());
    }

    // -------------------------------------------------------------------------
    // 3. Clickable Text Links
    // -------------------------------------------------------------------------
    // Renders text with the hand cursor (ImGuiMouseCursor_Hand) on hover and
    // returns true if clicked. Preserves the current ambient text color unless
    // an explicit 'col_override' is supplied.
    //
    // Note: 1-argument ImGui::TextLink(const char*) is provided natively by
    // Dear ImGui v1.91+ (which also sets ImGuiMouseCursor_Hand on hover).
    // Omitting the default parameter on the const char* overload below avoids
    // any overload ambiguity with Dear ImGui's native TextLink(const char*).

    inline IMGUI_API bool TextLinkUnformatted(const char* text, const char* text_end = nullptr)
    {
        TextUnformatted(text, text_end);
        AttachItemCursor();
        return IsItemClicked();
    }

    inline IMGUI_API bool TextLink(const char* label, const ImVec4* col_override)
    {
        if (col_override) {
            PushStyleColor(ImGuiCol_Text, *col_override);
        }
        TextUnformatted(label);
        if (col_override) {
            PopStyleColor();
        }

        AttachItemCursor();
        return IsItemClicked();
    }

    inline IMGUI_API bool TextLink(const std::string& label, const ImVec4* col_override = nullptr)
    {
        return TextLink(label.c_str(), col_override);
    }

    // -------------------------------------------------------------------------
    // 4. Input & Selection Controls with Hand Cursor
    // -------------------------------------------------------------------------

    inline IMGUI_API bool CheckboxLink(const char* label, bool* v)
    {
        bool result = Checkbox(label, v);
        AttachItemCursor();
        return result;
    }

    inline IMGUI_API bool RadioButtonLink(const char* label, bool active)
    {
        bool result = RadioButton(label, active);
        AttachItemCursor();
        return result;
    }

    inline IMGUI_API bool RadioButtonLink(const char* label, int* v, int v_button)
    {
        bool result = RadioButton(label, v, v_button);
        AttachItemCursor();
        return result;
    }

    inline IMGUI_API bool SelectableLink(const char* label,
                                         bool selected = false,
                                         ImGuiSelectableFlags flags = 0,
                                         const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Selectable(label, selected, flags, size);
        AttachItemCursor();
        return result;
    }

    inline IMGUI_API bool SelectableLink(const char* label,
                                         bool* p_selected,
                                         ImGuiSelectableFlags flags = 0,
                                         const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Selectable(label, p_selected, flags, size);
        AttachItemCursor();
        return result;
    }

} // namespace ImGui

#endif // IMGUI_EXTRA_CLICKABLE_LINK_H
#endif // IMGUI_DISABLE_EXTRA_CLICKABLE_LINK
