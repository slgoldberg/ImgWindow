/*
 * ImgWindow.cpp
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018,2020, Christopher Collins
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

#include "ImgWindow.h"

#include <cmath>
#include <XPLMUtilities.h>
#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>

#include "imgui_internal.h"

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

#ifdef IMGUI_V190_REFACTOR
static ImGuiKey vpXPLMKeyToImGuiKey(int inVirtualKey) {
    switch (inVirtualKey) {
        case XPLM_VK_TAB: return ImGuiKey_Tab;
        case XPLM_VK_LEFT: return ImGuiKey_LeftArrow;
        case XPLM_VK_RIGHT: return ImGuiKey_RightArrow;
        case XPLM_VK_UP: return ImGuiKey_UpArrow;
        case XPLM_VK_DOWN: return ImGuiKey_DownArrow;
        case XPLM_VK_PRIOR: return ImGuiKey_PageUp;
        case XPLM_VK_NEXT: return ImGuiKey_PageDown;
        case XPLM_VK_HOME: return ImGuiKey_Home;
        case XPLM_VK_END: return ImGuiKey_End;
        case XPLM_VK_INSERT: return ImGuiKey_Insert;
        case XPLM_VK_DELETE: return ImGuiKey_Delete;
        case XPLM_VK_BACK: return ImGuiKey_Backspace;
        case XPLM_VK_SPACE: return ImGuiKey_Space;
        case XPLM_VK_RETURN: return ImGuiKey_Enter;
        case XPLM_VK_ESCAPE: return ImGuiKey_Escape;
        case XPLM_VK_ENTER: return ImGuiKey_KeypadEnter;
        case XPLM_VK_NUMPAD0: return ImGuiKey_Keypad0;
        case XPLM_VK_NUMPAD1: return ImGuiKey_Keypad1;
        case XPLM_VK_NUMPAD2: return ImGuiKey_Keypad2;
        case XPLM_VK_NUMPAD3: return ImGuiKey_Keypad3;
        case XPLM_VK_NUMPAD4: return ImGuiKey_Keypad4;
        case XPLM_VK_NUMPAD5: return ImGuiKey_Keypad5;
        case XPLM_VK_NUMPAD6: return ImGuiKey_Keypad6;
        case XPLM_VK_NUMPAD7: return ImGuiKey_Keypad7;
        case XPLM_VK_NUMPAD8: return ImGuiKey_Keypad8;
        case XPLM_VK_NUMPAD9: return ImGuiKey_Keypad9;
        case XPLM_VK_A: return ImGuiKey_A;
        case XPLM_VK_C: return ImGuiKey_C;
        case XPLM_VK_V: return ImGuiKey_V;
        case XPLM_VK_X: return ImGuiKey_X;
        case XPLM_VK_Y: return ImGuiKey_Y;
        case XPLM_VK_Z: return ImGuiKey_Z;
        case XPLM_VK_0: return ImGuiKey_0;
        case XPLM_VK_1: return ImGuiKey_1;
        case XPLM_VK_2: return ImGuiKey_2;
        case XPLM_VK_3: return ImGuiKey_3;
        case XPLM_VK_4: return ImGuiKey_4;
        case XPLM_VK_5: return ImGuiKey_5;
        case XPLM_VK_6: return ImGuiKey_6;
        case XPLM_VK_7: return ImGuiKey_7;
        case XPLM_VK_8: return ImGuiKey_8;
        case XPLM_VK_9: return ImGuiKey_9;
    }
    return ImGuiKey_None;
}
#endif /* IMGUI_V190_REFACTOR */

// size of "frame" around a resizable window, by which its size can be changed
constexpr int WND_RESIZE_LEFT_WIDTH     = 15;
constexpr int WND_RESIZE_TOP_WIDTH      =  5;
constexpr int WND_RESIZE_RIGHT_WIDTH    = 15;
constexpr int WND_RESIZE_BOTTOM_WIDTH   = 15;

static XPLMDataRef gVrEnabledRef        = nullptr;
static XPLMDataRef gModelviewMatrixRef  = nullptr;
static XPLMDataRef gViewportRef         = nullptr;
static XPLMDataRef gProjectionMatrixRef = nullptr;
static XPLMDataRef gFrameRatePeriodRef  = nullptr;

std::shared_ptr<ImgFontAtlas> ImgWindow::sFontAtlas;

