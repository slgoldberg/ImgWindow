// ============================================================================
// ImGui Contrib: Directional Popup Menus & Select Pulldown Widgets
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Description:
//   Provides first-class popup placement, anchoring, and selection widgets:
//   1. Directional Item Anchoring:
//      - OpenPopupAtItem() / BeginAnchoredPopup():
//        Anchors popups relative to the preceding item's bounding box
//        (ImGui::GetItemRectMin/Max).
//      - Supports ImGuiPopupAnchor_AboveItem (blossoms UP for status bars docked
//        at the screen bottom) or ImGuiPopupAnchor_BelowItem (standard dropdown).
//      - Font-scale invariant: anchors to live pixel metrics with sub-pixel
//        precision, eliminating hardcoded magic offsets and text-zoom drift.
//      - Muscle-memory toggle safety: clicking the opener button when the popup
//        is open cleanly toggles it closed without re-opening flicker.
//      - Headroom reporting: ImGui::GetLastPopupHeight(str_id) lets window
//        managers (e.g. WidgetImgWindow) query the exact pixel height needed
//        for XPLM window expansion without manual "boxel" guessing.
//
//   2. SelectPulldown() Widget:
//      - Compact 1-line inline summary dropdown for dataref selectors (IS_SELECT).
//      - Renders active option with down-caret indicator.
//      - Scroll pass-through when closed: mouse-wheel events are NOT captured,
//        allowing parent scrolling lists to scroll smoothly without mutating settings.
//      - Mouse-wheel cycling when open: spinning the wheel inside the open popup
//        cycles active options dynamically without closing the menu.
//      - "Lens UI" open-menu tooltip latching: per-item tooltips dynamically target
//        the newly selected item during wheel navigation until mouse moves deliberately (> 6 px).
//      - Predictable step direction and circular wrapping: configurable via 'wrap'
//        and 'natural_list_order' parameters.
//      - Automatic focus tracking and scroll-into-view (SetItemDefaultFocus / SetScrollHereY).
//      - Optional rich Markdown per-item tooltips and custom widths.
// ============================================================================

#pragma once

#ifndef IMGUI_POPUP_MENUS_H
#define IMGUI_POPUP_MENUS_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#if !defined(CONTRIB_DISABLE_POPUP_MENUS) && \
    !defined(IMGUI_CONTRIB_DISABLE_POPUP_MENUS) && \
    !defined(IMGUI_DISABLE_POPUP_MENUS) && \
    !defined(IMGUI_DISABLE_CONTRIB_POPUP_MENUS)

#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cfloat>

#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
#if defined(__has_include)
    #if __has_include("../extensions/TimedTooltip/imgui_tooltip_ext.h")
        #include "../extensions/TimedTooltip/imgui_tooltip_ext.h"
    #endif
#endif
#endif

#ifndef IMGUI_API
#define IMGUI_API
#endif

// Optional compile-time icon strings for SelectPulldown carets:
#ifndef IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_DOWN
#define IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_DOWN nullptr // e.g. ICON_FA_CHEVRON_DOWN or "  v"
#endif

#ifndef IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_UP
#define IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_UP nullptr   // e.g. ICON_FA_CHEVRON_UP or "  ^"
#endif

enum ImGuiPopupAnchor {
    ImGuiPopupAnchor_BelowItem = 0, // Dropdown: popup top anchors below item
    ImGuiPopupAnchor_AboveItem = 1, // Status bar: popup bottom anchors above item
    ImGuiPopupAnchor_Auto      = 2  // Automatically selects Above or Below based on screen space
};

enum ImGuiPopupAlign {
    ImGuiPopupAlign_Left   = 0, // Align popup left edge with item left edge
    ImGuiPopupAlign_Right  = 1, // Align popup right edge with item right edge
    ImGuiPopupAlign_Center = 2, // Horizontally center popup over item
    ImGuiPopupAlign_Auto   = 3  // Left-align, but right-align if near display right boundary
};

namespace ImGui {

