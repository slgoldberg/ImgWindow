// ---------------------------------------------------------------------------
//  TeardownImGui.cpp
//
//  Steven L. Goldberg, 2026
//
//  A critical teardown function that must be called during XPluginStop or 
//  XPluginDisable to safely destroy the shared font atlas, prevent VRAM
//  leaks, and ensure a clean reload of the plugin without crashing.
// ---------------------------------------------------------------------------

#include "ImgWindow.h"
#include "ImgFontAtlas.h"

/// Remove the static data created by ImGui -- specifically, the Font Atlas,
/// so we can force it to re-load cleanly on the next plugin start, and flush
/// any deferred Vulkan textures.
void TeardownImGui ()
{
    // 1. Clear away our "Font Atlas" that may have been previously loaded for
    // clean shut-down. This physically destroys the texture in VRAM:
#ifdef IMGUI_V192_REFACTOR            /* needed with ImGui v1.92 and later: */
    if (ImGui::GetCurrentContext() != NULL) {
        // Disconnect the ImgWindow version of the shared atlas link from the
        // active context to avoid double deletion!
        ImGui::GetIO().Fonts = NULL;  
    }
#endif /* IMGUI_V192_REFACTOR */

    if (ImgWindow::sFontAtlas)
        ImgWindow::sFontAtlas.reset();    // release singleton to delete atlas

    // 2. Force all ImgWindow instances to skip ImGui rendering altogether for
    // the next few cycles, preventing texture flicker or artifacts during a reload.
    ImgWindow::sBlankoutUntilCycle = XPLMGetCycleNumber() + 4;

    // 3. Flush the deferred texture deletion queue to prevent VRAM leaks and
    // driver crashes during teardown (X-Plane 12 Panel Graphics).
    ImgWindow::Shutdown();
}