#ifdef IMGUI_V192_REFACTOR
// Helper to safely rebuild the atlas if it gets dirty at runtime.
// (IMPORTANT: This is required for ImGui v1.92+ since the font atlas is now self-managed, to preserve the semantics of XPLM windows created with ImgWindow on older versions of ImGui where the font atlas is shared across all such windows. This means it should "just work" to swap your legacy ImGui implementation for ImGui v1.92 or later, without having to change your code.)
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
void CheckAndRebuildAtlas(ImFontAtlas* atlas, void*& textureID)
#else
void CheckAndRebuildAtlas(ImFontAtlas* atlas, GLuint& textureID)
#endif
{
    // Honor "blankout" period, if any. (If so, skip rebuilding the atlas this frame.)
    if (XPLMGetCycleNumber() < ImgWindow::sBlankoutUntilCycle) {
        return;  // Skip checking/rebuilding this frame for now.
    }
    
    // Check 1: Is the atlas dirty? (e.g. User added a dynamic font)
    // Check 2: Is the texture missing? (e.g. X-Plane reloaded)
    // Check 3: Is the last font unbaked? (e.g. Partial build)
    bool need_rebuild = !atlas->TexIsBuilt;

    // if (!need_rebuild && atlas->TexID.GetTexID()) {
    //   if (!glIsTexture((GLuint)(uintptr_t)atlas->TexID.GetTexID())) need_rebuild = true;
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    if (!need_rebuild) {
        if (ImgPanelGraphics::IsAvailable()) {
            if ((void*)(intptr_t)atlas->TexData->GetTexRef().GetTexID() == nullptr) {
                need_rebuild = true;
            }
        } else {
            if (atlas->TexData->GetTexRef().GetTexID()) {
                if (!glIsTexture((GLuint)(uintptr_t)atlas->TexData->GetTexRef().GetTexID())) {
                    need_rebuild = true;
                }
            }
        }
    }
#else
    if (!need_rebuild && atlas->TexData->GetTexRef().GetTexID()) {
        if (!glIsTexture((GLuint)(uintptr_t)atlas->TexData->GetTexRef().GetTexID()))
            need_rebuild = true;
    }
#endif
    if (!need_rebuild && atlas->Fonts.Size > 0) {
        if (atlas->Fonts.back()->LastBaked == 0)
            need_rebuild = true;
    }

    if (need_rebuild)
    {
        // 1. Force full rebuild
        atlas->TexIsBuilt = false; 

        // 2. CPU Rasterize (RGBA32 for stability)
        ImgFontAtlas::strct_texture_info outInfo;
        ImgFontAtlas::GetCustomAtlasTextureData(atlas, outInfo);

        // 3. GPU Upload
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
        if (ImgPanelGraphics::IsAvailable()) {
            if (textureID != nullptr) {
                ImgPanelGraphics::DestroyTexture(textureID);
                textureID = nullptr;
            }
            // Sanitize texture RGB to accommodate anti-aliasing in light of straight-alpha blending
            if (outInfo.pixels && outInfo.width > 0 && outInfo.height > 0) {
                std::vector<unsigned char> lin_pixels(outInfo.pixels, outInfo.pixels + (outInfo.width * outInfo.height * 4));
                
                // N.B.: ImGui's font atlas generator can sometimes leave RGB values as black (0,0,0) in otherwise fully transparent areas.
                // Because X-Plane Panel Graphics uses straight alpha blending, those black pixels would bleed into the semi-transparent,
                // anti-aliased edges of the fonts, creating horrific muddy/black halos around the text.
                // By forcing every pixel's RGB to pure white (255), we guarantee such textures act strictly as pure opacity masks,
                // so the text color comes 100% from the vertex color.
                // (This loop only runs ONCE when the font atlas is rebuilt, so the CPU cost is negligible. It does NOT run every frame!)
                for (int i = 0; i < outInfo.width * outInfo.height; i++) {
                    unsigned char* p = &lin_pixels[i * 4];
                    p[0] = 255;
                    p[1] = 255;
                    p[2] = 255;
                }
                textureID = ImgPanelGraphics::CreateTexture(lin_pixels.data(), outInfo.width, outInfo.height);
            }
            // 4. Link
            // ... to the atlas's internal tracker, so ImGui can use it for rendering.
            atlas->TexData->SetTexID((ImTextureID)(intptr_t)textureID);
            if (ImgWindow::sFontAtlas && ImgWindow::sFontAtlas->getAtlas()) {
                ImgWindow::sFontAtlas->updateTextureTracking(textureID);
            }
        } else {
            // OpenGL fallback logic, but working with textureID as a void*
            GLuint glTextureID = (GLuint)(intptr_t)textureID;
            if (glTextureID == 0 || !glIsTexture(glTextureID)) {
                int texNum = 0;
                XPLMGenerateTextureNumbers(&texNum, 1);
                glTextureID = (GLuint)texNum;
            }

            XPLMBindTexture2d((int)glTextureID, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outInfo.width, outInfo.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outInfo.pixels);

            // 4. Link
            textureID = (void*)(intptr_t)glTextureID;
            atlas->TexData->SetTexID((ImTextureID)(uintptr_t)glTextureID);
            if (ImgWindow::sFontAtlas && ImgWindow::sFontAtlas->getAtlas()) {
                ImgWindow::sFontAtlas->updateTextureTracking(textureID);
            }
        }
#else
        // If texture ID is 0 or invalid, generate a new one.
        if (textureID == 0 || !glIsTexture(textureID)) {
            int texNum = 0;
            XPLMGenerateTextureNumbers(&texNum, 1);
            textureID = (GLuint)texNum;
        }

        XPLMBindTexture2d((int)textureID, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outInfo.width, outInfo.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outInfo.pixels);

        // 4. Link
        // ... to the atlas's internal tracker, so ImGui can use it for rendering.
        atlas->TexData->SetTexID((ImTextureID)(uintptr_t)textureID);
        if (ImgWindow::sFontAtlas && ImgWindow::sFontAtlas->getAtlas()) {
            // ... to our custom wrapper, so it can automatically delete the texture on destruction.
            ImgWindow::sFontAtlas->updateTextureTracking((int)textureID);
        }
#endif
    }
    else
    {
        if (atlas->TexData && atlas->TexData->GetTexRef().GetTexID() != 0)
        {
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
            void* currentAtlasId = (void*)(intptr_t)atlas->TexData->GetTexRef().GetTexID();
            if (textureID != currentAtlasId) {                
                textureID = currentAtlasId; // Catch up to the active pipeline instantly!
            }
#else
            GLuint currentAtlasId = static_cast<GLuint>((intptr_t)atlas->TexData->GetTexRef().GetTexID());
            if (textureID != currentAtlasId) {                
                textureID = currentAtlasId; // Catch up to the active pipeline instantly!
            }
#endif
        }
    }
}
#endif /* IMGUI_V192_REFACTOR */

