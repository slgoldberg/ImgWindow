// ============================================================================
// ImgWindow Contrib: Turnkey X-Plane Markdown Manual Window (ImgDocWindow.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE)
//
// Optional header-only X-Plane convenience wrapper subclassing ImgWindow and
// hosting ImGee's standalone ImGui::DocumentViewer (contrib/imgee.h).
//
// Automatically wires X-Plane window chrome and lifecycle callbacks:
//   - Proportional window resizing on zoom & [Fit] (via ScaleWindowGeometry /
//     ResizeToScale with unscaled 1.0x header + footer chrome)
//   - Deferred flight-loop Pop-Out <-> In-Sim <-> VR window mode transitions
//     with automatic in-sim boxel geometry save/restore
//   - Self-decorated top avionics drag grip (kept clear of Row 1 controls)
//   - Dynamic ImGui cursor toggle (SetImgCursorHandling)
//   - Plugin-relative Markdown & companion PDF path resolution
//
// Core ImgWindow.h remains 100% decoupled from ImGee -- include this header
// only in translation units that want a turnkey X-Plane manual/help window.
// ============================================================================

#pragma once

#ifndef IMGWINDOW_CONTRIB_IMGDOCWINDOW_H
#define IMGWINDOW_CONTRIB_IMGDOCWINDOW_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#include "../ImgWindow.h"
#include "imgee.h"

#if defined(IMGUI_DOC_VIEWER_H)

#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
#include <XPLMPlugin.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <sys/stat.h>

#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif

class ImgDocWindow : public ImgWindow {
public:
    static constexpr int kDefaultWidth     = 760;
    static constexpr int kDefaultHeight    = 680;
    static constexpr int kDefaultMinWidth  = 480;
    static constexpr int kDefaultMinHeight = 380;
    static constexpr int kDefaultMaxWidth  = 2560;
    static constexpr int kDefaultMaxHeight = 1600;
    static constexpr int kDefaultChromeH   = 78;

    explicit ImgDocWindow(
        const std::string& windowTitle = "Documentation",
        int width = kDefaultWidth,
        int height = kDefaultHeight,
        XPLMWindowDecoration decoration = xplm_WindowDecorationSelfDecoratedResizable,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
        bool cursors = true)
        : ImgWindow(width, height, decoration, layer, cursors)
    {
        initDocWindow(windowTitle, width, height, cursors);
    }

    ImgDocWindow(
        const std::string& windowTitle,
        const WindowGeometry& geometry,
        XPLMWindowDecoration decoration = xplm_WindowDecorationSelfDecoratedResizable,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
        bool cursors = true)
        : ImgWindow(geometry, decoration, layer, cursors)
    {
        const int initW = (std::max)(geometry.right - geometry.left, kDefaultMinWidth);
        const int initH = (std::max)(geometry.top - geometry.bottom, kDefaultMinHeight);
        initDocWindow(windowTitle, initW, initH, cursors);
    }

    ~ImgDocWindow() override
    {
#if defined(XPLM210)
        if (mFlightLoopId) {
            XPLMDestroyFlightLoop(mFlightLoopId);
            mFlightLoopId = nullptr;
        }
#else
        if (mLegacyFlightLoopRegistered) {
            XPLMUnregisterFlightLoopCallback(&ImgDocWindow::flightLoopCB, this);
            mLegacyFlightLoopRegistered = false;
        }
#endif
    }

    // ------------------------------------------------------------------------
    // Document Content & File Loading
    // ------------------------------------------------------------------------

    /** Sets in-memory Markdown content directly, with an optional base directory
     *  used to resolve relative inline image paths. */
    void SetMarkdownText(const std::string& markdown, const std::string& baseDir = std::string())
    {
        mMarkdownText = markdown;
        mSourceFilePath.clear();
        mBaseDir = baseDir;
        mConfig.showReloadButton = false;
        mState.InvalidateTocCache();
    }

    /** Sets fallback Markdown text shown if LoadMarkdownFile() does not find a file on disk. */
    void SetFallbackMarkdown(const std::string& fallbackMarkdown)
    {
        mFallbackMarkdown = fallbackMarkdown;
        if (mMarkdownText.empty() && mSourceFilePath.empty()) {
            mMarkdownText = mFallbackMarkdown;
            mState.InvalidateTocCache();
        }
    }

