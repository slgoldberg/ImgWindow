// ============================================================================
// ImGui Contrib Widgets: Badges, Pills & Key Caps (imgui_badges.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone C++ badge, status pill, and keyboard shortcut UI widgets for
// Dear ImGui. Provides pixel-balanced rounded backgrounds, semantic styling
// (success, warning, danger, info), and auto-contrasting text colors without
// requiring Markdown parsing.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_BADGES) && \
    !defined(CONTRIB_DISABLE_BADGE) && \
    !defined(IMGUI_CONTRIB_DISABLE_BADGES) && \
    !defined(IMGUI_CONTRIB_DISABLE_BADGE) && \
    !defined(IMGUI_DISABLE_CONTRIB_BADGES) && \
    !defined(IMGUI_DISABLE_CONTRIB_BADGE) && \
    !defined(IMGUI_DISABLE_EXTRA_BADGES) && \
    !defined(IMGUI_DISABLE_EXTRA_BADGE)
#ifndef IMGUI_BADGES_H
#define IMGUI_BADGES_H
#define IMGUI_EXTRA_BADGES_H

#include "imgui.h"
#if defined(__has_include)
    #if __has_include("imgui_internal.h")
        #include "imgui_internal.h"
    #endif
#else
    #include "imgui_internal.h"
#endif
#include <string>
#include <algorithm>
#include <cmath>

namespace ImGui {

    enum ImGuiBadgeVariant {
        ImGuiBadgeVariant_Default = 0,
        ImGuiBadgeVariant_Primary,
        ImGuiBadgeVariant_Success,
        ImGuiBadgeVariant_Warning,
        ImGuiBadgeVariant_Danger,
        ImGuiBadgeVariant_Info
    };

    namespace Detail {
        inline void GetBadgeColors(ImGuiBadgeVariant variant, ImVec4& out_bg, ImVec4& out_fg) {
            switch (variant) {
                case ImGuiBadgeVariant_Primary:
                    out_bg = ImVec4(0.20f, 0.50f, 0.95f, 1.0f);
                    out_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
                case ImGuiBadgeVariant_Success:
                    out_bg = ImVec4(0.15f, 0.70f, 0.25f, 1.0f);
                    out_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
                case ImGuiBadgeVariant_Warning:
                    out_bg = ImVec4(0.95f, 0.75f, 0.10f, 1.0f);
                    out_fg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                    break;
                case ImGuiBadgeVariant_Danger:
                    out_bg = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
                    out_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
                case ImGuiBadgeVariant_Info:
                    out_bg = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
                    out_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
                case ImGuiBadgeVariant_Default:
                default:
                    out_bg = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
                    out_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
            }
        }
    } // namespace Detail

    // Core Badge renderer
    inline IMGUI_API void Badge(const char* text, const ImVec4& bg_col, const ImVec4* fg_col = nullptr, float custom_rounding = -1.0f) {
        if (!text || !*text) return;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return;

        ImVec4 text_col;
        if (fg_col) {
            text_col = *fg_col;
        } else {
            // Auto-contrast: dark text on light backgrounds, light text on dark backgrounds
            float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
            text_col = (lum > 0.65f) ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        const float font_size = GetFontSize();
        const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
        const char* text_end = FindRenderedTextEnd(text);
        const ImVec2 text_size = CalcTextSize(text, text_end, true);

        // Intrinsic padding: isolated from outer FramePadding
        const bool is_pill = (custom_rounding > 4.0f * scale);
        const float pad_x = is_pill ? ImMax(7.5f * scale, 7.0f) : ImMax(6.0f * scale, 5.0f);
        const float pad_y = ImMax(2.0f * scale, 2.0f);

        const float total_w = text_size.x + pad_x * 2.0f;

        ImVec2 pos = window->DC.CursorPos;
        if (window->DC.CurrLineTextBaseOffset > 0.0f) {
            pos.y += window->DC.CurrLineTextBaseOffset;
        }

        const ImRect bb(ImVec2(pos.x, pos.y - pad_y), ImVec2(pos.x + total_w, pos.y + text_size.y + pad_y));
        ItemSize(ImVec2(total_w, text_size.y));
        const ImGuiID id = window->GetID(text);
        if (!ItemAdd(bb, id)) return;

        const float rounding = (custom_rounding >= 0.0f) ? custom_rounding : ImMax(3.0f * scale, GetStyle().FrameRounding);

        // Render background and centered text
        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(bg_col), rounding);
        GetWindowDrawList()->AddText(ImVec2(pos.x + pad_x, pos.y), GetColorU32(text_col), text, text_end);
    }