ImgWindow::ImgWindow(
    int left,
    int top,
    int right,
    int bottom,
    XPLMWindowDecoration decoration,
    XPLMWindowLayer layer,
    bool cursors) :
    mFirstRender(true),
    mFontAtlas(sFontAtlas),
    bHandleWndResize(xplm_WindowDecorationSelfDecoratedResizable == decoration),
    mPreferredLayer(layer),
    bUseImgCursors(cursors)
{
#if _DEBUG
    // Recommended by ImGui in imconfig.h:
    // Check to make sure the current data structures this file is using are matching the ones imgui.cpp is using.
    IMGUI_CHECKVERSION();
#endif

    ImFontAtlas *iFontAtlas = nullptr;
    if (mFontAtlas) {
        mFontAtlas->bindTexture();
        iFontAtlas = mFontAtlas->getAtlas();
    }
    mImGuiContext = ImGui::CreateContext(iFontAtlas);
    ImGui::SetCurrentContext(mImGuiContext);
    auto &io = ImGui::GetIO();

#ifdef IMGUI_V192_REFACTOR
    // Use "modern" self-managed font atlas to provide the same atlas semantics as the legacy code below, but in a way that is compatible with ImGui v1.92+.
    // (IMPORTANT: the font atlas is shared across all XPLM windows created from ImgWindow or ImgWindow-derived classes.)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    // Just-in-time atlas build & verification:
    // we check this immediately to handle initial builds, high-DPI switches, or fallback default fonts.
    // (This replaces the legacy binding logic #ifdef'ed out below for ImGui v1.90 and above.)
    if (io.Fonts) {
        CheckAndRebuildAtlas(io.Fonts, mFontTexture);
    }

    // "Stealth Mode":
    // Remove this atlas from the context's update list to prevent ImGui v1.92+
    // from trying to manage the texture lifecycle (which can cause crashes
    // since we want to manage this stuff, not let ImGui modify at will).
    if (iFontAtlas) {
        for (int i = 0; i < mImGuiContext->FontAtlases.Size; i++) {
            if (mImGuiContext->FontAtlases[i] == iFontAtlas) {
                mImGuiContext->FontAtlases.erase(mImGuiContext->FontAtlases.Data + i);
                break;
            }
        }
    }
#endif /* IMGUI_V192_REFACTOR */

    static bool first_init=false;
    if (!first_init) {
        gVrEnabledRef = XPLMFindDataRef("sim/graphics/VR/enabled");
        gModelviewMatrixRef = XPLMFindDataRef("sim/graphics/view/modelview_matrix");
        gViewportRef = XPLMFindDataRef("sim/graphics/view/viewport");
        gProjectionMatrixRef = XPLMFindDataRef("sim/graphics/view/projection_matrix");
        gFrameRatePeriodRef = XPLMFindDataRef("sim/operation/misc/frame_rate_period");
        first_init=true;
    }

#ifndef IMGUI_V192_REFACTOR
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    // we render ourselves, we don't use the DrawListsFunc
    io.RenderDrawListsFn = nullptr;
#endif
#endif /* IMGUI_V192_REFACTOR */

#ifndef IMGUI_V190_REFACTOR
    // set up the Keymap
    io.KeyMap[ImGuiKey_Tab] = XPLM_VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = XPLM_VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = XPLM_VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = XPLM_VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = XPLM_VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = XPLM_VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = XPLM_VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = XPLM_VK_HOME;
    io.KeyMap[ImGuiKey_End] = XPLM_VK_END;
    io.KeyMap[ImGuiKey_Insert] = XPLM_VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = XPLM_VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = XPLM_VK_BACK;
    io.KeyMap[ImGuiKey_Space] = XPLM_VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = XPLM_VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = XPLM_VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = XPLM_VK_ENTER;
    io.KeyMap[ImGuiKey_A] = XPLM_VK_A;
    io.KeyMap[ImGuiKey_C] = XPLM_VK_C;
    io.KeyMap[ImGuiKey_V] = XPLM_VK_V;
    io.KeyMap[ImGuiKey_X] = XPLM_VK_X;
    io.KeyMap[ImGuiKey_Y] = XPLM_VK_Y;
    io.KeyMap[ImGuiKey_Z] = XPLM_VK_Z;
#endif /* IMGUI_V190_REFACTOR */

    // disable window rounding since we're not rendering the frame anyway.
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 0;

#ifndef IMGUI_V192_REFACTOR
    // bind the font
    if (mFontAtlas) {
        mFontTexture = static_cast<GLuint>(reinterpret_cast<intptr_t>(io.Fonts->TexID));
    } else {
        if (!iFontAtlas || iFontAtlas->TexID == nullptr) {
            // fallback binding if an atlas wasn't explicitly set.
            unsigned char *pixels;
            int width, height;
            io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

            // slightly stupid dance around the texture number due to XPLM not using GLint here.
            int texNum = 0;
            XPLMGenerateTextureNumbers(&texNum, 1);
            mFontTexture = (GLuint)texNum;

            // upload texture.
            XPLMBindTexture2d((int)mFontTexture, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_ALPHA,
                         width,
                         height,
                         0,
                         GL_ALPHA,
                         GL_UNSIGNED_BYTE,
                         pixels);
            io.Fonts->SetTexID((void *)((intptr_t)(mFontTexture)));
        }
    }
#else
    // Sync texture ID:
    // if CheckAndRebuildAtlas() above did its job, mFontTexture is set; if the shared atlas was already built, grab the ID here.
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    if ((void*)(intptr_t)io.Fonts->TexData->GetTexID() != nullptr) {
        mFontTexture = (void*)(intptr_t)io.Fonts->TexData->GetTexID();
    }
#else
    if (io.Fonts->TexData->GetTexID() != 0) {
        mFontTexture = static_cast<GLuint>((intptr_t)io.Fonts->TexData->GetTexID());
    }
#endif
#endif /* IMGUI_V192_REFACTOR */

    // disable OSX-like keyboard behaviours always - we don't have the keymapping for it.
    io.ConfigMacOSXBehaviors = false;

#ifdef IMGUI_V190_REFACTOR
    // override to enable the required OSX behaviors specifically for macOS only.
#if defined(__APPLE__) || defined(__MACH__)
    io.ConfigMacOSXBehaviors = true;
#endif
#endif /* IMGUI_V190_REFACTOR */

    // try to inhibit a few resize/move behaviours that won't play nice with our window control.
    io.ConfigWindowsResizeFromEdges = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    if (IsUsingPanelGraphics()) {
#if defined(XPLM440)
        XPLMCreateWindow_t windowParams = {0};
        windowParams.structSize = sizeof(XPLMCreateWindow_t);
#else
        SpoofedXPLMCreateWindow_t_440 windowParams = {0};
        windowParams.structSize = sizeof(SpoofedXPLMCreateWindow_t_440);
#endif
        windowParams.left = left;
        windowParams.top = top;
        windowParams.right = right;
        windowParams.bottom = bottom;
        windowParams.visible = 0;
        windowParams.drawWindowFunc = DrawWindowCB;
        windowParams.handleMouseClickFunc = HandleMouseClickCB;
        windowParams.handleKeyFunc = HandleKeyFuncCB;
        windowParams.handleCursorFunc = HandleCursorFuncCB;
        windowParams.handleMouseWheelFunc = HandleMouseWheelFuncCB;
        windowParams.refcon = reinterpret_cast<void*>(this);
        windowParams.decorateAsFloatingWindow = decoration;
        windowParams.layer = layer;
        windowParams.handleRightClickFunc = HandleRightClickFuncCB;
        windowParams.windowContentType = xplm_WindowContentTypePanelGraphics;
        
        mWindowID = XPLMCreateWindowEx(reinterpret_cast<XPLMCreateWindow_t*>(&windowParams));
        
        if (sFontAtlasRebuildHandler == nullptr) {
            XPLMCreateFlightLoop_t flParams = {
                sizeof(XPLMCreateFlightLoop_t),
                xplm_FlightLoop_Phase_BeforeFlightModel,
                FontAtlasRebuildFLCB,
                nullptr
            };
            sFontAtlasRebuildHandler = XPLMCreateFlightLoop(&flParams);
            XPLMScheduleFlightLoop(sFontAtlasRebuildHandler, -1.0f, 1);
        }
    } else 
#endif
    {
        XPLMCreateWindow_t windowParams = {
            sizeof(windowParams),
            left,
            top,
            right,
            bottom,
            0,
            DrawWindowCB,
            HandleMouseClickCB,
            HandleKeyFuncCB,
            HandleCursorFuncCB,
            HandleMouseWheelFuncCB,
            reinterpret_cast<void*>(this),
            decoration,
            layer,
            HandleRightClickFuncCB,
        };
        mWindowID = XPLMCreateWindowEx(&windowParams);
    }
}