    using PopupAnchor = ImGuiPopupAnchor;
    using PopupAlign  = ImGuiPopupAlign;
    constexpr ImGuiPopupAnchor ImGuiPopupAnchor_BelowItem = ::ImGuiPopupAnchor_BelowItem;
    constexpr ImGuiPopupAnchor ImGuiPopupAnchor_AboveItem = ::ImGuiPopupAnchor_AboveItem;
    constexpr ImGuiPopupAnchor ImGuiPopupAnchor_Auto      = ::ImGuiPopupAnchor_Auto;
    constexpr ImGuiPopupAnchor PopupAnchor_BelowItem      = ::ImGuiPopupAnchor_BelowItem;
    constexpr ImGuiPopupAnchor PopupAnchor_AboveItem      = ::ImGuiPopupAnchor_AboveItem;
    constexpr ImGuiPopupAnchor PopupAnchor_Auto           = ::ImGuiPopupAnchor_Auto;

    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Left       = ::ImGuiPopupAlign_Left;
    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Right      = ::ImGuiPopupAlign_Right;
    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Center     = ::ImGuiPopupAlign_Center;
    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Auto       = ::ImGuiPopupAlign_Auto;
    constexpr ImGuiPopupAlign  PopupAlign_Left            = ::ImGuiPopupAlign_Left;
    constexpr ImGuiPopupAlign  PopupAlign_Right           = ::ImGuiPopupAlign_Right;
    constexpr ImGuiPopupAlign  PopupAlign_Center          = ::ImGuiPopupAlign_Center;
    constexpr ImGuiPopupAlign  PopupAlign_Auto            = ::ImGuiPopupAlign_Auto;

    namespace Detail {
        struct AnchoredPopupState {
            ImVec2 anchor_pos  = ImVec2(0.0f, 0.0f);
            ImVec2 item_min    = ImVec2(0.0f, 0.0f);
            ImVec2 item_max    = ImVec2(0.0f, 0.0f);
            ImVec2 last_size   = ImVec2(0.0f, 0.0f);
            ImGuiPopupAnchor anchor = ImGuiPopupAnchor_Auto;
            ImGuiPopupAlign  align  = ImGuiPopupAlign_Auto;
            bool was_open = false;
            bool force_close = false;
        };

        inline std::map<ImGuiID, AnchoredPopupState>& GetPopupStateMap() {
            static std::map<ImGuiID, AnchoredPopupState> s_Map;
            return s_Map;
        }

        inline const char*& GetDefaultPulldownCaretDownStorage() {
            static const char* s_Down = IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_DOWN;
            return s_Down;
        }

        inline const char*& GetDefaultPulldownCaretUpStorage() {
            static const char* s_Up = IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_UP;
            return s_Up;
        }

        inline float GetPopupMouseWheel(bool consume = true) {
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(CONTRIB_DISABLE_GET_MOUSE_WHEEL)
            return WheelSliderDetail::GetMouseWheelEither(consume);
#else
            ImGuiIO& io = ImGui::GetIO();
            float wheel = io.MouseWheel;
            if (wheel == 0.0f) {
                wheel = io.MouseWheelH;
                if (consume && wheel != 0.0f) {
                    io.MouseWheelH = 0.0f;
                }
            } else if (consume) {
                io.MouseWheel = 0.0f;
            }
            return wheel;
#endif
        }

        inline bool CycleIndex(int* current_idx,
                               int min_idx,
                               int max_idx,
                               float clicks,
                               bool wrap = false,
                               bool natural_list_order = true)
        {
            if (!current_idx || max_idx <= min_idx || clicks == 0.0f)
                return false;

#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif

            int dir = (clicks < 0.0f) ? (natural_list_order ? 1 : -1)
                                      : (natural_list_order ? -1 : 1);

            int prev = *current_idx;
            int count = (max_idx - min_idx) + 1;

            if (wrap) {
                int offset = (*current_idx - min_idx + dir) % count;
                if (offset < 0) offset += count;
                *current_idx = min_idx + offset;
            } else {
                int next = *current_idx + dir;
                if (next < min_idx) next = min_idx;
                if (next > max_idx) next = max_idx;
                *current_idx = next;
            }

            return (*current_idx != prev);
        }

        inline void AttachTooltipProxy(ImGuiID id, const char* text, bool is_hovered)
        {
            if (!text || text[0] == '\0') return;
#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && defined(IMGUI_TOOLTIP_EXT_H)
    #if !defined(CONTRIB_DISABLE_MARKDOWN) && !defined(IMGUI_CONTRIB_DISABLE_MARKDOWN) && defined(IMGUI_MARKDOWN_EXT_H)
            TimedTooltip::TextMDProxy(id, is_hovered, "%s", text);
    #else
            TimedTooltip::TextProxy(id, is_hovered, "%s", text);
    #endif
#else
            (void)id;
            if (is_hovered) {
                SetTooltip("%s", text);
            }
#endif
        }
    } // namespace Detail

