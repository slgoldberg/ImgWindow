// ============================================================================
// ImGui Contrib Widgets: Mouse-Wheel Controls (imgui_mouse_wheel_controls.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// Original Upstream SliderPercent Inspiration: Birger Hoppe (LiveTraffic)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Mouse-wheel enhanced sliders, drag boxes, discrete cyclers, and combos:
// 0. ImGui::GetMouseWheel / GetMouseWheelV / GetMouseWheelH / GetMouseWheelEither:
//    Core mouse-wheel query primitives with optional event consumption to
//    prevent background window scrolling.
// 1. ImGui::AdjustItemOnMouseWheel: Non-intrusive chaser attaching to any
//    preceding continuous range item (Slider*, Drag*, etc., float* or int*)
//    to handle vertical/horizontal scroll-wheel adjustments.
//    (Option C: shows <--> cursor strictly over the physical control track/box).
// 2. ImGui::CycleItemOnMouseWheel: Non-intrusive discrete index chaser
//    attaching to any preceding choice item (Button, Text, Combo, Selectable).
//    Clamps at boundaries by default so fast scrolls never wrap unexpectedly.
// 3. ImGui::ResetButton: Standalone DPI-scaled vector undo/reset button (↺).
// 4. ImGui::MouseWheelSlider / ImGui::PercentSlider / ImGui::MouseWheelDrag:
//    Turnkey sliders and drag boxes (overloaded for float* and int*) with
//    built-in mouse-wheel responsiveness.
// 5. ImGui::SliderWithReset / ImGui::PercentSliderWithReset / ImGui::DragWithReset
//    (also aliased as ResettableSlider / ResettablePercentSlider / ResettableDrag):
//    Composite controls featuring an abutting vector undo/reset button,
//    smart width reservation, built-in mouse-wheel, and dual-tooltip support.
// 6. ImGui::MouseWheelCombo & ImGui::ComboWithReset (aliased as ResettableCombo):
//    Dropdown combo boxes that allow hovering and spinning the mouse wheel
//    to cycle items both when closed AND when open (with Lens UI tooltip latch).
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_DISABLE_CONTRIB_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_DISABLE_EXTRA_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_DISABLE_EXTRA_WHEEL_SLIDER) && \
    !defined(IMGUI_DISABLE_EXTRA_MOUSE_WHEEL_SLIDER)
#ifndef IMGUI_MOUSE_WHEEL_CONTROLS_H
#define IMGUI_MOUSE_WHEEL_CONTROLS_H
#define IMGUI_EXTRA_WHEEL_SLIDER_H

#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <unordered_map>

// Combo chevron arrow direction styles (global or per-call):
enum ImGuiComboArrowStyle {
    ImGuiComboArrowStyle_DownWhenClosed  = 0, // Default (Option A): ▼ when closed -> ▲ when open (or ▼ if opened upward)
    ImGuiComboArrowStyle_Standard        = 0, // Alias for DownWhenClosed
    ImGuiComboArrowStyle_RightWhenClosed = 1, // Option B (ABC style): ▶ when closed -> ▼ (or ▲ if opened upward) when open
    ImGuiComboArrowStyle_UpWhenClosed    = 2, // ▲ when closed -> ▼ (or ▲ if opened upward) when open
    ImGuiComboArrowStyle_StaticDown      = 3  // Vanilla ImGui: always ▼
};

namespace ImGui {
    using ComboArrowStyle = ImGuiComboArrowStyle;
    constexpr ImGuiComboArrowStyle ComboArrowStyle_DownWhenClosed  = ImGuiComboArrowStyle_DownWhenClosed;
    constexpr ImGuiComboArrowStyle ComboArrowStyle_Standard        = ImGuiComboArrowStyle_Standard;
    constexpr ImGuiComboArrowStyle ComboArrowStyle_RightWhenClosed = ImGuiComboArrowStyle_RightWhenClosed;
    constexpr ImGuiComboArrowStyle ComboArrowStyle_UpWhenClosed    = ImGuiComboArrowStyle_UpWhenClosed;
    constexpr ImGuiComboArrowStyle ComboArrowStyle_StaticDown      = ImGuiComboArrowStyle_StaticDown;
} // namespace ImGui

#if !defined(IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE)
#if defined(IMGUI_EXTRA_WHEEL_COMBO_ARROW_STYLE)
#define IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE IMGUI_EXTRA_WHEEL_COMBO_ARROW_STYLE
#else
#define IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE ImGuiComboArrowStyle_DownWhenClosed
#endif
#endif

#if !defined(IMGUI_CONTRIB_DISABLE_RESET_UNDO) && !defined(CONTRIB_DISABLE_RESET_UNDO)
#define IMGUI_CONTRIB_DEFAULT_RESET_UNDO_ENABLED true
#else
#define IMGUI_CONTRIB_DEFAULT_RESET_UNDO_ENABLED false
#endif

#ifndef IMGUI_LINK_CLICK_RESULT_DEFINED
#define IMGUI_LINK_CLICK_RESULT_DEFINED
namespace ImGui {
namespace Detail {
    struct LinkClickToggleState {
        ImGuiContext* context                 = nullptr;
        int           frame_count             = -1;
        ImGuiID       item_id                 = 0;
        ImVec2        item_min                = ImVec2(-1.0f, -1.0f);
        bool          toggle_mode             = false;
        bool          click_consumed          = false;
        bool          is_menu_item            = false;
        bool          menu_item_selected      = false;
        ImGuiContext* menu_active_context     = nullptr;
        int           menu_active_frame       = -1;
        bool          wheel_latched           = false;
        ImGuiContext* wheel_latch_context     = nullptr;
        int           wheel_latch_alive_frame = -1;
        ImVec2        wheel_latch_mouse_pos   = ImVec2(-1.0f, -1.0f);
    };

    inline LinkClickToggleState& GetLinkClickToggleState() {
        static LinkClickToggleState s_State;
        return s_State;
    }

    inline void RegisterItemTooltipToggle(bool is_toggle_eligible = true) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        ImGuiID cur_id = GetItemID();
        ImVec2 cur_min = GetItemRectMin();
        bool same_item = (st.context == cur_ctx) && (st.frame_count == cur_frame) &&
                         ((cur_id != 0 && st.item_id == cur_id) ||
                          (cur_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
        if (!same_item) {
            st.context            = cur_ctx;
            st.frame_count        = cur_frame;
            st.item_id            = cur_id;
            st.item_min           = cur_min;
            st.click_consumed     = false;
            st.is_menu_item       = false;
            st.menu_item_selected = false;
        }
        if (IsItemClicked()) {
            st.wheel_latched = false;
        }
        st.toggle_mode = st.click_consumed ? false : is_toggle_eligible;
    }

    inline void RegisterMenuItemSelection(bool is_selected, bool is_clicked = false, bool is_toggle_eligible = false) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        st.context             = cur_ctx;
        st.frame_count         = cur_frame;
        st.item_id             = GetItemID();
        st.item_min            = GetItemRectMin();
        st.click_consumed      = false;
        st.toggle_mode         = is_toggle_eligible;
        st.is_menu_item        = true;
        st.menu_item_selected  = is_selected;
        st.menu_active_context = cur_ctx;
        st.menu_active_frame   = cur_frame;
        if (is_clicked) {
            st.wheel_latched = false;
        }
    }

    inline bool KeepWheelLatchAlive(bool wheel_stepped) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        ImVec2 mouse_pos = GetIO().MousePos;
        if (wheel_stepped) {
            st.wheel_latched           = true;
            st.wheel_latch_context     = cur_ctx;
            st.wheel_latch_alive_frame = cur_frame;
            st.wheel_latch_mouse_pos   = mouse_pos;
            return false;
        } else if (st.wheel_latched && st.wheel_latch_context == cur_ctx) {
            float dx = mouse_pos.x - st.wheel_latch_mouse_pos.x;
            float dy = mouse_pos.y - st.wheel_latch_mouse_pos.y;
            if (dx > 6.0f || dx < -6.0f || dy > 6.0f || dy < -6.0f) {
                st.wheel_latched = false;
                return true;
            } else {
                st.wheel_latch_alive_frame = cur_frame;
            }
        }
        return false;
    }

    inline bool ResolveItemTooltipHover(ImGuiID raw_item_id, bool raw_hovered) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        if (!st.wheel_latched || st.wheel_latch_context != cur_ctx) {
            return raw_hovered;
        }
        ImVec2 mouse_pos = GetIO().MousePos;
        float dx = mouse_pos.x - st.wheel_latch_mouse_pos.x;
        float dy = mouse_pos.y - st.wheel_latch_mouse_pos.y;
        bool menu_still_open = (st.menu_active_context == cur_ctx) && (cur_frame - st.menu_active_frame <= 1);
        if ((!menu_still_open && cur_frame - st.wheel_latch_alive_frame > 2) ||
            dx > 6.0f || dx < -6.0f || dy > 6.0f || dy < -6.0f) {
            st.wheel_latched = false;
            return raw_hovered;
        }
        if (menu_still_open) {
            ImVec2 cur_min = GetItemRectMin();
            bool same_item = (st.context == cur_ctx) && (st.frame_count == cur_frame) &&
                             ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                              (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
            if (same_item && st.is_menu_item) {
                return st.menu_item_selected;
            }
            return false;
        }
        return raw_hovered;
    }

    inline void MarkLastItemClickConsumed() {
        LinkClickToggleState& st = GetLinkClickToggleState();
        if (st.context == GetCurrentContext() && st.frame_count == GetFrameCount()) {
            st.click_consumed = true;
            st.toggle_mode    = false;
        }
    }

    inline bool ShouldToggleTooltipOnClick(ImGuiID raw_item_id, bool is_item_active) {
        (void)is_item_active;
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImVec2 cur_min = GetItemRectMin();
        if (st.context == GetCurrentContext() && st.frame_count == GetFrameCount()) {
            if ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y)) {
                return st.toggle_mode;
            }
        }
        return false;
    }
} // namespace Detail

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

#if !defined(IMGUI_TOOLTIP_EXT_H) && defined(__has_include)
#if __has_include("../extensions/TimedTooltip/imgui_tooltip_ext.h")
#include "../extensions/TimedTooltip/imgui_tooltip_ext.h"
#endif
#endif

namespace ImGui {

    // Internal implementation helpers so composite sliders/drags/combos always
    // compile regardless of which public functions are opted out or where
    // this header is included.
    namespace WheelSliderDetail {

        template<typename T>
        inline T Clamp(T v, T lo, T hi) {
            return (v < lo) ? lo : (v > hi) ? hi : v;
        }

        inline const char* FindTextDisplayEnd(const char* text, const char* text_end = nullptr) {
            const char* text_display_end = text;
            if (!text_end)
                text_end = (const char*)-1;
            while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
                text_display_end++;
            return text_display_end;
        }

