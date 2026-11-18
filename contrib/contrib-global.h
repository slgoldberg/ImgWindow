// ============================================================================
// ImGui Contrib: Forwarding Global Namespaces (contrib/contrib-global.h)
// ----------------------------------------------------------------------------
// Forwards to ImGee global namespace definitions in contrib/ImGee/imgee-global.h
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_FORWARD_GLOBAL_H
#define IMGUI_CONTRIB_FORWARD_GLOBAL_H

#if defined(__has_include)
  #if !__has_include("ImGee/imgee-global.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "ImGee/imgee-global.h"
  #endif
#else
  #include "ImGee/imgee-global.h"
#endif
#endif // IMGUI_CONTRIB_FORWARD_GLOBAL_H
