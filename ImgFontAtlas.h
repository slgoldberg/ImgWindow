/*
 * ImgFontAtlas.h
 *
 * Integration for dear imgui into X-Plane: ImGui Font Atlas
 *
 * Copyright (C) 2020, Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef IMGFONTATLAS_H
#define IMGFONTATLAS_H

#include "SystemGL.h"
#include <imgui.h>

/* ImGui version checks and refactor macros.
 * IMGUI_V190_REFACTOR defined for ImGui v1.90.0 and above (keyboard API refactor).
 * IMGUI_V192_REFACTOR defined for ImGui v1.92.0 and above (font atlas refactor).
 */
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19000)
#define IMGUI_V190_REFACTOR

#if IMGUI_VERSION_NUM >= 19200
#define IMGUI_V192_REFACTOR
#endif

#endif
/* End ImGui version checks and refactor macros. */

/** Construct an empty font atlas we can use later
 *
 * This also assigns the texture name which is necessary as, again, must be done
 * in an x-plane compatible manner.
 *
 */
class ImgFontAtlas {
public:
    ImgFontAtlas();

    virtual ~ImgFontAtlas();

    ImFont *AddFont(const ImFontConfig *font_cfg);

    ImFont *AddFontDefault(const ImFontConfig *font_cfg = NULL);

    ImFont *AddFontFromFileTTF(const char *filename,
                               float size_pixels,
                               const ImFontConfig *font_cfg = NULL,
                               const unsigned short *glyph_ranges = NULL);

    ImFont *AddFontFromMemoryTTF(void *font_data,
                                 int font_size,
                                 float size_pixels,
                                 const ImFontConfig *font_cfg = NULL,
                                 const unsigned short *glyph_ranges = NULL); // Note: Transfer ownership of 'ttf_data' to ImFontAtlas! Will be deleted after destruction of the atlas. Set font_cfg->FontDataOwnedByAtlas=false to keep ownership of your data and it won't be freed.
    ImFont *AddFontFromMemoryCompressedTTF(const void *compressed_font_data,
                                           int compressed_font_size,
                                           float size_pixels,
                                           const ImFontConfig *font_cfg = NULL,
                                           const unsigned short *glyph_ranges = NULL); // 'compressed_font_data' still owned by caller. Compress with binary_to_compressed_c.cpp.
    ImFont *AddFontFromMemoryCompressedBase85TTF(const char *compressed_font_data_base85,
                                                 float size_pixels,
                                                 const ImFontConfig *font_cfg = NULL,
                                                 const unsigned short *glyph_ranges = NULL);              // 'compressed_font_data_base85' still owned by caller. Compress with binary_to_compressed_c.cpp with -base85 parameter.

    //bindTexture creates and binds the font texture to OpenGL, ready for use.
    //This should be called after all fonts are loaded, before any rendering occurs!
    virtual void bindTexture();

    ImFontAtlas *getAtlas();

#ifdef IMGUI_V192_REFACTOR
    struct strct_texture_info {
        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        int bytesPerPixel = 4; // RGBA32
    };

    // Custom replacement function for extracting the font atlas pixel data in v1.92+
    static bool GetCustomAtlasTextureData(ImFontAtlas* atlas, strct_texture_info& outInfo);

    // Keep native trackers updated during runtime re-bakes.
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    void updateTextureTracking(void* textureID);
#else
    void updateTextureTracking(int textureID);
#endif
#endif /* IMGUI_V192_REFACTOR */

protected:
    ImFontAtlas *mOurAtlas;
    bool        mTextureBound;
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    void*       mTextureRef;
#else
    int         mGLTextureNum;
#endif
};

/** Define the structures we need for our panel graphics "bridge" support.
 *  This allows us to support dynamic binding to the panel graphics library
 *  if it's available, and to fall back to the standard OpenGL rendering if
 *  not -- but only if the user has defined IMGWINDOW_USE_PANEL_GRAPHICS.
 */
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)

#if !defined(XPLM440)

#include <stdint.h>
#include <XPLMDisplay.h> // Localized dependency for the spoofed window structs

// Define required types if NOT compiling against SDK v4.4!
// Note that these definitions below come directly from the XPLM v4.4 SDK
// header files, and are only used if the user has defined
// IMGWINDOW_USE_PANEL_GRAPHICS but is compiling against an older SDK, so
// that the code can still compile and link against the older SDK, and
// dynamically bind to the panel graphics library if it's available at
// runtime.

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    xplm_WindowContentTypeOpenGL             = 0,
    xplm_WindowContentTypePanelGraphics      = 1,
    xplm_WindowContentTypeBrowser            = 2
} XPLMWindowContentType;

typedef struct {
     void *                    tex_ref;
     float                     scissors[4];
     int                       idx_offset;
     int                       element_count;
     int                       vtx_offset;
} XPLMDrawCall_t;

typedef struct {
     float                     x;
     float                     y;
     float                     s;
     float                     t;
} XPLMTextureVertex_t;

typedef struct {
     int                       vertex_count;
     const float *             vertices;
     int                       index_count;
     const uint16_t*           indices;
} XPLMMesh_t;

// Spoofed struct to allow creation of Panel Graphics windows
// on older SDKs that don't have the new fields.
// This precisely mirrors the layout of XPLMCreateWindow_t in SDK 4.40 on 64-bit systems.
struct SpoofedXPLMCreateWindow_t_440 {
    int                       structSize;
    int                       left;
    int                       top;
    int                       right;
    int                       bottom;
    int                       visible;
    XPLMDrawWindow_f          drawWindowFunc;
    XPLMHandleMouseClick_f    handleMouseClickFunc;
    XPLMHandleKey_f           handleKeyFunc;
    XPLMHandleCursor_f        handleCursorFunc;
    XPLMHandleMouseWheel_f    handleMouseWheelFunc;
    void*                     refcon;
    XPLMWindowDecoration      decorateAsFloatingWindow;
    XPLMWindowLayer           layer;
    XPLMHandleMouseClick_f    handleRightClickFunc;
    union {
        XPLMWindowContentType     windowContentType; // Old name (SDK 12.4.0d4)
        XPLMWindowContentType     contentType;       // New name (SDK 12.4.0b1)
    };
    void*                     browserLoadFinishedFunc;
    void*                     browserLoadErrorFunc;
};

#ifdef __cplusplus
}
#endif

#else // XPLM440 is defined
#include <XPLMPanelGraphics.h>
#endif // !defined(XPLM440)

namespace ImgPanelGraphics {
    // True if runtime supports Panel Graphics
    bool IsAvailable();

    // Dynamically loaded Panel Graphics API wrappers
    void* CreateTexture(const unsigned char* rgba_image, int width, int height);
    void DestroyTexture(void* tex_ref);
    void DrawCalls(const XPLMMesh_t* inMesh, int inCount, const XPLMDrawCall_t inDrawCalls[]);
}

#endif // IMGWINDOW_USE_PANEL_GRAPHICS

#endif //IMGFONTATLAS_H
