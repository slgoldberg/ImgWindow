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

#if !defined(CONTRIB_DISABLE_ICONS) && !defined(IMGUI_CONTRIB_DISABLE_ICONS)
#if defined(IMGUI_ICONS_H)
    #define CONTRIB_BADGES_HAS_ICONS 1
#elif defined(__has_include)
    #if __has_include("imgui_icons.h")
        #include "imgui_icons.h"
        #define CONTRIB_BADGES_HAS_ICONS 1
    #elif __has_include("widgets/imgui_icons.h")
        #include "widgets/imgui_icons.h"
        #define CONTRIB_BADGES_HAS_ICONS 1
    #endif
#else
    #include "imgui_icons.h"
    #define CONTRIB_BADGES_HAS_ICONS 1
#endif
#endif

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
#ifndef IMGUI_NAMED_TEXTURE_RESOLVER_DEFINED
#define IMGUI_NAMED_TEXTURE_RESOLVER_DEFINED
        typedef ImTextureID (*NamedTextureResolverFn)(const char* name, ImVec2* outSize);
        inline NamedTextureResolverFn& GetNamedTextureResolver() {
            static NamedTextureResolverFn s_Resolver = nullptr;
            return s_Resolver;
        }
#endif

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

#ifndef IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED
#define IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED
    inline IMGUI_API void SetNamedTextureResolver(Detail::NamedTextureResolverFn fn) {
        Detail::GetNamedTextureResolver() = fn;
    }
