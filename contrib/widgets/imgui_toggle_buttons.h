// ============================================================================
// ImGui Contrib Widgets: ToggleButton & ToggleButtons (imgui_toggle_buttons.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Modern binary toggle controls for Dear ImGui:
// 1. ImGui::ToggleButton: Animated pill toggle switch with sliding circle thumb
//    (iOS / Material design style) and automatic hand cursor on hover.
// 2. ImGui::ToggleButtons: Segmented dual-button pair for explicit binary states.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_TOGGLE_BUTTONS) && \
    !defined(CONTRIB_DISABLE_TOGGLE_BUTTON) && \
    !defined(IMGUI_CONTRIB_DISABLE_TOGGLE_BUTTONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_TOGGLE_BUTTON) && \
    !defined(IMGUI_DISABLE_CONTRIB_TOGGLE_BUTTONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_TOGGLE_BUTTON) && \
    !defined(IMGUI_DISABLE_TOGGLE_BUTTONS) && \
    !defined(IMGUI_DISABLE_TOGGLE_BUTTON) && \
    !defined(IMGUI_DISABLE_EXTRA_TOGGLE_BUTTONS) && \
    !defined(IMGUI_DISABLE_EXTRA_TOGGLE_BUTTON)
#ifndef IMGUI_TOGGLE_BUTTONS_H
#define IMGUI_TOGGLE_BUTTONS_H
#define IMGUI_EXTRA_TOGGLE_BUTTON_H

#include "imgui.h"
#include <algorithm>
#include <cstring>

#include <cmath>

#ifndef IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR
#define IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR ImGuiMouseCursor_Hand
#endif

namespace ImGui {

    namespace ToggleButtonDetail {
        inline float ConsumeMouseWheel()
        {
#if defined(CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL) || \
    defined(IMGUI_CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL) || \
    defined(IMGUI_DISABLE_CONTRIB_TOGGLE_MOUSE_WHEEL) || \
    defined(IMGUI_DISABLE_TOGGLE_MOUSE_WHEEL) || \
    defined(IMGUI_DISABLE_EXTRA_TOGGLE_MOUSE_WHEEL)
            return 0.0f;
#else
            ImGuiIO& io = GetIO();
            float w = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;
            if (w != 0.0f) {
                io.MouseWheel = 0.0f;
                io.MouseWheelH = 0.0f;
            }
            return w;
#endif
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
    } // namespace ToggleButtonDetail

