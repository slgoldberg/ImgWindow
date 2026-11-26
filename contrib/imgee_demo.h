// ============================================================================
// ImGee: Opt-In Interactive Showcase Forwarding Header (contrib/imgee_demo.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Optional interactive showcase forwarding header for the ImGee suite:
//   #include "ImgWindow/contrib/imgee_demo.h"
//
// Exposes ImGui::ShowImGeeDemoWindow(bool* p_open = nullptr) and
// ImGui::ShowImGeeDemoContent(const char* str_id = "##ImGeeDemo").
// Omitted from contrib/imgee.h by default (zero production binary overhead)
// unless explicitly included or opted in via #define IMGEE_INCLUDE_DEMO.
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_IMGEE_DEMO_H
#define IMGUI_CONTRIB_IMGEE_DEMO_H

#if defined(__has_include)
  #if !__has_include("ImGee/imgee_demo.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized (or predates ImGee v1.4.0)! Please run: git submodule update --init --recursive"
  #endif
#endif
#include "ImGee/imgee_demo.h"
#endif // IMGUI_CONTRIB_IMGEE_DEMO_H