    /** Loads a UTF-8 Markdown file from disk. Relative paths are automatically
     *  searched relative to the calling X-Plane plugin's root directory, binary
     *  directory, and the simulator working directory. Automatically configures
     *  baseDir for relative images and enables the Row 2 [Reload] button. */
    bool LoadMarkdownFile(const std::string& pathOrRelPath)
    {
        mRequestedFilePath = pathOrRelPath;
        const std::string resolved = ResolvePluginPath(pathOrRelPath);
        if (!resolved.empty() && readUtf8File(resolved, mMarkdownText)) {
            mSourceFilePath = resolved;
            mBaseDir = extractParentDir(resolved);
            mConfig.showReloadButton = true;
            struct stat st;
            mLastMtime = (stat(resolved.c_str(), &st) == 0) ? st.st_mtime : 0;
            mAutoReloadAccum = 0.0f;
            mState.InvalidateTocCache();
            return true;
        }

        mSourceFilePath.clear();
        if (!mFallbackMarkdown.empty()) {
            mMarkdownText = mFallbackMarkdown;
        }
        mState.InvalidateTocCache();
        return false;
    }

    /** Reloads the active Markdown file from disk and clears cached Markdown textures. */
    bool ReloadDocument()
    {
#if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
        ImGui::MarkdownImageService::ClearTextureCache();
#endif
        if (!mRequestedFilePath.empty()) {
            return LoadMarkdownFile(mRequestedFilePath);
        }
        mState.InvalidateTocCache();
        return false;
    }

    /** Configures an optional companion PDF manual path or URL (enables [PDF] button in Row 1).
     *  Local relative paths are resolved against the calling plugin's folder and shown only if found. */
    void SetCompanionPdfPath(const std::string& pdfPathOrUrl)
    {
        if (pdfPathOrUrl.empty()) {
            mPdfPath.clear();
            return;
        }
        if (pdfPathOrUrl.compare(0, 7, "http://") == 0 ||
            pdfPathOrUrl.compare(0, 8, "https://") == 0 ||
            pdfPathOrUrl.compare(0, 7, "file://") == 0)
        {
            mPdfPath = pdfPathOrUrl;
            return;
        }
        mPdfPath = ResolvePluginPath(pdfPathOrUrl);
    }

    const std::string& GetDocumentPath() const { return mSourceFilePath; }
    const std::string& GetBaseDir() const { return mBaseDir; }
    const std::string& GetCompanionPdfPath() const { return mPdfPath; }

    // ------------------------------------------------------------------------
    // Viewer State, Theme, Zoom & Navigation Accessors
    // ------------------------------------------------------------------------

    ImGui::DocumentViewerState&       GetViewerState()       { return mState; }
    const ImGui::DocumentViewerState& GetViewerState() const { return mState; }

    ImGui::DocumentViewerConfig&       GetViewerConfig()       { return mConfig; }
    const ImGui::DocumentViewerConfig& GetViewerConfig() const { return mConfig; }

    void SetScaleWindowOnZoom(bool enable) { mScaleWindowOnZoom = enable; }
    bool GetScaleWindowOnZoom() const { return mScaleWindowOnZoom; }

    void SetEnableTopDragGrip(bool enable) { mEnableTopDragGrip = enable; }
    bool GetEnableTopDragGrip() const { return mEnableTopDragGrip; }

    void SetAutoReloadOnDiskChange(bool enable) { mAutoReloadOnDiskChange = enable; }
    bool GetAutoReloadOnDiskChange() const { return mAutoReloadOnDiskChange; }

    void SetZoom(float zoom, bool resizeWindow = false)
    {
        const float oldZoom = mState.viewport.zoom;
        if (mState.viewport.SetZoom(zoom) && resizeWindow && mScaleWindowOnZoom) {
            ScaleWindowGeometry(oldZoom, mState.viewport.zoom);
        }
    }
    float GetZoom() const { return mState.viewport.zoom; }

