// ============================================================================
// ImGui Contrib Widgets: Concentrator Header (contrib/widgets.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Shorthand concentrator header that includes all contributed Dear ImGui
// widgets in contrib/widgets/ unless disabled via CONTRIB_DISABLE_WIDGETS.
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_WIDGETS_H
#define IMGUI_CONTRIB_WIDGETS_H

#if !defined(CONTRIB_DISABLE_WIDGETS) && \
    !defined(IMGUI_CONTRIB_DISABLE_WIDGETS) && \
    !defined(IMGUI_DISABLE_CONTRIB_WIDGETS) && \
    !defined(IMGUI_DISABLE_WIDGETS) && \
    !defined(IMGUI_DISABLE_EXTRA_WIDGETS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_WIDGETS)
#include "widgets/includes.h"
#endif

#endif // IMGUI_CONTRIB_WIDGETS_H