ImgWindow::~ImgWindow()
{
    ImGui::SetCurrentContext(mImGuiContext);
#ifdef IMGUI_V192_REFACTOR
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts)
    {
        // 1. Get the shared data pointer for THIS context:
        ImDrawListSharedData* data = ImGui::GetDrawListSharedData();

        // 2. Manually find and remove it from the Atlas's list:
        for (int i = 0; i < io.Fonts->DrawListSharedDatas.Size; i++)
        {
            if (io.Fonts->DrawListSharedDatas[i] == data) {
                io.Fonts->DrawListSharedDatas.erase(io.Fonts->DrawListSharedDatas.Data + i);
                break; // Found and removed.
            }
        }

        // 3. NOW it is safe to sever the link (prevent double free):
        io.Fonts = NULL;
    }
#endif /* IMGUI_V192_REFACTOR */
    if (!mFontAtlas) {
        // if we didn't have an explicit font atlas, destroy the texture.
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
        if (mFontTexture) {
            if (IsUsingPanelGraphics()) {
                ImgPanelGraphics::DestroyTexture(mFontTexture);
            } else {
                GLuint glTextureID = (GLuint)(intptr_t)mFontTexture;
                glDeleteTextures(1, &glTextureID);
            }
        }
#else
        glDeleteTextures(1, &mFontTexture);
#endif
    }
    ImGui::DestroyContext(mImGuiContext);
    XPLMDestroyWindow(mWindowID);
}

void
ImgWindow::GetCurrentWindowGeometry (int& left, int& top, int& right, int& bottom) const
{
    if (IsPoppedOut())
        GetWindowGeometryOS(left, top, right, bottom);
    else if (IsInVR()) {
        left = bottom = 0;
        GetWindowGeometryVR(right, top);
    } else {
        GetWindowGeometry(left, top, right, bottom);
    }
}

void
ImgWindow::SetWindowResizingLimits (int minW, int minH, int maxW, int maxH)
{
    minWidth  = minW;
    minHeight = minH;
    maxWidth  = maxW;
    maxHeight = maxH;
    XPLMSetWindowResizingLimits(mWindowID, minW, minH, maxW, maxH);
}

void
ImgWindow::updateMatrices()
{
    // Get the current modelview matrix, viewport, and projection matrix from X-Plane
    XPLMGetDatavf(gModelviewMatrixRef, mModelView, 0, 16);
    XPLMGetDatavf(gProjectionMatrixRef, mProjection, 0, 16);
    XPLMGetDatavi(gViewportRef, mViewport, 0, 4);
}

static void multMatrixVec4f(GLfloat dst[4], const GLfloat m[16], const GLfloat v[4])
{
    dst[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + v[3] * m[12];
    dst[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + v[3] * m[13];
    dst[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
    dst[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];
}

void
ImgWindow::boxelsToNative(int x, int y, int &outX, int &outY)
{
    GLfloat boxelPos[4] = { (GLfloat)x, (GLfloat)y, 0, 1 };
    GLfloat eye[4], ndc[4];

    multMatrixVec4f(eye, mModelView, boxelPos);
    multMatrixVec4f(ndc, mProjection, eye);
    ndc[3] = 1.0f / ndc[3];
    ndc[0] *= ndc[3];
    ndc[1] *= ndc[3];

    outX = static_cast<int>((ndc[0] * 0.5f + 0.5f) * mViewport[2] + mViewport[0]);
    outY = static_cast<int>((ndc[1] * 0.5f + 0.5f) * mViewport[3] + mViewport[1]);
}

void
ImgWindow::RenderImGui(ImDrawData *draw_data)
{
    // Implement a mechanism to hold off on rendering any frames (across all windows) when sBlankoutUntilCycle is set to a future cycle number.
    // (This can be very useful to avoid flicker during a reload of one or more windows that have lots of fonts that need to be loaded.  Setting this to XPLMCycleNumber() + 4 is a nice middle ground to ensure smooth, flicker-free reloads.)
    if (XPLMGetCycleNumber() < ImgWindow::sBlankoutUntilCycle) {
        return;  // Skip rendering this frame.
    }
    
#ifdef IMGUI_V192_REFACTOR
    if (mFontAtlas && mFontAtlas->getAtlas()) {
        // rebuild and upload *only* if the atlas is actually out of date (e.g., dynamic font size or style changes, etc.)
        // (Note: very inexpensive with early-out returns in common case.)
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
        if (!IsUsingPanelGraphics()) {
            CheckAndRebuildAtlas(mFontAtlas->getAtlas(), mFontTexture);
        }
#else
        CheckAndRebuildAtlas(mFontAtlas->getAtlas(), mFontTexture);
#endif
    }
#endif /* IMGUI_V192_REFACTOR */

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplayFramebufferScale.x != 1.0 ||
        io.DisplayFramebufferScale.y != 1.0)
        draw_data->ScaleClipRects(io.DisplayFramebufferScale);

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    if (IsUsingPanelGraphics()) {
        static bool s_logged_backend = false;
        if (!s_logged_backend) {
            XPLMDebugString("ImgWindow: Rendering via XPLM v4.4 Panel Graphics\n");
            s_logged_backend = true;
        }

        if (mFontTexture == nullptr || draw_data->CmdListsCount == 0)
            return;

        // Render command lists
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];

            XPLMMesh_t mesh;
            mesh.vertex_count = cmd_list->VtxBuffer.Size;
            mesh.vertices = (const float*)cmd_list->VtxBuffer.Data;
            mesh.index_count = cmd_list->IdxBuffer.Size;
            static_assert(sizeof(ImDrawIdx) == 2, "X-Plane 12 Panel Graphics API strictly requires 16-bit ImGui indices.");
            mesh.indices = (const uint16_t*)cmd_list->IdxBuffer.Data;
            
            std::vector<XPLMDrawCall_t> draw_calls;
            draw_calls.reserve(cmd_list->CmdBuffer.Size);

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback) {
                    if (!draw_calls.empty()) {
                        ImgPanelGraphics::DrawCalls(&mesh, draw_calls.size(), draw_calls.data());
                        draw_calls.clear();
                    }
                    pcmd->UserCallback(cmd_list, pcmd);
                } else {
                    XPLMDrawCall_t dc;
    #ifndef IMGUI_V192_REFACTOR
                    dc.tex_ref = (void*)(intptr_t)pcmd->TextureId;
    #else
                    dc.tex_ref = (void*)(intptr_t)pcmd->GetTexID();
    #endif
                    dc.scissors[0] = pcmd->ClipRect.x;
                    dc.scissors[1] = pcmd->ClipRect.y;
                    dc.scissors[2] = pcmd->ClipRect.z;
                    dc.scissors[3] = pcmd->ClipRect.w;
                    dc.idx_offset = pcmd->IdxOffset;
                    dc.element_count = pcmd->ElemCount;
                    dc.vtx_offset = pcmd->VtxOffset;
                    draw_calls.push_back(dc);
                }
            }
            
            if (!draw_calls.empty()) {
                ImgPanelGraphics::DrawCalls(&mesh, draw_calls.size(), draw_calls.data());
            }
        }
    } else
