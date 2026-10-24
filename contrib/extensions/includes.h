// ============================================================================
// ImGui Contrib Extensions: Directory Roll-Up Header (contrib/extensions/includes.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Includes all Dear ImGui extensions in contrib/extensions/ (order-independent:
// ImGui::TimedTooltip::TextMD activates automatically regardless of whether
// Markdown or TimedTooltip is included first).
//
// Opt-Out Configuration:
//   #define CONTRIB_DISABLE_EXTENSIONS      // Exclude all extensions
//   #define CONTRIB_DISABLE_MARKDOWN        // Exclude Markdown extension
//   #define CONTRIB_DISABLE_TIMED_TOOLTIP   // Exclude TimedTooltip extension
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_EXTENSIONS_INCLUDES_H
#define IMGUI_CONTRIB_EXTENSIONS_INCLUDES_H

#if !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_EXTENSIONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_EXTENSIONS)

#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGUI_DISABLE_CONTRIB_TIMED_TOOLTIP) && \
    !defined(IMGUI_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_TIMED_TOOLTIP)
#include "TimedTooltip/imgui_tooltip_ext.h"
#endif

#if !defined(CONTRIB_DISABLE_MARKDOWN) && \
    !defined(IMGUI_CONTRIB_DISABLE_MARKDOWN) && \
    !defined(IMGUI_DISABLE_CONTRIB_MARKDOWN) && \
    !defined(IMGUI_DISABLE_MARKDOWN) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_MARKDOWN)
#include "Markdown/imgui_markdown_ext.h"
#endif

#endif // !CONTRIB_DISABLE_EXTENSIONS

#endif // IMGUI_CONTRIB_EXTENSIONS_INCLUDES_H
