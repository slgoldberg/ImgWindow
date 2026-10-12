// ============================================================================
// ImGui Contrib: Master Directory Roll-Up Header (contrib/includes.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Master roll-up header for all Dear ImGui extensions and immediate-mode
// widgets bundled in the contrib/ directory.
//
// Why use this header (or top-level "contrib.h")?
// 1. Guaranteed Dependency Order: Automatically includes TimedTooltip BEFORE
//    Markdown and Widgets so cross-extension bridges (such as
//    ImGui::TimedTooltip::TextMD and TimedTooltips on SliderWithReset / ComboWithReset)
//    activate seamlessly with zero manual header juggling.
// 2. Easy Customization & Opt-Out: Uncomment any of the #define toggles below
//    (or define them before including this file) to selectively exclude
//    individual widgets, helpers, or entire subsystems that might collide with
//    your existing codebase.
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_INCLUDES_H
#define IMGUI_CONTRIB_INCLUDES_H

#if !defined(CONTRIB_DISABLE_ALL) && \
    !defined(DISABLE_CONTRIB) && \
    !defined(IMGUI_DISABLE_CONTRIB) && \
    !defined(IMGUI_CONTRIB_DISABLE_ALL) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB)

// ============================================================================
// OPT-OUT CONFIGURATION (Uncomment any line below, or #define before including)
// ============================================================================

// --- Top-Level Subsystems ---
// #define CONTRIB_DISABLE_EXTENSIONS             // Exclude all extensions (contrib/extensions/)
// #define CONTRIB_DISABLE_WIDGETS                // Exclude all extra widgets (contrib/widgets/)

// --- Individual Extensions (contrib/extensions/) ---
// #define CONTRIB_DISABLE_TIMED_TOOLTIP          // Exclude TimedTooltip extension
// #define CONTRIB_DISABLE_MARKDOWN               // Exclude Markdown extension

// --- Widget Collections (contrib/widgets/) ---
// #define CONTRIB_DISABLE_TOGGLE_BUTTONS         // Exclude ToggleButton & ToggleButtons
// #define CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL     // Exclude mouse-wheel support on ToggleButton(s)
// #define CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS   // Exclude all wheel sliders, drags, cyclers & combos
// #define CONTRIB_DISABLE_CLICKABLE_LINKS        // Exclude ButtonLink, TextLink & AttachItemCursor

// --- Fine-Grained Mouse-Wheel Control Opt-Outs & Style Defaults ---
// #define CONTRIB_DISABLE_GET_MOUSE_WHEEL        // Exclude GetMouseWheel* primitives
// #define CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL     // Exclude AdjustItemOnMouseWheel()
// #define CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL      // Exclude CycleItemOnMouseWheel()
// #define CONTRIB_DISABLE_PERCENT_SLIDER         // Exclude PercentSlider() & SliderPercent()
// #define CONTRIB_DISABLE_SLIDER_PERCENT         // Exclude only the SliderPercent() alias
// #define CONTRIB_DISABLE_MOUSE_WHEEL_COMBO      // Exclude MouseWheelCombo & ComboWithReset
// #define IMGUI_CONTRIB_DISABLE_RESET_UNDO       // Disable 1-click Reset-with-Undo (restore previous value)
// #define IMGUI_CONTRIB_DEFAULT_RESET_LABEL      ICON_FA_UNDO  // Custom reset button icon (default: vector ↺)
// #define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT  ICON_FA_REDO  // Custom restore/redo icon (default: vector ↻)
// #define IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE ImGui::ComboArrowStyle_RightWhenClosed // Combo chevron style

// (Note: IMGUI_CONTRIB_DISABLE_*, IMGUI_DISABLE_CONTRIB_*, and legacy
//  IMGUI_DISABLE_EXTRA_* macro names are also honored automatically.)

// ============================================================================
// 1. EXTENSIONS (Included first to satisfy downstream cross-feature bridges)
// ============================================================================

#include "extensions.h"

// ============================================================================
// 2. WIDGETS (Automatically bridges with TimedTooltip if enabled above)
// ============================================================================

#include "widgets.h"

#endif // !CONTRIB_DISABLE_ALL

#endif // IMGUI_CONTRIB_INCLUDES_H
