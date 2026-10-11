// ============================================================================
// ImGui Contrib Widgets: Directory Roll-Up Header (contrib/widgets/includes.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Includes all immediate-mode Dear ImGui widgets in contrib/widgets/.
//
// Opt-Out Configuration:
// By default, including this header makes all contributed widgets available in
// the root ImGui:: namespace with zero friction.
// If any widget collides with existing identifiers in your project, define any
// of the following canonical macros BEFORE including this header:
//
//   #define CONTRIB_DISABLE_TOGGLE_BUTTONS        // Exclude ToggleButton & ToggleButtons
//   #define CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL    // Exclude mouse-wheel on ToggleButton(s)
//   #define CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS  // Exclude all wheel sliders, drags, cyclers & combos
//   #define CONTRIB_DISABLE_CLICKABLE_LINKS       // Exclude ButtonLink, TextLink & AttachItemCursor
//
// Fine-grained opt-outs within imgui_mouse_wheel_controls.h:
//   #define CONTRIB_DISABLE_GET_MOUSE_WHEEL       // Exclude GetMouseWheel* primitives
//   #define CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL    // Exclude AdjustItemOnMouseWheel()
//   #define CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL     // Exclude CycleItemOnMouseWheel()
//   #define CONTRIB_DISABLE_PERCENT_SLIDER        // Exclude PercentSlider() & SliderPercent()
//   #define CONTRIB_DISABLE_SLIDER_PERCENT        // Exclude only the SliderPercent() alias
//   #define CONTRIB_DISABLE_MOUSE_WHEEL_COMBO     // Exclude MouseWheelCombo & ResetCombo
//
// (Note: IMGUI_CONTRIB_DISABLE_*, IMGUI_DISABLE_CONTRIB_*, and legacy
//  IMGUI_DISABLE_EXTRA_* macro names are also honored automatically.)
//
// Or include individual widget headers directly:
//   #include "contrib/widgets/imgui_toggle_buttons.h"
//   #include "contrib/widgets/imgui_mouse_wheel_controls.h"
//   #include "contrib/widgets/imgui_clickable_links.h"
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_WIDGETS_INCLUDES_H
#define IMGUI_CONTRIB_WIDGETS_INCLUDES_H

#if !defined(CONTRIB_DISABLE_WIDGETS) && \
    !defined(IMGUI_CONTRIB_DISABLE_WIDGETS) && \
    !defined(IMGUI_DISABLE_CONTRIB_WIDGETS) && \
    !defined(IMGUI_DISABLE_WIDGETS) && \
    !defined(IMGUI_DISABLE_EXTRA_WIDGETS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_WIDGETS)

#include "imgui_toggle_buttons.h"
#include "imgui_mouse_wheel_controls.h"
#include "imgui_clickable_links.h"

#endif // !CONTRIB_DISABLE_WIDGETS

#endif // IMGUI_CONTRIB_WIDGETS_INCLUDES_H
