#ifndef IMGPANELGRAPHICSBRIDGE_H
#define IMGPANELGRAPHICSBRIDGE_H

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)

// Define required types if not compiling against SDK 4.40
#if !defined(XPLM440)

#include <stdint.h>
#include <XPLMDisplay.h> // for existing types

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
    XPLMWindowContentType     windowContentType;
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

#endif // IMGPANELGRAPHICSBRIDGE_H

