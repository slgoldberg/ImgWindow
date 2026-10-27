// ============================================================================
// ImGui Contrib Widgets: Avionics Rotary Knobs & Dimmer Dials (imgui_knobs.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) & Sirrogerio Il Sigundo
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone C++ immediate-mode avionics rotary knobs, dimmer dials, and
// potentiometers for Dear ImGui.
//
// Key Features:
// 1. Unmistakable Potentiometer Geometry:
//    - 270-degree "Omega" arc (open deadband at 6 o'clock) immediately signals
//      an interactive avionics rotary pot rather than a circular countdown timer.
//    - 3D knurled knob hub with rotating contrast needle/pip and perimeter graduation ticks.
// 2. Tactile Two-Step Detente Switch:
//    - Counter-clockwise rotation dims down and stops cleanly at minimum active brightness
//      (e.g., 5%), preventing accidental power-down in flight.
//    - A deliberate extra downward step past the stop snaps across the detente gap
//      directly into the powered-down "OFF" detente.
//    - Clockwise rotation from "OFF" snaps immediately back to minimum active power.
// 3. Dual-Mode Input Ergonomics:
//    - Mouse-Wheel Primary: Hover and roll wheel (with automatic scroll-defeat).
//    - Click & Drag Secondary: Vertical/horizontal drag or angular scrub.
//    - Range Affordance: Shows ImGuiMouseCursor_ResizeEW (<-->) by default.
// 4. Reset & Undo Integration:
//    - Double-click to restore calibrated default (or toggle between OFF and default).
//    - Turnkey composite KnobWithReset pairing with the vector ↺ ResetButton.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_KNOBS) && \
    !defined(CONTRIB_DISABLE_KNOB) && \
    !defined(IMGUI_CONTRIB_DISABLE_KNOBS) && \
    !defined(IMGUI_CONTRIB_DISABLE_KNOB) && \
    !defined(IMGUI_DISABLE_CONTRIB_KNOBS) && \
    !defined(IMGUI_DISABLE_CONTRIB_KNOB) && \
    !defined(IMGUI_DISABLE_EXTRA_KNOBS) && \
    !defined(IMGUI_DISABLE_EXTRA_KNOB)
#ifndef IMGUI_KNOBS_H
#define IMGUI_KNOBS_H
#define IMGUI_EXTRA_KNOBS_H

#include "imgui.h"
#if defined(__has_include)
    #if __has_include("imgui_internal.h")
        #include "imgui_internal.h"
    #endif
    #if __has_include("../extensions/TimedTooltip/imgui_tooltip_ext.h")
        #include "../extensions/TimedTooltip/imgui_tooltip_ext.h"
    #endif
#else
    #include "imgui_internal.h"
#endif

#include <cmath>
#include <algorithm>
#include <cstdio>

// -----------------------------------------------------------------------------
// Configuration Flags
// -----------------------------------------------------------------------------
enum ImGuiKnobFlags_ {
    ImGuiKnobFlags_None             = 0,
    ImGuiKnobFlags_NoInput          = 1 << 0, // Display-only dial / gauge
    ImGuiKnobFlags_NoDetente        = 1 << 1, // Disable two-step detente (clamp smoothly at min)
    ImGuiKnobFlags_ShowTicks        = 1 << 2, // Draw perimeter graduation tick marks
    ImGuiKnobFlags_ValueBelow       = 1 << 3, // Stacked layout: Label top, knob center, readout below
    ImGuiKnobFlags_NoValueText      = 1 << 4, // Suppress formatted value text
    ImGuiKnobFlags_CursorHand       = 1 << 5, // Use Hand cursor instead of default ResizeEW (<-->)
};
typedef int ImGuiKnobFlags;

namespace ImGui {

namespace Detail {

