// ============================================================================
// ImGui Contrib Extensions: Forwarding Header (contrib/extensions.h)
// ----------------------------------------------------------------------------
// Forwards to ImGee extensions suite in contrib/ImGee/extensions/includes.h
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_EXTENSIONS_H
#define IMGUI_CONTRIB_EXTENSIONS_H

#if defined(__has_include)
  #if !__has_include("ImGee/extensions/includes.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "ImGee/extensions/includes.h"
  #endif
#else
  #include "ImGee/extensions/includes.h"
#endif
#endif // IMGUI_CONTRIB_EXTENSIONS_H
