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
    // 1. Delete all custom textures loaded by your plugin before flushing!
    // if (gPluginTexture != (ImTextureID)0) {
    //     ImgWindow::DeleteTexture(gPluginTexture);
    //     gPluginTexture = (ImTextureID)0;
    // }

    // 2. Sever the active ImGui context's font atlas link FIRST.
    // In modern Dear ImGui (v1.92+), we MUST sever this pointer BEFORE calling
    // sFontAtlas.reset(). If sFontAtlas is reset first, ImFontAtlas is freed,
    // leaving io.Fonts pointing to deallocated memory (Use-After-Free dangling pointer trap).
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200) /* only on v1.92+ */
    if (ImGui::GetCurrentContext() != NULL) {
        ImGui::GetIO().Fonts = NULL;  
    }
#endif /* IMGUI_V192_REFACTOR */

    // 3. Reset the shared font atlas singleton to release memory and queue texture destruction.
    if (ImgWindow::sFontAtlas) {
        ImgWindow::sFontAtlas.reset();
    }

    // 4. Force all ImgWindow instances to skip ImGui rendering altogether for
    // the next few cycles, preventing texture flicker or artifacts during a reload.
    ImgWindow::sBlankoutUntilCycle = XPLMGetCycleNumber() + 4;

    // 5. Flush the deferred texture deletion queue immediately.
    // This physically destroys both the font atlas and any custom textures from VRAM.
    // MUST be called after all DeleteTexture() calls!
    ImgWindow::Shutdown();
}
