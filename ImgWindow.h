/*
 * ImgWindow.h
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018,2020 Christopher Collins
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

#ifndef IMGWINDOW_H
#define IMGWINDOW_H

// Version format: MmmPP (Major, 2-digit Minor, 2-digit Patch)
// e.g., v2.0.0 becomes 20000. v1.12.3 becomes 11203.
#define IMGWINDOW_VERSION       "2.3.0"
#define IMGWINDOW_VERSION_NUM   20300

#include "SystemGL.h"

#include <climits>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <utility>

#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
#include <imgui.h>
#include <queue>

#include "ImgFontAtlas.h"

#if !defined(IMGWINDOW_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
#include "contrib/extensions/Markdown/imgui_markdown.h"
#endif

// Enforce constraint that IMGWINDOW_USE_PANEL_GRAPHICS requires IMGUI_V192_REFACTOR (modern dynamic font atlas support).
#if defined(IMGWINDOW_USE_PANEL_GRAPHICS) && !defined(IMGUI_V192_REFACTOR)
    #error "IMGWINDOW_USE_PANEL_GRAPHICS requires IMGUI_V192_REFACTOR (modern dynamic font atlas support)."
#endif

/** ImgWindow is a Window for creating dear imgui widgets within.
 *
 * There's a few traps to be aware of when using dear imgui with X-Plane:
 *
 * 1) The Dear ImGUI coordinate scheme is inverted in the Y axis vs the X-Plane 
 *    (and OpenGL default) scheme. You must be careful if you're trying to 
 *    directly manipulate positioning of widgets rather than letting imgui 
 *    self-layout.  There are (private) functions in ImgWindow to do the 
 *    coordinate mapping.
 *
 * 2) The Dear ImGUI rendering space is only as big as the window - this means
 *    popup elements cannot be larger than the parent window.  This was
 *    unavoidable on XP11 because of how popup windows work and the possibility
 *    for negative coordinates (which imgui doesn't like).
 *
 * 3) There is no way to detect if the window is hidden without a per-frame
 *    processing loop or similar.
 *
 * @note It should be possible to map globally on XP9 & XP10 letting you run
 *     popups as large as you need, or to use the ImGUI native titlebars instead
 *     of the XP10 ones - source for this may be provided later, but could also
 *     be trivially adapted from this one by adjusting the way the space is
 *     translated and mapped in the DrawWindowCB and constructor.
 */
/** WindowSanitizeMode controls how off-screen or out-of-bounds window coordinates
 * are handled when restoring saved geometry from disk.
 */
enum class WindowSanitizeMode {
    None,              // O.G. style: Do not touch coordinates, leave exactly as passed
    KeepVisibleMargin, // "Poor-Man's Docking": If partially off-screen, keep at least N boxels visible; if 100% lost, snap to center
    SnapToCenter       // Strict: If lost or off-screen, forcibly re-center on the primary monitor
};

/** WindowGeometry is a lightweight parameter object for window bounds.
 * It eliminates inverted-Y boxel math and provides safe coordinate sanitization.
 */
struct WindowGeometry {
    int left   = 0;
    int top    = 0;
    int right  = 0;
    int bottom = 0;

    int Width()  const { return right - left; }
    int Height() const { return top - bottom; }

    // 1. Default (empty)
    WindowGeometry() = default;

    // 2. Direct boxels (left, top, right, bottom)
    WindowGeometry(int l, int t, int r, int b)
        : left(l), top(t), right(r), bottom(b) {}

    // 3. Top-left origin (x, y) + (width, height)
    static WindowGeometry FromTopLeft(int x, int y, int width, int height) {
        return WindowGeometry(x, y, x + width, y - height);
    }

    // 4. Auto-centered on main monitor
    static WindowGeometry Centered(int width, int height, XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle);

    // 5. Smart Auditing & Sanitization
    static WindowGeometry Sanitize(
        int l, int t, int r, int b,
        int defaultW = 640, int defaultH = 480,
        int minVisibleMargin = 40,
        WindowSanitizeMode mode = WindowSanitizeMode::KeepVisibleMargin,
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle);
};

