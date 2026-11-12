// ============================================================================
// ImGee: Master Suite Roll-Up Header (contrib/ImGee/imgee.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Master roll-up header for all ImGee Dear ImGui extensions and immediate-mode
// flight-deck widgets bundled in contrib/ImGee/.
//
// Why use this header (or top-level "contrib.h")?
// 1. Guaranteed Dependency Order: Automatically includes TimedTooltip BEFORE
//    Markdown and Widgets so cross-extension bridges (such as
//    ImGui::TimedTooltip::TextMD and TimedTooltips on SliderWithReset / ComboWithReset)
//    activate seamlessly with zero manual header juggling.
// 2. Easy Customization & Opt-Out: Uncomment any of the IMGEE_DISABLE_* toggles
//    below (or define them before including this file) to selectively exclude
//    individual widgets, helpers, or entire subsystems that might collide with
//    your existing codebase.
// ============================================================================

#pragma once

#ifndef IMGEE_H
#define IMGEE_H

// ============================================================================
// ImGee Version Specifications
// ============================================================================
#define IMGEE_VERSION     "1.0.0"
#define IMGEE_VERSION_NUM 10000

// ============================================================================
// Granular Legacy Alias Bridge: CONTRIB_DISABLE_* -> IMGEE_DISABLE_*
// (Maps feature-level legacy CONTRIB_* toggles to their proper IMGEE_* names)
// ============================================================================
#if defined(CONTRIB_DISABLE_EXTENSIONS) && !defined(IMGEE_DISABLE_EXTENSIONS)
    #define IMGEE_DISABLE_EXTENSIONS
#endif
#if defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_WIDGETS)
    #define IMGEE_DISABLE_WIDGETS
#endif
#if defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP)
    #define IMGEE_DISABLE_TIMED_TOOLTIP
#endif
#if defined(CONTRIB_DISABLE_MARKDOWN) && !defined(IMGEE_DISABLE_MARKDOWN)
    #define IMGEE_DISABLE_MARKDOWN
#endif
#if defined(CONTRIB_DISABLE_TYPOGRAPHY) && !defined(IMGEE_DISABLE_TYPOGRAPHY)
    #define IMGEE_DISABLE_TYPOGRAPHY
#endif
#if defined(CONTRIB_DISABLE_TEXT_MD) && !defined(IMGEE_DISABLE_TEXT_MD)
    #define IMGEE_DISABLE_TEXT_MD
#endif
#if defined(CONTRIB_DISABLE_TEXT_WRAPPED_MD) && !defined(IMGEE_DISABLE_TEXT_WRAPPED_MD)
    #define IMGEE_DISABLE_TEXT_WRAPPED_MD
#endif
#if defined(CONTRIB_DISABLE_BUTTON_MD) && !defined(IMGEE_DISABLE_BUTTON_MD)
    #define IMGEE_DISABLE_BUTTON_MD
#endif
#if (defined(CONTRIB_ENABLE_GLOBAL_NAMESPACES) || defined(IMGUI_CONTRIB_ENABLE_GLOBAL_NAMESPACES)) && !defined(IMGEE_ENABLE_GLOBAL_NAMESPACES)
    #define IMGEE_ENABLE_GLOBAL_NAMESPACES
#endif
#if defined(CONTRIB_DISABLE_TOGGLE_BUTTONS) && !defined(IMGEE_DISABLE_TOGGLE_BUTTONS)
    #define IMGEE_DISABLE_TOGGLE_BUTTONS
#endif
#if defined(CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL) && !defined(IMGEE_DISABLE_TOGGLE_MOUSE_WHEEL)
    #define IMGEE_DISABLE_TOGGLE_MOUSE_WHEEL
#endif
#if defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS)
    #define IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS
#endif
#if defined(CONTRIB_DISABLE_CLICKABLE_LINKS) && !defined(IMGEE_DISABLE_CLICKABLE_LINKS)
    #define IMGEE_DISABLE_CLICKABLE_LINKS
#endif
#if defined(CONTRIB_DISABLE_BADGES) && !defined(IMGEE_DISABLE_BADGES)
    #define IMGEE_DISABLE_BADGES
#endif
#if defined(CONTRIB_DISABLE_SPINNERS) && !defined(IMGEE_DISABLE_SPINNERS)
    #define IMGEE_DISABLE_SPINNERS