        inline float GetMouseWheel(bool consume = false)
        {
            ImGuiIO& io = GetIO();
            float result = io.MouseWheel;
            if (consume && result != 0.0f) {
                io.MouseWheel = 0.0f;
            }
            return result;
        }

        inline float GetMouseWheelV(bool consume = false)
        {
            return GetMouseWheel(consume);
        }

        inline float GetMouseWheelH(bool consume = false)
        {
            ImGuiIO& io = GetIO();
            float result = io.MouseWheelH;
            if (consume && result != 0.0f) {
                io.MouseWheelH = 0.0f;
            }
            return result;
        }

        inline float GetMouseWheelEither(bool consume = false)
        {
            float v = GetMouseWheel(consume);
            return (v != 0.0f) ? v : GetMouseWheelH(consume);
        }

        inline float GetMouseWheelAny(bool consume = false)
        {
            return GetMouseWheelEither(consume);
        }

        inline bool AdjustFloat(float* valRef,
                                float minVal = 0.0f,
                                float maxVal = 1.0f,
                                float scale = 0.01f,
                                bool setCustomCursor = true)
        {
            if (!valRef || !IsItemHovered())
                return false;

            ImGuiIO& io = GetIO();
            ImVec2 item_min = GetItemRectMin();
            float track_w = CalcItemWidth();
            if (io.MousePos.x < item_min.x || io.MousePos.x > item_min.x + track_w)
                return false;

            float clicks = GetMouseWheelEither(true);

            bool isHandled = false;
            if (clicks != 0.0f) {
                if (clicks < 0.0f)
                    clicks = std::min(-1.0f, clicks - 0.5f);
                else
                    clicks = std::max(1.0f, clicks + 0.5f);

                float prev = *valRef;
                float next = *valRef + scale * clicks;
                *valRef = (minVal < maxVal) ? Clamp(next, minVal, maxVal) : next;
                if (*valRef != prev) {
                    isHandled = true;
#if defined(IMGUI_TOOLTIP_EXT_H)
                    TimedTooltip::ClearActiveTooltip();
#endif
                }
            }

            if (setCustomCursor) {
                SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }

            return isHandled;
        }

        inline bool AdjustInt(int* valRef,
                              int minVal = 0,
                              int maxVal = 100,
                              float scale = 1.0f,
                              bool setCustomCursor = true)
        {
            if (!valRef || !IsItemHovered())
                return false;

            ImGuiIO& io = GetIO();
            ImVec2 item_min = GetItemRectMin();
            float track_w = CalcItemWidth();
            if (io.MousePos.x < item_min.x || io.MousePos.x > item_min.x + track_w)
                return false;

            float clicks = GetMouseWheelEither(true);

            bool isHandled = false;
            if (clicks != 0.0f) {
                int delta = 0;
                if (clicks < 0.0f)
                    delta = std::min(-1, (int)std::floor(scale * clicks - 0.5f));
                else
                    delta = std::max(1, (int)std::floor(scale * clicks + 0.5f));

                int prev = *valRef;
                int next = *valRef + delta;
                *valRef = (minVal < maxVal) ? Clamp(next, minVal, maxVal) : next;
                if (*valRef != prev) {
                    isHandled = true;
#if defined(IMGUI_TOOLTIP_EXT_H)
                    TimedTooltip::ClearActiveTooltip();
#endif
                }
            }

            if (setCustomCursor) {
                SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }

            return isHandled;
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

#if defined(IMGUI_TOOLTIP_EXT_H)
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
                *current_idx = Clamp(*current_idx + dir, min_idx, max_idx);
            }

            return (*current_idx != prev);
        }

        inline void AttachTooltipProxy(ImGuiID id, const char* text, bool is_hovered)
        {
            if (!text || text[0] == '\0') return;
#if defined(IMGUI_MARKDOWN_EXT_H) && defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::TextMDProxy(id, is_hovered, "%s", text);
#elif defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::TextProxy(id, is_hovered, "%s", text);
#else
            (void)id;
            if (is_hovered) {
                SetTooltip("%s", text);
            }
#endif
        }

        inline void AttachTooltip(const char* text, bool is_hovered)
        {
            if (!text || text[0] == '\0') return;
#if defined(IMGUI_MARKDOWN_EXT_H) && defined(IMGUI_TOOLTIP_EXT_H)
            (void)is_hovered;
            TimedTooltip::TextMD("%s", text);
#elif defined(IMGUI_TOOLTIP_EXT_H)
            (void)is_hovered;
            TimedTooltip::Text("%s", text);
#else
            if (is_hovered) {
                SetTooltip("%s", text);
            }
#endif
        }

        inline ImGuiComboArrowStyle& GetDefaultComboArrowStyleStorage()
        {
            static ImGuiComboArrowStyle s_Style = IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE;
            return s_Style;
        }

        inline bool& GetResetButtonUndoEnabledStorage()
        {
            static bool s_Enabled = IMGUI_CONTRIB_DEFAULT_RESET_UNDO_ENABLED;
            return s_Enabled;
        }

        struct ResetUndoEntry {
            float last_float = 0.0f;
            int   last_int   = 0;
            bool  has_last   = false;
        };

        inline ResetUndoEntry& GetResetUndoEntry(ImGuiID id)
        {
            static std::unordered_map<ImGuiID, ResetUndoEntry> s_Map;
            return s_Map[id];
        }

        inline void DrawComboArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float font_size, float scale = 1.0f)
        {
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

        inline bool ComboWithWheelEx(const char* label,
                                     int* current_item,
                                     const char* const items[],
                                     int items_count,
                                     int popup_max_height_in_items = -1,
                                     bool wrap = false,
                                     bool natural_list_order = true,
                                     const char* const item_tooltips[] = nullptr,
                                     ImGuiMouseCursor hover_cursor = ImGuiMouseCursor_ResizeNS,
                                     ImGuiMouseCursor popup_cursor = ImGuiMouseCursor_ResizeNS,
                                     bool* out_popup_open = nullptr,
                                     bool* out_is_scrolling = nullptr)
        {
            if (out_popup_open) *out_popup_open = false;
            if (out_is_scrolling) *out_is_scrolling = false;
            if (!current_item || !items || items_count <= 0)
                return false;

            ImGuiIO& io = GetIO();
            ImGuiID combo_id = GetID(label);

            // Track whether the open popup is currently in "wheel-selection tooltip latch" mode:
            // When the user spins the wheel while the dropdown menu is open, per-item tooltips
            // target the newly selected item (*current_item) instead of whichever row the
            // stationary mouse cursor happens to be resting over, until the mouse is deliberately moved.
            static ImGuiID s_WheelLatchComboID = 0;
            static ImVec2  s_WheelLatchMousePos = ImVec2(-1.0f, -1.0f);

            // Capture the physical combo box frame rect [ Preview | ▼ ] (excluding right-side label)
            // and split it into the Preview Text zone [box_min.x .. value_x2] and the
            // Chevron Arrow Button zone [value_x2 .. box_max.x]:
            ImDrawList* parent_draw_list = GetWindowDrawList();
            const float parent_font_size = GetFontSize();
            const ImGuiStyle& style = GetStyle();
            ImVec2 box_min = GetCursorScreenPos();
            float box_w = CalcItemWidth();
            float box_h = GetFrameHeight();
            ImVec2 box_max(box_min.x + box_w, box_min.y + box_h);
            float arrow_size = box_h;
            float value_x2 = std::max(box_min.x, box_max.x - arrow_size);

            const char* preview_value = (*current_item >= 0 && *current_item < items_count && items[*current_item])
                                        ? items[*current_item]
                                        : "";

            if (popup_max_height_in_items != -1) {
                float max_h = (GetTextLineHeightWithSpacing() * popup_max_height_in_items) +
                              (style.WindowPadding.y * 2.0f);
                SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, max_h));
            }

            bool changed = false;
            bool popup_open = BeginCombo(label, preview_value, ImGuiComboFlags_None);
            if (out_popup_open) *out_popup_open = popup_open;

            bool in_box_y = (io.MousePos.y >= box_min.y && io.MousePos.y <= box_max.y);
            bool preview_hovered_rect = in_box_y && (io.MousePos.x >= box_min.x && io.MousePos.x < value_x2);
            bool arrow_hovered_rect   = in_box_y && (io.MousePos.x >= value_x2  && io.MousePos.x <= box_max.x);
            bool combo_item_hovered   = popup_open ? (preview_hovered_rect || arrow_hovered_rect)
                                                   : IsItemHovered();
            bool preview_hovered      = combo_item_hovered && preview_hovered_rect;
            bool arrow_hovered        = combo_item_hovered && arrow_hovered_rect;

            // Determine chevron arrow direction based on configured ImGuiComboArrowStyle and open/closed state:
            bool opens_up = popup_open && (GetWindowPos().y < box_min.y);
            ImGuiComboArrowStyle arrow_style = GetDefaultComboArrowStyleStorage();
            ImGuiDir arrow_dir = ImGuiDir_Down;
            switch (arrow_style) {
            case ImGuiComboArrowStyle_DownWhenClosed:
                arrow_dir = popup_open ? (opens_up ? ImGuiDir_Down : ImGuiDir_Up) : ImGuiDir_Down;
                break;
            case ImGuiComboArrowStyle_RightWhenClosed:
                arrow_dir = popup_open ? (opens_up ? ImGuiDir_Up : ImGuiDir_Down) : ImGuiDir_Right;
                break;
            case ImGuiComboArrowStyle_UpWhenClosed:
                arrow_dir = popup_open ? (opens_up ? ImGuiDir_Up : ImGuiDir_Down) : ImGuiDir_Up;
                break;
            case ImGuiComboArrowStyle_StaticDown:
            default:
                arrow_dir = ImGuiDir_Down;
                break;
            }

            // Overpaint the chevron button square [value_x2 .. box_max.x] on the parent window draw list so:
            //  1) Its hover highlight (ImGuiCol_ButtonHovered) lights up strictly when hovering the chevron
            //     button (or when open), rather than whenever the preview text box is hovered, and
            //  2) The arrow reflects the configured ImGuiComboArrowStyle (e.g. rotating on open/close).
            if (parent_draw_list && box_w > 0.0f) {
                ImU32 bg_col = GetColorU32((popup_open || arrow_hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
                ImDrawFlags round_flags = (box_w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight;
                if (((bg_col >> IM_COL32_A_SHIFT) & 0xFF) < 0xFF) {
                    parent_draw_list->AddRectFilled(ImVec2(value_x2, box_min.y), box_max,
                                                    GetColorU32(ImGuiCol_WindowBg, 1.0f), style.FrameRounding, round_flags);
                }
                parent_draw_list->AddRectFilled(ImVec2(value_x2, box_min.y), box_max,
                                                bg_col, style.FrameRounding, round_flags);
                if (value_x2 + arrow_size - style.FramePadding.x <= box_max.x) {
                    DrawComboArrow(parent_draw_list,
                                   ImVec2(value_x2 + style.FramePadding.y, box_min.y + style.FramePadding.y),
                                   GetColorU32(ImGuiCol_Text), arrow_dir, parent_font_size, 1.0f);
                }
                if (style.FrameBorderSize > 0.0f) {
                    parent_draw_list->AddRect(box_min, box_max, GetColorU32(ImGuiCol_Border),
                                              style.FrameRounding, 0, style.FrameBorderSize);
                }
            }

            if (popup_open) {
                // Break the wheel-selection tooltip latch only when the mouse moves deliberately (> 6 px),
                // ignoring tiny 1-2 px physical mouse jiggles while spinning a notched scroll wheel:
                if (s_WheelLatchComboID == combo_id) {
                    if (std::abs(io.MousePos.x - s_WheelLatchMousePos.x) > 6.0f ||
                        std::abs(io.MousePos.y - s_WheelLatchMousePos.y) > 6.0f) {
                        s_WheelLatchComboID = 0;
#if defined(IMGUI_TOOLTIP_EXT_H)
                        TimedTooltip::ClearActiveTooltip();
#endif
                    }
                }

                // Dropdown menu is OPEN:
                //  - Hovering over the chevron button [▼] shows the Hand cursor and ignores mouse-wheel.
                //  - Hovering over the preview text box or inside the open popup list shows the scroll
                //    cursor and cycles items on mouse-wheel.
                bool popup_hovered = IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                                                     ImGuiHoveredFlags_ChildWindows);

                bool wheel_stepped = false;
                if (arrow_hovered && !popup_hovered) {
                    SetMouseCursor(ImGuiMouseCursor_Hand);
                } else if (popup_hovered || preview_hovered) {
                    if (popup_hovered && popup_cursor != ImGuiMouseCursor_None) {
                        SetMouseCursor(popup_cursor);
                    } else if (preview_hovered && hover_cursor != ImGuiMouseCursor_None) {
                        SetMouseCursor(hover_cursor);
                    }
                    float clicks = GetMouseWheelEither(true);
                    if (clicks != 0.0f) {
                        if (out_is_scrolling) *out_is_scrolling = true;
                        s_WheelLatchComboID = combo_id;
                        s_WheelLatchMousePos = io.MousePos;
#if defined(IMGUI_TOOLTIP_EXT_H)
                        TimedTooltip::ClearActiveTooltip();
#endif
                        if (CycleIndex(current_item, 0, items_count - 1, clicks, wrap, natural_list_order)) {
                            changed = true;
                            wheel_stepped = true;
                        }
                    }
                }

                for (int i = 0; i < items_count; i++) {
                    const char* item_text = items[i] ? items[i] : "*Unknown item*";
                    PushID(i);
                    const bool item_selected = (i == *current_item);
                    if (Selectable(item_text, item_selected)) {
                        s_WheelLatchComboID = 0;
                        if (*current_item != i) {
                            changed = true;
                            *current_item = i;
                        }
                    }
                    bool item_hovered = IsItemHovered();
                    if (item_hovered && popup_cursor != ImGuiMouseCursor_None) {
                        SetMouseCursor(popup_cursor);
                    }
                    if (item_tooltips && item_tooltips[i] && item_tooltips[i][0] != '\0') {
                        bool tooltip_target = (s_WheelLatchComboID == combo_id || ((preview_hovered || arrow_hovered) && !popup_hovered))
                                              ? ((i == *current_item) && !wheel_stepped)
                                              : item_hovered;
                        AttachTooltipProxy(GetItemID(), item_tooltips[i], tooltip_target);
                    }
                    if (item_selected) {
                        SetItemDefaultFocus();
                        if (wheel_stepped) {
                            SetScrollHereY();
                        }
                    }
                    PopID();
                }

                EndCombo();
            } else {
                if (s_WheelLatchComboID == combo_id) {
                    s_WheelLatchComboID = 0;
                }
                // Dropdown menu is CLOSED:
                //  - Hovering over the chevron button [▼] shows the Hand cursor (signaling it is a
                //    clickable dropdown button) and does NOT cycle on mouse-wheel.
                //  - Hovering over the preview text box [box_min.x .. value_x2] shows hover_cursor
                //    (ResizeNS / ResizeEW) and cycles items on mouse-wheel.
                if (arrow_hovered) {
                    SetMouseCursor(ImGuiMouseCursor_Hand);
                } else if (preview_hovered) {
                    if (hover_cursor != ImGuiMouseCursor_None) {
                        SetMouseCursor(hover_cursor);
                    }
                    float clicks = GetMouseWheelEither(true);
                    if (clicks != 0.0f) {
                        if (out_is_scrolling) *out_is_scrolling = true;
                        if (CycleIndex(current_item, 0, items_count - 1, clicks, wrap, natural_list_order)) {
                            changed = true;
                        }
                    }
                }
            }

            return changed;
        }

    } // namespace WheelSliderDetail

    // -------------------------------------------------------------------------
    // 0. Core Mouse-Wheel Primitives
    // -------------------------------------------------------------------------
    // Returns mouse-wheel clicks for the current frame. When 'consume' is true,
    // zeroes out GetIO().MouseWheel / MouseWheelH so parent scrollable windows
    // do not scroll while the user is adjusting a hovered widget.
