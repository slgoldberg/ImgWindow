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
// 2. ImGui::SliderFloatWithWheel & ImGui::SliderIntWithWheel: Sliders with
//    built-in mouse wheel responsiveness.
// 3. ImGui::SliderPercent: Percentage slider (0.0 to 1.0 displayed as 0% to 100%)
//    with automatic scroll wheel support.
// ============================================================================

#pragma once

#ifndef IMGUI_DISABLE_EXTRA_WHEEL_SLIDER
#ifndef IMGUI_EXTRA_WHEEL_SLIDER_H
#define IMGUI_EXTRA_WHEEL_SLIDER_H

#include "imgui.h"
#include <cmath>
#include <algorithm>

namespace ImGui {

    // -------------------------------------------------------------------------
    // 1. AdjustOnItemMouseWheel Helper
    // -------------------------------------------------------------------------
    // Attaches to the preceding widget via IsItemHovered().
    // Inspects mouse-wheel delta (vertical or horizontal), applies scaled step
    // increments/decrements with boundary clamping, and shows the East-West
    // resize cursor (ImGuiMouseCursor_ResizeEW) to signal mouse-wheel capability.
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
            SetMouseCursor(ImGuiMouseCursor_ResizeEW);
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
            SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        return isHandled;
    }

    // -------------------------------------------------------------------------
    // 2. Integrated Wheel Sliders
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
    // 3. SliderPercent (0.0 .. 1.0 displayed as 0% .. 100%)
    // -------------------------------------------------------------------------
    // Expects normalized values where 1.0f = 100%. Displays formatted percent text
    // and integrates seamless mouse wheel scrolling.
    inline bool SliderPercent(const char* label,
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

} // namespace ImGui

#endif // IMGUI_EXTRA_WHEEL_SLIDER_H
#endif // IMGUI_DISABLE_EXTRA_WHEEL_SLIDER