    // Helper for mouse-wheel query with scroll-defeat fallback
    inline float GetKnobMouseWheel(bool consume = true) {
        ImGuiContext& g = *GImGui;
        ImGuiIO& io = g.IO;
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(CONTRIB_DISABLE_GET_MOUSE_WHEEL)
        // Leverage the high-grade scroll-defeat engine in imgui_mouse_wheel_controls.h
        return WheelSliderDetail::GetMouseWheelEither(consume);
#else
        float wheel = io.MouseWheel;
        if (consume && wheel != 0.0f) {
            io.MouseWheel = 0.0f;
        }
        return wheel;
#endif
    }

} // namespace Detail

    // -------------------------------------------------------------------------
    // 1. Core Avionics Rotary Knob / Dimmer Dial
    // -------------------------------------------------------------------------
    // Renders an authentic 270° omega potentiometer with a solid 3D central hub,
    // rotating needle/pip, perimeter graduation ticks, and optional two-step OFF detente.
    inline IMGUI_API bool KnobEx(const char* label,
                                 float* p_value,
                                 float v_min,
                                 float v_max,
                                 float v_default = 1.0f,
                                 const char* format = "%.2f",
                                 float radius = 0.0f,
                                 float wheel_step = 0.0f,
                                 bool has_off_detente = false,
                                 float v_off = 0.0f,
                                 ImGuiKnobFlags flags = ImGuiKnobFlags_None,
                                 bool* out_is_active = nullptr,
                                 bool* out_is_hovered = nullptr,
                                 bool* out_is_scrolling = nullptr)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems || !p_value) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        // Standardize sizing relative to font scale and frame height
        if (radius <= 0.0f) {
            radius = GetFrameHeight() * 0.75f;
        }
        radius = ImMax(8.0f, radius);

        const float kPi = 3.14159265358979323846f;
        const float diameter = radius * 2.0f;

        // Potentiometer arc angles (open deadband at 6 o'clock):
        // Active range sweeps 270 degrees from bottom-left to bottom-right:
        const float kMaxAngle = 2.25f * kPi; // ~405 deg (~4:30 o'clock)
        float kMinAngleActive = 0.75f * kPi; // ~135 deg (~7:30 o'clock)
        float kOffAngle       = 0.62f * kPi; // ~111 deg (~7:00 o'clock, across the detente gap)

        if (has_off_detente) {
            kMinAngleActive = 0.82f * kPi; // ~148 deg (~8:00 o'clock)
            kOffAngle       = 0.65f * kPi; // ~117 deg (~7:00 o'clock)
        }

        const char* text_end = FindRenderedTextEnd(label);
        const bool has_label = (text_end > label);

        // Format value readout text (with automatic 100x scaling for percentage formats)
        char val_buf[64] = "";
        const bool is_in_off_detente = has_off_detente && (*p_value <= v_off);
        if (!(flags & ImGuiKnobFlags_NoValueText)) {
            if (is_in_off_detente) {
                ImFormatString(val_buf, sizeof(val_buf), "OFF");
            } else if (format && format[0]) {
                float display_val = *p_value;
                if (v_max <= 1.0f && v_min >= 0.0f && std::strstr(format, "%%") != nullptr) {
                    display_val *= 100.0f;
                }
                ImFormatString(val_buf, sizeof(val_buf), format, display_val);
            }
        }
        const bool has_val_text = (val_buf[0] != '\0');
        ImVec2 val_size = has_val_text ? CalcTextSize(val_buf, nullptr, true) : ImVec2(0.0f, 0.0f);
        ImVec2 label_size = has_label ? CalcTextSize(label, text_end, true) : ImVec2(0.0f, 0.0f);

        // Compute layout dimensions: Stacked (Avionics Unit) vs. Inline (Side-by-Side)
        const bool is_stacked = (flags & ImGuiKnobFlags_ValueBelow) != 0;
        float total_w = diameter;
        float total_h = diameter;
        ImVec2 pos = window->DC.CursorPos;

        if (is_stacked) {
            total_w = ImMax(diameter, ImMax(val_size.x, label_size.x));
            total_h = diameter;
            if (has_label) total_h += label_size.y + style.ItemInnerSpacing.y;
            if (has_val_text) total_h += val_size.y + style.ItemInnerSpacing.y;
        } else {
            // Inline: [Knob] [Value] [Label]
            float text_area_w = 0.0f;
            if (has_val_text) text_area_w += val_size.x;
            if (has_label) text_area_w += (has_val_text ? style.ItemInnerSpacing.x : 0.0f) + label_size.x;
            if (text_area_w > 0.0f) total_w += style.ItemInnerSpacing.x + text_area_w;
            total_h = ImMax(diameter, ImMax(val_size.y, label_size.y));
        }

        const ImRect bb(pos, ImVec2(pos.x + total_w, pos.y + total_h));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id)) return false;

        // Interaction behavior
        bool hovered = false, held = false;
        bool pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

        // Center coordinates of the circular knob dial
        ImVec2 center;
        if (is_stacked) {
            float cur_y = pos.y;
            if (has_label) cur_y += label_size.y + style.ItemInnerSpacing.y;
            center = ImVec2(pos.x + total_w * 0.5f, cur_y + radius);
        } else {
            center = ImVec2(pos.x + radius, pos.y + total_h * 0.5f);
        }

        // Mouse cursor affordance: Default to ResizeEW (<-->) for range selection
        if (hovered && !(flags & ImGuiKnobFlags_NoInput)) {
            if (flags & ImGuiKnobFlags_CursorHand) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
            } else {
                SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
        }

        // Value Step calculation
        if (wheel_step <= 0.0f) {
            wheel_step = (v_max - v_min) * 0.02f; // Default 2% per scroll notch
            if (wheel_step <= 0.0f) wheel_step = 0.01f;
        }

        bool value_changed = false;
        bool input_interacted = false;
        bool wheel_interacted = false;
        ImGuiStorage* storage = window->DC.StateStorage;
        const ImGuiID storage_key_was_min = id + 0x1000;

        // ---------------------------------------------------------------------
        // Input Handling: Mouse Wheel & Drag
        // ---------------------------------------------------------------------
        if (!(flags & ImGuiKnobFlags_NoInput)) {
            float step_delta = 0.0f;

            // 1. Mouse Wheel
            if (hovered) {
                float wheel = Detail::GetKnobMouseWheel(true);
                if (wheel != 0.0f) {
                    input_interacted = true;
                    wheel_interacted = true;
                    float speed_mult = 1.0f;
                    if (g.IO.KeyShift) speed_mult = 0.25f; // Fine adjustment
                    if (g.IO.KeyCtrl)  speed_mult = 4.0f;  // Coarse adjustment
                    step_delta += wheel * wheel_step * speed_mult;
                }
            }

            // 2. Mouse Drag (Vertical or Horizontal Scrub)
            if (held) {
                float drag_dy = -g.IO.MouseDelta.y; // Standard ImGui drag: up = increase
                if (std::abs(g.IO.MouseDelta.x) > std::abs(g.IO.MouseDelta.y)) {
                    drag_dy = g.IO.MouseDelta.x;   // Right = increase
                }
                if (drag_dy != 0.0f) {
                    input_interacted = true;
                    float speed_mult = 1.0f;
                    if (g.IO.KeyShift) speed_mult = 0.1f; // Shift = fine precision
                    if (g.IO.KeyCtrl)  speed_mult = 2.0f; // Ctrl  = coarse scrub
                    float drag_sens = (v_max - v_min) / 120.0f;
                    step_delta += drag_dy * drag_sens * speed_mult;
                }
            }

            // 3. Double-Click to Reset / Toggle Detente
            if (hovered && IsMouseDoubleClicked(0)) {
                input_interacted = true;
                if (has_off_detente) {
                    if (*p_value <= v_off) {
                        *p_value = v_default; // Snap to calibrated default from OFF
                    } else if (std::abs(*p_value - v_default) < 0.001f) {
                        *p_value = v_off;     // Snap to OFF if already at default
                    } else {
                        *p_value = v_default;
                    }
                } else {
                    *p_value = v_default;
                }
                value_changed = true;
                step_delta = 0.0f;
            }

            // -----------------------------------------------------------------
            // Two-Step Detente State Engine
            // -----------------------------------------------------------------
            if (step_delta != 0.0f) {
                bool was_at_min = storage->GetBool(storage_key_was_min, false);

                if (step_delta > 0.0f) {
                    // Clockwise rotation (Brightening / Incrementing)
                    if (has_off_detente && *p_value <= v_off) {
                        // First upward step clicks directly out of OFF to minimum active!
                        *p_value = v_min;
                        storage->SetBool(storage_key_was_min, true);
                        value_changed = true;
                    } else {
                        float new_val = *p_value + step_delta;
                        if (new_val > v_max) new_val = v_max;
                        if (new_val != *p_value) {
                            *p_value = new_val;
                            value_changed = true;
                        }
                        storage->SetBool(storage_key_was_min, false);
                    }
                } else {
                    // Counter-Clockwise rotation (Dimming / Decrementing)
                    if (has_off_detente) {
                        if (*p_value <= v_off) {
                            // Already OFF, keep in detente
                        } else if (*p_value <= v_min + 0.0001f) {
                            if (was_at_min) {
                                // Deliberate second step past the stop snaps into OFF!
                                *p_value = v_off;
                                storage->SetBool(storage_key_was_min, false);
                                value_changed = true;
                            } else {
                                // First downward click stops firmly at minimum active
                                *p_value = v_min;
                                storage->SetBool(storage_key_was_min, true);
                                value_changed = true;
                            }
                        } else {
                            float new_val = *p_value + step_delta;
                            if (new_val <= v_min) {
                                // Stop at minimum active; do NOT snap directly to OFF
                                new_val = v_min;
                                storage->SetBool(storage_key_was_min, true);
                            } else {
                                storage->SetBool(storage_key_was_min, false);
                            }
                            if (new_val != *p_value) {
                                *p_value = new_val;
                                value_changed = true;
                            }
                        }
                    } else {
                        // Standard clamp at minimum
                        float new_val = *p_value + step_delta;
                        if (new_val < v_min) new_val = v_min;
                        if (new_val != *p_value) {
                            *p_value = new_val;
                            value_changed = true;
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Graphics Rendering (ImDrawList)
        // ---------------------------------------------------------------------
        ImDrawList* draw_list = window->DrawList;
        const float track_thickness = ImMax(2.0f, radius * 0.16f);
        const float track_radius    = radius - track_thickness * 0.5f;
        const float hub_radius      = radius * 0.68f;

        // Compute Needle/Pip Angle
        float current_angle = kMinAngleActive;
        if (has_off_detente && *p_value <= v_off) {
            current_angle = kOffAngle;
        } else {
            float frac = 0.0f;
            if (v_max > v_min) {
                frac = ImClamp((*p_value - v_min) / (v_max - v_min), 0.0f, 1.0f);
            }
            current_angle = kMinAngleActive + frac * (kMaxAngle - kMinAngleActive);
        }

        // 1. Background Arc Track (Full active sweep range)
        ImU32 track_bg_col = GetColorU32(ImGuiCol_FrameBg);
        draw_list->PathClear();
        draw_list->PathArcTo(center, track_radius, kMinAngleActive, kMaxAngle, 32);
        draw_list->PathStroke(track_bg_col, false, track_thickness);

        // 2. Active Value Arc
        if (!is_in_off_detente && current_angle > kMinAngleActive) {
            ImU32 active_arc_col = GetColorU32(hovered ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
            if ((active_arc_col & 0xFF000000) == 0) {
                active_arc_col = GetColorU32(ImGuiCol_CheckMark);
            }
            draw_list->PathClear();
            draw_list->PathArcTo(center, track_radius, kMinAngleActive, current_angle, 32);
            draw_list->PathStroke(active_arc_col, false, track_thickness);
        }

        // 3. Detente Station Markings (OFF Notch & Graduation Ticks)
        if (has_off_detente) {
            ImVec2 off_p1(center.x + std::cos(kOffAngle) * (track_radius - track_thickness * 0.5f),
                          center.y + std::sin(kOffAngle) * (track_radius - track_thickness * 0.5f));
            ImVec2 off_p2(center.x + std::cos(kOffAngle) * (track_radius + track_thickness * 0.5f + 1.5f),
                          center.y + std::sin(kOffAngle) * (track_radius + track_thickness * 0.5f + 1.5f));
            ImU32 off_tick_col = is_in_off_detente ? IM_COL32(230, 126, 34, 255) : GetColorU32(ImGuiCol_TextDisabled);
            draw_list->AddLine(off_p1, off_p2, off_tick_col, 1.5f);
        }

        if (flags & ImGuiKnobFlags_ShowTicks) {
            for (int t = 0; t <= 4; ++t) {
                float tick_frac = t / 4.0f;
                float tick_angle = kMinAngleActive + tick_frac * (kMaxAngle - kMinAngleActive);
                ImVec2 t1(center.x + std::cos(tick_angle) * (track_radius + track_thickness * 0.5f + 1.0f),
                          center.y + std::sin(tick_angle) * (track_radius + track_thickness * 0.5f + 1.0f));
                ImVec2 t2(center.x + std::cos(tick_angle) * (track_radius + track_thickness * 0.5f + 3.5f),
                          center.y + std::sin(tick_angle) * (track_radius + track_thickness * 0.5f + 3.5f));
                draw_list->AddLine(t1, t2, GetColorU32(ImGuiCol_Border), 1.0f);
            }
        }

        // 4. Central Solid 3D Knob Hub / Cap
        ImU32 hub_shadow_col = IM_COL32(0, 0, 0, 120);
        draw_list->AddCircleFilled(ImVec2(center.x + 1.0f, center.y + 1.5f), hub_radius, hub_shadow_col, 32);

        ImU32 hub_body_col = GetColorU32(held ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
        draw_list->AddCircleFilled(center, hub_radius, hub_body_col, 32);

        ImU32 rim_col = GetColorU32(ImGuiCol_Border);
        draw_list->AddCircle(center, hub_radius, rim_col, 32, 1.0f);

        // 5. Index Pointer Pip / Radial Needle Line
        float needle_start = hub_radius * 0.35f;
        float needle_end   = hub_radius * 0.85f;
        ImVec2 np1(center.x + std::cos(current_angle) * needle_start,
                   center.y + std::sin(current_angle) * needle_start);
        ImVec2 np2(center.x + std::cos(current_angle) * needle_end,
                   center.y + std::sin(current_angle) * needle_end);

        ImU32 needle_col = IM_COL32(255, 255, 255, 255);
        if (is_in_off_detente) {
            needle_col = IM_COL32(230, 126, 34, 230);
        } else if (held) {
            needle_col = GetColorU32(ImGuiCol_SliderGrabActive);
        }
        draw_list->AddLine(np1, np2, needle_col, ImMax(1.5f, radius * 0.12f));

        draw_list->AddCircleFilled(center, ImMax(1.5f, hub_radius * 0.20f), needle_col, 16);

        // ---------------------------------------------------------------------
        // Text Rendering: Labels & Formatted Value Readouts
        // ---------------------------------------------------------------------
        if (is_stacked) {
            float cur_y = pos.y;
            if (has_label) {
                float label_x = pos.x + (total_w - label_size.x) * 0.5f;
                RenderText(ImVec2(label_x, cur_y), label, text_end);
                cur_y += label_size.y + style.ItemInnerSpacing.y;
            }
            cur_y += diameter + style.ItemInnerSpacing.y;
            if (has_val_text) {
                float val_x = pos.x + (total_w - val_size.x) * 0.5f;
                ImU32 text_col = is_in_off_detente ? IM_COL32(230, 126, 34, 255) : GetColorU32(ImGuiCol_Text);
                PushStyleColor(ImGuiCol_Text, text_col);
                RenderText(ImVec2(val_x, cur_y), val_buf);
                PopStyleColor();
            }
        } else {
            float text_x = pos.x + diameter + style.ItemInnerSpacing.x;
            float text_y = pos.y + (total_h - ImMax(val_size.y, label_size.y)) * 0.5f;

            if (has_val_text) {
                ImU32 text_col = is_in_off_detente ? IM_COL32(230, 126, 34, 255) : GetColorU32(ImGuiCol_Text);
                PushStyleColor(ImGuiCol_Text, text_col);
                RenderText(ImVec2(text_x, text_y), val_buf);
                PopStyleColor();
                text_x += val_size.x + style.ItemInnerSpacing.x;
            }

            if (has_label) {
                RenderText(ImVec2(text_x, text_y), label, text_end);
            }
        }

        if (out_is_active)    *out_is_active = held;
        if (out_is_hovered)   *out_is_hovered = hovered;
        if (out_is_scrolling) *out_is_scrolling = wheel_interacted;

        // Active interaction suppresses tooltips and isolates hover
        if (input_interacted || held) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }

        return value_changed;
    }

    // -------------------------------------------------------------------------
    // 2. Convenience Wrappers & Avionics Aliases
    // -------------------------------------------------------------------------

    inline IMGUI_API bool Knob(const char* label,
                               float* p_value,
                               float v_min = 0.0f,
                               float v_max = 1.0f,
                               const char* format = "%.2f",
                               float radius = 0.0f,
                               float wheel_step = 0.0f,
                               ImGuiKnobFlags flags = ImGuiKnobFlags_None)
    {
        return KnobEx(label, p_value, v_min, v_max, v_min, format, radius, wheel_step, false, 0.0f, flags);
    }

    inline IMGUI_API bool DimmerKnob(const char* label,
                                     float* p_value,
                                     float v_min_active = 0.05f,
                                     float v_max = 1.0f,
                                     float v_default = 1.0f,
                                     const char* format = "%.0f%%",
                                     float radius = 0.0f,
                                     float wheel_step = 0.02f,
                                     bool has_off_detente = true,
                                     float v_off = 0.0f,
                                     ImGuiKnobFlags flags = ImGuiKnobFlags_None)
    {
        return KnobEx(label, p_value, v_min_active, v_max, v_default, format, radius, wheel_step, has_off_detente, v_off, flags);
    }

    inline IMGUI_API bool RotaryDial(const char* label,
                                     float* p_value,
                                     float v_min = 0.0f,
                                     float v_max = 1.0f,
                                     const char* format = "%.2f",
                                     float radius = 0.0f,
                                     float wheel_step = 0.0f,
                                     ImGuiKnobFlags flags = ImGuiKnobFlags_None)
    {
        return Knob(label, p_value, v_min, v_max, format, radius, wheel_step, flags);
    }

    inline IMGUI_API bool KnobPercent(const char* label,
                                      float* p_value,
                                      float v_default = 1.0f,
                                      float v_min = 0.0f,
                                      float v_max = 1.0f,
                                      const char* format = "%.0f%%",
                                      float radius = 0.0f,
                                      float wheel_step = 0.01f)
    {
        return KnobEx(label, p_value, v_min, v_max, v_default, format, radius, wheel_step, false, 0.0f, ImGuiKnobFlags_None);
    }

    // -------------------------------------------------------------------------
    // 3. Composite Rotary Knob with 1-Click Vector Reset (↺)
    // -------------------------------------------------------------------------
    inline IMGUI_API bool KnobWithReset(const char* label,
                                        float* p_value,
                                        float v_default,
                                        float v_min,
                                        float v_max,
                                        const char* format = "%.2f",
                                        float radius = 0.0f,
                                        float wheel_step = 0.0f,
                                        const char* reset_tooltip = nullptr,
                                        ImGuiKnobFlags flags = ImGuiKnobFlags_None)
    {
        ImGuiWindow* window = GetCurrentWindow();
        ImGuiContext& g = *GImGui;
        ImGuiID group_id = window->GetID(label);

        BeginGroup();
        PushID(label);

        const char* id_pos = std::strstr(label, "##");
        bool has_label = (id_pos != label) && (label[0] != '\0');

        // Capture pre-interaction wheel clicks before KnobEx consumes them:
        ImGuiIO& io = GetIO();
        float pre_wheel = (io.MouseWheel != 0.0f) ? io.MouseWheel : io.MouseWheelH;

        bool knob_active = false;
        bool knob_hovered = false;
        bool is_scrolling = false;

        // Draw knob + formatted value readout, suppressing label inside KnobEx
        // so the reset button [↺] sits immediately between value and label text:
        bool changed = KnobEx("##knob", p_value, v_min, v_max, v_default, format, radius, wheel_step, false, 0.0f, flags,
                              &knob_active, &knob_hovered, &is_scrolling);
        if (!knob_active && (g.ActiveId != 0 && g.ActiveId == window->GetID("##knob"))) {
            knob_active = true;
        }
        if (!is_scrolling && pre_wheel != 0.0f && knob_hovered) {
            is_scrolling = true;
        }

        bool reset_hovered = false;
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(CONTRIB_DISABLE_RESET_BUTTON)
        SameLine(0.0f, GetStyle().ItemInnerSpacing.x);
        bool is_modified = std::abs(*p_value - v_default) > 0.0001f;
        char reset_id[64];
        ImFormatString(reset_id, sizeof(reset_id), "##reset_%s", label);
        if (ResetButton(reset_id, is_modified)) {
            *p_value = v_default;
            changed = true;
        }
        reset_hovered = IsItemHovered();

        char default_buf[160];
        if (reset_tooltip && reset_tooltip[0] != '\0') {
            ImFormatString(default_buf, sizeof(default_buf), "%s", reset_tooltip);
        } else {
            char val_str[64];
            float display_def = v_default;
            if (v_max <= 1.0f && v_min >= 0.0f && std::strstr(format, "%%") != nullptr) {
                display_def *= 100.0f;
            }
            ImFormatString(val_str, sizeof(val_str), format, display_def);
            if (is_modified) {
                ImFormatString(default_buf, sizeof(default_buf), "Click to revert to default (%s).", val_str);
            } else {
                ImFormatString(default_buf, sizeof(default_buf), "Already at default (%s).", val_str);
            }
        }

#if defined(IMGUI_MARKDOWN_EXT_H) && defined(IMGUI_TOOLTIP_EXT_H)
        TimedTooltip::TextMD("%s", default_buf);
#elif defined(IMGUI_TOOLTIP_EXT_H)
        TimedTooltip::Text("%s", default_buf);
#else
        if (reset_hovered) {
            SetTooltip("%s", default_buf);
        }
#endif
#endif

        if (has_label) {
            SameLine(0.0f, GetStyle().ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
        }

        PopID();
        EndGroup();
        g.LastItemData.ID = group_id;

        if (knob_active || is_scrolling || changed) {
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (reset_hovered || knob_active || is_scrolling) {
            ImVec2 save_cur = GetCursorPos();
            SetCursorPos(ImVec2(-10000.0f, -10000.0f));
            Dummy(ImVec2(0.0f, 0.0f));
            SetCursorPos(save_cur);
        }
        return changed;
    }

} // namespace ImGui

#endif // IMGUI_KNOBS_H
#endif // Collision guards