    void SetTheme(int themeIndex)
    {
        mState.theme_index = ImClamp(themeIndex, 0, ImGui::DocumentViewerTheme_COUNT - 1);
        ImGuiContext* prevCtx = ImGui::GetCurrentContext();
        if (mDocImGuiContext) ImGui::SetCurrentContext(mDocImGuiContext);
        ImGui::ApplyDocumentViewerTheme(
            mState.theme_index,
            mState.transparent_hud && IsSelfDecorated(),
            mState.window_opacity);
        if (prevCtx) ImGui::SetCurrentContext(prevCtx);
    }
    int GetTheme() const { return mState.theme_index; }

    void SetWindowOpacity(float opacity)
    {
        mState.window_opacity = ImClamp(opacity, 0.10f, 1.00f);
        SetTheme(mState.theme_index);
    }
    float GetWindowOpacity() const { return mState.window_opacity; }

    void SetTransparentHud(bool enable)
    {
        mState.transparent_hud = enable;
        SetTheme(mState.theme_index);
    }
    bool GetTransparentHud() const { return mState.transparent_hud; }

    bool JumpToAnchor(const char* anchorOrSlug)
    {
        return ImGui::JumpToDocumentAnchor(mState, anchorOrSlug);
    }

    bool JumpToSection(int tocIndex)
    {
        return ImGui::JumpToDocumentHeading(mState, tocIndex);
    }