#endif
    {
    updateMatrices();

    // We are using the OpenGL fixed pipeline because messing with the
    // shader-state in X-Plane is not very well documented, but using the fixed
    // function pipeline is.

    // 1TU + Alpha settings, no depth, no fog.
    XPLMSetGraphicsState(0, 1, 0, 1, 1, 0, 0);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glScalef(1.0f, -1.0f, 1.0f);
    glTranslatef(static_cast<GLfloat>(mLeft), static_cast<GLfloat>(-mTop), 0.0f);

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
#ifndef IMGUI_V192_REFACTOR
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));
#else /* IMGUI_V192_REFACTOR: */
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + offsetof(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + offsetof(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + offsetof(ImDrawVert, col)));
#endif /* IMGUI_V192_REFACTOR */

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
#ifndef IMGUI_V192_REFACTOR
                XPLMBindTexture2d((int)(intptr_t)pcmd->TextureId, 0);
#else
                XPLMBindTexture2d((int)(intptr_t)pcmd->GetTexID(), 0);
#endif /* IMGUI_V192_REFACTOR */

                // Scissors work in viewport space - must translate the coordinates from ImGui -> Boxels, then Boxels -> Native.
                //FIXME: it must be possible to apply the scale+transform manually to the projection matrix so we don't need to doublestep.
                int bTop, bLeft, bRight, bBottom;
                translateImguiToBoxel(pcmd->ClipRect.x, pcmd->ClipRect.y, bLeft, bTop);
                translateImguiToBoxel(pcmd->ClipRect.z, pcmd->ClipRect.w, bRight, bBottom);
                int nTop, nLeft, nRight, nBottom;
                boxelsToNative(bLeft, bTop, nLeft, nTop);
                boxelsToNative(bRight, bBottom, nRight, nBottom);
                glScissor(nLeft, nBottom, nRight-nLeft, nTop-nBottom);
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    // Restore modified state
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);    // fix from Mark Parker 2026
    glPopAttrib();
    glPopClientAttrib();
    }
}

void
ImgWindow::translateToImguiSpace(int inX, int inY, float &outX, float &outY)
{
    outX = static_cast<float>(inX - mLeft);
    if (outX < 0.0f || outX > (float)(mRight - mLeft)) {
        outX = -FLT_MAX;
        outY = -FLT_MAX;
        return;
    }
    outY = static_cast<float>(mTop-inY);
    if (outY < 0.0f || outY > (float)(mTop - mBottom)) {
        outX = -FLT_MAX;
        outY = -FLT_MAX;
        return;
    }
}

void
ImgWindow::translateImguiToBoxel(float inX, float inY, int &outX, int &outY)
{
    outX = (int)(mLeft + inX);
    outY = (int)(mTop - inY);
}


void
ImgWindow::updateImgui()
{
    ImGui::SetCurrentContext(mImGuiContext);
    auto &io = ImGui::GetIO();

    // transfer the window geometry to ImGui
    XPLMGetWindowGeometry(mWindowID, &mLeft, &mTop, &mRight, &mBottom);

    float win_width = static_cast<float>(mRight - mLeft);
    float win_height = static_cast<float>(mTop - mBottom);

    // Needed to add this to prevent io.DeltaTime causing a CTD because when X-Plane starts FrameRatePeriod is equal to 0.0f
    float FrameRatePeriod = XPLMGetDataf(gFrameRatePeriodRef);
    if (FrameRatePeriod > 0.0f) {
        io.DeltaTime = XPLMGetDataf(gFrameRatePeriodRef);
    }
    if (io.DeltaTime <= 0.0f) {
        io.DeltaTime = 1.0f / 60.0f;  // ensure cursors blink and stuff
    }
    io.DisplaySize = ImVec2(win_width, win_height);
    // in boxels, we're always scale 1, 1.
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

#ifdef IMGUI_V192_REFACTOR
    // Just-in-time rebuild:
    // If the ImGui client code added a font or scaled text since the last frame, the atlas will be "dirty".
    // (So we catch such things here and rebuild what's needed instantly before ImGui tries to draw.)
    if (mFontAtlas && mFontAtlas->getAtlas()) {
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
        if (!IsUsingPanelGraphics()) {
            CheckAndRebuildAtlas(mFontAtlas->getAtlas(), mFontTexture);
        }
#else
        CheckAndRebuildAtlas(mFontAtlas->getAtlas(), mFontTexture);
#endif
    }
#endif /* IMGUI_V192_REFACTOR */

    ImGui::NewFrame();

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    bool isGhosting = (mGhostFramesRemaining > 0);
    if (isGhosting) {
        // Hide everything while ImGui lays out the glyphs.
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f);
    }
#endif

    ImGui::SetNextWindowPos(ImVec2((float) 0.0, (float) 0.0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(win_width, win_height), ImGuiCond_Always);

    // ...and construct the window, detecting whether the window flags allow moving via right-drag.
    ImGuiWindowFlags_ userFlags = beforeBegin();
    bCanMove = !(userFlags & ImGuiWindowFlags_NoMove);
    ImGui::Begin(mWindowTitle.c_str(), nullptr, userFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    buildInterface();
    ImGui::End();

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    if (isGhosting) {
        ImGui::PopStyleVar();
        mGhostFramesRemaining--;
    }
#endif

    // finally, handle window focus.
    int hasKeyboardFocus = XPLMHasKeyboardFocus(mWindowID);

#ifdef IMGUI_V190_REFACTOR
    if (hasKeyboardFocus != bLastKeyboardFocused) {
        bLastKeyboardFocused = hasKeyboardFocus;
        io.AddFocusEvent(bLastKeyboardFocused != 0);
    }
#endif /* IMGUI_V190_REFACTOR */

    if (io.WantTextInput && !hasKeyboardFocus) {
        XPLMTakeKeyboardFocus(mWindowID);
    }
    else if (!io.WantTextInput && hasKeyboardFocus) {
        XPLMTakeKeyboardFocus(nullptr);
        // reset keysdown otherwise we'll think any keys used to defocus the keyboard are still down!
#ifndef IMGUI_V190_REFACTOR
        for (auto &key : io.KeysDown) {
            key = false;
        }
#endif /* IMGUI_V190_REFACTOR */
    }
    mFirstRender = false;
}

void
ImgWindow::DrawWindowCB(XPLMWindowID /* inWindowID */, void *inRefcon)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);

    thisWindow->updateImgui();

    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGui::Render();

    thisWindow->RenderImGui(ImGui::GetDrawData());

    // Give subclasses a chance to do something after all rendering
    thisWindow->afterRendering();

    // Hack: Reset the Backspace key if in VR (see HandleKeyFuncCB for details)
    if (thisWindow->bResetBackspace) {
        ImGuiIO& io = ImGui::GetIO();
#ifndef IMGUI_V190_REFACTOR
        io.KeysDown[XPLM_VK_BACK] = false;
#else
        io.AddKeyEvent(ImGuiKey_Backspace, false);
#endif /* IMGUI_V190_REFACTOR */
        thisWindow->bResetBackspace = false;
    }
}