#if !defined(CONTRIB_DISABLE_GET_MOUSE_WHEEL) && \
    !defined(IMGUI_CONTRIB_DISABLE_GET_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CONTRIB_GET_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_GET_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_EXTRA_GET_MOUSE_WHEEL)
    inline IMGUI_API float GetMouseWheel(bool consume = false)
    {
        return WheelSliderDetail::GetMouseWheel(consume);
    }

    inline IMGUI_API float GetMouseWheelV(bool consume = false)
    {
        return WheelSliderDetail::GetMouseWheelV(consume);
    }

    inline IMGUI_API float GetMouseWheelH(bool consume = false)
    {
        return WheelSliderDetail::GetMouseWheelH(consume);
    }

    // Semantically equivalent to: (v = GetMouseWheel(consume)) != 0 ? v : GetMouseWheelH(consume)
    inline IMGUI_API float GetMouseWheelEither(bool consume = false)
    {
        return WheelSliderDetail::GetMouseWheelEither(consume);
    }

    inline IMGUI_API float GetMouseWheelAny(bool consume = false)
    {
        return WheelSliderDetail::GetMouseWheelEither(consume);
    }
#endif // !CONTRIB_DISABLE_GET_MOUSE_WHEEL

    // -------------------------------------------------------------------------
    // 1. AdjustItemOnMouseWheel Chaser (Continuous Scalar Ranges: Slider*, Drag*)
    // -------------------------------------------------------------------------
#if !defined(CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL) && \
    !defined(CONTRIB_DISABLE_ADJUST_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CONTRIB_ADJUST_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_ADJUST_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_EXTRA_ADJUST_MOUSE_WHEEL) && \
    !defined(IMGUI_CONTRIB_DISABLE_ADJUST_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CONTRIB_ADJUST_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_ADJUST_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_EXTRA_ADJUST_ITEM_ON_MOUSE_WHEEL)
    // Attaches to any preceding range item (SliderFloat, SliderInt, DragFloat,
    // DragInt, etc.) via IsItemHovered().
    // Inspects mouse-wheel delta (vertical or horizontal), applies scaled step
    // increments/decrements with boundary clamping, and consumes the wheel event.
    // Option C: Shows the East-West resize cursor (ImGuiMouseCursor_ResizeEW)
    // strictly when the mouse hovers over the physical control box (not label).
    //
    // Returns true if the referenced value was adjusted.
    inline IMGUI_API bool AdjustItemOnMouseWheel(float* valRef,
                                                 float minVal = 0.0f,
                                                 float maxVal = 1.0f,
                                                 float scale = 0.01f,
                                                 bool setCustomCursor = true)
    {
        return WheelSliderDetail::AdjustFloat(valRef, minVal, maxVal, scale, setCustomCursor);
    }

    inline IMGUI_API bool AdjustItemOnMouseWheel(int* valRef,
                                                 int minVal = 0,
                                                 int maxVal = 100,
                                                 float scale = 1.0f,
                                                 bool setCustomCursor = true)
    {
        return WheelSliderDetail::AdjustInt(valRef, minVal, maxVal, scale, setCustomCursor);
    }
#endif // !CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL

#if !defined(CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL) && \
    !defined(CONTRIB_DISABLE_CYCLE_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CONTRIB_CYCLE_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CYCLE_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_EXTRA_CYCLE_MOUSE_WHEEL) && \
    !defined(IMGUI_CONTRIB_DISABLE_CYCLE_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CONTRIB_CYCLE_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_CYCLE_ITEM_ON_MOUSE_WHEEL) && \
    !defined(IMGUI_DISABLE_EXTRA_CYCLE_ITEM_ON_MOUSE_WHEEL)
    // -------------------------------------------------------------------------
    // 2. CycleItemOnMouseWheel Chaser (Discrete Choices / Menus / Combos)
    // -------------------------------------------------------------------------
    // Configurable default cursor when hovering over a CycleItemOnMouseWheel target.
    // Define IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR before including this header
    // (e.g. to ImGuiMouseCursor_ResizeEW or ImGuiMouseCursor_None) to override globally.
