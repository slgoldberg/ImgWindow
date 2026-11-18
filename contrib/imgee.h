// ============================================================================
// ImGee: Package Forwarding Header (contrib/imgee.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Primary package forwarding header for the ImGee suite:
//   #include "ImgWindow/contrib/imgee.h"
//
// Forwards directly to the canonical ImGee master suite header:
//   #include "ImGee/imgee.h"
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_IMGEE_H
#define IMGUI_CONTRIB_IMGEE_H

#if defined(__has_include)
  #if !__has_include("ImGee/imgee.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #endif
#endif
#include "ImGee/imgee.h"
#endif // IMGUI_CONTRIB_IMGEE_H
