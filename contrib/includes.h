// ============================================================================
// ImGui Contrib: Master Directory Roll-Up Header (contrib/includes.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Master package orchestrator for the contrib/ directory. Cascades inclusions
// to active contributed packages unless disabled via CONTRIB_DISABLE_ALL.
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_INCLUDES_H
#define IMGUI_CONTRIB_INCLUDES_H

#if !defined(CONTRIB_DISABLE_ALL) && \
    !defined(DISABLE_CONTRIB) && \
    !defined(IMGUI_DISABLE_CONTRIB) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB)

// Package 1: ImGee (Avionics & Flight-Sim UI Toolkit)
#if !defined(CONTRIB_IMGEE_DISABLE_ALL) && !defined(IMGEE_DISABLE_ALL)
#include "ImGee/imgee.h"
#endif

// Placeholder: Future packages will be auto-included here as subdirectories are added!
// #if !defined(CONTRIB_NEXTPACKAGE_DISABLE_ALL)
// #include "NextPackage/includes.h"
// #endif

#endif // !CONTRIB_DISABLE_ALL

#endif // IMGUI_CONTRIB_INCLUDES_H