#ifndef IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR
#define IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR ImGuiMouseCursor_ResizeNS
#endif

    // Attaches to any preceding widget (Button, Text, Combo, Selectable) via
    // IsItemHovered(). Shows a directional scroll cursor (ResizeNS by default,
    // or pass ImGuiMouseCursor_ResizeEW / ImGuiMouseCursor_None) whenever the
    // widget is hovered and "hot" for mouse-wheel input.
    //
    // Steps 'current_idx' by 1 item per scroll gesture and clamps to
    // [min_idx, max_idx] (or [0, item_count - 1]) by default (wrap = false) so
    // fast scrolling stops cleanly at the ends without wrapping unexpectedly.
    //
    // By default (natural_list_order = true), scrolling down (negative wheel)
    // advances to the next item (+1) and scrolling up moves to the previous (-1),
    // matching vertical dropdown menu order. Pass natural_list_order = false to
    // reverse.
    inline IMGUI_API bool CycleItemOnMouseWheel(int* current_idx,
                                                int min_idx,
                                                int max_idx,
                                                bool wrap = false,
                                                bool natural_list_order = true,
                                                bool consume = true,
                                                ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR)
    {
        if (!current_idx || max_idx <= min_idx || !IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            return false;

        if (hover_cursor != ImGuiMouseCursor_None) {
            // When attached to a container group (GetItemID() == 0, e.g. after EndGroup() inside BeginPopup),
            // preserve any explicit cursor (such as ImGuiMouseCursor_Hand on an inner TextLink or MenuItemLink)
            // already set by a hovered child widget inside the group:
            if (GetItemID() != 0 || GetMouseCursor() == ImGuiMouseCursor_Arrow) {
                SetMouseCursor(hover_cursor);
            }
        }

        // Allow clicking a passive wheel-cycled button/widget to toggle its TimedTooltip ON/OFF
        // (unless the click was already consumed by an `if (ImGui::ButtonLink(...))` handler):
        Detail::RegisterItemTooltipToggle(true);

        float clicks = WheelSliderDetail::GetMouseWheelEither(consume);
        bool cycled = WheelSliderDetail::CycleIndex(current_idx, min_idx, max_idx, clicks, wrap, natural_list_order);
        bool unlatched = Detail::KeepWheelLatchAlive(cycled || clicks != 0.0f);
        if (cycled || clicks != 0.0f || unlatched) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }
        return cycled;
    }

    inline IMGUI_API bool CycleItemOnMouseWheel(int* current_idx,
                                                int item_count,
                                                bool wrap = false,
                                                bool natural_list_order = true,
                                                bool consume = true,
                                                ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR)
    {
        if (item_count <= 1) return false;
        return CycleItemOnMouseWheel(current_idx, 0, item_count - 1, wrap, natural_list_order, consume, hover_cursor);
    }

    // Boolean / Checkbox mouse-wheel chaser:
    // When attached to an ImGui::Checkbox(label, &v), 'only_over_box = true' (default)
    // restricts both the scroll cursor and mouse-wheel toggling strictly to the
    // square [✓] box on the left (width = GetFrameHeight()), while hovering over
    // the text label to the right shows ImGuiMouseCursor_Hand and ignores the wheel!
    // By default (wrap = false), scrolling up/right turns the checkbox ON (true)
    // and scrolling down/left turns it OFF (false), clamping cleanly at both ends.
    inline IMGUI_API bool CycleItemOnMouseWheel(bool* v,
                                                bool wrap = false,
                                                bool only_over_box = true,
                                                bool consume = true,
                                                ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR)
    {
        if (!v || !IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            return false;

        if (only_over_box) {
            ImVec2 item_min = GetItemRectMin();
            float box_max_x = item_min.x + GetFrameHeight();
            if (GetIO().MousePos.x > box_max_x) {
                // Mouse is over the checkbox text label: show Hand cursor (clickable)
                // and ignore mouse-wheel scrolling!
                SetMouseCursor(ImGuiMouseCursor_Hand);
                return false;
            }
        }

        if (hover_cursor != ImGuiMouseCursor_None) {
            if (GetItemID() != 0 || GetMouseCursor() == ImGuiMouseCursor_Arrow) {
                SetMouseCursor(hover_cursor);
            }
        }

        Detail::RegisterItemTooltipToggle(true);

        float clicks = WheelSliderDetail::GetMouseWheelEither(consume);
        bool changed = false;
        if (clicks != 0.0f) {
            if (wrap) {
                int steps = (int)std::round(std::fabs(clicks));
                if (steps < 1) steps = 1;
                if (steps % 2 != 0) {
                    *v = !(*v);
                    changed = true;
                }
            } else {
                if (clicks > 0.0f && !(*v)) {
                    *v = true;
                    changed = true;
                } else if (clicks < 0.0f && *v) {
                    *v = false;
                    changed = true;
                }
            }
        }

        bool unlatched = Detail::KeepWheelLatchAlive(changed || clicks != 0.0f);
        if (changed || clicks != 0.0f || unlatched) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }
        return changed;
    }

    inline IMGUI_API bool CycleItemOnMouseWheel(bool* v,
                                                ImGuiMouseCursor_ hover_cursor,
                                                bool wrap = false,
                                                bool only_over_box = true,
                                                bool consume = true)
    {
        return CycleItemOnMouseWheel(v, wrap, only_over_box, consume, (ImGuiMouseCursor)hover_cursor);
    }

    inline IMGUI_API bool ToggleItemOnMouseWheel(bool* v,
                                                 bool wrap = false,
                                                 bool only_over_box = true,
                                                 bool consume = true,
                                                 ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR)
    {
        return CycleItemOnMouseWheel(v, wrap, only_over_box, consume, hover_cursor);
    }

    inline IMGUI_API bool ToggleItemOnMouseWheel(bool* v,
                                                 ImGuiMouseCursor_ hover_cursor,
                                                 bool wrap = false,
                                                 bool only_over_box = true,
                                                 bool consume = true)
    {
        return CycleItemOnMouseWheel(v, wrap, only_over_box, consume, (ImGuiMouseCursor)hover_cursor);
    }

    // Turnkey Mouse-Wheel Checkbox (toggles on wheel ONLY over the square [✓] box,
    // shows Hand cursor over the text label):
    inline IMGUI_API bool MouseWheelCheckbox(const char* label,
                                             bool* v,
                                             bool wrap = false,
                                             bool only_over_box = true,
                                             ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_CYCLE_DEFAULT_CURSOR)
    {
        bool changed = Checkbox(label, v);
        if (CycleItemOnMouseWheel(v, wrap, only_over_box, true, hover_cursor)) {
            changed = true;
        }
        return changed;
    }

    inline IMGUI_API bool MouseWheelCheckbox(const char* label,
                                             bool* v,
                                             ImGuiMouseCursor_ hover_cursor,
                                             bool wrap = false,
                                             bool only_over_box = true)
    {
        return MouseWheelCheckbox(label, v, wrap, only_over_box, (ImGuiMouseCursor)hover_cursor);
    }
#endif // !CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL

    // -------------------------------------------------------------------------
    // 3. Undo/Reset Button Helper (Custom Icon Strings or Vector Fallback)
    // -------------------------------------------------------------------------
    // Callers with FontAwesome (or custom icon fonts/text) can configure the
    // default reset button label (and optional restore/redo label) globally via:
    //   ImGui::SetDefaultResetButtonLabel(ICON_FA_UNDO, ICON_FA_REDO);
    // or at compile time before including this header via:
    //   #define IMGUI_CONTRIB_DEFAULT_RESET_LABEL      ICON_FA_UNDO
    //   #define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT  ICON_FA_REDO // (or IMGUI_CONTRIB_DEFAULT_REDO_LABEL / IMGUI_CONTRIB_DEFAULT_RESTORE_LABEL)
    // or per-call via `reset_label` / `restore_label`.
    // When no label string is set (nullptr or ""), ResetButton automatically draws
    // a crisp, DPI-scaled counter-clockwise vector undo arrow (↺) for Reset, and
    // flips to a clockwise vector redo arrow (↻) when in Restore-Previous mode!

#ifndef IMGUI_CONTRIB_DEFAULT_RESET_LABEL
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL nullptr
#endif