    // -------------------------------------------------------------------------
    // 1. Animated Toggle Switch (Pill Track + Sliding Thumb)
    // -------------------------------------------------------------------------
    // Renders a smooth, resolution-independent toggle pill.
    // If 'label' contains visible text before '##', the label is drawn to the
    // right of the toggle switch, matching standard ImGui::Checkbox semantics.
    // Clicking either the switch or label toggles the state and returns true.
    // Spinning the mouse wheel over the pill switch clamps cleanly between
    // Off (wheel down/left) and On (wheel up/right) without wrapping.
    // Optional 'item_tooltips' (2-element array: { off_tooltip, on_tooltip })
    // automatically displays the tooltip for the active state.
    inline IMGUI_API bool ToggleButton(const char* label,
                                       bool* v,
                                       const ImVec2& size_arg = ImVec2(0.0f, 0.0f),
                                       const char* const item_tooltips[] = nullptr,
                                       bool allow_mouse_wheel = true,
                                       ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        if (!v || !label) return false;

        BeginGroup();
        ImVec2 p = GetCursorScreenPos();
        ImDrawList* draw_list = GetWindowDrawList();
        const ImGuiStyle& style = GetStyle();

        // Proportional sizing derived from ambient frame height:
        float height = (size_arg.y > 0.0f) ? size_arg.y : GetFrameHeight();
        float width  = (size_arg.x > 0.0f) ? size_arg.x : (height * 1.62f);
        float radius = height * 0.5f;

        PushID(label);
        bool changed = InvisibleButton("##toggle_switch", ImVec2(width, height));
        bool switch_hovered = IsItemHovered();
        bool held           = IsItemActive();
        ImGuiID off_tt_id   = GetID("##tt_off");
        ImGuiID on_tt_id    = GetID("##tt_on");
        PopID();

        if (changed) {
            *v = !(*v);
        }

        if (switch_hovered) {
            if (hover_cursor != ImGuiMouseCursor_None) {
                SetMouseCursor(hover_cursor);
            }
            if (allow_mouse_wheel) {
                float wheel = ToggleButtonDetail::ConsumeMouseWheel();
                if (wheel > 0.0f && !(*v)) {
                    *v = true;
                    changed = true;
                } else if (wheel < 0.0f && *v) {
                    *v = false;
                    changed = true;
                }
            }
        }

        // Animate sliding thumb smoothly between 0.0f (off) and 1.0f (on)
        ImGuiStorage* storage = GetStateStorage();
        ImGuiID id = GetID(label);
        float* p_t = storage ? storage->GetFloatRef(id, *v ? 1.0f : 0.0f) : nullptr;
        float target = *v ? 1.0f : 0.0f;
        if (p_t) {
            float speed = 12.0f; // animation rate
            float dt = GetIO().DeltaTime;
            if (*p_t < target) {
                *p_t = std::min(target, *p_t + speed * dt);
            } else if (*p_t > target) {
                *p_t = std::max(target, *p_t - speed * dt);
            }
        }
        float t = p_t ? *p_t : target;

        // Render Track (Pill Background)
        ImVec4 col_off = style.Colors[ImGuiCol_FrameBg];
        ImVec4 col_on  = style.Colors[ImGuiCol_HeaderActive];
        if (col_on.w <= 0.0f) col_on = style.Colors[ImGuiCol_ButtonActive];

        ImVec4 track_vec = ImVec4(
            col_off.x + (col_on.x - col_off.x) * t,
            col_off.y + (col_on.y - col_off.y) * t,
            col_off.z + (col_on.z - col_off.z) * t,
            col_off.w + (col_on.w - col_off.w) * t
        );
        ImU32 track_col = GetColorU32(track_vec);
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), track_col, radius);

        if (style.FrameBorderSize > 0.0f) {
            draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), GetColorU32(ImGuiCol_Border), radius, 0, style.FrameBorderSize);
        }

        // Render Sliding Circular Thumb
        float thumb_radius = radius - 2.5f;
        if (thumb_radius < 3.0f) thumb_radius = 3.0f;
        float x_min = p.x + radius;
        float x_max = p.x + width - radius;
        float thumb_x = x_min + (x_max - x_min) * t;
        float thumb_y = p.y + radius;

        ImU32 thumb_col = GetColorU32(held ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
        draw_list->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius, thumb_col);

        // Render Optional Label Text (Matches ImGui::Checkbox semantics)
        bool label_hovered = false;
        const char* id_pos = std::strstr(label, "##");
        bool has_visible_text = (id_pos != label) && (label[0] != '\0');
        if (has_visible_text) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
            label_hovered = IsItemHovered();
            if (label_hovered) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
                if (IsItemClicked()) {
                    *v = !(*v);
                    changed = true;
                }
            }
        }

        EndGroup();

#if defined(IMGUI_TOOLTIP_EXT_H)
        if (changed) {
            TimedTooltip::ClearActiveTooltip();
        }