    /** Resolves a path against (1) direct path, (2) <PluginRoot>/<relPath>, and (3) <PluginBinDir>/<relPath>. */
    static std::string ResolvePluginPath(const std::string& pathOrRel)
    {
        if (pathOrRel.empty()) return std::string();
        if (fileExistsRegular(pathOrRel)) return pathOrRel;

        std::vector<std::string> roots;
        const XPLMPluginID myId = XPLMGetMyID();
        if (myId != XPLM_NO_PLUGIN_ID) {
            char rawPluginPath[512] = {0};
            XPLMGetPluginInfo(myId, nullptr, rawPluginPath, nullptr, nullptr);
            std::string pPath = normalizePluginPath(rawPluginPath);
            if (!pPath.empty()) {
                const std::string binDir = extractParentDir(pPath);
                if (!binDir.empty()) {
                    const std::string parentDir = extractParentDir(binDir);
                    if (!parentDir.empty()) roots.push_back(parentDir);
                    roots.push_back(binDir);
                }
            }
        }

        for (size_t i = 0; i < roots.size(); ++i) {
            const std::string candidate = joinPath(roots[i], pathOrRel);
            if (fileExistsRegular(candidate)) {
                return candidate;
            }
        }
        return std::string();
    }

protected:
    ImGuiWindowFlags_ beforeBegin() override
    {
        if (mFirstRender) {
            ImGui::ApplyDocumentViewerTheme(
                mState.theme_index,
                mState.transparent_hud && IsSelfDecorated(),
                mState.window_opacity);
        }
        return static_cast<ImGuiWindowFlags_>(
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }

    float getFontAdjust() const override
    {
        return mState.viewport.zoom;
    }

    /** Optional subclass notification hooks (e.g. to persist settings or sync menu checkmarks) */
    virtual void onDocumentClosed() {}
    virtual void onDocumentZoomChanged(float /*oldZoom*/, float /*newZoom*/) {}
    virtual void onDocumentThemeChanged(int /*themeIndex*/, bool /*transparentHud*/, float /*opacity*/) {}

    void buildInterface() override
    {
        mDocImGuiContext = ImGui::GetCurrentContext();
        mState.is_open = true;

        // 0. Optional throttled mtime hot-reload when editing Markdown files on disk
        if (mAutoReloadOnDiskChange && !mSourceFilePath.empty()) {
            mAutoReloadAccum += ImGui::GetIO().DeltaTime;
            if (mAutoReloadAccum >= 1.0f) {
                mAutoReloadAccum = 0.0f;
                struct stat st;
                if (stat(mSourceFilePath.c_str(), &st) == 0 && st.st_mtime != mLastMtime) {
                    ReloadDocument();
                }
            }
        }

        // 1. Optional Top Avionics Glass Drag Handle (Self-Decorated In-Sim Mode)
        updateSelfDecoratedDragGrip();

        // 2. Sync live X-Plane host state into DocumentViewerConfig
        ImGui::DocumentViewerConfig cfg = mConfig;
        if (!cfg.title || cfg.title[0] == '\0') {
            cfg.title = mDocTitle.c_str();
        }
        if ((!cfg.baseDir || cfg.baseDir[0] == '\0') && !mBaseDir.empty()) {
            cfg.baseDir = mBaseDir.c_str();
        }
        if ((!cfg.externalPdfPath || cfg.externalPdfPath[0] == '\0') && !mPdfPath.empty()) {
            cfg.externalPdfPath = mPdfPath.c_str();
        }
        cfg.isPoppedOut        = IsPoppedOut();
        cfg.isInVr             = IsInVR();
        cfg.showOpacityControls = mConfig.showOpacityControls && IsSelfDecorated();
        cfg.userData           = this;

        cfg.onZoomChanged         = &ImgDocWindow::cbZoomChanged;
        cfg.onFitRequested        = &ImgDocWindow::cbFitRequested;
        cfg.onCursorToggled       = &ImgDocWindow::cbCursorToggled;
        cfg.onPopOutToggled       = &ImgDocWindow::cbPopOutToggled;
        cfg.onVrRequested         = &ImgDocWindow::cbVrRequested;
        cfg.onCloseRequested      = &ImgDocWindow::cbCloseRequested;
        cfg.onReloadRequested     = &ImgDocWindow::cbReloadRequested;
        cfg.onTransparencyToggled = &ImgDocWindow::cbTransparencyToggled;
        cfg.onOpacityChanged      = &ImgDocWindow::cbOpacityChanged;
        cfg.onThemeChanged        = &ImgDocWindow::cbThemeChanged;

        // 3. Inset inside X-Plane's 15px self-resize grab border when in-sim & self-resizable
        //    so [X], Theme combo, vertical scrollbar, and footer stay 100% clickable.
        ImVec2 viewerSize(0.0f, 0.0f);
        if (IsInsideSim() && GetWindowDecoration() == xplm_WindowDecorationSelfDecoratedResizable) {
            const float padX   = 16.0f;
            const float padTop = 10.0f;
            const float padBot = 16.0f;
            const ImVec2 winSz = ImGui::GetWindowSize();
            ImGui::SetCursorPos(ImVec2(padX, padTop));
            viewerSize = ImVec2(
                (std::max)(64.0f, winSz.x - padX * 2.0f),
                (std::max)(64.0f, winSz.y - padTop - padBot));
        }

        // 4. Render composite 2-row header + Nx zoomed Markdown viewport + 1.0x footer
        ImGui::DocumentViewer(
            "##img_doc_window_viewer",
            mMarkdownText.c_str(),
            mMarkdownText.size(),
            mState,
            &cfg,
            viewerSize);
    }

private:
    void initDocWindow(const std::string& windowTitle, int initW, int initH, bool cursors)
    {
        mDocTitle = windowTitle;
        SetWindowTitle(windowTitle);
        SetWindowResizingLimits(kDefaultMinWidth, kDefaultMinHeight, kDefaultMaxWidth, kDefaultMaxHeight);
        SetBaselineWindowSize(initW, initH);
        SetUnscaledChrome(0, kDefaultChromeH);

        mState.dynamic_cursors_enabled = cursors;
        mState.transparent_hud         = IsSelfDecorated();

        mConfig.showFitButton    = true;
        mConfig.showCursorToggle = true;
        mConfig.showPopOutButton = true;
        mConfig.showVrButton     = true;
        mConfig.showCloseButton  = true;
        mConfig.showFooter       = true;

        GetWindowGeometry(mSavedSimLeft, mSavedSimTop, mSavedSimRight, mSavedSimBottom);
        mHasSavedSimGeometry = (mSavedSimRight > mSavedSimLeft && mSavedSimTop > mSavedSimBottom);

        ImGuiContext* prevCtx = ImGui::GetCurrentContext();
        ImGuiContext* ownCtx  = getOwnImGuiContext();
        if (ownCtx) {
            mDocImGuiContext = ownCtx;
            ImGui::SetCurrentContext(ownCtx);
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.LogFilename = nullptr;
            ImGui::ApplyDocumentViewerTheme(
                mState.theme_index,
                mState.transparent_hud && IsSelfDecorated(),
                mState.window_opacity);
        }
        if (prevCtx) {
            ImGui::SetCurrentContext(prevCtx);
        }
    }

    ImGuiContext* getOwnImGuiContext() const
    {
        // ImgWindow activates its context during construction; grab if still current,
        // otherwise mDocImGuiContext is latched on first buildInterface() frame.
        return ImGui::GetCurrentContext();
    }

    void updateSelfDecoratedDragGrip()
    {
        if (!mEnableTopDragGrip || !IsSelfDecorated() || IsPoppedOut() || IsInVR()) {
            ClearWindowDragArea();
            return;
        }

        const ImGuiStyle& style = ImGui::GetStyle();
        const float winW = ImGui::GetWindowWidth();
        const float centerLocal = winW * 0.5f;

        // Estimate Row 1 title right edge and Row 1 button suite left edge so the
        // drag zone never overlaps interactive Row 1 buttons or the Zoom slider.
        const std::string& ttl = !mDocTitle.empty() ? mDocTitle : std::string("Documentation");
        const float titleEstW  = ImGui::CalcTextSize(ttl.c_str()).x + 34.0f;
        const float minLeft    = 16.0f + titleEstW + 8.0f;
        const float suiteEstW  = 356.0f + (mPdfPath.empty() ? 0.0f : 44.0f);
        const float maxRight   = winW - suiteEstW - 16.0f - 8.0f;

        // Start at y = 6 so y = 0..5 stays reserved for X-Plane's top self-resize grab strip
        int dragT = (GetWindowDecoration() == xplm_WindowDecorationSelfDecoratedResizable) ? 6 : 0;
        int dragL = static_cast<int>(std::max(centerLocal - 56.0f, minLeft));
        int dragR = static_cast<int>(std::min(centerLocal + 56.0f, maxRight));
        int dragB = 24;

        if (dragR - dragL < 28) {
            // Narrow window fallback: thin top strip between resize border (y=6) and Row 1 (y=10)
            dragL = static_cast<int>((std::max)(16.0f, style.WindowPadding.x));
            dragR = static_cast<int>((std::max)(40.0f, winW - 16.0f));
            dragB = 10;
        }

        SetWindowDragArea(dragL, dragT, dragR, dragB);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 winPos = ImGui::GetWindowPos();
        if (dragB >= 16) {
            const float gripCenterX = winPos.x + 0.5f * static_cast<float>(dragL + dragR);
            const float gripHalfW   = (std::min)(24.0f, 0.4f * static_cast<float>(dragR - dragL));
            dl->AddRectFilled(
                ImVec2(gripCenterX - gripHalfW, winPos.y + 5.5f),
                ImVec2(gripCenterX + gripHalfW, winPos.y + 9.0f),
                IM_COL32(255, 255, 255, 95),
                2.0f);
        }

        const ImVec2 gripMin(winPos.x + static_cast<float>(dragL), winPos.y + static_cast<float>(dragT));
        const ImVec2 gripMax(winPos.x + static_cast<float>(dragR), winPos.y + static_cast<float>(dragB));
        const bool gripHovered = ImGui::IsMouseHoveringRect(gripMin, gripMax) && !IsWindowMoving();
        if ((gripHovered || IsWindowMoving()) && mState.dynamic_cursors_enabled) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
#if defined(IMGUI_TOOLTIP_EXT_H) && defined(IMGUI_MARKDOWN_EXT_H) && \
    !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
        ImGui::TimedTooltip::TextMDProxy(
            ImGui::GetID("##img_doc_window_drag_grip"),
            gripHovered,
            "**Window Drag Handle**\n\nLeft-drag here to move, or **right-drag anywhere** inside the window.");
#endif
    }

    void schedulePositionMode(XPLMWindowPositioningMode mode)
    {
        mPendingPosMode = static_cast<int>(mode);
#if defined(XPLM210)
        if (!mFlightLoopId) {
            XPLMCreateFlightLoop_t flDef;
            std::memset(&flDef, 0, sizeof(flDef));
            flDef.structSize   = sizeof(flDef);
            flDef.phase        = xplm_FlightLoop_Phase_BeforeFlightModel;
            flDef.callbackFunc = &ImgDocWindow::flightLoopCB;
            flDef.refcon       = this;
            mFlightLoopId      = XPLMCreateFlightLoop(&flDef);
        }
        if (mFlightLoopId) {
            XPLMScheduleFlightLoop(mFlightLoopId, -1.0f, 1);
        }
#else
        if (!mLegacyFlightLoopRegistered) {
            XPLMRegisterFlightLoopCallback(&ImgDocWindow::flightLoopCB, -1.0f, this);
            mLegacyFlightLoopRegistered = true;
        } else {
            XPLMSetFlightLoopCallbackInterval(&ImgDocWindow::flightLoopCB, -1.0f, 1, this);
        }
#endif
    }

    static float flightLoopCB(float, float, int, void* inRefcon)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(inRefcon);
        if (!self || self->mPendingPosMode < 0) {
            return 0.0f;
        }

        const XPLMWindowPositioningMode targetMode =
            static_cast<XPLMWindowPositioningMode>(self->mPendingPosMode);
        self->mPendingPosMode = -1;

        if ((targetMode == xplm_WindowPopOut || targetMode == xplm_WindowVR) && self->IsInsideSim()) {
            self->GetWindowGeometry(
                self->mSavedSimLeft, self->mSavedSimTop, self->mSavedSimRight, self->mSavedSimBottom);
            self->mHasSavedSimGeometry =
                (self->mSavedSimRight > self->mSavedSimLeft && self->mSavedSimTop > self->mSavedSimBottom);
        }

        self->SetWindowPositioningMode(targetMode, -1);

        if (targetMode == xplm_WindowPositionFree && self->mHasSavedSimGeometry) {
            int l = self->mSavedSimLeft;
            int t = self->mSavedSimTop;
            int r = self->mSavedSimRight;
            int b = self->mSavedSimBottom;
            ClampGeometryToScreen(
                l, t, r, b,
                kDefaultMinWidth, kDefaultMinHeight, kDefaultMaxWidth, kDefaultMaxHeight,
                self->GetEffectiveTopMargin());
            self->SetWindowGeometry(l, t, r, b);
        }
        return 0.0f;
    }