#if !defined(IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT)
#if defined(IMGUI_CONTRIB_DEFAULT_REDO_LABEL)
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT IMGUI_CONTRIB_DEFAULT_REDO_LABEL
#elif defined(IMGUI_CONTRIB_DEFAULT_RESTORE_LABEL)
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT IMGUI_CONTRIB_DEFAULT_RESTORE_LABEL
#elif defined(IMGUI_CONTRIB_DEFAULT_RESTORE_LAST_LABEL)
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT IMGUI_CONTRIB_DEFAULT_RESTORE_LAST_LABEL
#else
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT nullptr
#endif
#endif

    inline IMGUI_API void SetDefaultComboArrowStyle(ImGuiComboArrowStyle style)
    {
        WheelSliderDetail::GetDefaultComboArrowStyleStorage() = style;
    }

    inline IMGUI_API ImGuiComboArrowStyle GetDefaultComboArrowStyle()
    {
        return WheelSliderDetail::GetDefaultComboArrowStyleStorage();
    }

    inline IMGUI_API void SetResetButtonUndoEnabled(bool enabled)
    {
        WheelSliderDetail::GetResetButtonUndoEnabledStorage() = enabled;
    }

    inline IMGUI_API bool GetResetButtonUndoEnabled()
    {
        return WheelSliderDetail::GetResetButtonUndoEnabledStorage();
    }

    namespace WheelSliderDetail {
        inline const char*& GetDefaultResetButtonLabelStorage()
        {
            static const char* s_DefaultLabel = IMGUI_CONTRIB_DEFAULT_RESET_LABEL;
            return s_DefaultLabel;
        }

        inline const char*& GetDefaultRestoreButtonLabelStorage()
        {
            static const char* s_DefaultRestoreLabel = IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT;
            return s_DefaultRestoreLabel;
        }

        inline float CalcResetButtonWidth(float height,
                                          bool abutting,
                                          const char* reset_label = nullptr,
                                          const char* restore_label = nullptr)
        {
            float width = abutting ? std::round(height * 0.85f) : height;
            const char* primary = (reset_label != nullptr) ? reset_label : GetDefaultResetButtonLabelStorage();
            const char* alt     = (restore_label != nullptr) ? restore_label : GetDefaultRestoreButtonLabelStorage();
            const float pad_x   = GetStyle().FramePadding.x * 1.5f;
            if (primary && primary[0] != '\0') {
                ImVec2 sz = CalcTextSize(primary, nullptr, true);
                if (sz.x + pad_x > width) width = std::ceil(sz.x + pad_x);
            }
            if (alt && alt[0] != '\0') {
                ImVec2 sz = CalcTextSize(alt, nullptr, true);
                if (sz.x + pad_x > width) width = std::ceil(sz.x + pad_x);
            }
            return width;
        }

        inline bool ResolveFloatUndo(ImGuiID undo_id,
                                     float current_val,
                                     float default_val,
                                     float eps,
                                     const float* v_last_ptr,
                                     float* out_restore_val)
        {
            if (!GetResetButtonUndoEnabledStorage()) return false;
            if (std::abs(current_val - default_val) > eps) return false;
            if (v_last_ptr != nullptr) {
                if (!std::isnan(*v_last_ptr) && std::abs(*v_last_ptr - default_val) > eps) {
                    if (out_restore_val) *out_restore_val = *v_last_ptr;
                    return true;
                }
            }
            ResetUndoEntry& entry = GetResetUndoEntry(undo_id);
            if (entry.has_last && std::abs(entry.last_float - default_val) > eps) {
                if (out_restore_val) *out_restore_val = entry.last_float;
                return true;
            }
            return false;
        }

        inline void RecordFloatReset(ImGuiID undo_id, float val_before_reset, float* v_last_ptr)
        {
            if (v_last_ptr != nullptr) {
                *v_last_ptr = val_before_reset;
            }
            ResetUndoEntry& entry = GetResetUndoEntry(undo_id);
            entry.last_float = val_before_reset;
            entry.has_last   = true;
        }

        inline void ClearFloatUndo(ImGuiID undo_id, float default_val, float* v_last_ptr)
        {
            if (v_last_ptr != nullptr) {
                *v_last_ptr = default_val;
            }
            ResetUndoEntry& entry = GetResetUndoEntry(undo_id);
            entry.has_last = false;
        }

        inline bool ResolveIntUndo(ImGuiID undo_id,
                                   int current_val,
                                   int default_val,
                                   const int* v_last_ptr,
                                   int* out_restore_val)
        {
            if (!GetResetButtonUndoEnabledStorage()) return false;
            if (current_val != default_val) return false;
            if (v_last_ptr != nullptr) {
                if (*v_last_ptr != default_val) {
                    if (out_restore_val) *out_restore_val = *v_last_ptr;
                    return true;
                }
            }
            ResetUndoEntry& entry = GetResetUndoEntry(undo_id);
            if (entry.has_last && entry.last_int != default_val) {
                if (out_restore_val) *out_restore_val = entry.last_int;
                return true;
            }
            return false;
        }

        inline void RecordIntReset(ImGuiID undo_id, int val_before_reset, int* v_last_ptr)
        {
            if (v_last_ptr != nullptr) {
                *v_last_ptr = val_before_reset;
            }
            ResetUndoEntry& entry = GetResetUndoEntry(undo_id);
            entry.last_int = val_before_reset;
            entry.has_last = true;
        }

        inline void ClearIntUndo(ImGuiID undo_id, int default_val, int* v_last_ptr)
        {
            if (v_last_ptr != nullptr) {
                *v_last_ptr = default_val;
            }
            ResetUndoEntry& entry = GetResetUndoEntry(undo_id);
            entry.has_last = false;
        }
    } // namespace WheelSliderDetail

    inline IMGUI_API void SetDefaultResetButtonLabel(const char* reset_label,
                                                     const char* restore_label = IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT)
    {
        WheelSliderDetail::GetDefaultResetButtonLabelStorage() = reset_label;
        WheelSliderDetail::GetDefaultRestoreButtonLabelStorage() = restore_label;
    }

    inline IMGUI_API const char* GetDefaultResetButtonLabel()
    {
        return WheelSliderDetail::GetDefaultResetButtonLabelStorage();
    }

    inline IMGUI_API void SetDefaultRestoreButtonLabel(const char* restore_label)
    {
        WheelSliderDetail::GetDefaultRestoreButtonLabelStorage() = restore_label;
    }

    inline IMGUI_API const char* GetDefaultRestoreButtonLabel()
    {
        return WheelSliderDetail::GetDefaultRestoreButtonLabelStorage();
    }

    inline IMGUI_API void SetDefaultRedoButtonLabel(const char* redo_label)
    {
        WheelSliderDetail::GetDefaultRestoreButtonLabelStorage() = redo_label;
    }

    inline IMGUI_API const char* GetDefaultRedoButtonLabel()
    {
        return WheelSliderDetail::GetDefaultRestoreButtonLabelStorage();
    }

    inline IMGUI_API bool ResetButton(const char* str_id,
                                      bool is_modified = true,
                                      float width_arg = 0.0f,
                                      bool abutting = false,
                                      const char* reset_label = nullptr,
                                      bool is_restore_mode = false,
                                      const char* restore_label = nullptr)
    {
        const char* primary_label = (reset_label != nullptr) ? reset_label : GetDefaultResetButtonLabel();
        const char* alt_label     = (restore_label != nullptr) ? restore_label : GetDefaultRestoreButtonLabel();
        if (!alt_label || alt_label[0] == '\0') {
            alt_label = primary_label;
        }

        const bool use_restore_visual = (!is_modified && is_restore_mode);
        const bool is_actionable      = (is_modified || is_restore_mode);
        const char* label_to_draw     = use_restore_visual ? alt_label : primary_label;
        const bool has_custom_label   = (label_to_draw != nullptr && label_to_draw[0] != '\0');

        float height = GetFrameHeight();
        float width  = (width_arg > 0.0f)
                       ? width_arg
                       : WheelSliderDetail::CalcResetButtonWidth(height, abutting, primary_label, alt_label);

        PushID(str_id);
        ImVec2 p = GetCursorScreenPos();
        bool pressed = InvisibleButton("##reset_btn", ImVec2(width, height));
        bool hovered = IsItemHovered();
        bool active = IsItemActive();
        PopID();

        if (hovered) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        // When already at default with nothing to restore (!is_actionable), clicking the
        // dimmed reset button immediately toggles its status tooltip ON/OFF!
        Detail::RegisterItemTooltipToggle(!is_actionable);

        ImDrawList* draw_list = GetWindowDrawList();
        const ImGuiStyle& style = GetStyle();
        ImU32 bg_col = GetColorU32(active ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
        float rounding = style.FrameRounding;
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18200
        ImDrawCornerFlags round_flags = abutting ? ImDrawCornerFlags_Right : ImDrawCornerFlags_All;
#else
        ImDrawFlags round_flags = abutting ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersAll;
#endif
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg_col, rounding, round_flags);
        if (style.FrameBorderSize > 0.0f) {
            draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), GetColorU32(ImGuiCol_Border), rounding, round_flags, style.FrameBorderSize);
        }

        // Color: prominent text color if actionable (modified OR restore-previous ready), dimmed if idle at default!
        ImU32 icon_col = GetColorU32(is_actionable ? ImGuiCol_Text : ImGuiCol_TextDisabled);

        if (has_custom_label) {
            const char* text_end = WheelSliderDetail::FindTextDisplayEnd(label_to_draw);
            ImVec2 text_sz = CalcTextSize(label_to_draw, text_end, false);
            ImVec2 text_pos(std::floor(p.x + (width - text_sz.x) * 0.5f),
                            std::floor(p.y + (height - text_sz.y) * 0.5f));
            draw_list->AddText(text_pos, icon_col, label_to_draw, text_end);
        } else if (use_restore_visual) {
            // Draw clockwise redo/restore circular arc (matching ICON_FA_REDO / U+21BB):
            // Horizontal mirror of the counter-clockwise undo arc: sweeps clockwise from
            // bottom-right (~4:30) around the left and top to the top-right arrowhead (~1:15).
            ImVec2 center(p.x + width * 0.48f, p.y + height * 0.52f);
            float radius = height * 0.25f;
            float thickness = std::max(1.2f, height * 0.095f);

            const float kPI = 3.14159265f;
            const float a_tail = 0.28f * kPI;
            const float a_head = 1.62f * kPI;
            draw_list->PathArcTo(center, radius, a_tail, a_head, 16);
            draw_list->PathStroke(icon_col, 0, thickness);

            // Tangent-aligned arrowhead at a_head pointing clockwise (increasing angle):
            float cos_a = std::cos(a_head);
            float sin_a = std::sin(a_head);
            float tx = -sin_a; // unit tangent in clockwise (increasing angle) direction
            float ty =  cos_a;
            float nx =  cos_a; // unit outward normal
            float ny =  sin_a;

            ImVec2 base(center.x + radius * cos_a, center.y + radius * sin_a);
            float arrow_len = radius * 0.95f;
            float arrow_w   = radius * 0.68f;
            ImVec2 tip(base.x + tx * (arrow_len * 0.65f), base.y + ty * (arrow_len * 0.65f));
            ImVec2 back(base.x - tx * (arrow_len * 0.35f), base.y - ty * (arrow_len * 0.35f));
            ImVec2 p1(back.x + nx * arrow_w, back.y + ny * arrow_w);
            ImVec2 p2(back.x - nx * arrow_w, back.y - ny * arrow_w);
            draw_list->AddTriangleFilled(tip, p1, p2, icon_col);
        } else {
            // Draw counter-clockwise undo circular arc (matching ICON_FA_UNDO / U+21BA):
            // In ImGui screen coordinates (+Y down):
            //   -0.62*PI is ~10:45 (top-left arrowhead), -0.50*PI is 12:00 (top),
            //    0.00*PI is  3:00 (right),              +0.50*PI is  6:00 (bottom),
            //   +0.72*PI is ~7:30 (bottom-left tail).
            // Thus the arch sits on the right/top/bottom with the open gap on the left,
            // sweeping counter-clockwise from bottom-left around the right to top-left!
            ImVec2 center(p.x + width * 0.52f, p.y + height * 0.52f);
            float radius = height * 0.25f;
            float thickness = std::max(1.2f, height * 0.095f);

            const float kPI = 3.14159265f;
            const float a_head = -0.62f * kPI;
            const float a_tail =  0.72f * kPI;
            draw_list->PathArcTo(center, radius, a_head, a_tail, 16);
            draw_list->PathStroke(icon_col, 0, thickness);

            // Tangent-aligned arrowhead at a_head pointing counter-clockwise (decreasing angle):
            float cos_a = std::cos(a_head);
            float sin_a = std::sin(a_head);
            float tx =  sin_a; // unit tangent in counter-clockwise (decreasing angle) direction
            float ty = -cos_a;
            float nx =  cos_a; // unit outward normal
            float ny =  sin_a;

            ImVec2 base(center.x + radius * cos_a, center.y + radius * sin_a);
            float arrow_len = radius * 0.95f;
            float arrow_w   = radius * 0.68f;
            ImVec2 tip(base.x + tx * (arrow_len * 0.65f), base.y + ty * (arrow_len * 0.65f));
            ImVec2 back(base.x - tx * (arrow_len * 0.35f), base.y - ty * (arrow_len * 0.35f));
            ImVec2 p1(back.x + nx * arrow_w, back.y + ny * arrow_w);
            ImVec2 p2(back.x - nx * arrow_w, back.y - ny * arrow_w);
            draw_list->AddTriangleFilled(tip, p1, p2, icon_col);
        }

        return pressed;
    }

    // -------------------------------------------------------------------------
    // 4. Turnkey Mouse-Wheel Sliders & Drag Boxes (Overloaded for float* & int*)
    // -------------------------------------------------------------------------

    inline IMGUI_API bool MouseWheelSlider(const char* label,
                                           float* v,
                                           float v_min,
                                           float v_max,
                                           const char* format = "%.3f",
                                           ImGuiSliderFlags flags = 0,
                                           float wheel_step = 0.01f)
    {
        bool changed = SliderFloat(label, v, v_min, v_max, format, flags);
        if (WheelSliderDetail::AdjustFloat(v, v_min, v_max, wheel_step))
            changed = true;
        return changed;
    }

    inline IMGUI_API bool MouseWheelSlider(const char* label,
                                           int* v,
                                           int v_min,
                                           int v_max,
                                           const char* format = "%d",
                                           ImGuiSliderFlags flags = 0,
                                           float wheel_step = 1.0f)
    {
        bool changed = SliderInt(label, v, v_min, v_max, format, flags);
        if (WheelSliderDetail::AdjustInt(v, v_min, v_max, wheel_step))
            changed = true;
        return changed;
    }

    // Turnkey Mouse-Wheel Drag Boxes (Overloaded for float* & int*):
    // When 'wheel_step' is 0.0f (default), steps by 'v_speed' per wheel notch.
    inline IMGUI_API bool MouseWheelDrag(const char* label,
                                         float* v,
                                         float v_speed = 1.0f,
                                         float v_min = 0.0f,
                                         float v_max = 0.0f,
                                         const char* format = "%.3f",
                                         ImGuiSliderFlags flags = 0,
                                         float wheel_step = 0.0f)
    {
        bool changed = DragFloat(label, v, v_speed, v_min, v_max, format, flags);
        float step = (wheel_step > 0.0f) ? wheel_step : ((v_speed > 0.0f) ? v_speed : 1.0f);
        if (WheelSliderDetail::AdjustFloat(v, v_min, v_max, step))
            changed = true;
        return changed;
    }

    inline IMGUI_API bool MouseWheelDrag(const char* label,
                                         int* v,
                                         float v_speed = 1.0f,
                                         int v_min = 0,
                                         int v_max = 0,
                                         const char* format = "%d",
                                         ImGuiSliderFlags flags = 0,
                                         float wheel_step = 0.0f)
    {
        bool changed = DragInt(label, v, v_speed, v_min, v_max, format, flags);
        float step = (wheel_step > 0.0f) ? wheel_step : std::max(1.0f, v_speed);
        if (WheelSliderDetail::AdjustInt(v, v_min, v_max, step))
            changed = true;
        return changed;
    }

    // -------------------------------------------------------------------------
    // 5. PercentSlider (0.0 .. 1.0 displayed as 0% .. 100%, with Mouse-Wheel)
    // -------------------------------------------------------------------------