int
ImgWindow::HandleMouseClickCB(XPLMWindowID /* inWindowID */, int x, int y, XPLMMouseStatus inMouse, void *inRefcon)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);

    // If this is an overlay window that cannot be moved, ignore mouse clicks.
    if (thisWindow->mPreferredLayer == xplm_WindowLayerFlightOverlay &&
        !thisWindow->bCanMove)
        return 0;    // (ignore mouse clicks in this case)

    return thisWindow->HandleMouseClickGeneric(x, y, inMouse, 0);
}

int
ImgWindow::HandleMouseClickGeneric(int x, int y, XPLMMouseStatus inMouse, int button)
{
    ImGui::SetCurrentContext(mImGuiContext);
    ImGuiIO& io = ImGui::GetIO();

    // Tell ImGui the mous position relative to the window
    translateToImguiSpace(x, y, io.MousePos.x, io.MousePos.y);
    const int loc_x = int(io.MousePos.x);       // local x, relative to top/left corner
    const int loc_y = int(io.MousePos.y);
    const int dx = x - lastMouseDragX;          // dragged how far since last down/drag event?
    const int dy = y - lastMouseDragY;

    switch (inMouse) {

        case xplm_MouseDrag:
            io.MouseDown[button] = true;

            // Any kind of self-dragging/resizing only happens with a floating window in the sim
            if ((button == 0 || bCanMove) &&  // left button OR right-drag (if move allowed)
                IsInsideSim() &&            // floating window in sim
                dragWhat &&                 // and if there actually _is_ dragging
                (dx != 0 || dy != 0))
            {
                // shall we drag the entire window?
                if (dragWhat.wnd)
                {
                    mLeft   += dx;                      // move the window
                    mRight  += dx;
                    mTop    += dy;
                    mBottom += dy;
                } else {
                    // do we need to handle window resize?
                    if (dragWhat.left)   mLeft   += dx;
                    if (dragWhat.top)    mTop    += dy;
                    if (dragWhat.right)  mRight  += dx;
                    if (dragWhat.bottom) mBottom += dy;

                    // Make sure resizing limits are honored
                    if (mRight-mLeft < minWidth)
                    {
                        if (dragWhat.left) mLeft = mRight - minWidth;
                        else mRight = mLeft + minWidth;
                    }
                    if (mRight-mLeft > maxWidth)
                    {
                        if (dragWhat.left) mLeft = mRight - maxWidth;
                        else mRight = mLeft + maxWidth;
                    }
                    if (mTop-mBottom < minHeight) {
                        if (dragWhat.top) mTop = mBottom + minHeight;
                        else mBottom = mTop - minHeight;
                    }
                    if (mTop-mBottom > maxHeight) {
                        if (dragWhat.top) mTop = mBottom + maxHeight;
                        else mBottom = mTop - maxHeight;
                    }
                    // FIXME: If we had to apply resizing restricitons, then mouse and window frame will now be out of synch
                }

                // Change window geometry
                SetWindowGeometry(mLeft, mTop, mRight, mBottom);
                // now that the window has moved under the mouse we need to update relative mouse pos
                translateToImguiSpace(x, y, io.MousePos.x, io.MousePos.y);
                // Update the last handled position
                lastMouseDragX = x;
                lastMouseDragY = y;
            }
            break;

        case xplm_MouseDown:
            io.MouseDown[button] = true;

            // Which part of the window would we drag, if any?
            dragWhat.clear();
            if ((button == 0 || bCanMove) &&  // left button OR right-drag (if move allowed)
                IsInsideSim() &&            // floating window in simulator
                loc_x >= 0 && loc_y >= 0)   // valid local position
            {
                // Shall we drag the entire window?
                // (We allow right-dragging to move the entire window if bCanMove is true, which is set based on the ImGuiWindowFlags_NoMove flag.)
                if (IsInsideWindowDragArea(loc_x, loc_y) || (button == 1 && bCanMove))
                {
                    dragWhat.wnd = true;
                }
                // Do we need to handle window resize?
                else if (bHandleWndResize && button == 0) // left button ONLY for resizing
                {
                    dragWhat.left   = loc_x <= WND_RESIZE_LEFT_WIDTH;
                    dragWhat.top    = loc_y <= WND_RESIZE_TOP_WIDTH;
                    dragWhat.right  = loc_x >= (mRight - mLeft) - WND_RESIZE_RIGHT_WIDTH;
                    dragWhat.bottom = loc_y >= (mTop - mBottom) - WND_RESIZE_BOTTOM_WIDTH;
                }
                // Anything to drag?
                if (dragWhat) {
                    // Remember pos in case of dragging
                    lastMouseDragX = x;
                    lastMouseDragY = y;
                }
            }
            break;

        case xplm_MouseUp:
            io.MouseDown[button] = false;
            lastMouseDragX = lastMouseDragY = -1;
            dragWhat.clear();
            break;
        default:
            // dunno!
            break;
    }

    return 1;
}


