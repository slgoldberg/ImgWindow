// ============================================================================
// ImGui Contrib Widgets: Forwarding Header (contrib/widgets.h)
// ----------------------------------------------------------------------------
// Forwards to ImGee widgets suite in contrib/ImGee/widgets/includes.h
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_WIDGETS_H
#define IMGUI_CONTRIB_WIDGETS_H

#if defined(__has_include)
  #if !__has_include("ImGee/widgets/includes.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #endif
#endif
#include "ImGee/widgets/includes.h"
#endif // IMGUI_CONTRIB_WIDGETS_H