    // ========================================================================
    // Global Caret Configuration & Procedural Drawing
    // ========================================================================

    /// Configure global default caret strings for SelectPulldown widgets (e.g. ICON_FA_CHEVRON_DOWN, ICON_FA_CHEVRON_UP).
    /// If set to nullptr or "", SelectPulldown automatically renders crisp procedural vector arrows!
    inline IMGUI_API void SetDefaultPulldownCaret(const char* down_caret, const char* up_caret = nullptr) {
        Detail::GetDefaultPulldownCaretDownStorage() = down_caret;
        Detail::GetDefaultPulldownCaretUpStorage() = up_caret;
    }

    /// Returns the global default down-caret string (or nullptr if vector fallback is active).
    inline IMGUI_API const char* GetDefaultPulldownCaretDown() {
        return Detail::GetDefaultPulldownCaretDownStorage();
    }

    /// Returns the global default up-caret string (or nullptr if vector fallback is active).
    inline IMGUI_API const char* GetDefaultPulldownCaretUp() {
        return Detail::GetDefaultPulldownCaretUpStorage();
    }

    /// Procedural vector arrow rendering helper (DPI-scaled equilateral triangle rendered via ImDrawList).
    /// Draws crisp vector arrows without requiring font icons or Unicode Geometric Shapes glyphs.
    inline IMGUI_API void DrawPulldownArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float font_size, float scale = 1.0f) {
        if (!draw_list) return;
        const float h = font_size;
        float r = h * 0.40f * scale;
        ImVec2 center(pos.x + h * 0.50f, pos.y + h * 0.50f * scale);

        ImVec2 a(0.0f, 0.0f), b(0.0f, 0.0f), c(0.0f, 0.0f);
        switch (dir) {
        case ImGuiDir_Up:
        case ImGuiDir_Down:
            if (dir == ImGuiDir_Up) r = -r;
            a = ImVec2(+0.000f * r, +0.750f * r);
            b = ImVec2(-0.866f * r, -0.750f * r);
            c = ImVec2(+0.866f * r, -0.750f * r);
            break;
        case ImGuiDir_Left:
        case ImGuiDir_Right:
            if (dir == ImGuiDir_Left) r = -r;
            a = ImVec2(+0.750f * r, +0.000f * r);
            b = ImVec2(-0.750f * r, +0.866f * r);
            c = ImVec2(-0.750f * r, -0.866f * r);
            break;
        default:
            return;
        }
        draw_list->AddTriangleFilled(ImVec2(center.x + a.x, center.y + a.y),
                                     ImVec2(center.x + b.x, center.y + b.y),
                                     ImVec2(center.x + c.x, center.y + c.y),
                                     col);
    }

    // ========================================================================
    // Headroom Queries (For Window Managers / Subclasses)
    // ========================================================================

    /// Returns the last measured content size of the specified popup window,
    /// or ImVec2(0, 0) if it has not yet been rendered.
    inline IMGUI_API ImVec2 GetLastPopupSize(const char* str_id) {
        if (!str_id || !*str_id) return ImVec2(0.0f, 0.0f);
        ImGuiID id = ImGui::GetID(str_id);
        auto& m = Detail::GetPopupStateMap();
        auto it = m.find(id);
        if (it != m.end()) return it->second.last_size;
        return ImVec2(0.0f, 0.0f);
    }

    /// Returns the last measured height in pixels of the specified popup window.
    inline IMGUI_API float GetLastPopupHeight(const char* str_id) {
        return GetLastPopupSize(str_id).y;
    }

    // ========================================================================
    // Anchored Popup Openers & Chasers
    // ========================================================================

    /// Open a popup anchored to the preceding item (Button, ButtonMD, Pill, Selectable, etc.).
    /// Call immediately after submitting the item:
    ///   if (ImGui::Button("View Mode")) {
    ///       ImGui::OpenPopupAtItem("##ViewMenu", ImGuiPopupAnchor_AboveItem);
    ///   }
    /// Returns true if the popup was opened, or false if it was toggled closed.
    inline IMGUI_API bool OpenPopupAtItem(const char* str_id,
                                          ImGuiPopupAnchor anchor = ImGuiPopupAnchor_Auto,
                                          ImGuiPopupAlign align = ImGuiPopupAlign_Auto,
                                          ImGuiPopupFlags popup_flags = 0)
    {
        if (!str_id || !*str_id) return false;
        ImGuiID id = ImGui::GetID(str_id);
        auto& state = Detail::GetPopupStateMap()[id];

        // Muscle-memory toggle safety: if already open, toggle it closed cleanly
        if (ImGui::IsPopupOpen(str_id, popup_flags)) {
            state.force_close = true;
            state.was_open = false;
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        state.item_min = ImGui::GetItemRectMin();
        state.item_max = ImGui::GetItemRectMax();
        state.anchor   = anchor;
        state.align    = align;

        // Auto resolution based on screen position
        if (state.anchor == ImGuiPopupAnchor_Auto) {
            state.anchor = (state.item_max.y > io.DisplaySize.y * 0.70f)
                ? ImGuiPopupAnchor_AboveItem
                : ImGuiPopupAnchor_BelowItem;
        }
        if (state.align == ImGuiPopupAlign_Auto) {
            state.align = (state.item_max.x > io.DisplaySize.x * 0.75f)
                ? ImGuiPopupAlign_Right
                : ImGuiPopupAlign_Left;
        }

        // Calculate target anchor coordinates
        float target_y = (state.anchor == ImGuiPopupAnchor_AboveItem)
            ? (state.item_min.y - style.ItemSpacing.y)
            : (state.item_max.y + style.ItemSpacing.y);

        float target_x = state.item_min.x;
        if (state.align == ImGuiPopupAlign_Right) {
            target_x = state.item_max.x;
        } else if (state.align == ImGuiPopupAlign_Center) {
            target_x = (state.item_min.x + state.item_max.x) * 0.5f;
        }

        state.anchor_pos = ImVec2(target_x, target_y);
        state.was_open   = true;

        ImGui::OpenPopup(str_id, popup_flags);
        return true;
    }

    /// Open a popup anchored to an explicit screen position.
    inline IMGUI_API bool OpenPopupAtPos(const char* str_id,
                                         ImVec2 pos,
                                         ImGuiPopupAnchor anchor = ImGuiPopupAnchor_Auto,
                                         ImGuiPopupAlign align = ImGuiPopupAlign_Auto,
                                         ImGuiPopupFlags popup_flags = 0)
    {
        if (!str_id || !*str_id) return false;
        ImGuiID id = ImGui::GetID(str_id);
        auto& state = Detail::GetPopupStateMap()[id];

        if (ImGui::IsPopupOpen(str_id, popup_flags)) {
            ImGui::CloseCurrentPopup();
            state.was_open = false;
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        state.item_min = pos;
        state.item_max = pos;
        state.anchor   = anchor;
        state.align    = align;

        if (state.anchor == ImGuiPopupAnchor_Auto) {
            state.anchor = (pos.y > io.DisplaySize.y * 0.70f)
                ? ImGuiPopupAnchor_AboveItem
                : ImGuiPopupAnchor_BelowItem;
        }
        if (state.align == ImGuiPopupAlign_Auto) {
            state.align = (pos.x > io.DisplaySize.x * 0.75f)
                ? ImGuiPopupAlign_Right
                : ImGuiPopupAlign_Left;
        }

        state.anchor_pos = pos;
        state.was_open   = true;

        ImGui::OpenPopup(str_id, popup_flags);
        return true;
    }

    /// Begin a popup window positioned according to its anchor coordinates.
    /// Call in place of standard ImGui::BeginPopup():
    ///   if (ImGui::BeginAnchoredPopup("##ViewMenu")) {
    ///       // ... menu items ...
    ///       ImGui::EndPopup();
    ///   }
    inline IMGUI_API bool BeginAnchoredPopup(const char* str_id, ImGuiWindowFlags flags = 0) {
        if (!str_id || !*str_id) return false;
        ImGuiID id = ImGui::GetID(str_id);
        auto& m = Detail::GetPopupStateMap();
        auto it = m.find(id);

        if (it != m.end() && it->second.was_open) {
            auto& state = it->second;
            ImVec2 pivot(0.0f, (state.anchor == ImGuiPopupAnchor_AboveItem) ? 1.0f : 0.0f);
            if (state.align == ImGuiPopupAlign_Right) pivot.x = 1.0f;
            else if (state.align == ImGuiPopupAlign_Center) pivot.x = 0.5f;

            if (state.last_size.y > 0.0f) {
                // Known size from previous opening: exact pivot placement
                ImGui::SetNextWindowPos(state.anchor_pos, ImGuiCond_Always, pivot);
            } else {
                // First appearance before measurement: estimate top-left to avoid jumping
                float est_h = ImGui::GetFontSize() * 3.5f;
                float start_y = (state.anchor == ImGuiPopupAnchor_AboveItem)
                    ? (state.anchor_pos.y - est_h)
                    : state.anchor_pos.y;
                ImGui::SetNextWindowPos(ImVec2(state.anchor_pos.x, start_y), ImGuiCond_Always, ImVec2(pivot.x, 0.0f));
            }
        }

        if (it != m.end() && it->second.force_close) {
            it->second.force_close = false;
            it->second.was_open = false;
            if (ImGui::BeginPopup(str_id, flags)) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            return false;
        }

        bool open = ImGui::BeginPopup(str_id, flags);
        if (open) {
            if (it != m.end()) {
                it->second.last_size = ImGui::GetWindowSize();
            }
        } else {
            if (it != m.end()) {
                it->second.was_open = false;
            }
        }
        return open;
    }

    // ========================================================================
    // SelectPulldown Widget (For Settings & Inline Datarefs)
    // ========================================================================

    /// Renders a compact 1-line dataref / radio pulldown selector.
    /// - When closed: Renders active item with caret (vector arrow or custom icon font).
    ///   Mouse-wheel events are NOT captured, allowing parent container scrolling without accidental edits.
    /// - When open: Mouse-wheel cycles active selection dynamically without closing popup.
    /// - "Lens UI" tooltip latching: When wheel is spun inside popup, per-item tooltips target
    ///   the newly selected item (*current_item) until mouse moves deliberately (> 6 px).
    /// - Supports directional anchoring (ImGuiPopupAnchor_BelowItem or ImGuiPopupAnchor_AboveItem).
    /// - Optional per-item tooltips displayed on option hover (using TimedTooltip and Markdown if available).
    /// - Supports circular wrapping (wrap = true) and natural list ordering (natural_list_order).
    inline IMGUI_API bool SelectPulldown(const char* label,
                                         int* current_item,
                                         const char* const items[],
                                         int items_count,
                                         const char* const tooltips[] = nullptr,
                                         float width = 0.0f,
                                         ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                         ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                         bool wrap = false,
                                         bool natural_list_order = true)
    {
        if (!current_item || !items || items_count <= 0) return false;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float font_size = ImGui::GetFontSize();
        float btn_h = ImGui::GetFrameHeight();
        float pad_x = style.FramePadding.x;

        ImGuiID id = ImGui::GetID(label);
        char popup_id[64];
        std::snprintf(popup_id, sizeof(popup_id), "##SelectPulldown_%08X", (unsigned int)id);

        static ImGuiID s_WheelLatchPulldownID = 0;
        static ImVec2  s_WheelLatchMousePos = ImVec2(-1.0f, -1.0f);

        int cur_idx = *current_item;
        if (cur_idx < 0) cur_idx = 0;
        if (cur_idx >= items_count) cur_idx = items_count - 1;

        const char* preview_text = items[cur_idx];
        if (!preview_text) preview_text = "";

        bool is_open = ImGui::IsPopupOpen(popup_id);

        // Reset wheel latch if popup closed
        if (!is_open && s_WheelLatchPulldownID == id) {
            s_WheelLatchPulldownID = 0;
        }

        // Determine caret direction & custom icon string:
        // Anchored above (e.g. status bar): closed points UP (▲), open points DOWN (▼).
        // Anchored below (e.g. dropdown): closed points DOWN (▼), open points UP (▲).
        bool points_up = (anchor == ImGuiPopupAnchor_AboveItem) ? !is_open : is_open;

        const char* down_caret = GetDefaultPulldownCaretDown();
        const char* up_caret   = GetDefaultPulldownCaretUp();
        const char* custom_icon = points_up ? (up_caret ? up_caret : down_caret) : down_caret;
        bool has_custom_icon = (custom_icon != nullptr && custom_icon[0] != '\0');

        // Calculate button width:
        ImVec2 text_sz = ImGui::CalcTextSize(preview_text, nullptr, true);
        float arrow_w = has_custom_icon 
            ? ImGui::CalcTextSize(custom_icon, nullptr, true).x 
            : (font_size * 0.70f);
        float arrow_zone_w = arrow_w + pad_x;
        float min_btn_w = text_sz.x + pad_x * 2.0f + arrow_zone_w;

        float btn_w = min_btn_w;
        if (width > 0.0f) {
            btn_w = width;
        } else if (width < 0.0f) {
            btn_w = (std::max)(min_btn_w, ImGui::GetContentRegionAvail().x + width);
        }

        if (is_open) {
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        std::string btn_id = std::string(preview_text) + "##" + popup_id;
        bool clicked = ImGui::Button(btn_id.c_str(), ImVec2(btn_w, btn_h));
        ImGuiHoveredFlags hover_flags = is_open ? ImGuiHoveredFlags_AllowWhenBlockedByPopup : ImGuiHoveredFlags_None;
        bool btn_hovered = ImGui::IsItemHovered(hover_flags);
        if (btn_hovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImGui::PopStyleVar();

        if (is_open) {
            ImGui::PopStyleColor();
        }

        // Draw caret / arrow docked at the right edge of the button:
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 icon_col = ImGui::GetColorU32(ImGuiCol_Text);

        if (has_custom_icon) {
            ImVec2 icon_sz = ImGui::CalcTextSize(custom_icon, nullptr, true);
            ImVec2 icon_pos(item_max.x - pad_x - icon_sz.x, item_min.y + (btn_h - icon_sz.y) * 0.5f);
            draw_list->AddText(icon_pos, icon_col, custom_icon);
        } else {
            ImGuiDir arrow_dir = points_up ? ImGuiDir_Up : ImGuiDir_Down;
            ImVec2 arrow_pos(item_max.x - pad_x - font_size * 0.85f, item_min.y + (btn_h - font_size) * 0.5f);
            DrawPulldownArrow(draw_list, arrow_pos, icon_col, arrow_dir, font_size, 1.0f);
        }

        bool value_changed = false;
        if (clicked) {
            OpenPopupAtItem(popup_id, anchor, align);
        }

        // Enforce popup minimum width to match button width
        ImGui::SetNextWindowSizeConstraints(ImVec2(btn_w, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

        // Render options inside open popup
        if (BeginAnchoredPopup(popup_id)) {
            // Break the wheel-selection tooltip latch only when the mouse moves deliberately (> 6 px),
            // ignoring tiny 1-2 px physical mouse jiggles while spinning a notched scroll wheel:
            if (s_WheelLatchPulldownID == id) {
                if (std::abs(io.MousePos.x - s_WheelLatchMousePos.x) > 6.0f ||
                    std::abs(io.MousePos.y - s_WheelLatchMousePos.y) > 6.0f) {
                    s_WheelLatchPulldownID = 0;
#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && defined(IMGUI_TOOLTIP_EXT_H)
                    TimedTooltip::ClearActiveTooltip();
#endif
                }
            }

            bool popup_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) ||
                                 ImGui::IsWindowHovered() ||
                                 ImGui::IsAnyItemHovered();
            bool wheel_stepped = false;

            // In-menu wheel cycling: allow spinning while hovering popup items OR the opener button!
            if (popup_hovered || btn_hovered) {
                if (popup_hovered) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                float clicks = Detail::GetPopupMouseWheel(true);
                if (clicks != 0.0f) {
                    s_WheelLatchPulldownID = id;
                    s_WheelLatchMousePos = io.MousePos;
#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && defined(IMGUI_TOOLTIP_EXT_H)
                    TimedTooltip::ClearActiveTooltip();
#endif
                    if (Detail::CycleIndex(current_item, 0, items_count - 1, clicks, wrap, natural_list_order)) {
                        value_changed = true;
                        wheel_stepped = true;
                    }
                }
            }

            for (int i = 0; i < items_count; i++) {
                ImGui::PushID(i);
                const char* item_text = items[i] ? items[i] : "*Unknown item*";
                bool is_selected = (i == *current_item);
                if (ImGui::Selectable(item_text, is_selected)) {
                    s_WheelLatchPulldownID = 0;
                    if (*current_item != i) {
                        *current_item = i;
                        value_changed = true;
                    }
                }
                bool item_hovered = ImGui::IsItemHovered();
                if (item_hovered) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                if (tooltips && tooltips[i] && tooltips[i][0] != '\0') {
                    bool tooltip_target = (s_WheelLatchPulldownID == id)
                                          ? ((i == *current_item) && !wheel_stepped)
                                          : item_hovered;
                    Detail::AttachTooltipProxy(ImGui::GetItemID(), tooltips[i], tooltip_target);
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                    if (wheel_stepped) {
                        ImGui::SetScrollHereY();
                    }
                }
                ImGui::PopID();
            }

            ImGui::EndPopup();
        }

        return value_changed;
    }

    /// std::vector overload of SelectPulldown
    inline IMGUI_API bool SelectPulldown(const char* label,
                                         int* current_item,
                                         const std::vector<std::string>& items,
                                         const std::vector<std::string>* tooltips = nullptr,
                                         float width = 0.0f,
                                         ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                         ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                         bool wrap = false,
                                         bool natural_list_order = true)
    {
        if (!current_item || items.empty()) return false;
        std::vector<const char*> item_ptrs;
        item_ptrs.reserve(items.size());
        for (const auto& s : items) item_ptrs.push_back(s.c_str());

        std::vector<const char*> tip_ptrs;
        if (tooltips && tooltips->size() == items.size()) {
            tip_ptrs.reserve(tooltips->size());
            for (const auto& t : *tooltips) tip_ptrs.push_back(t.c_str());
        }

        return SelectPulldown(label, current_item, item_ptrs.data(), (int)item_ptrs.size(),
                              tip_ptrs.empty() ? nullptr : tip_ptrs.data(), width, anchor, align,
                              wrap, natural_list_order);
    }

    // ========================================================================
    // Explicit Markdown Shorthands & Sub-Namespace Aliases
    // ========================================================================

    /// Explicit Markdown alias: ImGui::SelectPulldownMD(...)
    inline IMGUI_API bool SelectPulldownMD(const char* label,
                                           int* current_item,
                                           const char* const items[],
                                           int items_count,
                                           const char* const tooltips[] = nullptr,
                                           float width = 0.0f,
                                           ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                           ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                           bool wrap = false,
                                           bool natural_list_order = true)
    {
        return SelectPulldown(label, current_item, items, items_count, tooltips, width, anchor, align, wrap, natural_list_order);
    }

    /// Explicit Markdown alias (std::vector overload): ImGui::SelectPulldownMD(...)
    inline IMGUI_API bool SelectPulldownMD(const char* label,
                                           int* current_item,
                                           const std::vector<std::string>& items,
                                           const std::vector<std::string>* tooltips = nullptr,
                                           float width = 0.0f,
                                           ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                           ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                           bool wrap = false,
                                           bool natural_list_order = true)
    {
        return SelectPulldown(label, current_item, items, tooltips, width, anchor, align, wrap, natural_list_order);
    }

    namespace Markdown {
        /// Sub-namespace alias: ImGui::Markdown::SelectPulldown(...) / ImGui::MD::SelectPulldown(...)
        inline bool SelectPulldown(const char* label,
                                   int* current_item,
                                   const char* const items[],
                                   int items_count,
                                   const char* const tooltips[] = nullptr,
                                   float width = 0.0f,
                                   ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                   ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                   bool wrap = false,
                                   bool natural_list_order = true)
        {
            return ImGui::SelectPulldown(label, current_item, items, items_count, tooltips, width, anchor, align, wrap, natural_list_order);
        }

        inline bool SelectPulldown(const char* label,
                                   int* current_item,
                                   const std::vector<std::string>& items,
                                   const std::vector<std::string>* tooltips = nullptr,
                                   float width = 0.0f,
                                   ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                   ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                   bool wrap = false,
                                   bool natural_list_order = true)
        {
            return ImGui::SelectPulldown(label, current_item, items, tooltips, width, anchor, align, wrap, natural_list_order);
        }
    } // namespace Markdown

    namespace MD = Markdown;

} // namespace ImGui

#endif // !CONTRIB_DISABLE_POPUP_MENUS

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_POPUP_MENUS_H
