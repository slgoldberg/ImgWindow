#include "ImgPanelGraphicsBridge.h"

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)

#include <XPLMUtilities.h>

namespace ImgPanelGraphics {

    // Function pointers
    static void* (*s_CreateTexture)(const unsigned char*, int, int) = nullptr;
    static void (*s_DestroyTexture)(void*) = nullptr;
    static void (*s_DrawCalls)(const XPLMMesh_t*, int, const XPLMDrawCall_t*) = nullptr;

    static bool s_initialized = false;
    static bool s_available = false;

    static void InitDynamic() {
        if (s_initialized) return;
        s_initialized = true;

        s_CreateTexture = (void* (*)(const unsigned char*, int, int)) XPLMFindSymbol("XPLMCreateTexture");
        s_DestroyTexture = (void (*)(void*)) XPLMFindSymbol("XPLMDestroyTexture");
        s_DrawCalls = (void (*)(const XPLMMesh_t*, int, const XPLMDrawCall_t*)) XPLMFindSymbol("XPLMDrawCalls");

        if (s_CreateTexture && s_DestroyTexture && s_DrawCalls) {
            s_available = true;
        }
    }

    bool IsAvailable() {
        InitDynamic();
        return s_available;
    }

    void* CreateTexture(const unsigned char* rgba_image, int width, int height) {
        if (s_CreateTexture) return s_CreateTexture(rgba_image, width, height);
        return nullptr;
    }

    void DestroyTexture(void* tex_ref) {
        if (s_DestroyTexture) s_DestroyTexture(tex_ref);
    }

    void DrawCalls(const XPLMMesh_t* inMesh, int inCount, const XPLMDrawCall_t inDrawCalls[]) {
        if (s_DrawCalls) s_DrawCalls(inMesh, inCount, inDrawCalls);
    }
}

#endif // IMGWINDOW_USE_PANEL_GRAPHICS

