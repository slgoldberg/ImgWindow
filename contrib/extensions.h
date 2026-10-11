// ============================================================================
// ImGui Contrib Extensions: Concentrator Header (contrib/extensions.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Shorthand concentrator header that includes all contributed Dear ImGui
// extensions in contrib/extensions/ unless disabled via
// CONTRIB_DISABLE_EXTENSIONS.
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_EXTENSIONS_H
#define IMGUI_CONTRIB_EXTENSIONS_H

#if !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_EXTENSIONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_EXTENSIONS)
#include "extensions/includes.h"
#endif

#endif // IMGUI_CONTRIB_EXTENSIONS_H
