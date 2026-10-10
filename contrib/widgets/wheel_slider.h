// ============================================================================
// ImgWindow Contrib Widget: WheelSlider
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// Original Upstream SliderPercent Inspiration: Birger Hoppe (LiveTraffic)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Mouse-wheel enhanced sliders and value adjustment controls:
// 1. ImGui::AdjustOnItemMouseWheel: Non-intrusive helper attaching to any
//    preceding widget to handle vertical/horizontal scroll wheel adjustments.
//    (Option C: shows <--> cursor strictly over the physical slider track).
// 2. ImGui::SliderFloatWithWheel & ImGui::SliderIntWithWheel: Sliders with
//    built-in mouse wheel responsiveness.
// 3. ImGui::SliderPercentWithWheel: Percentage slider (0.0 to 1.0 displayed as 0%
//    to 100%) with automatic scroll wheel support.
// 4. ImGui::ResetSliderFloatWithWheel / ResetSliderPercentWithWheel:
//    Composite controls featuring an abutting vector undo/reset button,
//    smart width reservation, and dual-tooltip support.
// ============================================================================

#pragma once

#ifndef IMGUI_DISABLE_EXTRA_WHEEL_SLIDER
#ifndef IMGUI_EXTRA_WHEEL_SLIDER_H
#define IMGUI_EXTRA_WHEEL_SLIDER_H

#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdio>

#if !defined(IMGUI_TOOLTIP_EXT_H) && defined(__has_include)
#if __has_include("../extensions/TimedTooltip/imgui_tooltip_ext.h")
#include "../extensions/TimedTooltip/imgui_tooltip_ext.h"
#endif
#endif