class
ImgWindow {
public:
    /** sFontAtlas is the global shared font-atlas.
     *
     * If you want to share fonts between windows, this needs to be set before
     * any dialogs are actually instantiated.  It will be automatically handed
     * over to the contexts as they're created.
     */
    static std::shared_ptr<ImgFontAtlas> sFontAtlas;

    /** Tracks the window currently executing buildInterface() so that global
     * ImGui:: extensions (like Tooltips or Markdown) can fetch plugin-specific
     * window metadata (like scale multipliers or focus state) natively without
     * polluting their API signatures.
     */
    static ImgWindow* s_CurrentBuildingWindow;
    static ImgWindow* GetCurrentBuildingWindow() { return s_CurrentBuildingWindow; }

    /** Default safety inset in boxels from the top of the X-Plane main screen to keep
     *  the window clear of the X-Plane system menu bar trigger zone. */
    static constexpr int kDefaultTopMenuBarMargin = 30;

    /** Estimated height in boxels of X-Plane's native round-rectangle window title bar
     *  which is rendered ABOVE the window's top content coordinate. */
    static constexpr int kDefaultSystemTitleBarHeight = 25;

    /** Gets the active base top menu bar margin in boxels (default: 30) */
    static int GetTopMenuBarMargin() { return sTopMenuBarMargin; }

    /** Sets the base top menu bar margin in boxels */
    static void SetTopMenuBarMargin(int margin) { sTopMenuBarMargin = margin; }

    /** Gets the estimated native system title bar height in boxels (default: 25) */
    static int GetSystemTitleBarHeight() { return sSystemTitleBarHeight; }

    /** Sets the estimated native system title bar height in boxels */
    static void SetSystemTitleBarHeight(int height) { sSystemTitleBarHeight = height; }

    /** Computes the total required top clearance for a given decoration style:
     *  - Standard decorated (xplm_WindowDecorationRoundRectangle): menu margin + title bar (~55 boxels)
     *  - Self-decorated or None: menu margin (~30 boxels)
     */
    static int GetTopMargin(XPLMWindowDecoration decoration) {
        bool hasSysDec = (decoration == xplm_WindowDecorationRoundRectangle);
        return sTopMenuBarMargin + (hasSysDec ? sSystemTitleBarHeight : 0);
    }

    /** Returns the custom layout scale factor for this window. Subclasses
     * should override this to return their combined text zoom multiplier 
     * (e.g., _fontAdjust * gSeniorCitizenRatio) so extensions can scale 
     * geometry proportionally to match the text.
     * Defaults to (ImGui::GetFontSize() / 14.0f) if not explicitly set,
     * dynamically tracking the ambient font size and Senior Citizen mode.
     */
    virtual float GetExtensionScaleFactor() const {
        if (mCustomExtensionScale > 0.0f) return mCustomExtensionScale;
        float fontSize = ImGui::GetCurrentContext() ? ImGui::GetFontSize() : 14.0f;
        return (fontSize > 0.0f) ? (fontSize / 14.0f) : 1.0f;
    }
    void SetExtensionScaleFactor(float factor) { mCustomExtensionScale = factor; }

    /** sBlankoutUntilCycle is a global XPLM cycle number until which all
     * windows will skip rendering.  This is useful for plugins that want to
     * temporarily blank out all ImGui windows for a cycle or two, e.g., to
     * avoid flicker during a plugin reload.
     */
    static int sBlankoutUntilCycle;

    virtual ~ImgWindow();
    
    /** Gets the current window geometry */
    void GetWindowGeometry (int& left, int& top, int& right, int& bottom) const
    { XPLMGetWindowGeometry(mWindowID, &left, &top, &right, &bottom); }
    
    /** Sets the current window geometry */
    void SetWindowGeometry (int left, int top, int right, int bottom)
    { XPLMSetWindowGeometry(mWindowID, left, top, right, bottom); }

    /** Gets the current window geometry of a popped out window */
    void GetWindowGeometryOS (int& left, int& top, int& right, int& bottom) const
    { XPLMGetWindowGeometryOS(mWindowID, &left, &top, &right, &bottom); }
    
    /** Sets the current window geometry of a popped out window */
    void SetWindowGeometryOS (int left, int top, int right, int bottom)
    { XPLMSetWindowGeometryOS(mWindowID, left, top, right, bottom); }

    /** Gets the current window size of window in VR */
    void GetWindowGeometryVR (int& width, int& height) const
    { XPLMGetWindowGeometryVR(mWindowID, &width, &height); }
    
    /** Sets the current window size of window in VR */
    void SetWindowGeometryVR (int width, int height)
    { XPLMSetWindowGeometryVR(mWindowID, width, height); }
    
    /** Gets the current valid geometry (free, OS, or VR
        If VR, then left=bottom=0 and right=width and top=height */
    void GetCurrentWindowGeometry (int& left, int& top, int& right, int& bottom) const;
    
    /** Set resize limits. If set this way then the window object knows. */
    void SetWindowResizingLimits (int minW, int minH, int maxW, int maxH);

    /** Sets the 1.0x baseline reference dimensions for ResizeToScale().
     *  Initialized automatically to the window's initial construction dimensions. */
    void SetBaselineWindowSize (int width, int height) { mBaselineWidth = width; mBaselineHeight = height; }

    /** Gets the 1.0x baseline reference dimensions */
    void GetBaselineWindowSize (int& width, int& height) const { width = mBaselineWidth; height = mBaselineHeight; }

    /** Sets invariant, unscaled chrome dimensions (e.g. fixed headers, footers, or sidebars)
     *  that should not be multiplied by font scale ratio during proportional window resizing. */
    void SetUnscaledChrome (int fixed_width, int fixed_height) {
        mChromeWidth  = (fixed_width > 0) ? fixed_width : 0;
        mChromeHeight = (fixed_height > 0) ? fixed_height : 0;
    }

    /** Gets invariant, unscaled chrome dimensions */
    void GetUnscaledChrome (int& fixed_width, int& fixed_height) const {
        fixed_width  = mChromeWidth;
        fixed_height = mChromeHeight;
    }
    int  GetUnscaledChromeWidth () const  { return mChromeWidth; }
    int  GetUnscaledChromeHeight () const { return mChromeHeight; }

    /** Pure math converter: scales (cur_w, cur_h) proportionally from from_scale to to_scale,
     *  optionally accounting for invariant unscaled chrome dimensions. */
    static void ConvertSizeForScale (int cur_w, int cur_h, float from_scale, float to_scale, int& out_w, int& out_h,
                                     int chrome_w = 0, int chrome_h = 0);

    /** Proportionally scales current window dimensions from from_scale to to_scale,
     *  anchoring the top-left corner and clamping to min/max window limits and screen bounds.
     *  Mode-aware: handles in-sim 2D, OS popped-out, and VR windows seamlessly. */
    bool ScaleWindowGeometry (float from_scale, float to_scale);

    /** Resizes window proportionally to target_scale based on 1.0x baseline reference dimensions.
     *  If baseline_scale != 1.0f, scales relative to baseline_scale instead. */
    bool ResizeToScale (float target_scale, float baseline_scale = 1.0f);

    /** Clamps window geometry (l, t, r, b) to stay completely within legal screen boundaries,
     *  honoring top menu bar safety margins and min/max limits without mutating aspect ratio.
     *  If top_margin is < 0, defaults to GetTopMargin(xplm_WindowDecorationRoundRectangle). */
    static void ClampGeometryToScreen(int& in_out_l, int& in_out_t, int& in_out_r, int& in_out_b,
                                      int min_w = 0, int min_h = 0, int max_w = 0, int max_h = 0,
                                      int top_margin = -1);

    /** SetVisible() makes the window visible after making the onShow() call.
     * It is also at this time that the window will be relocated onto the VR
     * display if the VR headset is in use.
     *
     * @param inIsVisible true to be displayed, false if the window is to be
     * hidden.
     */
    virtual void SetVisible(bool inIsVisible);

    /** GetVisible() returns the current window visibility.
     * @return true if the window is visible, false otherwise.
    */
    bool GetVisible() const;
    
    /** Is Window popped out */
    bool IsPoppedOut () const { return XPLMWindowIsPoppedOut(mWindowID) != 0; }

    /** Is Window in VR? */
    bool IsInVR () const { return XPLMWindowIsInVR(mWindowID) != 0; }
    
    /** Is Window inside the sim? */
    bool IsInsideSim () const { return !IsPoppedOut() && !IsInVR(); }

    /** Gets the decoration style specified at creation */
    XPLMWindowDecoration GetWindowDecoration() const { return mDecoration; }

    /** Returns true if window uses system window decorations (e.g. round rectangle) */
    bool HasSystemDecoration() const {
        return mDecoration == xplm_WindowDecorationRoundRectangle;
    }

    /** Returns true if window is self-decorated or undecorated (no system title bar) */
    bool IsSelfDecorated() const {
        return mDecoration == xplm_WindowDecorationSelfDecorated ||
               mDecoration == xplm_WindowDecorationSelfDecoratedResizable ||
               mDecoration == xplm_WindowDecorationNone;
    }

    /** Calculates the effective top clearance margin for this window instance,
     *  accounting for both the system menu bar trigger zone and any native window title bar. */
    int GetEffectiveTopMargin() const {
        return GetTopMargin(mDecoration);
    }
    
    /** Is Window using XPLM v4.4+'s Panel Graphics API instead of OpenGL? */
    bool IsUsingPanelGraphics () const;
    
    /** Set the positioning mode
     * @see https://developer.x-plane.com/sdk/XPLMDisplay/#XPLMWindowPositioningMode */
    void SetWindowPositioningMode (XPLMWindowPositioningMode inPosMode,
                                   int                       inMonitorIdx = -1)
    { XPLMSetWindowPositioningMode (mWindowID, inPosMode, inMonitorIdx); }
    
    /** Bring window to front of Z-order */
    void BringWindowToFront () { XPLMBringWindowToFront(mWindowID); }
    
    /** Is Window in front of Z-order? */
    bool IsWindowInFront () const { return XPLMIsWindowInFront(mWindowID) != 0; }
    
    /** Is the window actively being moved or resized by the user? */
    bool IsWindowMoving() const { return (bool)dragWhat; }
    
    /** @brief Define Window drag area, ie. an area in which dragging with the mouse
     * moves the entire window.
     * @details Useful for windows without decoration.
     * For convenience (often you want a strip at the top of the window to be the drag area,
     * much like a little title bar), coordinates originate in the top-left
     * corner of the window and go right/down, ie. vertical axis is contrary
     * to what X-Plane usually uses, but in line with the ImGui system.
     * Right/Bottom can be set much large than window size just to extend the
     * drag area to the window's edges. So 0,0,INT_MAX,INT_MAX will surely
     * make the entire window the drag area.
     * @param left Left begin of drag area, relative to window's origin
     * @param top Ditto, top begin
     * @param right Ditto, right end
     * @param bottom Ditto, bottom end
     */
    void SetWindowDragArea (int left=0, int top=0, int right=INT_MAX, int bottom=INT_MAX);
    
    /** Clear the drag area, ie. stop the drag-the-window functionality */
    void ClearWindowDragArea ();
    
    /** Is a drag area defined? Return its sizes if wanted */
    bool HasWindowDragArea (int* pL = nullptr, int* pT = nullptr,
                            int* pR = nullptr, int* pB = nullptr) const;
    
    /** Is given position inside the defined drag area?
     * @param x Horizontal position in ImGui coordinates (0,0 in top/left corner)
     * @param y Vertical position in ImGui coordinates
     */
    bool IsInsideWindowDragArea (int x, int y) const;
    
#ifdef IMGUI_V192_REFACTOR
    [[deprecated("SafeDeleteTexture is obsolete in Phase 6 Redux. X-Plane 12.4.4b3+ permits synchronous destruction. Please migrate to the new unified DeleteTexture() method.")]]
    void SafeDeleteTexture(ImTextureID texture) {
        DeleteTexture(texture);
    }
#endif /* IMGUI_V192_REFACTOR */

    /** Unified API to create and destroy custom textures dynamically 
     *  using either Panel Graphics (if available) or OpenGL fallback.
     */
    static ImTextureID CreateTexture(const unsigned char* pixels, int width, int height);
    static void DeleteTexture(ImTextureID textureID);

    /** Clear ActiveID early if an InputText currently holds focus and the user clicked outside its bounding rect.
     *  Automatically invoked every frame in updateImgui() right after ImGui::NewFrame().
     *  Can also be called manually by custom event loops or toolbar renderers.
     *  @return true if ActiveId was cleared, false otherwise. */
    static bool ClearActiveInputTextIfClickedOutside();

    /** Keyboard focus management methods */
    void SetExplicitKeyboardFocus(bool req) { bExplicitKeyboardFocus = req; }
    bool GetExplicitKeyboardFocus() const { return bExplicitKeyboardFocus; }
    void SetCaptureKeyboardOnMouseFocus(bool enable) { bCaptureKeyboardOnMouseFocus = enable; }
    bool GetCaptureKeyboardOnMouseFocus() const { return bCaptureKeyboardOnMouseFocus; }

    // -----------------------------------------------------------------------
    // DEPRECATED ALIASES (Retained for API backwards compatibility for now)
    // -----------------------------------------------------------------------
    [[deprecated("Use CreateTexture() instead. The 'Custom' component of the method name is obsolete.")]]
    static inline ImTextureID CreateCustomTexture(const unsigned char* pixels, int width, int height)
    { return CreateTexture(pixels, width, height); }
    // -----------------------------------------------------------------------
    [[deprecated("Use DeleteTexture() instead. The 'Custom' component of the method name is obsolete.")]]
    static inline void DestroyCustomTexture(ImTextureID textureID)
    { DeleteTexture(textureID); }
    // -----------------------------------------------------------------------

    /** Flushes all pending texture destructions immediately. 
     *  Must be called during XPluginStop/XPluginDisable to prevent VRAM leaks. */
    static void Shutdown();

#if !defined(IMGWINDOW_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
    // ------------------------------------------------------------------------
    // Markdown Image Services & Texture Registry (Tiers 1, 2 & 3)
    // ------------------------------------------------------------------------
    /** Tier 1: In-Memory Named Texture Registry
     *  Allows plugins to register pre-loaded textures by name or URI (e.g. "my_logo" or "texture://my_logo")
     *  so Markdown references like ![Logo](texture://my_logo) or ![Logo](my_logo) resolve instantly. */
    static void RegisterMarkdownNamedTexture(const std::string& name, ImTextureID textureID, ImVec2 size = ImVec2(0.0f, 0.0f), ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f), ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4 border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    static void UnregisterMarkdownNamedTexture(const std::string& name);
    static bool GetMarkdownNamedTexture(const std::string& name, ImGui::MarkdownImageData& outData);

    /** Tier 2: Built-in Default Markdown Image Callback
     *  Automatically resolves named textures, loads local disk image files via stb_image,
     *  uploads 4-channel RGBA textures via ImgWindow::CreateTexture(), and caches them in VRAM.
     *  Includes negative caching for missing files and automatic cleanup on Shutdown(). */
    static ImGui::MarkdownImageData DefaultMarkdownImageCallback(ImGui::MarkdownLinkCallbackData data);

    /** Enable or disable the built-in default image callback for all Markdown renderers.
     *  Automatically enabled when an ImgWindow is instantiated. */
    static void EnableDefaultMarkdownImageService(bool enable = true);

    /** Tier 3: Asynchronous Remote Web Image Fetching (HTTP / HTTPS)
     *  Downloads remote images on a background worker thread, decodes off-thread via stb_image,
     *  and safely transfers the decoded pixel buffer to the main simulator thread for GPU upload. */
    typedef bool (*MarkdownRemoteFetchCallback)(const std::string& url, std::vector<uint8_t>& outData);

    /** Sets an optional custom remote image fetch callback (e.g. for plugins with their own HTTP managers) */
    static void SetMarkdownRemoteFetchCallback(MarkdownRemoteFetchCallback callback);

    /** Gets the active custom remote image fetch callback, if any */
    static MarkdownRemoteFetchCallback GetMarkdownRemoteFetchCallback();

    /** Processes any downloaded remote markdown textures waiting to be uploaded to the GPU.
     *  Called automatically every frame during markdown rendering and DrawWindowCB. */
    static void ProcessPendingMarkdownTextures();

    /** Destroys all GPU textures loaded by the markdown image cache. */
    static void ClearMarkdownTextureCache();
#endif

    [[deprecated("Texture bake delays are no longer required. X-Plane 12.4.4b3+ synchronously builds textures instantly. This method is a no-op and can be safely removed.")]]
    void SetTextureBakeDelay(bool enableDelay, int frameCount = 2) {
        (void)enableDelay;
        (void)frameCount;
    }

protected:
    bool mIsPendingDestruction = false;

    /** mFirstRender can be checked during buildInterface() to see if we're
     * being rendered for the first time or not.  This is particularly
     * important for windows that use Columns as SetColumnWidth() should only
     * be called once.
     *
     * There may be other times where it's advantageous to make specific ImGui
     * calls once and once only.
     */
    bool mFirstRender;

    /** Construct a window with the specified bounds
     *
     * @param left Left edge of the window's contents in global boxels.
     * @param top Top edge of the window's contents in global boxels.
     * @param right Right edge of the window's contents in global boxels.
     * @param bottom Bottom edge of the window's contents in global boxels.
     * @param decoration The decoration style to use (see notes)
     * @param layer the preferred layer to present this window in (see notes)
     * @param cursors switch to ImGui mouse cursors inside window  (brat)
     *
     * @note The decoration should generally be one presented/rendered by XP -
     *     the ImGui window decorations are very intentionally supressed by
     *     ImgWindow to allow them to fit in with the rest of the simulator.
     *
     * @note The only layers that really make sense are Floating and Modal.  Do
     *     not set VR layer here however unless the window is ONLY to be
     *     rendered in VR.
     */
    ImgWindow(
        int left,
        int top,
        int right,
        int bottom,
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
        bool cursors = false);

    /** Construct a window using a WindowGeometry parameter object.
     *
     * @param geometry WindowGeometry parameter object containing boxel coordinates.
     * @param decoration The decoration style to use.
     * @param layer The preferred layer to present this window in.
     * @param cursors Switch to ImGui mouse cursors inside window.
     */
    ImgWindow(
        const WindowGeometry& geometry,
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
        bool cursors = false)
        : ImgWindow(geometry.left, geometry.top, geometry.right, geometry.bottom, decoration, layer, cursors)
    {}

    /** Construct a window auto-centered on the main monitor by width and height.
     *
     * @param width Desired window width in boxels.
     * @param height Desired window height in boxels.
     * @param decoration The decoration style to use.
     * @param layer The preferred layer to present this window in.
     * @param cursors Switch to ImGui mouse cursors inside window.
     */
    ImgWindow(
        int width,
        int height,
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
        bool cursors = false)
        : ImgWindow(WindowGeometry::Centered(width, height, decoration), decoration, layer, cursors)
    {}
    
    /** An ImgWindow object must not be copied!
     */
    ImgWindow (const ImgWindow&) = delete;
    ImgWindow& operator = (const ImgWindow&) = delete;

    /** SetWindowTitle sets the title of the window both in the ImGui layer and
     * at the XPLM layer.
     *
     * @param title the title to set.
     */
    void SetWindowTitle(const std::string &title);

    /** moveForVR() is an internal helper for moving the window to either it's
     * preferred layer or the VR layer depending on if the headset is in use.
     */
    void moveForVR();
    
    /** A hook called right before ImGui::Begin in case you want to set up something
     * before interface building begins
     * @return addition flags to be passed to the imgui::begin() call,
     *         like for example ImGuiWindowFlags_MenuBar */
    virtual ImGuiWindowFlags_ beforeBegin() { return ImGuiWindowFlags_None; }

    /** buildInterface() is the method where you can define your ImGui interface
     * and handle events.  It is called every frame the window is drawn.
     *
     * @note You must NOT delete the window object inside buildInterface() -
     *     use SafeDelete() for that.
     */
    virtual void buildInterface() = 0;

    /** A hook called after all rendering is done, right before the
     * X-Plane window draw call back returns
     * in case you want to do something that otherwise would conflict with rendering. */
    virtual void afterRendering() {}

    /** onShow() is called before making the Window visible.  It provides an
     * opportunity to prevent the window being shown.
     *
     * @note the implementation in the base-class is a null handler.  You can
     *     safely override this without chaining.
     *
     * @return true if the Window should be shown, false if the attempt to show
     *     should be suppressed.
     */
    virtual bool onShow();

    /** SafeDelete() can be used within buildInterface() to get the object to
     *     self-delete once it's finished rendering this frame.
     */
    void SafeDelete();
    
    /** Returns X-Plane's internal Window id */
    XPLMWindowID GetWindowId () const { return mWindowID; }

    /** Enables/disables ImGui handling of all mouse cursors inside window. */
    inline void SetImgCursorHandling (bool inIsEnabled)
    { bUseImgCursors = inIsEnabled; }

    /** Returns whether ImGui handling of mouse cursors is enabled for this window. */
    inline bool GetImgCursorHandling () const
    { return bUseImgCursors; }

private:
    std::shared_ptr<ImgFontAtlas> mFontAtlas;

    static void DrawWindowCB(XPLMWindowID inWindowID, void *inRefcon);

    static int HandleMouseClickCB(
        XPLMWindowID inWindowID,
        int x, int y,
        XPLMMouseStatus inMouse,
        void *inRefcon);

    static void HandleKeyFuncCB(
        XPLMWindowID inWindowID,
        char inKey,
        XPLMKeyFlags inFlags,
        char inVirtualKey,
        void *inRefcon,
        int losingFocus);

    static XPLMCursorStatus HandleCursorFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        void *inRefcon);

    static int HandleMouseWheelFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        int wheel,
        int clicks,
        void *inRefcon);

    static int HandleRightClickFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        XPLMMouseStatus inMouse,
        void *inRefcon);

    static float SelfDestructCallback(float inElapsedSinceLastCall,
                                      float inElapsedTimeSinceLastFlightLoop,
                                      int inCounter,
                                      void *inRefcon);
    static std::queue<ImgWindow *>  sPendingDestruction;
    static XPLMFlightLoopID         sSelfDestructHandler;


    int HandleMouseClickGeneric(
        int x, int y,
        XPLMMouseStatus inMouse,
        int button = 0);

    void RenderImGui(ImDrawData *draw_data);

    void updateImgui();

    void updateMatrices();

    void boxelsToNative(int x, int y, int &outX, int &outY);

    void translateImguiToBoxel(float inX, float inY, int &outX, int &outY);

    void translateToImguiSpace(int inX, int inY, float &outX, float &outY);

    float mModelView[16], mProjection[16];
    int mViewport[4];

    std::string mWindowTitle;

    XPLMWindowID mWindowID;
    ImGuiContext *mImGuiContext;
    ImTextureID mFontTexture = (ImTextureID)0;

    int mTop;
    int mBottom;
    int mLeft;
    int mRight;

    /** Shall window be locked in place, e.g. invisible full-screen root container? */
    bool bCanMove = true;

    /** Shall reset the backspace key? (see HandleKeyFuncCB for details) */
    bool bResetBackspace = false;

    /** Set if `xplm_WindowDecorationSelfDecoratedResizable`,
     *  ie. we need to handle resizing ourselves: X-Plane provides
     *  the "hand" mouse icon but as we catch mouse events X-Plane
     *  cannot handle resizing. (And passing on mouse events is
     *  discouraged.
     *  @see https://developer.x-plane.com/sdk/XPLMHandleMouseClick_f/ */
    const bool bHandleWndResize;

    XPLMWindowDecoration mDecoration = xplm_WindowDecorationRoundRectangle;
    XPLMWindowLayer mPreferredLayer;

    /** Shall window allow ImGui to manage the mouse cursor inside windows? */
    bool bUseImgCursors = false;

    /** Resize limits. There's no way to query XP, so we need to keep track ourself */
    int minWidth    = 100;
    int minHeight   = 100;
    int maxWidth    = INT_MAX;
    int maxHeight   = INT_MAX;

    /** 1.0x baseline reference dimensions for ResizeToScale() */
    int mBaselineWidth  = 0;
    int mBaselineHeight = 0;

    /** Invariant unscaled chrome dimensions (fixed headers, footers, sidebars) */
    int mChromeWidth    = 0;
    int mChromeHeight   = 0;

    /** Active top menu bar margin in boxels */
    static int sTopMenuBarMargin;

    /** Estimated native system title bar height in boxels */
    static int sSystemTitleBarHeight;

    /** Window drag area in ImGui coordinates (0,0 is top/left corner) */
    int dragLeft    = -1;
    int dragTop     = -1;
    int dragRight   = -1;       // right > left
    int dragBottom  = -1;       // bottom > top!

    /** Last (processed) mouse drag pos while moving/resizing */
    int lastMouseDragX  = -1;
    int lastMouseDragY  = -1;

#ifdef IMGUI_V190_REFACTOR
private:
    int bLastKeyboardFocused = 0;  // last known keyboard focus state from XPLMHasKeyboardFocus
#endif /* IMGUI_V190_REFACTOR */

protected:
    bool bExplicitKeyboardFocus = false;
    bool bCaptureKeyboardOnMouseFocus = false;
    bool bHasMouseFocus = false;
    float mCustomExtensionScale = 0.0f;

    /** What are we dragging right now? */
    struct DragTy {
        bool wnd    : 1;
        bool left   : 1;
        bool top    : 1;
        bool right  : 1;
        bool bottom : 1;
        
        DragTy () { clear(); }
        void clear () { wnd = left = top = right = bottom = false; }
        operator bool() const { return wnd || left || top || right || bottom; }
    } dragWhat;
};
 
#endif // #ifndef IMGWINDOW_H