void
ImgWindow::HandleKeyFuncCB(
    XPLMWindowID         /*inWindowID*/,
    char                 inKey,
    XPLMKeyFlags         inFlags,
    char                 inVirtualKey,
    void *               inRefcon,
    int                  blosingFocus)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGuiIO& io = ImGui::GetIO();

    // Loosing focus? That's not exactly something ImGui allows us to do...
    // we try convincing ImGui to let it go by sending an [Esc] key for the legacy version, or by sending a focus-lost event for the modern version.
    if (blosingFocus) {
#ifdef IMGUI_V190_REFACTOR
        io.AddFocusEvent(false); // tell ImGui it lost focus
#endif /* IMGUI_V190_REFACTOR */
        if (ImGui::GetCurrentContext()) {
            // Clear any active ID, to prevent ImGui from thinking it still has focus:
            ImGui::ClearActiveID();
        }

        return;  // don't process any further key events if we're losing focus
    }

    if (io.WantCaptureKeyboard) {
        // Hack for the Backspace key in VR:
        // Apparently, the virtual VR keyboard sends both the Up and the Down
        // event within the same drawing cycle, which would overwrite
        // io.KeyDown[XPLM_VK_BACK] with false again before we could pass on true.
        // Also see https://forums.x-plane.org/index.php?/forums/topic/147139-dear-imgui-x-plane/&do=findComment&comment=2032062
        // though I am following a different solution:
        // So we ignore the "up" event (release key) here, and do the actual
        // release only after the next drawing cycle (flag bResetBackspace).
        // (And this little delay doesn't hurt in non-VR either, so we don't even test for VR.)

#ifndef IMGUI_V190_REFACTOR
        // If Backspace is _released_ ...
        if (inVirtualKey == XPLM_VK_BACK && !(inFlags & xplm_DownFlag)) {
            thisWindow->bResetBackspace = true; // have it reset only later in DrawWindowCB
        }
        else {
            // in all normal cases: save the up/down flag as it comes from XP
            io.KeysDown[int(inVirtualKey)] = (inFlags & xplm_DownFlag) == xplm_DownFlag;
        }
        
        io.KeyShift = (inFlags & xplm_ShiftFlag) == xplm_ShiftFlag;
        io.KeyAlt = (inFlags & xplm_OptionAltFlag) == xplm_OptionAltFlag;
        io.KeyCtrl = (inFlags & xplm_ControlFlag) == xplm_ControlFlag;

        // inKey will only includes printable characters,
        // but also those created with key combinations like @ or {}
        if ((inFlags & xplm_DownFlag) == xplm_DownFlag &&
            inKey > '\0')
        {
            char smallStr[2] = { inKey, 0 };
            io.AddInputCharactersUTF8(smallStr);
        }
#else /* IMGUI_V190_REFACTOR */
        bool isDown = (inFlags & xplm_DownFlag) == xplm_DownFlag;
        ImGuiKey key = vpXPLMKeyToImGuiKey(inVirtualKey);

        // Sync modifiers *first*.
        // (We send these regardless of whether it's a key down or up, to ensure ImGui's internal modifier bitmask is current.)
        bool shift = (inFlags & xplm_ShiftFlag) != 0;
        bool alt = (inFlags & xplm_OptionAltFlag) != 0; // 'Option' on macOS (though no special handling needed).
        bool ctrl = (inFlags & xplm_ControlFlag) != 0; // 'Super' on macOS (see below).

        // Sync platform-common basic modifiers via the Event System (modern way).
        io.AddKeyEvent(ImGuiMod_Shift, shift);
        io.AddKeyEvent(ImGuiMod_Alt,   alt);
        
        // Sync legacy structural booleans to avoid known Windows race condition single-frame sync lag with stb_textedit.)
        io.KeyShift = shift;
        io.KeyAlt = alt;
        
#if defined(__APPLE__) || defined(__MACH__)
        // On macOS: re-map both 'Ctrl' and 'Cmd' to 'Super'.
        // (X-Plane's 'Ctrl' and 'Cmd' are indistinguishable, and both need to appear as 'Super' here instead.)
        io.AddKeyEvent(ImGuiMod_Super, ctrl);
        io.AddKeyEvent(ImGuiMod_Ctrl,  false); // 'Super' subsumes and thus cancels 'Ctrl' in X-Plane!
        
        // Match legacy structural mappings for macOS:
        io.KeySuper = ctrl;
        io.KeyCtrl =  false;  // 'Super' subsumes and thus cancels 'Ctrl' in X-Plane!

#else
        io.AddKeyEvent(ImGuiMod_Ctrl,  ctrl);
        io.AddKeyEvent(ImGuiMod_Super, false); // 'Super' is not used on non-macOS platforms (so don't let it somehow leak through).
        
        // Match legacy structural mappings for Windows/Linux:
        io.KeyCtrl = ctrl;
        io.KeySuper = false;
#endif

        // If backspace is _released_ ...
        if (inVirtualKey == XPLM_VK_BACK && !isDown) {
            thisWindow->bResetBackspace = true; // have it reset only later in DrawWindowCB
        }
        else if (key != ImGuiKey_None) {
            io.AddKeyEvent(key, isDown); // send the actual key
        }

        // Filter out and only show any true input characters after the key events (i.e., don't send 'v' with "ctrl+v" since we added the shortcut event above instead).
        bool isAnyModifierActive = ctrl || alt || io.KeySuper || io.KeyCtrl;  // (don't check Shift!)
        if (isDown && (unsigned char)inKey >= 32 && !isAnyModifierActive)
        {
            io.AddInputCharacter((unsigned char)inKey);
        }
#endif /* IMGUI_V190_REFACTOR */
    }
}

XPLMCursorStatus
ImgWindow::HandleCursorFuncCB(
    XPLMWindowID         /*inWindowID*/,
    int                  x,
    int                  y,
    void *               inRefcon)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);

    // If this is an overlay window that cannot be moved, ignore mouse cursor changes.
    if (thisWindow->mPreferredLayer == xplm_WindowLayerFlightOverlay && !thisWindow->bCanMove)
        return xplm_CursorDefault;  // (ignore for immovable overlays)

    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGuiIO& io = ImGui::GetIO();
    float outX, outY;
    thisWindow->translateToImguiSpace(x, y, outX, outY);
    io.MousePos = ImVec2(outX, outY);
    
    // Don't use ImGui mouse cursor handling if disabled, or if cursor is
    // within special XPLM self-styled resize handling regions.
    if (!thisWindow->bUseImgCursors ||
        (thisWindow->IsInsideSim() && thisWindow->bHandleWndResize &&
         (x < (thisWindow->mLeft + WND_RESIZE_LEFT_WIDTH) ||
          x > (thisWindow->mRight - WND_RESIZE_RIGHT_WIDTH) ||
          y > (thisWindow->mTop - WND_RESIZE_TOP_WIDTH) ||
          y < (thisWindow->mBottom + WND_RESIZE_BOTTOM_WIDTH))))
    {
        // Defer to XPLM's hand cursor if disabled, or when inside the XPLM-managed resize grab regions:
        io.MouseDrawCursor = false;
        return xplm_CursorDefault;
    }
    
    // Have ImGui take over the mouse cursor for the rest:
    io.MouseDrawCursor = true;
    return xplm_CursorHidden;
}

int
ImgWindow::HandleMouseWheelFuncCB(
    XPLMWindowID         /*inWindowID*/,
    int                  x,
    int                  y,
    int                  wheel,
    int                  clicks,
    void *               inRefcon)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);

    // If this is an overlay window that cannot be moved, ignore mouse wheel events.
    if (thisWindow->mPreferredLayer == xplm_WindowLayerFlightOverlay &&
        !thisWindow->bCanMove)
        return 0;  // (ignore mouse-wheel for immovable overlays)
    
    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGuiIO& io = ImGui::GetIO();

    float outX, outY;
    thisWindow->translateToImguiSpace(x, y, outX, outY);
    io.MousePos = ImVec2(outX, outY);
    switch (wheel) {
    case 0:
        io.MouseWheel += static_cast<float>(clicks);
        break;
    case 1:
        io.MouseWheelH += static_cast<float>(clicks);
        break;
    default:
        // unknown wheel
        break;
    }
    return 1;
}