#endif
#if defined(CONTRIB_DISABLE_POPUP_MENUS) && !defined(IMGEE_DISABLE_POPUP_MENUS)
    #define IMGEE_DISABLE_POPUP_MENUS
#endif
#if defined(CONTRIB_DISABLE_ICONS) && !defined(IMGEE_DISABLE_ICONS)
    #define IMGEE_DISABLE_ICONS
#endif
#if defined(CONTRIB_DISABLE_ICON_BUTTON) && !defined(IMGEE_DISABLE_ICON_BUTTON)
    #define IMGEE_DISABLE_ICON_BUTTON
#endif

#if !defined(IMGEE_DISABLE_ALL) && \
    !defined(DISABLE_IMGEE) && \
    !defined(IMGUI_DISABLE_IMGEE) && \
    !defined(CONTRIB_IMGEE_DISABLE_ALL)

// ============================================================================
// OPT-OUT CONFIGURATION (Uncomment any line below, or #define before including)
// ============================================================================

// --- ImGee Subsystems ---
// #define IMGEE_DISABLE_EXTENSIONS             // Exclude all ImGee extensions
// #define IMGEE_DISABLE_WIDGETS                // Exclude all ImGee extra widgets

// --- Individual Extensions (contrib/ImGee/extensions/) ---
// #define IMGEE_DISABLE_TIMED_TOOLTIP          // Exclude TimedTooltip extension
// #define IMGEE_DISABLE_MARKDOWN               // Exclude Markdown extension
// #define IMGEE_DISABLE_TYPOGRAPHY             // Exclude Typography & StyledText extensions
// #define IMGEE_DISABLE_TEXT_MD                // Exclude all root ImGui::TextMD / TextWrappedMD aliases
// #define IMGEE_DISABLE_TEXT_WRAPPED_MD        // Exclude ONLY root ImGui::TextWrappedMD (retains TextMD)
// #define IMGEE_DISABLE_BUTTON_MD              // Exclude all root ImGui::ButtonMD / ButtonMarkdown aliases
// #define IMGEE_ENABLE_GLOBAL_NAMESPACES       // Publish TimedTooltip:: and Markdown:: to global scope

// --- Widget Collections (contrib/ImGee/widgets/) ---
// #define IMGEE_DISABLE_TOGGLE_BUTTONS         // Exclude ToggleButton & ToggleButtons
// #define IMGEE_DISABLE_TOGGLE_MOUSE_WHEEL     // Exclude mouse-wheel support on ToggleButton(s)
// #define IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS   // Exclude all wheel sliders, drags, cyclers & combos
// #define IMGEE_DISABLE_CLICKABLE_LINKS        // Exclude ButtonLink, TextLink & AttachItemCursor
// #define IMGEE_DISABLE_BADGES                 // Exclude Badge, Pill & Kbd widgets
// #define IMGEE_DISABLE_SPINNERS               // Exclude Spinner, RadialTimer & CountdownTimer
// #define IMGEE_DISABLE_POPUP_MENUS            // Exclude SelectPulldown, OpenPopupAtItem & BeginAnchoredPopup
// #define IMGEE_DISABLE_ICONS                  // Exclude procedural vector icon engine & ImGui::Icon
// #define IMGEE_DISABLE_ICON_BUTTON            // Exclude IconButton & ButtonIcon

// --- Fine-Grained Mouse-Wheel Control Opt-Outs & Style Defaults ---
// #define IMGEE_DISABLE_GET_MOUSE_WHEEL        // Exclude GetMouseWheel* primitives
// #define IMGEE_DISABLE_ADJUST_MOUSE_WHEEL     // Exclude AdjustItemOnMouseWheel()
// #define IMGEE_DISABLE_CYCLE_MOUSE_WHEEL      // Exclude CycleItemOnMouseWheel()
// #define IMGEE_DISABLE_PERCENT_SLIDER         // Exclude PercentSlider() & SliderPercent()
// #define IMGEE_DISABLE_SLIDER_PERCENT         // Exclude only the SliderPercent() alias
// #define IMGEE_DISABLE_MOUSE_WHEEL_COMBO      // Exclude MouseWheelCombo & ComboWithReset
// #define IMGEE_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT // Disable automatic mouse-wheel defeat on scrollable viewports