#if !defined(CONTRIB_DISABLE_PERCENT_SLIDER) && \
    !defined(IMGUI_CONTRIB_DISABLE_PERCENT_SLIDER) && \
    !defined(IMGUI_DISABLE_CONTRIB_PERCENT_SLIDER) && \
    !defined(IMGUI_DISABLE_PERCENT_SLIDER) && \
    !defined(IMGUI_DISABLE_EXTRA_PERCENT_SLIDER)
    inline IMGUI_API bool PercentSlider(const char* label,
                                        float* v,
                                        float v_min = 0.0f,
                                        float v_max = 1.0f,
                                        const char* format = "%.0f%%",
                                        ImGuiSliderFlags flags = 0,
                                        float wheel_step = 0.01f)
    {
        if (!v) return false;

        float pct = *v * 100.0f;
        bool changed = SliderFloat(label, &pct, v_min * 100.0f, v_max * 100.0f, format, flags);
        *v = pct / 100.0f;

        if (WheelSliderDetail::AdjustFloat(v, v_min, v_max, wheel_step))
            changed = true;

        return changed;
    }

#if !defined(CONTRIB_DISABLE_SLIDER_PERCENT) && \
    !defined(IMGUI_CONTRIB_DISABLE_SLIDER_PERCENT) && \
    !defined(IMGUI_DISABLE_CONTRIB_SLIDER_PERCENT) && \
    !defined(IMGUI_DISABLE_SLIDER_PERCENT) && \
    !defined(IMGUI_DISABLE_EXTRA_SLIDER_PERCENT)
    inline IMGUI_API bool SliderPercent(const char* label,
                                        float* v,
                                        float v_min = 0.0f,
                                        float v_max = 1.0f,
                                        const char* format = "%.0f%%",
                                        ImGuiSliderFlags flags = 0,
                                        float wheel_step = 0.01f)
    {
        return PercentSlider(label, v, v_min, v_max, format, flags, wheel_step);
    }