int
ImgWindow::HandleRightClickFuncCB(XPLMWindowID /* inWindowID */, int x, int y, XPLMMouseStatus inMouse, void *inRefcon)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    
    // If this is an overlay window that cannot be moved, ignore right-clicks altogether.
    //FIXME: is this the right thing to do?  We could allow right-clicks for context menus, but not allow dragging...
    if (thisWindow->mPreferredLayer == xplm_WindowLayerFlightOverlay &&
        !thisWindow->bCanMove)
        return 0;   // (always ignore right-clicks for overlay layers)
    
    return thisWindow->HandleMouseClickGeneric(x, y, inMouse, 1);
}


void
ImgWindow::SetWindowTitle(const std::string &title)
{
    mWindowTitle = title;
    XPLMSetWindowTitle(mWindowID, mWindowTitle.c_str());
}

void
ImgWindow::SetVisible(bool inIsVisible)
{
    if (inIsVisible)
        moveForVR();
    if (GetVisible() == inIsVisible) {
        // if the state is already correct, no-op.
        return;
    }
    if (inIsVisible) {
        if (!onShow()) {
            // chance to early abort.
            return;
        }
    }
    XPLMSetWindowIsVisible(mWindowID, inIsVisible);
}

void
ImgWindow::moveForVR()
{
    // if we're trying to display the window, check the state of the VR flag
    // - if we're VR enabled, explicitly move the window to the VR world.
    if (XPLMGetDatai(gVrEnabledRef)) {
        XPLMSetWindowPositioningMode(mWindowID, xplm_WindowVR, 0);
    } else {
        if (IsInVR()) {
            XPLMSetWindowPositioningMode(mWindowID, mPreferredLayer, -1);
        }
    }
}

bool
ImgWindow::GetVisible() const
{
    return XPLMGetWindowIsVisible(mWindowID) != 0;
}

bool
ImgWindow::IsUsingPanelGraphics () const
{
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    return ImgPanelGraphics::IsAvailable();
#else
    return false;
#endif
}

bool
ImgWindow::onShow()
{
    return true;
}

void
ImgWindow::SetWindowDragArea (int left, int top, int right, int bottom)
{
    dragLeft    = left;
    dragTop     = top;
    dragRight   = right;
    dragBottom  = bottom;
}

void
ImgWindow::ClearWindowDragArea ()
{
    dragLeft = dragTop = dragRight = dragBottom = -1;
}

bool
ImgWindow::HasWindowDragArea (int* pL, int* pT,
                              int* pR, int* pB) const
{
    // return definition if requested
    if (pL) *pL = dragLeft;
    if (pT) *pT = dragTop;
    if (pR) *pR = dragRight;
    if (pB) *pB = dragBottom;
    
    // is a valid drag area defined?
    return
        dragLeft  >= 0          && dragTop    >= 0 &&
        dragRight >  dragLeft   && dragBottom >= dragTop;
}

bool
ImgWindow::IsInsideWindowDragArea (int x, int y) const
{
    // values outside the window aren't valid
    if (x == -FLT_MAX || y == -FLT_MAX)
        return false;
    
    // is a drag area defined in the first place?
    if (!HasWindowDragArea())
        return false;
    
    // inside the defined drag area?
    return
        dragLeft <= x && x <= dragRight &&
        dragTop  <= y && y <= dragBottom;
}

void ImgWindow::SetTextureBakeDelay(bool enableDelay, int frameCount) {
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
    // Texture bake delay only makes sense for Panel Graphics windows...
    if (!IsUsingPanelGraphics()) {
        return;  // Ignore the request entirely for OpenGL windows.
    }
    mGhostFramesRemaining = enableDelay ? frameCount : 0;
#else
    // Logically, this method isn't useful whatsoever without Panel Graphics.
    // We thus ignore the request here, since it has no effect for OpenGL.
    // (This is provided the caller doesn't need to know whether Panel Graphics is being used or not.)
    (void)enableDelay;
    (void)frameCount;
#endif
}

void
ImgWindow::SafeDelete()
{
    sPendingDestruction.push(this);
    if (sSelfDestructHandler == nullptr) {
        XPLMCreateFlightLoop_t flParams{
            sizeof(flParams),
            xplm_FlightLoop_Phase_BeforeFlightModel,
            &ImgWindow::SelfDestructCallback,
            nullptr,
        };
        sSelfDestructHandler = XPLMCreateFlightLoop(&flParams);
    }
    XPLMScheduleFlightLoop(sSelfDestructHandler, -1, 1);
}

std::queue<ImgWindow *>  ImgWindow::sPendingDestruction;
XPLMFlightLoopID         ImgWindow::sSelfDestructHandler = nullptr;
XPLMFlightLoopID         ImgWindow::sFontAtlasRebuildHandler = nullptr;

float
ImgWindow::SelfDestructCallback(float /*inElapsedSinceLastCall*/,
                                float /*inElapsedTimeSinceLastFlightLoop*/,
                                int   /*inCounter*/,
                                void* /*inRefcon*/)
{
    while (!sPendingDestruction.empty()) {
        auto *thisObj = sPendingDestruction.front();
        sPendingDestruction.pop();
        delete thisObj;
    }
    return 0;
}

#if defined(IMGWINDOW_USE_PANEL_GRAPHICS)
float ImgWindow::FontAtlasRebuildFLCB(float inElapsedSinceLastCall,
                                      float inElapsedTimeSinceLastFlightLoop,
                                      int inCounter,
                                      void *inRefcon)
{
    // Before anything, check whether we are still in the "blankout" period.
    // (If so, skip this frame and wait for the next one.)
    if (XPLMGetCycleNumber() < ImgWindow::sBlankoutUntilCycle) {
        return -1.0f;  // Skip rendering this frame.
    }
    
    // Check if the font atlas needs rebuilding before the draw phase begins.
    // If it's dirty, CheckAndRebuildAtlas will recreate the XPLMCreateTexture resource safely.
    if (sFontAtlas && sFontAtlas->getAtlas()) {
        // We pass a dummy ID reference because the shared atlas texture ID tracker 
        // handles the actual updates underneath inside CheckAndRebuildAtlas.
        void* dummyTexID = (void*)(intptr_t)sFontAtlas->getAtlas()->TexData->GetTexRef().GetTexID();
        CheckAndRebuildAtlas(sFontAtlas->getAtlas(), dummyTexID);
    }
    return -1.0f; // Call every frame
}

/** Support dynamic binding to the panel graphics library.
 *  This allows us to use the panel graphics library if requested when it
 *  is available, or to fall back to the standard OpenGL rendering if
 *  not -- but only if the plugin has defined IMGWINDOW_USE_PANEL_GRAPHICS
 *  in its build configuration.
 */
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

#endif /* IMGWINDOW_USE_PANEL_GRAPHICS */