    inline IMGUI_API void Badge(const char* text, ImU32 bg_col, ImU32 fg_col = 0, float custom_rounding = -1.0f) {
        ImVec4 bg = ColorConvertU32ToFloat4(bg_col);
        if (fg_col != 0) {
            ImVec4 fg = ColorConvertU32ToFloat4(fg_col);
            Badge(text, bg, &fg, custom_rounding);
        } else {
            Badge(text, bg, nullptr, custom_rounding);
        }
    }

    inline IMGUI_API void Badge(const char* text, ImGuiBadgeVariant variant = ImGuiBadgeVariant_Default, float custom_rounding = -1.0f) {
        ImVec4 bg, fg;
        Detail::GetBadgeColors(variant, bg, fg);
        Badge(text, bg, &fg, custom_rounding);
    }

    // Pill aliases (fully rounded badge)
    inline IMGUI_API void Pill(const char* text, ImGuiBadgeVariant variant = ImGuiBadgeVariant_Default) {
        float pill_rounding = GetFontSize() * 0.65f;
        Badge(text, variant, pill_rounding);
    }

    inline IMGUI_API void Pill(const char* text, const ImVec4& bg_col, const ImVec4* fg_col = nullptr) {
        float pill_rounding = GetFontSize() * 0.65f;
        Badge(text, bg_col, fg_col, pill_rounding);
    }

    inline IMGUI_API void Pill(const char* text, ImU32 bg_col, ImU32 fg_col = 0) {
        float pill_rounding = GetFontSize() * 0.65f;
        Badge(text, bg_col, fg_col, pill_rounding);
    }

    // Keyboard Key Cap widget: <kbd> or Kbd("Ctrl+C")
    inline IMGUI_API void Kbd(const char* key_label) {
        if (!key_label || !*key_label) return;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return;

        const float font_size = GetFontSize();
        const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
        const char* text_end = FindRenderedTextEnd(key_label);
        const ImVec2 text_size = CalcTextSize(key_label, text_end, true);

        // Intrinsic keycap padding
        const float pad_x = ImMax(6.0f * scale, 5.0f);
        const float pad_y = ImMax(2.0f * scale, 2.0f);

        const float total_w = text_size.x + pad_x * 2.0f;

        ImVec2 pos = window->DC.CursorPos;
        if (window->DC.CurrLineTextBaseOffset > 0.0f) {
            pos.y += window->DC.CurrLineTextBaseOffset;
        }

        const float top_y = pos.y - pad_y;
        const float bottom_y = pos.y + text_size.y + pad_y;
        const ImRect bb(ImVec2(pos.x, top_y), ImVec2(pos.x + total_w, bottom_y + 1.5f * scale));
        ItemSize(ImVec2(total_w, text_size.y));
        const ImGuiID id = window->GetID(key_label);
        if (!ItemAdd(bb, id)) return;

        ImU32 bg_col = GetColorU32(ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
        ImU32 border_col = GetColorU32(ImVec4(0.40f, 0.40f, 0.45f, 0.8f));
        ImU32 bottom_shadow = GetColorU32(ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

        const float rounding = 3.5f * scale;
        ImDrawList* dl = GetWindowDrawList();

        // 3D bottom bevel / shadow for keycap effect
        dl->AddRectFilled(ImVec2(pos.x, top_y + 1.0f * scale), ImVec2(pos.x + total_w, bottom_y + 1.5f * scale), bottom_shadow, rounding);
        dl->AddRectFilled(ImVec2(pos.x, top_y), ImVec2(pos.x + total_w, bottom_y), bg_col, rounding);
        dl->AddRect(ImVec2(pos.x, top_y), ImVec2(pos.x + total_w, bottom_y), border_col, rounding, 0, 1.0f * scale);

        dl->AddText(ImVec2(pos.x + pad_x, pos.y), GetColorU32(GetStyle().Colors[ImGuiCol_Text]), key_label, text_end);
    }

    // Friendly C++ aliases matching Markdown tags (<tag>, <key>, <keycap>):
    inline IMGUI_API void Tag(const char* text, ImGuiBadgeVariant variant = ImGuiBadgeVariant_Default, float custom_rounding = -1.0f) {
        Badge(text, variant, custom_rounding);
    }
    inline IMGUI_API void Tag(const char* text, const ImVec4& bg_col, const ImVec4* fg_col = nullptr, float custom_rounding = -1.0f) {
        Badge(text, bg_col, fg_col, custom_rounding);
    }
    inline IMGUI_API void Tag(const char* text, ImU32 bg_col, ImU32 fg_col = 0, float custom_rounding = -1.0f) {
        Badge(text, bg_col, fg_col, custom_rounding);
    }
    inline IMGUI_API void Key(const char* key_label) { Kbd(key_label); }
    inline IMGUI_API void KeyCap(const char* key_label) { Kbd(key_label); }

} // namespace ImGui

#endif // IMGUI_BADGES_H
#endif // Collision guards