#endif
#endif // !CONTRIB_DISABLE_PERCENT_SLIDER

    // -------------------------------------------------------------------------
    // 6. Composite Sliders & Drag Boxes with Reset [ Control | ↺/↻ ] Label
    // -------------------------------------------------------------------------
    // Eliminates all developer layout headache: automatically reserves width,
    // places an abutting reset/restore button next to the control box, appends
    // the label, and provides dual-tooltip, mouse-wheel, and Reset-with-Undo
    // (1-click A/B toggle between default and previous value) support!
    // Pass an optional `v_last` pointer if you want the pre-reset value persisted
    // in your own variable/dataref across sessions; otherwise session memory is
    // managed automatically per widget ID.

    inline IMGUI_API bool SliderWithReset(const char* label,
                                          float* v,
                                          float v_default,
                                          float v_min,
                                          float v_max,
                                          const char* format = "%.3f",
                                          ImGuiSliderFlags flags = 0,
                                          float wheel_step = 0.01f,
                                          const char* reset_tooltip = nullptr,
                                          float* v_last = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = WheelSliderDetail::CalcResetButtonWidth(btn_size, true);
        const ImGuiStyle& style = GetStyle();
        float total_item_w = CalcItemWidth();
        float slider_w = std::max(50.0f, total_item_w - btn_w);

        // 1. Slider Track:
        PushItemWidth(slider_w);
        bool changed = SliderFloat("##slider", v, v_min, v_max, format, flags);
        PopItemWidth();
        bool slider_active = IsItemActive();

        ImGuiIO& io = GetIO();
        float wheel_clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;
        bool is_scrolling = (wheel_clicks != 0.0f && IsItemHovered());

        if (WheelSliderDetail::AdjustFloat(v, v_min, v_max, wheel_step))
            changed = true;

        // 2. Abutting Reset / Restore-Previous Button (0 gap, flat left edge):
        const float kEps = 0.0005f;
        ImGuiID undo_id = GetID("##reset_undo");
        if (changed && std::abs(*v - v_default) <= kEps) {
            WheelSliderDetail::ClearFloatUndo(undo_id, v_default, v_last);
        }

        SameLine(0.0f, 0.0f);
        bool is_modified = std::abs(*v - v_default) > kEps;
        float restore_val = v_default;
        bool can_restore = !is_modified && WheelSliderDetail::ResolveFloatUndo(undo_id, *v, v_default, kEps, v_last, &restore_val);

        if (ResetButton("##reset", is_modified, btn_w, true, nullptr, can_restore)) {
            if (is_modified) {
                WheelSliderDetail::RecordFloatReset(undo_id, *v, v_last);
                *v = v_default;
                changed = true;
            } else if (can_restore) {
                *v = (v_min < v_max) ? WheelSliderDetail::Clamp(restore_val, v_min, v_max) : restore_val;
                changed = true;
            }
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            std::snprintf(val_str, sizeof(val_str), format, v_default);
            if (is_modified) {
                std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
            } else if (can_restore) {
                char prev_str[64];
                std::snprintf(prev_str, sizeof(prev_str), format, restore_val);
                std::snprintf(default_buf, sizeof(default_buf), "At default (%s). Click to restore previous (%s).", val_str, prev_str);
            } else {
                std::snprintf(default_buf, sizeof(default_buf), "Already at default (%s).", val_str);
            }
        }

        WheelSliderDetail::AttachTooltip(default_buf, reset_hovered);

        // 3. Label Text:
        if (has_label) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();

        // 4. Input Blocking & Hover Isolation:
        if (slider_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || slider_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    inline IMGUI_API bool SliderWithReset(const char* label,
                                          int* v,
                                          int v_default,
                                          int v_min,
                                          int v_max,
                                          const char* format = "%d",
                                          ImGuiSliderFlags flags = 0,
                                          float wheel_step = 1.0f,
                                          const char* reset_tooltip = nullptr,
                                          int* v_last = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = WheelSliderDetail::CalcResetButtonWidth(btn_size, true);
        const ImGuiStyle& style = GetStyle();
        float total_item_w = CalcItemWidth();
        float slider_w = std::max(50.0f, total_item_w - btn_w);

        // 1. Slider Track:
        PushItemWidth(slider_w);
        bool changed = SliderInt("##slider", v, v_min, v_max, format, flags);
        PopItemWidth();
        bool slider_active = IsItemActive();

        ImGuiIO& io = GetIO();
        float wheel_clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;
        bool is_scrolling = (wheel_clicks != 0.0f && IsItemHovered());

        if (WheelSliderDetail::AdjustInt(v, v_min, v_max, wheel_step))
            changed = true;

        // 2. Abutting Reset / Restore-Previous Button (0 gap, flat left edge):
        ImGuiID undo_id = GetID("##reset_undo");
        if (changed && *v == v_default) {
            WheelSliderDetail::ClearIntUndo(undo_id, v_default, v_last);
        }

        SameLine(0.0f, 0.0f);
        bool is_modified = (*v != v_default);
        int restore_val = v_default;
        bool can_restore = !is_modified && WheelSliderDetail::ResolveIntUndo(undo_id, *v, v_default, v_last, &restore_val);

        if (ResetButton("##reset", is_modified, btn_w, true, nullptr, can_restore)) {
            if (is_modified) {
                WheelSliderDetail::RecordIntReset(undo_id, *v, v_last);
                *v = v_default;
                changed = true;
            } else if (can_restore) {
                *v = (v_min < v_max) ? WheelSliderDetail::Clamp(restore_val, v_min, v_max) : restore_val;
                changed = true;
            }
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            std::snprintf(val_str, sizeof(val_str), format, v_default);
            if (is_modified) {
                std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
            } else if (can_restore) {
                char prev_str[64];
                std::snprintf(prev_str, sizeof(prev_str), format, restore_val);
                std::snprintf(default_buf, sizeof(default_buf), "At default (%s). Click to restore previous (%s).", val_str, prev_str);
            } else {
                std::snprintf(default_buf, sizeof(default_buf), "Already at default (%s).", val_str);
            }
        }

        WheelSliderDetail::AttachTooltip(default_buf, reset_hovered);

        // 3. Label Text:
        if (has_label) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();

        // 4. Input Blocking & Hover Isolation:
        if (slider_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || slider_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    inline IMGUI_API bool PercentSliderWithReset(const char* label,
                                                 float* v,
                                                 float v_default = 1.0f,
                                                 float v_min = 0.0f,
                                                 float v_max = 1.0f,
                                                 const char* format = "%.0f%%",
                                                 ImGuiSliderFlags flags = 0,
                                                 float wheel_step = 0.01f,
                                                 const char* reset_tooltip = nullptr,
                                                 float* v_last = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = WheelSliderDetail::CalcResetButtonWidth(btn_size, true);
        const ImGuiStyle& style = GetStyle();
        float total_item_w = CalcItemWidth();
        float slider_w = std::max(50.0f, total_item_w - btn_w);

        // 1. Slider Track (Percent):
        float pct = *v * 100.0f;
        PushItemWidth(slider_w);
        bool changed = SliderFloat("##slider", &pct, v_min * 100.0f, v_max * 100.0f, format, flags);
        PopItemWidth();
        *v = pct / 100.0f;
        bool slider_active = IsItemActive();

        ImGuiIO& io = GetIO();
        float wheel_clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;
        bool is_scrolling = (wheel_clicks != 0.0f && IsItemHovered());

        if (WheelSliderDetail::AdjustFloat(v, v_min, v_max, wheel_step))
            changed = true;

        // 2. Abutting Reset / Restore-Previous Button (0 gap, flat left edge):
        const float kEps = 0.001f;
        ImGuiID undo_id = GetID("##reset_undo");
        if (changed && std::abs(*v - v_default) <= kEps) {
            WheelSliderDetail::ClearFloatUndo(undo_id, v_default, v_last);
        }

        SameLine(0.0f, 0.0f);
        bool is_modified = std::abs(*v - v_default) > kEps;
        float restore_val = v_default;
        bool can_restore = !is_modified && WheelSliderDetail::ResolveFloatUndo(undo_id, *v, v_default, kEps, v_last, &restore_val);

        if (ResetButton("##reset", is_modified, btn_w, true, nullptr, can_restore)) {
            if (is_modified) {
                WheelSliderDetail::RecordFloatReset(undo_id, *v, v_last);
                *v = v_default;
                changed = true;
            } else if (can_restore) {
                *v = (v_min < v_max) ? WheelSliderDetail::Clamp(restore_val, v_min, v_max) : restore_val;
                changed = true;
            }
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            std::snprintf(val_str, sizeof(val_str), format, v_default * 100.0f);
            if (is_modified) {
                std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
            } else if (can_restore) {
                char prev_str[64];
                std::snprintf(prev_str, sizeof(prev_str), format, restore_val * 100.0f);
                std::snprintf(default_buf, sizeof(default_buf), "At default (%s). Click to restore previous (%s).", val_str, prev_str);
            } else {
                std::snprintf(default_buf, sizeof(default_buf), "Already at default (%s).", val_str);
            }
        }

        WheelSliderDetail::AttachTooltip(default_buf, reset_hovered);

        // 3. Label Text:
        if (has_label) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();

        // 4. Input Blocking & Hover Isolation:
        if (slider_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || slider_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    // Composite Drag Boxes with Reset [ Drag Box | ↺/↻ ] Label (Overloaded for float* & int*):
    inline IMGUI_API bool DragWithReset(const char* label,
                                        float* v,
                                        float v_default,
                                        float v_speed = 1.0f,
                                        float v_min = 0.0f,
                                        float v_max = 0.0f,
                                        const char* format = "%.3f",
                                        ImGuiSliderFlags flags = 0,
                                        float wheel_step = 0.0f,
                                        const char* reset_tooltip = nullptr,
                                        float* v_last = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = WheelSliderDetail::CalcResetButtonWidth(btn_size, true);
        const ImGuiStyle& style = GetStyle();
        float total_item_w = CalcItemWidth();
        float drag_w = std::max(50.0f, total_item_w - btn_w);

        // 1. Drag Box:
        PushItemWidth(drag_w);
        bool changed = DragFloat("##drag", v, v_speed, v_min, v_max, format, flags);
        PopItemWidth();
        bool drag_active = IsItemActive();

        ImGuiIO& io = GetIO();
        float wheel_clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;
        bool is_scrolling = (wheel_clicks != 0.0f && IsItemHovered());

        float step = (wheel_step > 0.0f) ? wheel_step : ((v_speed > 0.0f) ? v_speed : 1.0f);
        if (WheelSliderDetail::AdjustFloat(v, v_min, v_max, step))
            changed = true;

        // 2. Abutting Reset / Restore-Previous Button (0 gap, flat left edge):
        const float kEps = 0.0005f;
        ImGuiID undo_id = GetID("##reset_undo");
        if (changed && std::abs(*v - v_default) <= kEps) {
            WheelSliderDetail::ClearFloatUndo(undo_id, v_default, v_last);
        }

        SameLine(0.0f, 0.0f);
        bool is_modified = std::abs(*v - v_default) > kEps;
        float restore_val = v_default;
        bool can_restore = !is_modified && WheelSliderDetail::ResolveFloatUndo(undo_id, *v, v_default, kEps, v_last, &restore_val);

        if (ResetButton("##reset", is_modified, btn_w, true, nullptr, can_restore)) {
            if (is_modified) {
                WheelSliderDetail::RecordFloatReset(undo_id, *v, v_last);
                *v = v_default;
                changed = true;
            } else if (can_restore) {
                *v = (v_min < v_max) ? WheelSliderDetail::Clamp(restore_val, v_min, v_max) : restore_val;
                changed = true;
            }
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            std::snprintf(val_str, sizeof(val_str), format, v_default);
            if (is_modified) {
                std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
            } else if (can_restore) {
                char prev_str[64];
                std::snprintf(prev_str, sizeof(prev_str), format, restore_val);
                std::snprintf(default_buf, sizeof(default_buf), "At default (%s). Click to restore previous (%s).", val_str, prev_str);
            } else {
                std::snprintf(default_buf, sizeof(default_buf), "Already at default (%s).", val_str);
            }
        }

        WheelSliderDetail::AttachTooltip(default_buf, reset_hovered);

        // 3. Label Text:
        if (has_label) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();

        // 4. Input Blocking & Hover Isolation:
        if (drag_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || drag_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    inline IMGUI_API bool DragWithReset(const char* label,
                                        int* v,
                                        int v_default,
                                        float v_speed = 1.0f,
                                        int v_min = 0,
                                        int v_max = 0,
                                        const char* format = "%d",
                                        ImGuiSliderFlags flags = 0,
                                        float wheel_step = 0.0f,
                                        const char* reset_tooltip = nullptr,
                                        int* v_last = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = WheelSliderDetail::CalcResetButtonWidth(btn_size, true);
        const ImGuiStyle& style = GetStyle();
        float total_item_w = CalcItemWidth();
        float drag_w = std::max(50.0f, total_item_w - btn_w);

        // 1. Drag Box:
        PushItemWidth(drag_w);
        bool changed = DragInt("##drag", v, v_speed, v_min, v_max, format, flags);
        PopItemWidth();
        bool drag_active = IsItemActive();

        ImGuiIO& io = GetIO();
        float wheel_clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;
        bool is_scrolling = (wheel_clicks != 0.0f && IsItemHovered());

        float step = (wheel_step > 0.0f) ? wheel_step : std::max(1.0f, v_speed);
        if (WheelSliderDetail::AdjustInt(v, v_min, v_max, step))
            changed = true;

        // 2. Abutting Reset / Restore-Previous Button (0 gap, flat left edge):
        ImGuiID undo_id = GetID("##reset_undo");
        if (changed && *v == v_default) {
            WheelSliderDetail::ClearIntUndo(undo_id, v_default, v_last);
        }

        SameLine(0.0f, 0.0f);
        bool is_modified = (*v != v_default);
        int restore_val = v_default;
        bool can_restore = !is_modified && WheelSliderDetail::ResolveIntUndo(undo_id, *v, v_default, v_last, &restore_val);

        if (ResetButton("##reset", is_modified, btn_w, true, nullptr, can_restore)) {
            if (is_modified) {
                WheelSliderDetail::RecordIntReset(undo_id, *v, v_last);
                *v = v_default;
                changed = true;
            } else if (can_restore) {
                *v = (v_min < v_max) ? WheelSliderDetail::Clamp(restore_val, v_min, v_max) : restore_val;
                changed = true;
            }
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            std::snprintf(val_str, sizeof(val_str), format, v_default);
            if (is_modified) {
                std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
            } else if (can_restore) {
                char prev_str[64];
                std::snprintf(prev_str, sizeof(prev_str), format, restore_val);
                std::snprintf(default_buf, sizeof(default_buf), "At default (%s). Click to restore previous (%s).", val_str, prev_str);
            } else {
                std::snprintf(default_buf, sizeof(default_buf), "Already at default (%s).", val_str);
            }
        }

        WheelSliderDetail::AttachTooltip(default_buf, reset_hovered);

        // 3. Label Text:
        if (has_label) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();

        // 4. Input Blocking & Hover Isolation:
        if (drag_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || drag_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    // Alternate adjective & explicit mouse-wheel forms (Resettable*, MouseWheel*WithReset):
    inline IMGUI_API bool ResettableSlider(const char* label, float* v, float v_default, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, float wheel_step = 0.01f, const char* reset_tooltip = nullptr, float* v_last = nullptr)
    {
        return SliderWithReset(label, v, v_default, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool ResettableSlider(const char* label, int* v, int v_default, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, float wheel_step = 1.0f, const char* reset_tooltip = nullptr, int* v_last = nullptr)
    {
        return SliderWithReset(label, v, v_default, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool ResettablePercentSlider(const char* label, float* v, float v_default = 1.0f, float v_min = 0.0f, float v_max = 1.0f, const char* format = "%.0f%%", ImGuiSliderFlags flags = 0, float wheel_step = 0.01f, const char* reset_tooltip = nullptr, float* v_last = nullptr)
    {
        return PercentSliderWithReset(label, v, v_default, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool SliderPercentWithReset(const char* label, float* v, float v_default = 1.0f, float v_min = 0.0f, float v_max = 1.0f, const char* format = "%.0f%%", ImGuiSliderFlags flags = 0, float wheel_step = 0.01f, const char* reset_tooltip = nullptr, float* v_last = nullptr)
    {
        return PercentSliderWithReset(label, v, v_default, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool ResettableDrag(const char* label, float* v, float v_default, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, float wheel_step = 0.0f, const char* reset_tooltip = nullptr, float* v_last = nullptr)
    {
        return DragWithReset(label, v, v_default, v_speed, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool ResettableDrag(const char* label, int* v, int v_default, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0, float wheel_step = 0.0f, const char* reset_tooltip = nullptr, int* v_last = nullptr)
    {
        return DragWithReset(label, v, v_default, v_speed, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool MouseWheelSliderWithReset(const char* label, float* v, float v_default, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, float wheel_step = 0.01f, const char* reset_tooltip = nullptr, float* v_last = nullptr)
    {
        return SliderWithReset(label, v, v_default, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool MouseWheelSliderWithReset(const char* label, int* v, int v_default, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, float wheel_step = 1.0f, const char* reset_tooltip = nullptr, int* v_last = nullptr)
    {
        return SliderWithReset(label, v, v_default, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool MouseWheelDragWithReset(const char* label, float* v, float v_default, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, float wheel_step = 0.0f, const char* reset_tooltip = nullptr, float* v_last = nullptr)
    {
        return DragWithReset(label, v, v_default, v_speed, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

    inline IMGUI_API bool MouseWheelDragWithReset(const char* label, int* v, int v_default, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0, float wheel_step = 0.0f, const char* reset_tooltip = nullptr, int* v_last = nullptr)
    {
        return DragWithReset(label, v, v_default, v_speed, v_min, v_max, format, flags, wheel_step, reset_tooltip, v_last);
    }

#if !defined(CONTRIB_DISABLE_MOUSE_WHEEL_COMBO) && \
    !defined(IMGUI_CONTRIB_DISABLE_MOUSE_WHEEL_COMBO) && \
    !defined(IMGUI_DISABLE_CONTRIB_MOUSE_WHEEL_COMBO) && \
    !defined(IMGUI_DISABLE_MOUSE_WHEEL_COMBO) && \
    !defined(IMGUI_DISABLE_EXTRA_MOUSE_WHEEL_COMBO) && \
    !defined(CONTRIB_DISABLE_WHEEL_COMBO) && \
    !defined(IMGUI_CONTRIB_DISABLE_WHEEL_COMBO) && \
    !defined(IMGUI_DISABLE_CONTRIB_WHEEL_COMBO) && \
    !defined(IMGUI_DISABLE_WHEEL_COMBO) && \
    !defined(IMGUI_DISABLE_EXTRA_WHEEL_COMBO)
    // -------------------------------------------------------------------------
    // 7. MouseWheelCombo & ComboWithReset
    // -------------------------------------------------------------------------
    // Configurable default cursors for MouseWheelCombo / ComboWithReset:
    //  - IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR: shown when hovering over the
    //    combo box preview area (default: ImGuiMouseCursor_ResizeNS). Hovering
    //    over the right-hand chevron button [▼] always shows ImGuiMouseCursor_Hand
    //    and ignores mouse-wheel scrolling.
    //  - IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR: shown when hovering over items
    //    inside the open dropdown popup list (default: ImGuiMouseCursor_ResizeNS).
    // Define either macro to ImGuiMouseCursor_None (or ImGuiMouseCursor_Arrow /
    // ImGuiMouseCursor_Hand) before including this header to change the default,
    // or pass 'hover_cursor' / 'popup_cursor' per call.
#ifndef IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR
#define IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR ImGuiMouseCursor_ResizeNS
#endif

#ifndef IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR
#define IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR ImGuiMouseCursor_ResizeNS
#endif

    // Turnkey dropdown combo controls that allow the user to:
    //  a) Hover over the closed combo box preview text and spin the mouse wheel
    //     to step through items directly in-place (clamped to [0, items_count - 1]), OR
    //  b) Click the combo or chevron button [▼] to open the dropdown popup list
    //     and STILL spin the mouse wheel while hovering over the open list (or
    //     preview box) to step through items live, OR click any item to select and close.
    // Supports optional per-item tooltips ('item_tooltips[]') with open-menu
    // wheel-selection tooltip latching ("Lens UI"), independent cursor
    // customization for the combo preview ('hover_cursor') vs open popup ('popup_cursor'),
    // and configurable chevron arrow styles (ImGui::SetDefaultComboArrowStyle).

    inline IMGUI_API bool MouseWheelCombo(const char* label,
                                          int* current_item,
                                          const char* const items[],
                                          int items_count,
                                          int popup_max_height_in_items = -1,
                                          bool wrap = false,
                                          const char* const item_tooltips[] = nullptr,
                                          ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR,
                                          ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR)
    {
        bool popup_open = false;
        bool is_scrolling = false;
        bool changed = WheelSliderDetail::ComboWithWheelEx(label, current_item, items, items_count,
                                                           popup_max_height_in_items, wrap, true,
                                                           item_tooltips, hover_cursor, popup_cursor,
                                                           &popup_open, &is_scrolling);
        bool combo_header_active = IsItemActive();
        if (combo_header_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }
        if (popup_open || combo_header_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }
        return changed;
    }

    // Null-separated string overload ("Item 1\0Item 2\0Item 3\0\0")
    inline IMGUI_API bool MouseWheelCombo(const char* label,
                                          int* current_item,
                                          const char* items_separated_by_zeros,
                                          int popup_max_height_in_items = -1,
                                          bool wrap = false,
                                          const char* const item_tooltips[] = nullptr,
                                          ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR,
                                          ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR)
    {
        if (!items_separated_by_zeros) return false;
        const char* items_buf[64];
        int count = 0;
        const char* p = items_separated_by_zeros;
        while (*p && count < 64) {
            items_buf[count++] = p;
            p += std::strlen(p) + 1;
        }
        return MouseWheelCombo(label, current_item, items_buf, count,
                               popup_max_height_in_items, wrap, item_tooltips, hover_cursor, popup_cursor);
    }

    // Composite Combo with Reset: [ Combo Box | ↺/↻ ] Label (with Built-In Mouse-Wheel & Reset-with-Undo)
    inline IMGUI_API bool ComboWithReset(const char* label,
                                         int* current_item,
                                         int default_item,
                                         const char* const items[],
                                         int items_count,
                                         int popup_max_height_in_items = -1,
                                         bool wrap = false,
                                         const char* reset_tooltip = nullptr,
                                         const char* const item_tooltips[] = nullptr,
                                         ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR,
                                         ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR,
                                         int* last_item = nullptr)
    {
        if (!current_item || !items || items_count <= 0) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = WheelSliderDetail::CalcResetButtonWidth(btn_size, true);
        const ImGuiStyle& style = GetStyle();
        float total_item_w = CalcItemWidth();
        float combo_w = std::max(50.0f, total_item_w - btn_w);

        // 1. Combo Box (supports wheel cycling both when closed AND while popup is open):
        bool popup_open = false;
        bool is_scrolling = false;
        PushItemWidth(combo_w);
        bool changed = WheelSliderDetail::ComboWithWheelEx("##combo", current_item, items, items_count,
                                                           popup_max_height_in_items, wrap, true,
                                                           item_tooltips, hover_cursor, popup_cursor,
                                                           &popup_open, &is_scrolling);
        PopItemWidth();
        bool combo_header_active = IsItemActive();

        // 2. Abutting Reset / Restore-Previous Button (0 gap, flat left edge):
        int clamped_default = WheelSliderDetail::Clamp(default_item, 0, items_count - 1);
        ImGuiID undo_id = GetID("##reset_undo");
        if (changed && *current_item == clamped_default) {
            WheelSliderDetail::ClearIntUndo(undo_id, clamped_default, last_item);
        }

        SameLine(0.0f, 0.0f);
        bool is_modified = (*current_item != clamped_default);
        int restore_item = clamped_default;
        bool can_restore = !is_modified &&
                           WheelSliderDetail::ResolveIntUndo(undo_id, *current_item, clamped_default, last_item, &restore_item) &&
                           (restore_item >= 0 && restore_item < items_count);

        if (ResetButton("##reset", is_modified, btn_w, true, nullptr, can_restore)) {
            if (is_modified) {
                WheelSliderDetail::RecordIntReset(undo_id, *current_item, last_item);
                *current_item = clamped_default;
                changed = true;
            } else if (can_restore) {
                *current_item = WheelSliderDetail::Clamp(restore_item, 0, items_count - 1);
                changed = true;
            }
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            const char* def_name = (clamped_default >= 0 && clamped_default < items_count && items[clamped_default])
                                   ? items[clamped_default] : "default";
            if (is_modified) {
                std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", def_name);
            } else if (can_restore) {
                const char* prev_name = (restore_item >= 0 && restore_item < items_count && items[restore_item])
                                        ? items[restore_item] : "previous";
                std::snprintf(default_buf, sizeof(default_buf), "At default (%s). Click to restore previous (%s).", def_name, prev_name);
            } else {
                std::snprintf(default_buf, sizeof(default_buf), "Already at default (%s).", def_name);
            }
        }

        WheelSliderDetail::AttachTooltip(default_buf, reset_hovered);

        // 3. Label Text:
        if (has_label) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();

        // 4. Input Blocking & Hover Isolation:
        // Note: Do NOT clear active tooltip merely because popup_open is true,
        // so per-item tooltips inside the open popup can display undisturbed.
        if (combo_header_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || popup_open || combo_header_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    // Null-separated string overload for ComboWithReset ("Item 1\0Item 2\0Item 3\0\0")
    inline IMGUI_API bool ComboWithReset(const char* label,
                                         int* current_item,
                                         int default_item,
                                         const char* items_separated_by_zeros,
                                         int popup_max_height_in_items = -1,
                                         bool wrap = false,
                                         const char* reset_tooltip = nullptr,
                                         const char* const item_tooltips[] = nullptr,
                                         ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR,
                                         ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR,
                                         int* last_item = nullptr)
    {
        if (!items_separated_by_zeros) return false;
        const char* items_buf[64];
        int count = 0;
        const char* p = items_separated_by_zeros;
        while (*p && count < 64) {
            items_buf[count++] = p;
            p += std::strlen(p) + 1;
        }
        return ComboWithReset(label, current_item, default_item, items_buf, count,
                              popup_max_height_in_items, wrap, reset_tooltip, item_tooltips,
                              hover_cursor, popup_cursor, last_item);
    }

    // Alternate adjective & explicit mouse-wheel forms (ResettableCombo, MouseWheelComboWithReset):
    inline IMGUI_API bool ResettableCombo(const char* label, int* current_item, int default_item, const char* const items[], int items_count, int popup_max_height_in_items = -1, bool wrap = false, const char* reset_tooltip = nullptr, const char* const item_tooltips[] = nullptr, ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR, ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR, int* last_item = nullptr)
    {
        return ComboWithReset(label, current_item, default_item, items, items_count, popup_max_height_in_items, wrap, reset_tooltip, item_tooltips, hover_cursor, popup_cursor, last_item);
    }

    inline IMGUI_API bool ResettableCombo(const char* label, int* current_item, int default_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1, bool wrap = false, const char* reset_tooltip = nullptr, const char* const item_tooltips[] = nullptr, ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR, ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR, int* last_item = nullptr)
    {
        return ComboWithReset(label, current_item, default_item, items_separated_by_zeros, popup_max_height_in_items, wrap, reset_tooltip, item_tooltips, hover_cursor, popup_cursor, last_item);
    }

    inline IMGUI_API bool MouseWheelComboWithReset(const char* label, int* current_item, int default_item, const char* const items[], int items_count, int popup_max_height_in_items = -1, bool wrap = false, const char* reset_tooltip = nullptr, const char* const item_tooltips[] = nullptr, ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR, ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR, int* last_item = nullptr)
    {
        return ComboWithReset(label, current_item, default_item, items, items_count, popup_max_height_in_items, wrap, reset_tooltip, item_tooltips, hover_cursor, popup_cursor, last_item);
    }

    inline IMGUI_API bool MouseWheelComboWithReset(const char* label, int* current_item, int default_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1, bool wrap = false, const char* reset_tooltip = nullptr, const char* const item_tooltips[] = nullptr, ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_WHEEL_COMBO_DEFAULT_CURSOR, ImGuiMouseCursor popup_cursor = IMGUI_EXTRA_WHEEL_COMBO_POPUP_CURSOR, int* last_item = nullptr)
    {
        return ComboWithReset(label, current_item, default_item, items_separated_by_zeros, popup_max_height_in_items, wrap, reset_tooltip, item_tooltips, hover_cursor, popup_cursor, last_item);
    }
#endif // !CONTRIB_DISABLE_MOUSE_WHEEL_COMBO

} // namespace ImGui

#endif // IMGUI_MOUSE_WHEEL_CONTROLS_H
#endif // !IMGUI_DISABLE_MOUSE_WHEEL_CONTROLS