    // ------------------------------------------------------------------------
    // Static DocumentViewer Callback Trampolines
    // ------------------------------------------------------------------------

    static void cbZoomChanged(float oldZoom, float newZoom, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        if (self->mScaleWindowOnZoom && std::fabs(newZoom - oldZoom) > 0.001f) {
            self->ScaleWindowGeometry(oldZoom, newZoom);
        }
        self->onDocumentZoomChanged(oldZoom, newZoom);
    }

    static void cbFitRequested(float currentZoom, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->ResizeToScale(currentZoom);
    }

    static void cbCursorToggled(bool cursorsEnabled, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->SetImgCursorHandling(cursorsEnabled);
        if (!cursorsEnabled) {
            ImGui::GetIO().MouseDrawCursor = false;
        }
    }

    static void cbPopOutToggled(bool wantPopOut, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->schedulePositionMode(wantPopOut ? xplm_WindowPopOut : xplm_WindowPositionFree);
    }

    static void cbVrRequested(void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->schedulePositionMode(xplm_WindowVR);
    }

    static void cbCloseRequested(void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->SetVisible(false);
        self->onDocumentClosed();
    }

    static void cbReloadRequested(void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->ReloadDocument();
    }

    static void cbTransparencyToggled(bool transparentHud, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->onDocumentThemeChanged(self->mState.theme_index, transparentHud, self->mState.window_opacity);
    }

