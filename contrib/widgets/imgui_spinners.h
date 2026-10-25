// ============================================================================
// ImGui Contrib Widgets: Procedural Spinners & Countdown Dials (imgui_spinners.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone C++ immediate-mode activity spinners and radial countdown timer
// dials for Dear ImGui. Procedurally rendered via ImDrawList antialiased paths
// with zero image textures, zero font dependencies, and smooth frame-rate
// independent animations.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_SPINNERS) && \
    !defined(CONTRIB_DISABLE_SPINNER) && \
    !defined(IMGUI_CONTRIB_DISABLE_SPINNERS) && \
    !defined(IMGUI_CONTRIB_DISABLE_SPINNER) && \
    !defined(IMGUI_DISABLE_CONTRIB_SPINNERS) && \
    !defined(IMGUI_DISABLE_CONTRIB_SPINNER) && \
    !defined(IMGUI_DISABLE_EXTRA_SPINNERS) && \
    !defined(IMGUI_DISABLE_EXTRA_SPINNER)
#ifndef IMGUI_SPINNERS_H
#define IMGUI_SPINNERS_H
#define IMGUI_EXTRA_SPINNERS_H

#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>
#include <algorithm>

namespace ImGui {

    // -------------------------------------------------------------------------
    // 1. Procedural Activity Spinner
    // -------------------------------------------------------------------------
    // Renders an organic, smooth rotating activity arc for loading or busy states.
    // Animates automatically using ImGui::GetTime() * speed.
    inline IMGUI_API bool Spinner(const char* label, float radius = 0.0f, float thickness = -1.0f, ImU32 color = 0, float speed = 1.0f) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        if (radius <= 0.0f) {
            radius = GetFontSize() * 0.55f;
        }
        if (thickness <= 0.0f) {
            thickness = ImMax(2.0f, radius * 0.22f);
        }
        if (color == 0) {
            color = GetColorU32(ImGuiCol_CheckMark);
            if ((color & 0xFF000000) == 0) {
                color = GetColorU32(ImGuiCol_ButtonActive);
            }
        }

        const char* text_end = FindRenderedTextEnd(label);
        const bool has_label = (text_end > label);
        ImVec2 text_size = has_label ? CalcTextSize(label, text_end, true) : ImVec2(0.0f, 0.0f);

        ImVec2 pos = window->DC.CursorPos;
        float total_width = radius * 2.0f;
        if (has_label) {
            total_width += style.ItemInnerSpacing.x + text_size.x;
        }
        float total_height = ImMax(radius * 2.0f, text_size.y);

