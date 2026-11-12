// ============================================================================
// ImGee: Dedicated stb_image Compilation Unit (contrib/ImGee/imgee_stb_image.cpp)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone compilation unit that provides the stb_image implementation
// for host applications that do not already compile or link stb_image in
// their own plugin sources.
//
// If your plugin already compiles stb_image (such as in imgui4xp or FlyWithLua),
// you do NOT need to add this file to your build.
// ============================================================================

#define STB_IMAGE_IMPLEMENTATION

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if defined(__has_include)
    #if __has_include(<stb_image.h>)
        #include <stb_image.h>
    #elif __has_include("stb_image.h")
        #include "stb_image.h"
    #elif __has_include("../stb_image.h")
        #include "../stb_image.h"
    #endif
#else
    #include "../stb_image.h"
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