// --- Image Decoding & Build Optimization ---
// #define IMGEE_DISABLE_STB_IMAGE              // Disable ImGee's isolated built-in stb_image decoder
// #define IMGEE_EXTERNAL_STB_IMAGE             // Compile ImGee's decoder once via contrib/ImGee/imgee_stb_image.cpp

// ============================================================================
// Bidirectional Opt-Out Propagation: IMGEE_DISABLE_* -> CONTRIB_DISABLE_*
// (Ensures sub-headers checking legacy CONTRIB_DISABLE_* flags honor IMGEE_*)
// ============================================================================
#if defined(IMGEE_DISABLE_EXTENSIONS) && !defined(CONTRIB_DISABLE_EXTENSIONS)
    #define CONTRIB_DISABLE_EXTENSIONS
#endif
#if defined(IMGEE_DISABLE_EXTENSIONS) || defined(CONTRIB_DISABLE_EXTENSIONS)
    #ifndef IMGEE_DISABLE_TIMED_TOOLTIP
        #define IMGEE_DISABLE_TIMED_TOOLTIP
    #endif
    #ifndef IMGEE_DISABLE_MARKDOWN
        #define IMGEE_DISABLE_MARKDOWN
    #endif
    #ifndef IMGEE_DISABLE_TYPOGRAPHY
        #define IMGEE_DISABLE_TYPOGRAPHY
    #endif
#endif
#if defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS)
    #define CONTRIB_DISABLE_WIDGETS
#endif
#if defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP)
    #define CONTRIB_DISABLE_TIMED_TOOLTIP
#endif
#if defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
    #define CONTRIB_DISABLE_MARKDOWN
#endif
#if defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
    #define CONTRIB_DISABLE_TYPOGRAPHY
#endif
#if defined(IMGEE_DISABLE_TEXT_MD) && !defined(CONTRIB_DISABLE_TEXT_MD)
    #define CONTRIB_DISABLE_TEXT_MD
#endif
#if defined(IMGEE_DISABLE_TEXT_WRAPPED_MD) && !defined(CONTRIB_DISABLE_TEXT_WRAPPED_MD)
    #define CONTRIB_DISABLE_TEXT_WRAPPED_MD
#endif
#if defined(IMGEE_DISABLE_BUTTON_MD) && !defined(CONTRIB_DISABLE_BUTTON_MD)
    #define CONTRIB_DISABLE_BUTTON_MD
#endif
#if defined(IMGEE_DISABLE_TOGGLE_BUTTONS) && !defined(CONTRIB_DISABLE_TOGGLE_BUTTONS)
    #define CONTRIB_DISABLE_TOGGLE_BUTTONS
#endif
#if defined(IMGEE_DISABLE_TOGGLE_MOUSE_WHEEL) && !defined(CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL)
    #define CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL
#endif
#if defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS)
    #define CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS
#endif
#if defined(IMGEE_DISABLE_CLICKABLE_LINKS) && !defined(CONTRIB_DISABLE_CLICKABLE_LINKS)
    #define CONTRIB_DISABLE_CLICKABLE_LINKS
#endif
#if defined(IMGEE_DISABLE_BADGES) && !defined(CONTRIB_DISABLE_BADGES)
    #define CONTRIB_DISABLE_BADGES
#endif
#if defined(IMGEE_DISABLE_SPINNERS) && !defined(CONTRIB_DISABLE_SPINNERS)
    #define CONTRIB_DISABLE_SPINNERS
#endif
#if defined(IMGEE_DISABLE_COPY_BUTTON) && !defined(CONTRIB_DISABLE_COPY_BUTTON)
    #define CONTRIB_DISABLE_COPY_BUTTON
#endif
#if defined(IMGEE_DISABLE_KNOBS) && !defined(CONTRIB_DISABLE_KNOBS)
    #define CONTRIB_DISABLE_KNOBS
#endif
#if defined(IMGEE_DISABLE_FILE_DIALOG) && !defined(CONTRIB_DISABLE_FILE_DIALOG)
    #define CONTRIB_DISABLE_FILE_DIALOG
#endif
#if defined(IMGEE_DISABLE_POPUP_MENUS) && !defined(CONTRIB_DISABLE_POPUP_MENUS)
    #define CONTRIB_DISABLE_POPUP_MENUS
#endif
#if defined(IMGEE_DISABLE_ICONS) && !defined(CONTRIB_DISABLE_ICONS)
    #define CONTRIB_DISABLE_ICONS
