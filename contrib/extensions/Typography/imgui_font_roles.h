// ============================================================================
// ImGui Contrib Typography: Font Role Registry (imgui_font_roles.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Provides a unified, semantic typography registry (ImFontRole) mapping logical
// design roles (Regular, Bold, Italic, Monospace, Headings) to rasterized
// Dear ImGui fonts. Compatible with Dear ImGui v1.78 through v1.92+.
// ============================================================================

#pragma once

#ifndef IMGUI_FONT_ROLES_H
#define IMGUI_FONT_ROLES_H

#include "imgui.h"
#include <cstddef>
#include <string>

namespace ImGui {

    /** Semantic typography roles for application and plugin UI */
    enum class ImFontRole {
        Regular = 0,        // Slot 0: Primary interface font (e.g. Roboto Regular 14px)
        Title,              // Slot 1: Dialog titles, card headers (e.g. Roboto Medium 16px)
        Small,              // Slot 2: Captions, annotations, secondary subtext (e.g. Roboto 12px)
        Bold,               // Slot 3: Strong emphasis, column headers (e.g. Roboto Bold 14px)
        BoldLarge,          // Slot 4: Hero titles, prominent badges (e.g. Roboto Bold 16px)
        Monospace,          // Slot 5: Code, callsigns, datarefs, frequencies (e.g. RobotoMono Regular 14px)
        Italic,             // Slot 6: Soft emphasis, citations, hints (e.g. Roboto Italic 14px)
        MonospaceMedium,    // Slot 7: Numeric avionics readouts, dials (e.g. RobotoMono Medium 15px)
        Heading1,           // Markdown H1 banner title
        Heading2,           // Markdown H2 section header
        Heading3,           // Markdown H3 subsection header
        Count
    };

    namespace Detail {
        inline ImFont*& GetFontRoleSlot(ImFontRole role) {
            static ImFont* s_FontRoles[(size_t)ImFontRole::Count] = { nullptr };
            size_t idx = (size_t)role;
            if (idx >= (size_t)ImFontRole::Count) idx = 0;
            return s_FontRoles[idx];
        }
    } // namespace Detail

    /** Explicitly assigns an ImFont pointer to a semantic font role */
    inline void SetFontRole(ImFontRole role, ImFont* font) {
        Detail::GetFontRoleSlot(role) = font;
    }

    /** Clears all registered font roles back to default fallbacks */
    inline void ClearFontRoles() {
        for (size_t i = 0; i < (size_t)ImFontRole::Count; ++i) {
            Detail::GetFontRoleSlot((ImFontRole)i) = nullptr;
        }
    }

    /** Resolves the active ImFont for a given role with multi-tier fallback */
    inline ImFont* GetFont(ImFontRole role) {
        // Tier 1: Explicitly registered font role
        ImFont* f = Detail::GetFontRoleSlot(role);
        if (f) return f;

        // Tier 2: Automatic canonical 8-slot font atlas mapping (Slots 0..7)
        if ((size_t)role < 8) {
            if (ImGui::GetCurrentContext()) {
                ImGuiIO& io = ImGui::GetIO();
                if (io.Fonts && (int)role < io.Fonts->Fonts.Size) {
                    ImFont* slotFont = io.Fonts->Fonts[(int)role];
                    if (slotFont) return slotFont;
                }
            }
        }

        // Tier 3: Hierarchical semantic fallbacks
        switch (role) {
            case ImFontRole::Heading1:
                f = GetFont(ImFontRole::BoldLarge);
                if (f) return f;
                f = GetFont(ImFontRole::Title);
                if (f) return f;
                break;
            case ImFontRole::Heading2:
                f = GetFont(ImFontRole::Title);
                if (f) return f;
                f = GetFont(ImFontRole::Bold);
                if (f) return f;
                break;
            case ImFontRole::Heading3:
                f = GetFont(ImFontRole::Bold);
                if (f) return f;
                break;
            case ImFontRole::BoldLarge:
                f = GetFont(ImFontRole::Bold);
                if (f) return f;
                break;
            case ImFontRole::MonospaceMedium:
                f = GetFont(ImFontRole::Monospace);
                if (f) return f;
                break;
            default:
                break;
        }

        // Tier 4: Fallback to ambient window font
        if (ImGui::GetCurrentContext()) {
            return ImGui::GetFont();
        }
        return nullptr;
    }

    /** Pushes the font corresponding to the given role onto Dear ImGui's font stack */
    inline void PushFontRole(ImFontRole role) {
        ImFont* f = GetFont(role);
        if (!f && ImGui::GetCurrentContext()) {
            f = ImGui::GetFont();
        }
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        ImGui::PushFont(f, f ? f->LegacySize : 0.0f);
#else
        ImGui::PushFont(f);
#endif
    }

    /** Pops the font pushed by PushFontRole() */
    inline void PopFontRole() {
        ImGui::PopFont();
    }

    /** RAII Scoped Guard for Font Roles */
    struct ScopedFontRole {
        ScopedFontRole(ImFontRole role) {
            PushFontRole(role);
        }
        ~ScopedFontRole() {
            PopFontRole();
        }
    };

} // namespace ImGui

// Export to global scope for convenient unqualified usage (e.g. ImFontRole::Bold):
using ImFontRole = ImGui::ImFontRole;

#endif // IMGUI_FONT_ROLES_H