#endif

        if (item_tooltips) {
            bool any_hovered = (switch_hovered || label_hovered) && !changed;
            if (item_tooltips[0] && item_tooltips[0][0] != '\0') {
                ToggleButtonDetail::AttachTooltipProxy(off_tt_id, item_tooltips[0], any_hovered && !(*v));
            }
            if (item_tooltips[1] && item_tooltips[1][0] != '\0') {
                ToggleButtonDetail::AttachTooltipProxy(on_tt_id, item_tooltips[1], any_hovered && (*v));
            }
        }

        if (changed) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    // Overload without item_tooltips for direct (label, v, size_arg, allow_mouse_wheel, hover_cursor) calls:
    inline IMGUI_API bool ToggleButton(const char* label,
                                       bool* v,
                                       const ImVec2& size_arg,
                                       bool allow_mouse_wheel,
                                       ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        return ToggleButton(label, v, size_arg, nullptr, allow_mouse_wheel, hover_cursor);
    }

    // Overloads accepting int* v (0 = Off, non-zero = On), matching ToggleButtons:
    inline IMGUI_API bool ToggleButton(const char* label,
                                       int* v,
                                       const ImVec2& size_arg = ImVec2(0.0f, 0.0f),
                                       const char* const item_tooltips[] = nullptr,
                                       bool allow_mouse_wheel = true,
                                       ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        if (!v) return false;
        bool val = (*v != 0);
        bool res = ToggleButton(label, &val, size_arg, item_tooltips, allow_mouse_wheel, hover_cursor);
        if (res) *v = val ? 1 : 0;
        return res;
    }

    inline IMGUI_API bool ToggleButton(const char* label,
                                       int* v,
                                       const ImVec2& size_arg,
                                       bool allow_mouse_wheel,
                                       ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        return ToggleButton(label, v, size_arg, nullptr, allow_mouse_wheel, hover_cursor);
    }

    // -------------------------------------------------------------------------
    // 2. Segmented Dual-State Toggle Buttons
    // -------------------------------------------------------------------------
    // Side-by-side joined button pair for explicit binary states.
    // Highlights the active button and toggles the target variable.
    // - If 'label' contains visible text before '##', draws a clickable right-hand
    //   label (which toggles on click, shows Hand cursor, and ignores mouse-wheel).
    // - Spinning the mouse wheel over either button clamps cleanly between
    //   Off (0, wheel down/left) and On (1, wheel up/right) without wrapping.
    // - Supports optional per-state 'item_tooltips' ({ off_tooltip, on_tooltip })
    //   and 'out_hovered_item' (-1 = none, 0 = Off button, 1 = On button) with
    //   automatic wheel-selection tooltip latching until the mouse moves (> 1.5px).
    inline IMGUI_API bool ToggleButtons(const char* label,
                                        int* v,
                                        const char* label_off,
                                        const char* label_on,
                                        const ImVec2& button_size = ImVec2(0.0f, 0.0f),
                                        const char* const item_tooltips[] = nullptr,
                                        int* out_hovered_item = nullptr,
                                        bool allow_mouse_wheel = true,
                                        ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        if (out_hovered_item) *out_hovered_item = -1;
        if (!v || !label || !label_off || !label_on) return false;

        ImGuiIO& io = GetIO();
        ImGuiID toggle_id = GetID(label);

        static ImGuiID s_WheelLatchToggleID = 0;
        static ImVec2  s_WheelLatchMousePos = ImVec2(-1.0f, -1.0f);

        if (s_WheelLatchToggleID == toggle_id) {
            if (std::abs(io.MousePos.x - s_WheelLatchMousePos.x) > 1.5f ||
                std::abs(io.MousePos.y - s_WheelLatchMousePos.y) > 1.5f) {
                s_WheelLatchToggleID = 0;
#if defined(IMGUI_TOOLTIP_EXT_H)
                TimedTooltip::ClearActiveTooltip();
#endif
            }
        }

        BeginGroup();
        PushID(label);
        bool changed = false;
        bool is_on = (*v != 0);

        const ImGuiStyle& style = GetStyle();
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, style.ItemSpacing.y));

        // Off Button (0)
        if (is_on) {
            PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_FrameBg]);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
        } else {
            PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text]);
        }
        if (Button(label_off, button_size)) {
            s_WheelLatchToggleID = 0;
            if (is_on) {
                *v = 0;
                is_on = false;
                changed = true;
            }
        }
        bool off_hovered = IsItemHovered();
        ImGuiID off_id   = GetItemID();
        PopStyleColor(2);

        SameLine(0.0f, 0.0f);

        // On Button (1)
        if (is_on) {
            PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text]);
        } else {
            PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_FrameBg]);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
        }
        if (Button(label_on, button_size)) {
            s_WheelLatchToggleID = 0;
            if (!is_on) {
                *v = 1;
                is_on = true;
                changed = true;
            }
        }
        bool on_hovered = IsItemHovered();
        ImGuiID on_id   = GetItemID();
        PopStyleColor(2);

        PopStyleVar();

        // Optional Right-Side Clickable Label (Matches ImGui::Checkbox & ToggleButton)
        bool label_hovered = false;
        const char* id_pos = std::strstr(label, "##");
        bool has_visible_text = (id_pos != label) && (label[0] != '\0');
        if (has_visible_text) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
            label_hovered = IsItemHovered();
            if (label_hovered) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
                if (IsItemClicked()) {
                    s_WheelLatchToggleID = 0;
                    *v = is_on ? 0 : 1;
                    is_on = !is_on;
                    changed = true;
                }
            }
        }

        if (off_hovered || on_hovered) {
            if (hover_cursor != ImGuiMouseCursor_None) {
                SetMouseCursor(hover_cursor);
            }
            if (allow_mouse_wheel) {
                float wheel = ToggleButtonDetail::ConsumeMouseWheel();
                if (wheel != 0.0f) {
                    s_WheelLatchToggleID = toggle_id;
                    s_WheelLatchMousePos = io.MousePos;
#if defined(IMGUI_TOOLTIP_EXT_H)
                    TimedTooltip::ClearActiveTooltip();
#endif
                    if (wheel > 0.0f && !is_on) {
                        *v = 1;
                        is_on = true;
                        changed = true;
                    } else if (wheel < 0.0f && is_on) {
                        *v = 0;
                        is_on = false;
                        changed = true;
                    }
                }
            }
        } else if (!label_hovered && s_WheelLatchToggleID == toggle_id) {
            s_WheelLatchToggleID = 0;
        }

        int effective_hovered = -1;
        if (off_hovered || on_hovered) {
            if (s_WheelLatchToggleID == toggle_id) {
                effective_hovered = is_on ? 1 : 0;
            } else {
                effective_hovered = on_hovered ? 1 : 0;
            }
        } else if (label_hovered) {
            effective_hovered = is_on ? 1 : 0;
        }
        if (out_hovered_item) {
            *out_hovered_item = effective_hovered;
        }

        if (item_tooltips) {
            if (item_tooltips[0] && item_tooltips[0][0] != '\0') {
                ToggleButtonDetail::AttachTooltipProxy(off_id, item_tooltips[0], (effective_hovered == 0) && !changed);
            }
            if (item_tooltips[1] && item_tooltips[1][0] != '\0') {
                ToggleButtonDetail::AttachTooltipProxy(on_id, item_tooltips[1], (effective_hovered == 1) && !changed);
            }
        }

        PopID();
        EndGroup();