namespace ImGui {

#ifndef IMGUI_DISABLE_EXTRA_ADJUST_MOUSE_WHEEL
    // -------------------------------------------------------------------------
    // 1. AdjustOnItemMouseWheel Helper
    // -------------------------------------------------------------------------
    // Attaches to the preceding widget via IsItemHovered().
    // Inspects mouse-wheel delta (vertical or horizontal), applies scaled step
    // increments/decrements with boundary clamping.
    // Option C: Shows the East-West resize cursor (ImGuiMouseCursor_ResizeEW)
    // strictly when the mouse hovers over the physical slider track (not label).
    //
    // Returns true if the referenced value was adjusted.
    inline bool AdjustOnItemMouseWheel(float* valRef,
                                       float minVal = 0.0f,
                                       float maxVal = 1.0f,
                                       float scale = 0.01f,
                                       bool setCustomCursor = true)
    {
        if (!valRef || !IsItemHovered())
            return false;

        ImGuiIO& io = GetIO();
        float clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;

        bool isHandled = false;
        if (clicks != 0.0f) {
            // Apply scaled clicks (round up to at least one discrete unit):
            if (clicks < 0.0f)
                clicks = std::min(-1.0f, clicks - 0.5f);
            else
                clicks = std::max(1.0f, clicks + 0.5f);

            float prev = *valRef;
            *valRef = std::clamp(*valRef + scale * clicks, minVal, maxVal);
            if (*valRef != prev)
                isHandled = true;
        }

        if (setCustomCursor) {
            // Option C: Restrict <--> cursor strictly to the slider track bounds
            ImVec2 item_min = GetItemRectMin();
            float track_w = CalcItemWidth();
            if (io.MousePos.x >= item_min.x && io.MousePos.x <= item_min.x + track_w) {
                SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
        }

        return isHandled;
    }

    inline bool AdjustOnItemMouseWheel(int* valRef,
                                       int minVal = 0,
                                       int maxVal = 100,
                                       int scale = 1,
                                       bool setCustomCursor = true)
    {
        if (!valRef || !IsItemHovered())
            return false;

        ImGuiIO& io = GetIO();
        float clicks = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;

        bool isHandled = false;
        if (clicks != 0.0f) {
            int delta = 0;
            if (clicks < 0.0f)
                delta = std::min(-1, (int)std::floor(scale * clicks - 0.5f));
            else
                delta = std::max(1, (int)std::floor(scale * clicks + 0.5f));

            int prev = *valRef;
            *valRef = std::clamp(*valRef + delta, minVal, maxVal);
            if (*valRef != prev)
                isHandled = true;
        }

        if (setCustomCursor) {
            ImVec2 item_min = GetItemRectMin();
            float track_w = CalcItemWidth();
            if (io.MousePos.x >= item_min.x && io.MousePos.x <= item_min.x + track_w) {
                SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
        }

        return isHandled;
    }
#endif // IMGUI_DISABLE_EXTRA_ADJUST_MOUSE_WHEEL

    // -------------------------------------------------------------------------
    // 2. Vector Undo/Reset Button Helper (Zero FontAwesome Dependency)
    // -------------------------------------------------------------------------
    // Draws a crisp, anti-aliased 240-degree counter-clockwise arc with a
    // vector arrowhead pointing left at the top. Scales automatically to any DPI.
    inline bool ResetButton(const char* str_id, bool is_modified = true, float width_arg = 0.0f, bool abutting = false)
    {
        float height = GetFrameHeight();
        float width  = (width_arg > 0.0f) ? width_arg : (abutting ? std::round(height * 0.85f) : height);
        PushID(str_id);
        ImVec2 p = GetCursorScreenPos();
        bool pressed = InvisibleButton("##reset_btn", ImVec2(width, height));
        bool hovered = IsItemHovered();
        bool active = IsItemActive();
        PopID();

        if (hovered) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        ImDrawList* draw_list = GetWindowDrawList();
        const ImGuiStyle& style = GetStyle();
        ImU32 bg_col = GetColorU32(active ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
        float rounding = style.FrameRounding;
        ImDrawFlags round_flags = abutting ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersAll;
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg_col, rounding, round_flags);
        if (style.FrameBorderSize > 0.0f) {
            draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), GetColorU32(ImGuiCol_Border), rounding, round_flags, style.FrameBorderSize);
        }

        // Color: prominent text color if modified, dimmed if already at default!
        ImU32 icon_col = GetColorU32(is_modified ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        ImVec2 center(p.x + width * 0.48f, p.y + height * 0.5f);
        float radius = height * 0.26f;
        float thickness = std::max(1.0f, height * 0.08f);

        // Draw counter-clockwise undo circular arc:
        const float kPI = 3.14159265f;
        draw_list->PathArcTo(center, radius, -0.05f * kPI, 1.25f * kPI, 16);
        draw_list->PathStroke(icon_col, 0, thickness);

        // Arrowhead pointing counter-clockwise (leftwards) at top of arc:
        float arrow_len = radius * 0.85f;
        float arrow_w   = radius * 0.65f;
        ImVec2 tip(center.x - radius * 0.1f, center.y - radius);
        ImVec2 p1(tip.x + arrow_len, tip.y - arrow_w);
        ImVec2 p2(tip.x + arrow_len, tip.y + arrow_w);
        draw_list->AddTriangleFilled(tip, p1, p2, icon_col);

        return pressed;
    }

    // -------------------------------------------------------------------------
    // 3. Integrated Wheel Sliders
    // -------------------------------------------------------------------------

    inline bool SliderFloatWithWheel(const char* label,
                                     float* v,
                                     float v_min,
                                     float v_max,
                                     const char* format = "%.3f",
                                     ImGuiSliderFlags flags = 0,
                                     float wheel_step = 0.01f)
    {
        bool changed = SliderFloat(label, v, v_min, v_max, format, flags);
        if (AdjustOnItemMouseWheel(v, v_min, v_max, wheel_step))
            changed = true;
        return changed;
    }

    inline bool SliderIntWithWheel(const char* label,
                                   int* v,
                                   int v_min,
                                   int v_max,
                                   const char* format = "%d",
                                   ImGuiSliderFlags flags = 0,
                                   int wheel_step = 1)
    {
        bool changed = SliderInt(label, v, v_min, v_max, format, flags);
        if (AdjustOnItemMouseWheel(v, v_min, v_max, wheel_step))
            changed = true;
        return changed;
    }

    // -------------------------------------------------------------------------
    // 4. SliderPercentWithWheel (0.0 .. 1.0 displayed as 0% .. 100%)
    // -------------------------------------------------------------------------
    inline bool SliderPercentWithWheel(const char* label,
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

        if (AdjustOnItemMouseWheel(v, v_min, v_max, wheel_step))
            changed = true;

        return changed;
    }

#ifndef IMGUI_DISABLE_EXTRA_SLIDER_PERCENT
    inline bool SliderPercent(const char* label,
                              float* v,
                              float v_min = 0.0f,
                              float v_max = 1.0f,
                              const char* format = "%.0f%%",
                              ImGuiSliderFlags flags = 0,
                              float wheel_step = 0.01f)
    {
        return SliderPercentWithWheel(label, v, v_min, v_max, format, flags, wheel_step);
    }
#endif

    // -------------------------------------------------------------------------
    // 5. Composite Reset Sliders [ Track ] [ ↺ Reset ] Label
    // -------------------------------------------------------------------------
    // Eliminates all developer layout headache: automatically reserves width,
    // places an abutting vector reset button next to the slider track, appends
    // the label, and provides dual-tooltip support!

    inline bool ResetSliderFloatWithWheel(const char* label,
                                          float* v,
                                          float v_default,
                                          float v_min,
                                          float v_max,
                                          const char* format = "%.3f",
                                          ImGuiSliderFlags flags = 0,
                                          float wheel_step = 0.01f,
                                          const char* reset_tooltip = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = std::round(btn_size * 0.85f);
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

        if (AdjustOnItemMouseWheel(v, v_min, v_max, wheel_step))
            changed = true;

        // 2. Abutting Vector Reset Button (0 gap, flat left edge):
        SameLine(0.0f, 0.0f);
        bool is_modified = std::abs(*v - v_default) > 0.0005f;
        if (ResetButton("##reset", is_modified, btn_w, true)) {
            *v = v_default;
            changed = true;
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[128];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            std::snprintf(val_str, sizeof(val_str), format, v_default);
            std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
        }

#if defined(IMGUI_TOOLTIP_EXT_H)
        TimedTooltip::Text("%s", default_buf);
#else
        if (reset_hovered) {
            SetTooltip("%s", default_buf);
        }
#endif

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

    inline bool ResetSliderPercentWithWheel(const char* label,
                                            float* v,
                                            float v_default = 1.0f,
                                            float v_min = 0.0f,
                                            float v_max = 1.0f,
                                            const char* format = "%.0f%%",
                                            ImGuiSliderFlags flags = 0,
                                            float wheel_step = 0.01f,
                                            const char* reset_tooltip = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = std::round(btn_size * 0.85f);
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

        if (AdjustOnItemMouseWheel(v, v_min, v_max, wheel_step))
            changed = true;

        // 2. Abutting Vector Reset Button (0 gap, flat left edge):
        SameLine(0.0f, 0.0f);
        bool is_modified = std::abs(*v - v_default) > 0.001f;
        if (ResetButton("##reset", is_modified, btn_w, true)) {
            *v = v_default;
            changed = true;
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[128];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%.0f%%).", v_default * 100.0f);
        }

#if defined(IMGUI_TOOLTIP_EXT_H)
        TimedTooltip::Text("%s", default_buf);
#else
        if (reset_hovered) {
            SetTooltip("%s", default_buf);
        }
#endif

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

    inline bool ResetSliderIntWithWheel(const char* label,
                                        int* v,
                                        int v_default,
                                        int v_min,
                                        int v_max,
                                        const char* format = "%d",
                                        ImGuiSliderFlags flags = 0,
                                        int wheel_step = 1,
                                        const char* reset_tooltip = nullptr)
    {
        if (!v) return false;

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        float btn_size = GetFrameHeight();
        float btn_w = std::round(btn_size * 0.85f);
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

        if (AdjustOnItemMouseWheel(v, v_min, v_max, wheel_step))
            changed = true;

        // 2. Abutting Vector Reset Button (0 gap, flat left edge):
        SameLine(0.0f, 0.0f);
        bool is_modified = (*v != v_default);
        if (ResetButton("##reset", is_modified, btn_w, true)) {
            *v = v_default;
            changed = true;
        }
        bool reset_hovered = IsItemHovered();

        char default_buf[128];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            std::snprintf(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            std::snprintf(default_buf, sizeof(default_buf), "Click to revert to default (%d).", v_default);
        }

#if defined(IMGUI_TOOLTIP_EXT_H)
        TimedTooltip::Text("%s", default_buf);
#else
        if (reset_hovered) {
            SetTooltip("%s", default_buf);
        }
#endif

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

} // namespace ImGui

#endif // IMGUI_EXTRA_WHEEL_SLIDER_H
#endif // IMGUI_DISABLE_EXTRA_WHEEL_SLIDER
