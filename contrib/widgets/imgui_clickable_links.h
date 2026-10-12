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

#ifndef IMGUI_LINK_CLICK_RESULT_DEFINED
#define IMGUI_LINK_CLICK_RESULT_DEFINED
namespace ImGui {
namespace Detail {
    struct LinkClickToggleState {
        int     frame_count    = -1;
        ImGuiID item_id        = 0;
        ImVec2  item_min       = ImVec2(-1.0f, -1.0f);
        bool    toggle_mode    = false;
        bool    click_consumed = false;
    };

    inline LinkClickToggleState& GetLinkClickToggleState() {
        static LinkClickToggleState s_State;
        return s_State;
    }

    inline void RegisterItemTooltipToggle(bool is_toggle_eligible = true) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        int cur_frame = GetFrameCount();
        ImGuiID cur_id = GetItemID();
        ImVec2 cur_min = GetItemRectMin();
        bool same_item = (st.frame_count == cur_frame) &&
                         ((cur_id != 0 && st.item_id == cur_id) ||
                          (cur_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
        if (!same_item) {
            st.frame_count    = cur_frame;
            st.item_id        = cur_id;
            st.item_min       = cur_min;
            st.click_consumed = false;
        }
        st.toggle_mode = st.click_consumed ? false : is_toggle_eligible;
    }

    inline void MarkLastItemClickConsumed() {
        LinkClickToggleState& st = GetLinkClickToggleState();
        if (st.frame_count == GetFrameCount()) {
            st.click_consumed = true;
            st.toggle_mode    = false;
        }
    }

    inline bool ShouldToggleTooltipOnClick(ImGuiID raw_item_id, bool is_item_active) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImVec2 cur_min = GetItemRectMin();
        if (st.frame_count == GetFrameCount()) {
            if ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y)) {
                return st.toggle_mode;
            }
        }
        return !is_item_active;
    }
} // namespace Detail

    // Return-value proxy for *Link widgets (ButtonLink, SmallButtonLink, TextLink, MD::TextLink):
    // Implicitly converts to bool when tested in `if (...)` or assigned to a bool, marking the
    // click as handled by the caller. When called as a standalone statement without testing
    // the return value, `operator bool()` is not called, allowing a trailing TimedTooltip
    // to automatically toggle ON/OFF when the widget is clicked!
    struct LinkClickResult {
        bool clicked;

        constexpr LinkClickResult(bool is_clicked = false) noexcept : clicked(is_clicked) {}

        operator bool() const noexcept {
            Detail::MarkLastItemClickConsumed();
            return clicked;
        }
    };
} // namespace ImGui
#endif // IMGUI_LINK_CLICK_RESULT_DEFINED

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

    inline IMGUI_API void ShowLinkCursorOnHover()
    {
        AttachItemCursor(ImGuiMouseCursor_Hand);
    }

    // -------------------------------------------------------------------------
    // 2. Buttons with Hand Cursor (and Auto-Toggle Tooltip Support)
    // -------------------------------------------------------------------------
    // If called inside `if (ImGui::ButtonLink(...))`, clicking runs the `if` body
    // and dismisses any open TimedTooltip. If called as a standalone statement
    // `ImGui::ButtonLink(...);` followed by `ImGui::TimedTooltip::TextMD(...)`,
    // clicking automatically toggles the tooltip ON and OFF!

    inline IMGUI_API LinkClickResult ButtonLink(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        bool result = Button(label, size);
        AttachItemCursor();
        Detail::RegisterItemTooltipToggle(true);
        return LinkClickResult(result);
    }

    inline IMGUI_API LinkClickResult ButtonLink(const std::string& label, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        return ButtonLink(label.c_str(), size);
    }

    inline IMGUI_API LinkClickResult SmallButtonLink(const char* label)
    {
        bool result = SmallButton(label);
        AttachItemCursor();
        Detail::RegisterItemTooltipToggle(true);
        return LinkClickResult(result);
    }

    inline IMGUI_API LinkClickResult SmallButtonLink(const std::string& label)
    {
        return SmallButtonLink(label.c_str());
    }

    // -------------------------------------------------------------------------
    // 3. Clickable Text Links
    // -------------------------------------------------------------------------
    // Renders text with the hand cursor (ImGuiMouseCursor_Hand) on hover and
    // returns true if clicked (or automatically toggles a trailing TimedTooltip
    // on click when the return value is not tested in an `if (...)` block).
    // Preserves the current ambient text color unless an explicit 'col_override'
    // is supplied.
    //
    // Note: Dear ImGui v1.91+ added `bool ImGui::TextLink(const char*)` to imgui.h,
    // which draws in ImGuiCol_TextLink (baby blue) with an underline and uses
    // ButtonBehavior. To ensure 1-argument string literals (`ImGui::TextLink("...")`)
    // resolve to Contrib's ambient-color, click-to-toggle TextLink instead of
    // Dear ImGui's hyperlink, `TextLink` is redirected to `TextLinkContrib` below
    // (with Dear ImGui's native hyperlink preserved as `ImGui::NativeTextLink`).

    inline IMGUI_API bool NativeTextLink(const char* label)
    {
        return ::ImGui::TextLink(label);
    }

    inline IMGUI_API LinkClickResult TextLinkUnformatted(const char* text, const char* text_end = nullptr)
    {
        TextUnformatted(text, text_end);
        AttachItemCursor();
        bool clicked = IsItemClicked();
        Detail::RegisterItemTooltipToggle(true);
        return LinkClickResult(clicked);
    }

    inline IMGUI_API LinkClickResult TextLinkContrib(const char* label, const ImVec4* col_override = nullptr)
    {
        if (col_override) {
            PushStyleColor(ImGuiCol_Text, *col_override);
        }
        TextUnformatted(label);
        if (col_override) {
            PopStyleColor();
        }

        AttachItemCursor();
        bool clicked = IsItemClicked();
        Detail::RegisterItemTooltipToggle(true);
        return LinkClickResult(clicked);
    }

    inline IMGUI_API LinkClickResult TextLinkContrib(const std::string& label, const ImVec4* col_override = nullptr)
    {
        return TextLinkContrib(label.c_str(), col_override);
    }

    inline IMGUI_API LinkClickResult TextLink(const char* label, const ImVec4* col_override)
    {
        return TextLinkContrib(label, col_override);
    }

    inline IMGUI_API LinkClickResult TextLink(const std::string& label, const ImVec4* col_override = nullptr)
    {
        return TextLinkContrib(label.c_str(), col_override);
    }

#ifndef TextLink
#define TextLink TextLinkContrib
#endif

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

    inline IMGUI_API LinkClickResult MenuItemLink(const char* label,
                                                  const char* shortcut = nullptr,
                                                  bool selected = false,
                                                  bool enabled = true)
    {
        bool result = MenuItem(label, shortcut, selected, enabled);
        if (enabled) {
            AttachItemCursor();
        }
        Detail::RegisterItemTooltipToggle(true);
        return LinkClickResult(result);
    }

    inline IMGUI_API LinkClickResult MenuItemLink(const char* label,
                                                  const char* shortcut,
                                                  bool* p_selected,
                                                  bool enabled = true)
    {
        bool result = MenuItem(label, shortcut, p_selected, enabled);
        if (enabled) {
            AttachItemCursor();
        }
        Detail::RegisterItemTooltipToggle(true);
        return LinkClickResult(result);
    }

} // namespace ImGui

#endif // IMGUI_EXTRA_CLICKABLE_LINK_H
#endif // IMGUI_DISABLE_EXTRA_CLICKABLE_LINK