#endif
#if defined(IMGEE_DISABLE_ICON_BUTTON) && !defined(CONTRIB_DISABLE_ICON_BUTTON)
    #define CONTRIB_DISABLE_ICON_BUTTON
#endif
#if defined(IMGEE_DISABLE_GET_MOUSE_WHEEL) && !defined(CONTRIB_DISABLE_GET_MOUSE_WHEEL)
    #define CONTRIB_DISABLE_GET_MOUSE_WHEEL
#endif
#if defined(IMGEE_DISABLE_ADJUST_MOUSE_WHEEL) && !defined(CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL)
    #define CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL
#endif
#if defined(IMGEE_DISABLE_CYCLE_MOUSE_WHEEL) && !defined(CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL)
    #define CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL
#endif
#if defined(IMGEE_DISABLE_PERCENT_SLIDER) && !defined(CONTRIB_DISABLE_PERCENT_SLIDER)
    #define CONTRIB_DISABLE_PERCENT_SLIDER
#endif
#if defined(IMGEE_DISABLE_SLIDER_PERCENT) && !defined(CONTRIB_DISABLE_SLIDER_PERCENT)
    #define CONTRIB_DISABLE_SLIDER_PERCENT
#endif
#if defined(IMGEE_DISABLE_MOUSE_WHEEL_COMBO) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_COMBO)
    #define CONTRIB_DISABLE_MOUSE_WHEEL_COMBO
#endif
#if defined(IMGEE_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT)
    #define CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT
#endif

// ============================================================================
// 0. ISOLATED INTERNAL IMAGE DECODER (Turnkey PNG/JPEG/BMP/GIF/TGA support)
// ============================================================================

#if !defined(IMGEE_DISABLE_STB_IMAGE)
#include "imgee_stbi_internal.h"
#endif

// ============================================================================
// 1. EXTENSIONS (Included first to satisfy downstream cross-feature bridges)
// ============================================================================

#include "extensions/includes.h"

// ============================================================================
// 2. WIDGETS (Automatically bridges with TimedTooltip if enabled above)
// ============================================================================

#include "widgets/includes.h"

// ============================================================================
// 3. THEMES (Curated cockpit & studio style presets)
// ============================================================================

#include "themes/includes.h"

// ============================================================================
// 4. OPT-IN GLOBAL NAMESPACES (Only active if explicitly requested)
// ============================================================================

#if defined(IMGEE_ENABLE_GLOBAL_NAMESPACES)
#include "imgee-global.h"
#endif

// ============================================================================
// 5. HOST PLATFORM BRIDGES (ImgWindow Named Texture Resolver, Decoder & Shutdown)
// ============================================================================

#if defined(IMGWINDOW_H) || (defined(__has_include) && __has_include("ImgWindow.h"))
#if !defined(IMGWINDOW_H)
#include "ImgWindow.h"
#endif
#ifndef IMGEE_IMGWINDOW_RESOLVER_BRIDGED
#define IMGEE_IMGWINDOW_RESOLVER_BRIDGED
namespace ImGeeDetail {
    struct AutoImgWindowResolverBridge {
        AutoImgWindowResolverBridge() {
#if defined(IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED)
            // 1. Bridge ImGui named texture queries to ImgWindow
            ImGui::SetNamedTextureResolver([](const char* name, ImVec2* outSize) -> ImTextureID {
                return ImgWindow::GetNamedTexture(name ? name : "", outSize);
            });
#endif

#if defined(IMGEE_HAS_STBI_INTERNAL)
            // 2. Auto-wire in-memory compressed image decoder via ImGee's isolated stb_image
            ImgWindow::SetMemoryImageDecoder(
                &ImGeeStbi::LoadFromMemory,
                &ImGeeStbi::ImageFree
            );
#endif

#if defined(IMGUI_MARKDOWN_IMAGE_SERVICE_H)
            // 3. Register shutdown handler to purge Markdown VRAM cache on plugin stop/reload
            ImgWindow::AddShutdownHandler(&ImGui::MarkdownImageService::ClearTextureCache);
#endif
        }
    };
    static AutoImgWindowResolverBridge s_AutoImgWindowResolverBridge;
}
#endif
#endif

#endif // !IMGEE_DISABLE_ALL

#endif // IMGEE_H