        const ImRect bb(pos, ImVec2(pos.x + total_width, pos.y + total_height));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id)) return false;
        ButtonBehavior(bb, id, nullptr, nullptr);

        // Animate arc using sinusoidal expansion and contraction
        float t = (float)GetTime() * speed;
        float start_angle = t * 3.5f;
        float min_arc = 3.14159265f * 0.15f;
        float max_arc = 3.14159265f * 1.50f;
        float arc = min_arc + (max_arc - min_arc) * (0.5f + 0.5f * std::sin(t * 2.0f));

        ImVec2 center(pos.x + radius, pos.y + total_height * 0.5f);
        float track_radius = radius - thickness * 0.5f;

        // Background track (subtle translucent ring):
        ImU32 track_col = (color & 0x00FFFFFF) | 0x28000000;
        window->DrawList->AddCircle(center, track_radius, track_col, 32, thickness);

        // Animated foreground arc:
        window->DrawList->PathClear();
        window->DrawList->PathArcTo(center, track_radius, start_angle, start_angle + arc, 32);
        window->DrawList->PathStroke(color, false, thickness);

        // Optional text label beside spinner:
        if (has_label) {
            ImVec2 text_pos(pos.x + radius * 2.0f + style.ItemInnerSpacing.x, pos.y + (total_height - text_size.y) * 0.5f);
            RenderText(text_pos, label, text_end);
        }

        return true;
    }

    inline IMGUI_API bool Spinner(const char* label, float radius, float thickness, const ImVec4& color, float speed = 1.0f) {
        return Spinner(label, radius, thickness, ColorConvertFloat4ToU32(color), speed);
    }

    // -------------------------------------------------------------------------
    // 2. Radial Countdown Dial
    // -------------------------------------------------------------------------
    // Renders a circular countdown dial representing fraction_remaining (0.0f .. 1.0f).
    // Begins at 12 o'clock (-90 deg) and sweeps clockwise (or counter-clockwise).
    inline IMGUI_API bool RadialTimer(const char* label, float fraction_remaining, float radius = 0.0f, float thickness = -1.0f,
                                      ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        if (radius <= 0.0f) {
            radius = GetFontSize() * 0.55f;
        }
        if (thickness <= 0.0f) {
            thickness = ImMax(2.0f, radius * 0.22f);
        }
        if (color == 0) {
            color = GetColorU32(ImGuiCol_PlotHistogram);
            if ((color & 0xFF000000) == 0) {
                color = GetColorU32(ImGuiCol_CheckMark);
            }
        }

        const char* text_end = FindRenderedTextEnd(label);
        const bool has_label = (text_end > label);
        ImVec2 text_size = has_label ? CalcTextSize(label, text_end, true) : ImVec2(0.0f, 0.0f);

        ImVec2 pos = window->DC.CursorPos;
        float total_width = radius * 2.0f;
        if (has_label) {
            total_width += style.ItemInnerSpacing.x + text_size.x;
        }
        float total_height = ImMax(radius * 2.0f, text_size.y);

        const ImRect bb(pos, ImVec2(pos.x + total_width, pos.y + total_height));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id)) return false;
        ButtonBehavior(bb, id, nullptr, nullptr);

        ImVec2 center(pos.x + radius, pos.y + total_height * 0.5f);
        float track_radius = radius - thickness * 0.5f;

        // Background track:
        ImU32 track_col = (bg_color != 0) ? bg_color : ((color & 0x00FFFFFF) | 0x33000000);
        window->DrawList->AddCircle(center, track_radius, track_col, 32, thickness);

        // Progress arc starting at 12 o'clock (-PI / 2):
        float frac = ImClamp(fraction_remaining, 0.0f, 1.0f);
        if (frac > 0.0f) {
            const float kPi = 3.14159265358979323846f;
            float start_angle = -kPi * 0.5f;
            float sweep = frac * 2.0f * kPi;
            float end_angle = clockwise ? (start_angle + sweep) : (start_angle - sweep);

            window->DrawList->PathClear();
            window->DrawList->PathArcTo(center, track_radius, start_angle, end_angle, 32);
            window->DrawList->PathStroke(color, false, thickness);
        }

        // Optional text label beside timer dial:
        if (has_label) {
            ImVec2 text_pos(pos.x + radius * 2.0f + style.ItemInnerSpacing.x, pos.y + (total_height - text_size.y) * 0.5f);
            RenderText(text_pos, label, text_end);
        }

        return true;
    }

    inline IMGUI_API bool RadialTimer(const char* label, float fraction_remaining, float radius, float thickness,
                                      const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        ImU32 c = ColorConvertFloat4ToU32(color);
        ImU32 bg = bg_color ? ColorConvertFloat4ToU32(*bg_color) : 0;
        return RadialTimer(label, fraction_remaining, radius, thickness, c, bg, clockwise);
    }

    // -------------------------------------------------------------------------
    // 3. Countdown Timer (Seconds Helper)
    // -------------------------------------------------------------------------
    // Convenience wrapper accepting current remaining seconds and total max seconds.
    inline IMGUI_API bool CountdownTimer(const char* label, float current_sec, float max_sec, float radius = 0.0f, float thickness = -1.0f,
                                         ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        float frac = (max_sec > 0.0f) ? (current_sec / max_sec) : 0.0f;
        return RadialTimer(label, frac, radius, thickness, color, bg_color, clockwise);
    }

    inline IMGUI_API bool CountdownTimer(const char* label, float current_sec, float max_sec, float radius, float thickness,
                                         const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        float frac = (max_sec > 0.0f) ? (current_sec / max_sec) : 0.0f;
        return RadialTimer(label, frac, radius, thickness, color, bg_color, clockwise);
    }

    // Friendly C++ aliases:
    inline IMGUI_API bool ActivitySpinner(const char* label, float radius = 0.0f, float thickness = -1.0f, ImU32 color = 0, float speed = 1.0f) {
        return Spinner(label, radius, thickness, color, speed);
    }
    inline IMGUI_API bool ActivitySpinner(const char* label, float radius, float thickness, const ImVec4& color, float speed = 1.0f) {
        return Spinner(label, radius, thickness, color, speed);
    }
    inline IMGUI_API bool ProgressDial(const char* label, float fraction_remaining, float radius = 0.0f, float thickness = -1.0f,
                                      ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        return RadialTimer(label, fraction_remaining, radius, thickness, color, bg_color, clockwise);
    }
    inline IMGUI_API bool ProgressDial(const char* label, float fraction_remaining, float radius, float thickness,
                                      const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        return RadialTimer(label, fraction_remaining, radius, thickness, color, bg_color, clockwise);
    }
    inline IMGUI_API bool CountdownDial(const char* label, float current_sec, float max_sec, float radius = 0.0f, float thickness = -1.0f,
                                        ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        return CountdownTimer(label, current_sec, max_sec, radius, thickness, color, bg_color, clockwise);
    }
    inline IMGUI_API bool CountdownDial(const char* label, float current_sec, float max_sec, float radius, float thickness,
                                        const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        return CountdownTimer(label, current_sec, max_sec, radius, thickness, color, bg_color, clockwise);
    }

} // namespace ImGui

#endif // IMGUI_SPINNERS_H
#endif // Collision guards