#endif

    // Core Badge renderer (supports text, texture icons, and icon-only annunciators)
    inline IMGUI_API void Badge(const char* text,
                                const ImVec4& bg_col,
                                const ImVec4* fg_col = nullptr,
                                float custom_rounding = -1.0f,
                                ImTextureID icon = (ImTextureID)0,
                                ImVec2 icon_size = ImVec2(0.0f, 0.0f),
                                const char* icon_name = nullptr) {
        const bool has_tex_icon = (icon != (ImTextureID)0);
        bool has_vec_icon = false;
        std::string vec_icon_name;
#if defined(CONTRIB_BADGES_HAS_ICONS)
        if (!has_tex_icon && icon_name && *icon_name) {
            std::string norm = NormalizeIconName(icon_name);
            if (IsVectorIcon(norm.c_str())) {
                has_vec_icon = true;
                vec_icon_name = norm;
            }
        }
#endif
        const bool has_icon = (has_tex_icon || has_vec_icon);
        const bool has_text = (text != nullptr && *text != '\0');
        if (!has_icon && !has_text) return;

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
        const char* text_end = has_text ? FindRenderedTextEnd(text) : nullptr;
        const ImVec2 text_size = has_text ? CalcTextSize(text, text_end, true) : ImVec2(0.0f, 0.0f);

        // Calculate icon dimensions
        ImVec2 actual_icon_size = icon_size;
        if (has_icon) {
            if (actual_icon_size.y <= 0.0f) {
                float target_h = has_vec_icon ? (font_size * 0.90f) : font_size;
                float aspect = (actual_icon_size.x > 0.0f && actual_icon_size.y > 0.0f) ? (actual_icon_size.x / actual_icon_size.y) : 1.0f;
                actual_icon_size = ImVec2(target_h * aspect, target_h);
            } else if (actual_icon_size.x <= 0.0f) {
                actual_icon_size.x = actual_icon_size.y;
            }
        } else {
            actual_icon_size = ImVec2(0.0f, 0.0f);
        }

        // Intrinsic padding: backdrop frames the content symmetrically
        const bool is_pill = (custom_rounding > 4.0f * scale || custom_rounding >= 900.0f);
        float pad_x = is_pill ? ImMax(7.0f * scale, 6.0f) : ImMax(5.0f * scale, 4.0f);
        const float pad_y = ImMax(2.0f * scale, 2.0f);
        if (has_icon && !has_text) {
            // Unary / icon-only annunciator capsule
            pad_x = is_pill ? ImMax(5.0f * scale, 4.0f) : ImMax(4.0f * scale, 3.0f);
        }
        const float gap_x = (has_icon && has_text) ? ImMax(4.0f * scale, 3.0f) : 0.0f;

        const float content_w = (has_icon ? actual_icon_size.x : 0.0f) + gap_x + (has_text ? text_size.x : 0.0f);
        const float content_h = ImMax(has_icon ? actual_icon_size.y : 0.0f, has_text ? text_size.y : 0.0f);

        const float total_w = content_w + pad_x * 2.0f;
        const float total_h = content_h + pad_y * 2.0f;

        // Continuous baseline alignment:
        const ImVec2 pos = window->DC.CursorPos;
        const float text_y = pos.y + window->DC.CurrLineTextBaseOffset;

        // Expand the pill/badge bounding box symmetrically around the content:
        const ImRect bb(ImVec2(pos.x, text_y - pad_y), ImVec2(pos.x + total_w, text_y + content_h + pad_y));

        ItemSize(ImVec2(total_w, content_h), 0.0f);
        const ImGuiID id = window->GetID(has_text ? text : (has_vec_icon ? vec_icon_name.c_str() : (has_tex_icon ? (const void*)icon : "##badge")));
        if (!ItemAdd(bb, id)) return;

        const float default_rounding = is_pill ? (total_h * 0.5f) : ImMax(3.0f * scale, GetStyle().FrameRounding);
        const float rounding = (custom_rounding >= 900.0f) ? (total_h * 0.5f) : ((custom_rounding >= 0.0f) ? custom_rounding : default_rounding);

        // Render background framing
        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(bg_col), rounding);

        // Render icon
        if (has_icon) {
            float icon_y = text_y + (content_h - actual_icon_size.y) * 0.5f;
            ImVec2 icon_min(pos.x + pad_x, icon_y);
            if (has_tex_icon) {
                ImVec2 icon_max(icon_min.x + actual_icon_size.x, icon_min.y + actual_icon_size.y);
                GetWindowDrawList()->AddImage(icon, icon_min, icon_max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
            }
#if defined(CONTRIB_BADGES_HAS_ICONS)
            else if (has_vec_icon) {
                DrawVectorIcon(GetWindowDrawList(), icon_min, actual_icon_size.y, GetColorU32(text_col), vec_icon_name.c_str());
            }
#endif
        }

        // Render text
        if (has_text) {
            float text_x = pos.x + pad_x + (has_icon ? (actual_icon_size.x + gap_x) : 0.0f);
            GetWindowDrawList()->AddText(ImVec2(text_x, text_y), GetColorU32(text_col), text, text_end);
        }
    }

    inline IMGUI_API void Badge(const char* text, ImU32 bg_col, ImU32 fg_col = 0, float custom_rounding = -1.0f, ImTextureID icon = (ImTextureID)0, ImVec2 icon_size = ImVec2(0.0f, 0.0f), const char* icon_name = nullptr) {
        ImVec4 bg = ColorConvertU32ToFloat4(bg_col);
        if (fg_col != 0) {
            ImVec4 fg = ColorConvertU32ToFloat4(fg_col);
            Badge(text, bg, &fg, custom_rounding, icon, icon_size, icon_name);
        } else {
            Badge(text, bg, nullptr, custom_rounding, icon, icon_size, icon_name);
        }
    }

    inline IMGUI_API void Badge(const char* text, ImGuiBadgeVariant variant = ImGuiBadgeVariant_Default, float custom_rounding = -1.0f, ImTextureID icon = (ImTextureID)0, ImVec2 icon_size = ImVec2(0.0f, 0.0f), const char* icon_name = nullptr) {
        ImVec4 bg, fg;
        Detail::GetBadgeColors(variant, bg, fg);
        Badge(text, bg, &fg, custom_rounding, icon, icon_size, icon_name);
    }

    inline IMGUI_API void Badge(ImTextureID icon, const ImVec4& bg_col = ImVec4(0.25f, 0.25f, 0.30f, 1.0f), float custom_rounding = -1.0f, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        Badge(nullptr, bg_col, nullptr, custom_rounding, icon, icon_size);
    }

    inline IMGUI_API void Badge(ImTextureID icon, ImU32 bg_col, float custom_rounding = -1.0f, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        Badge(nullptr, bg_col, 0, custom_rounding, icon, icon_size);
    }

    // Pill aliases (fully rounded badge)
    inline IMGUI_API void Pill(const char* text, ImGuiBadgeVariant variant = ImGuiBadgeVariant_Default, ImTextureID icon = (ImTextureID)0, ImVec2 icon_size = ImVec2(0.0f, 0.0f), const char* icon_name = nullptr) {
        Badge(text, variant, 999.0f, icon, icon_size, icon_name);
    }

    inline IMGUI_API void Pill(const char* text, const ImVec4& bg_col, const ImVec4* fg_col = nullptr, ImTextureID icon = (ImTextureID)0, ImVec2 icon_size = ImVec2(0.0f, 0.0f), const char* icon_name = nullptr) {
        Badge(text, bg_col, fg_col, 999.0f, icon, icon_size, icon_name);
    }

    inline IMGUI_API void Pill(const char* text, ImU32 bg_col, ImU32 fg_col = 0, ImTextureID icon = (ImTextureID)0, ImVec2 icon_size = ImVec2(0.0f, 0.0f), const char* icon_name = nullptr) {
        Badge(text, bg_col, fg_col, 999.0f, icon, icon_size, icon_name);
    }

    inline IMGUI_API void Pill(ImTextureID icon, const ImVec4& bg_col = ImVec4(0.20f, 0.50f, 0.95f, 1.0f), ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        Badge(nullptr, bg_col, nullptr, 999.0f, icon, icon_size);
    }

    inline IMGUI_API void Pill(ImTextureID icon, ImU32 bg_col, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        Badge(nullptr, bg_col, 0, 999.0f, icon, icon_size);
    }

    // Named Texture & Vector Icon Pill and Badge APIs
    inline IMGUI_API void BadgeNamed(const char* imageName, const char* text = nullptr, const ImVec4& bg_col = ImVec4(0.25f, 0.25f, 0.30f, 1.0f), const ImVec4* fg_col = nullptr, float custom_rounding = -1.0f, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        ImTextureID tex = (ImTextureID)0;
        ImVec2 native_sz(0.0f, 0.0f);
        if (imageName && Detail::GetNamedTextureResolver()) {
            tex = Detail::GetNamedTextureResolver()(imageName, &native_sz);
        }
        if (icon_size.x <= 0.0f && icon_size.y <= 0.0f) icon_size = native_sz;
        Badge(text, bg_col, fg_col, custom_rounding, tex, icon_size, imageName);
    }

    inline IMGUI_API void BadgeNamed(const char* imageName, const ImVec4& bg_col, const ImVec4* fg_col = nullptr, float custom_rounding = -1.0f, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        BadgeNamed(imageName, nullptr, bg_col, fg_col, custom_rounding, icon_size);
    }

    inline IMGUI_API void BadgeNamed(const char* imageName, const char* text, ImU32 bg_col, ImU32 fg_col = 0, float custom_rounding = -1.0f, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        ImVec4 bg = ColorConvertU32ToFloat4(bg_col);
        if (fg_col != 0) {
            ImVec4 fg = ColorConvertU32ToFloat4(fg_col);
            BadgeNamed(imageName, text, bg, &fg, custom_rounding, icon_size);
        } else {
            BadgeNamed(imageName, text, bg, nullptr, custom_rounding, icon_size);
        }
    }

    inline IMGUI_API void BadgeNamed(const char* imageName, ImU32 bg_col, float custom_rounding = -1.0f, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        BadgeNamed(imageName, nullptr, bg_col, 0, custom_rounding, icon_size);
    }

    inline IMGUI_API void PillNamed(const char* imageName, const char* text = nullptr, const ImVec4& bg_col = ImVec4(0.20f, 0.50f, 0.95f, 1.0f), const ImVec4* fg_col = nullptr, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        BadgeNamed(imageName, text, bg_col, fg_col, 999.0f, icon_size);
    }

    inline IMGUI_API void PillNamed(const char* imageName, const ImVec4& bg_col, const ImVec4* fg_col = nullptr, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        BadgeNamed(imageName, nullptr, bg_col, fg_col, 999.0f, icon_size);
    }

    inline IMGUI_API void PillNamed(const char* imageName, const char* text, ImU32 bg_col, ImU32 fg_col = 0, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        BadgeNamed(imageName, text, bg_col, fg_col, 999.0f, icon_size);
    }

    inline IMGUI_API void PillNamed(const char* imageName, ImU32 bg_col, ImVec2 icon_size = ImVec2(0.0f, 0.0f)) {
        PillNamed(imageName, nullptr, bg_col, 0, icon_size);
    }

    // Keyboard Key Cap widget: <kbd> or Kbd("Ctrl+C")
    inline IMGUI_API void Kbd(const char* key_label) {
        if (!key_label || !*key_label) return;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return;

        const float font_size = GetFontSize();
        const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;

        bool is_icon = false;
        std::string icon_name;
#if defined(CONTRIB_BADGES_HAS_ICONS)
        if (key_label && *key_label) {
            std::string norm = NormalizeIconName(key_label);
            if (IsVectorIcon(norm.c_str())) {
                is_icon = true;
                icon_name = norm;
            }
        }
#endif

        const char* text_end = FindRenderedTextEnd(key_label);
        const ImVec2 text_size = is_icon ? ImVec2(font_size * 0.90f, font_size * 0.90f) : CalcTextSize(key_label, text_end, true);

        // Intrinsic keycap padding: matching Badge/Pill inline optical bounds
        const float pad_x = ImMax(5.5f * scale, 5.0f);
        const float pad_y = ImMax(2.0f * scale, 2.0f);
        const float bevel = 1.0f * scale;

        const float total_w = text_size.x + pad_x * 2.0f;
        const float total_h = text_size.y + pad_y * 2.0f + bevel;

        // Continuous baseline alignment:
        const ImVec2 pos = window->DC.CursorPos;
        const float text_y = pos.y + window->DC.CurrLineTextBaseOffset;
        const ImVec2 text_pos(pos.x + pad_x, text_y);

        const ImRect bb(ImVec2(pos.x, text_y - pad_y), ImVec2(pos.x + total_w, text_y + text_size.y + pad_y + bevel));

        ItemSize(ImVec2(total_w, text_size.y), 0.0f);
        const ImGuiID id = window->GetID(key_label);
        if (!ItemAdd(bb, id)) return;

        ImU32 bg_col = GetColorU32(ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
        ImU32 border_col = GetColorU32(ImVec4(0.40f, 0.40f, 0.45f, 0.8f));
        ImU32 bottom_shadow = GetColorU32(ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

        const float rounding = 3.5f * scale;
        ImDrawList* dl = GetWindowDrawList();

        // 3D bottom bevel / shadow for keycap effect
        dl->AddRectFilled(ImVec2(pos.x, bb.Min.y + bevel), bb.Max, bottom_shadow, rounding);
        dl->AddRectFilled(bb.Min, ImVec2(bb.Max.x, bb.Max.y - bevel), bg_col, rounding);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        dl->AddRect(bb.Min, ImVec2(bb.Max.x, bb.Max.y - bevel), border_col, rounding, 1.0f * scale, 0);
#else
        dl->AddRect(bb.Min, ImVec2(bb.Max.x, bb.Max.y - bevel), border_col, rounding, 0, 1.0f * scale);
#endif

#if defined(CONTRIB_BADGES_HAS_ICONS)
        if (is_icon) {
            DrawVectorIcon(dl, text_pos, text_size.x, GetColorU32(GetStyle().Colors[ImGuiCol_Text]), icon_name.c_str());
        } else
#endif
        {
            dl->AddText(text_pos, GetColorU32(GetStyle().Colors[ImGuiCol_Text]), key_label, text_end);
        }
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
