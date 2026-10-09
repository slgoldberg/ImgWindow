// ============================================================================
// ImgWindow Contrib: Extra Widgets Roll-Up Header
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Master convenience header that includes all lightweight, immediate-mode
// widgets in contrib/widgets/.
//
// Opt-Out Configuration:
// By default, including this header makes all extra widgets available in the
// root ImGui:: namespace with zero friction.
// If any widget collides with existing identifiers in your project, define any
// of the following macros BEFORE including this header to disable that widget:
//
//   #define IMGUI_DISABLE_EXTRA_TOGGLE_BUTTON
//   #define IMGUI_DISABLE_EXTRA_WHEEL_SLIDER
//   #define IMGUI_DISABLE_EXTRA_CLICKABLE_LINK
//
// Or include individual widget headers directly:
//   #include "contrib/widgets/toggle_button.h"
//   #include "contrib/widgets/wheel_slider.h"
//   #include "contrib/widgets/clickable_link.h"
// ============================================================================

#pragma once

#ifndef IMGUI_EXTRA_WIDGETS_H
#define IMGUI_EXTRA_WIDGETS_H

#include "toggle_button.h"
#include "wheel_slider.h"
#include "clickable_link.h"

#endif // IMGUI_EXTRA_WIDGETS_H