#if defined(IMGUI_TOOLTIP_EXT_H)
        if (changed) {
            TimedTooltip::ClearActiveTooltip();
        }
#endif

        if (changed) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return changed;
    }

    inline IMGUI_API bool ToggleButtons(const char* label,
                                        bool* v,
                                        const char* label_off,
                                        const char* label_on,
                                        const ImVec2& button_size = ImVec2(0.0f, 0.0f),
                                        const char* const item_tooltips[] = nullptr,
                                        int* out_hovered_item = nullptr,
                                        bool allow_mouse_wheel = true,
                                        ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        if (!v) return false;
        int val = *v ? 1 : 0;
        bool res = ToggleButtons(label, &val, label_off, label_on, button_size,
                                 item_tooltips, out_hovered_item, allow_mouse_wheel, hover_cursor);
        if (res) *v = (val != 0);
        return res;
    }

    // Overloads for direct (label, v, label_off, label_on, button_size, allow_mouse_wheel, hover_cursor) calls:
    inline IMGUI_API bool ToggleButtons(const char* label,
                                        int* v,
                                        const char* label_off,
                                        const char* label_on,
                                        const ImVec2& button_size,
                                        bool allow_mouse_wheel,
                                        ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        return ToggleButtons(label, v, label_off, label_on, button_size, nullptr, nullptr, allow_mouse_wheel, hover_cursor);
    }

    inline IMGUI_API bool ToggleButtons(const char* label,
                                        bool* v,
                                        const char* label_off,
                                        const char* label_on,
                                        const ImVec2& button_size,
                                        bool allow_mouse_wheel,
                                        ImGuiMouseCursor hover_cursor = IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR)
    {
        return ToggleButtons(label, v, label_off, label_on, button_size, nullptr, nullptr, allow_mouse_wheel, hover_cursor);
    }

} // namespace ImGui

#endif // IMGUI_EXTRA_TOGGLE_BUTTON_H
#endif // IMGUI_DISABLE_EXTRA_TOGGLE_BUTTON
