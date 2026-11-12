// ============================================================================
// ImGee: Optional Dedicated stb_image TU (contrib/ImGee/imgee_stb_image.cpp)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// ImGee works 100% header-only out of the box with zero CMake changes.
//
// Optional Compile-Speed Optimization for Large Multi-TU Plugins:
// If your plugin includes "contrib.h" across dozens of .cpp files and you wish
// to compile ImGee's isolated image decoder only once instead of per-TU:
//   1. Define -DIMGEE_EXTERNAL_STB_IMAGE in your target compile definitions.
//   2. Add contrib/ImGee/imgee_stb_image.cpp to your target sources.
// (Safe to use even if your plugin also compiles its own <stb_image.h>.)
// ============================================================================

#ifndef IMGEE_EXTERNAL_STB_IMAGE
#define IMGEE_EXTERNAL_STB_IMAGE
#endif
#define IMGEE_STB_IMAGE_TU

#include "imgee_stbi_internal.h"
