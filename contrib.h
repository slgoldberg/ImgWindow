// ============================================================================
// ImGui Contrib: Top-Level Concentrator Header (contrib.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Single-include front door in the top-level ImgWindow directory that pulls in
// all contributed Dear ImGui extensions and widgets from contrib/includes.h,
// unless disabled via #define CONTRIB_DISABLE_ALL (or IMGUI_DISABLE_CONTRIB).
//
// Usage:
//   #include "third-party/ImgWindow/contrib.h"
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_H
#define IMGUI_CONTRIB_H

#if !defined(CONTRIB_DISABLE_ALL) && \
    !defined(DISABLE_CONTRIB) && \
    !defined(IMGUI_DISABLE_CONTRIB) && \
    !defined(IMGUI_CONTRIB_DISABLE_ALL) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB)
#include "contrib/includes.h"
#endif

#endif // IMGUI_CONTRIB_H