    static void cbOpacityChanged(float opacity, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->onDocumentThemeChanged(self->mState.theme_index, self->mState.transparent_hud, opacity);
    }

    static void cbThemeChanged(int themeIndex, void* userData)
    {
        ImgDocWindow* self = static_cast<ImgDocWindow*>(userData);
        if (!self) return;
        self->onDocumentThemeChanged(themeIndex, self->mState.transparent_hud, self->mState.window_opacity);
    }

    // ------------------------------------------------------------------------
    // Small Filesystem Helpers
    // ------------------------------------------------------------------------

    static bool fileExistsRegular(const std::string& p)
    {
        if (p.empty()) return false;
        struct stat st;
        return (stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode));
    }

    static bool readUtf8File(const std::string& p, std::string& out)
    {
        FILE* fp = std::fopen(p.c_str(), "rb");
        if (!fp) return false;
        if (std::fseek(fp, 0, SEEK_END) != 0) {
            std::fclose(fp);
            return false;
        }
        const long sz = std::ftell(fp);
        if (sz < 0 || std::fseek(fp, 0, SEEK_SET) != 0) {
            std::fclose(fp);
            return false;
        }
        std::string buf;
        if (sz > 0) {
            buf.resize(static_cast<size_t>(sz));
            const size_t n = std::fread(&buf[0], 1, static_cast<size_t>(sz), fp);
            buf.resize(n);
        }
        std::fclose(fp);
        out.swap(buf);
        return true;
    }

    static std::string extractParentDir(const std::string& p)
    {
        if (p.empty()) return std::string();
        size_t end = p.size();
        while (end > 1 && (p[end - 1] == '/' || p[end - 1] == '\\')) {
            --end;
        }
        const size_t slash = p.find_last_of("/\\", end - 1);
        if (slash == std::string::npos) return std::string();
        return (slash == 0) ? p.substr(0, 1) : p.substr(0, slash);
    }

    static std::string joinPath(const std::string& dir, const std::string& rel)
    {
        if (dir.empty()) return rel;
        if (rel.empty()) return dir;
        const char back = dir[dir.size() - 1];
        if (back == '/' || back == '\\') return dir + rel;
        return dir + "/" + rel;
    }

    static std::string normalizePluginPath(const char* raw)
    {
        if (!raw || raw[0] == '\0') return std::string();
        std::string s(raw);
#if defined(__APPLE__) || defined(__MACH__)
        // Convert legacy macOS HFS colon-delimited XPLM paths if XPLM_USE_NATIVE_PATHS was not enabled
        if (s.find('/') == std::string::npos && s.find(':') != std::string::npos) {
            const size_t firstColon = s.find(':');
            std::string tail = s.substr(firstColon);
            for (size_t i = 0; i < tail.size(); ++i) {
                if (tail[i] == ':') tail[i] = '/';
            }
            if (fileExistsRegular(tail)) {
                return tail;
            }
            std::string vol = "/Volumes/" + s.substr(0, firstColon) + tail;
            if (fileExistsRegular(vol)) {
                return vol;
            }
        }
#endif
        return s;
    }

    std::string                 mDocTitle;
    std::string                 mMarkdownText;
    std::string                 mFallbackMarkdown;
    std::string                 mRequestedFilePath;
    std::string                 mSourceFilePath;
    std::string                 mBaseDir;
    std::string                 mPdfPath;
    ImGui::DocumentViewerState  mState;
    ImGui::DocumentViewerConfig mConfig{};
    ImGuiContext*               mDocImGuiContext = nullptr;

    bool   mScaleWindowOnZoom      = true;
    bool   mEnableTopDragGrip      = true;
    bool   mAutoReloadOnDiskChange = true;
    time_t mLastMtime              = 0;
    float  mAutoReloadAccum        = 0.0f;

    int  mSavedSimLeft        = 0;
    int  mSavedSimTop         = 0;
    int  mSavedSimRight       = 0;
    int  mSavedSimBottom      = 0;
    bool mHasSavedSimGeometry = false;
    int  mPendingPosMode      = -1;

#if defined(XPLM210)
    XPLMFlightLoopID mFlightLoopId = nullptr;
#else
    bool mLegacyFlightLoopRegistered = false;
#endif
};

#endif // defined(IMGUI_DOC_VIEWER_H)

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGWINDOW_CONTRIB_IMGDOCWINDOW_H
