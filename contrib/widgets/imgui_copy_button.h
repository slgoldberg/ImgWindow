// ============================================================================
// ImGui Contrib Widgets: Universal Standalone & Chaser CopyButton (imgui_copy_button.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone C++ and Lua copy button widgets for Dear ImGui.
// Features:
// - Vector clipboard icon (two overlapping sheets) with smooth transition into
//   a crisp vector checkmark during the transient "Copied!" feedback state.
// - Stable fixed-width layout calculation to eliminate UI twitching on state change.
// - Per-button transient memory via ImGuiStorage keyed to ImGuiID (independent duration).
// - TimedTooltip integration on hover ("Copy to Clipboard" / "Copied!").
// - Automatic Hand mouse cursor on hover.
// - Proxy form (CopyButton): takes an explicit string/buffer.
// - Chaser form (CopyLastItemButton): automatically queries the preceding widget's
//   InputTextState to copy contents with zero redundant buffer passing.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_COPY_BUTTON) && \
    !defined(CONTRIB_DISABLE_COPY_BUTTONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_COPY_BUTTON) && \
    !defined(IMGUI_CONTRIB_DISABLE_COPY_BUTTONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_COPY_BUTTON) && \
    !defined(IMGUI_DISABLE_CONTRIB_COPY_BUTTONS) && \
    !defined(IMGUI_DISABLE_COPY_BUTTON) && \
    !defined(IMGUI_DISABLE_COPY_BUTTONS)
#ifndef IMGUI_COPY_BUTTON_H
#define IMGUI_COPY_BUTTON_H

#include "imgui.h"
#include <string>
#include <algorithm>
#include <cmath>

#ifndef IMGUI_CONTRIB_INTERNAL_INCLUDED
  #if defined(__has_include)
    #if __has_include("imgui_internal.h")
      #include "imgui_internal.h"
      #define IMGUI_CONTRIB_INTERNAL_INCLUDED 1
    #endif
  #endif
#endif

#if !defined(IMGUI_TOOLTIP_EXT_H) && defined(__has_include)
#if __has_include("../extensions/TimedTooltip/imgui_tooltip_ext.h")
#include "../extensions/TimedTooltip/imgui_tooltip_ext.h"
#endif
#endif

#if !defined(IMGUI_MARKDOWN_EXT_H) && defined(__has_include)
#if __has_include("../extensions/Markdown/imgui_markdown_ext.h")
#include "../extensions/Markdown/imgui_markdown_ext.h"
#endif
#endif

namespace ImGui {

    namespace Detail {
        inline void AttachCopyButtonTooltip(const char* text, bool is_hovered)
        {
            if (!text || text[0] == '\0') return;
#if defined(IMGUI_MARKDOWN_EXT_H) && defined(IMGUI_TOOLTIP_EXT_H)
            (void)is_hovered;
            TimedTooltip::TextMD("%s", text);
#elif defined(IMGUI_TOOLTIP_EXT_H)
            (void)is_hovered;
            TimedTooltip::Text("%s", text);
#else
            if (is_hovered) {
                SetTooltip("%s", text);
            }
#endif
        }
    } // namespace Detail

    IMGUI_API inline bool CopyButtonEx(const char* str_id, 
                                       const char* text_to_copy, 
                                       float duration = 1.5f, 
                                       const ImVec2& size = ImVec2(0, 0), 
                                       bool* p_is_copied = nullptr)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(str_id ? str_id : "##CopyBtn");

        float scale = (GetFontSize() > 0.0f) ? (GetFontSize() / 14.0f) : 1.0f;
        float font_sz = GetFontSize();
        float icon_w = 10.0f * scale;
        float icon_gap = 4.0f * scale;

        // Extract visible label (before ##) if provided, otherwise default to "Copy"
        const char* label_str = str_id ? str_id : "";
        const char* text_end = FindRenderedTextEnd(label_str);
        std::string custom_label;
        if (text_end > label_str)
        {
            custom_label.assign(label_str, text_end);
        }
        const char* normal_label = custom_label.empty() ? "Copy" : custom_label.c_str();

        ImGuiStorage* storage = GetStateStorage();
        float now = (float)GetTime();
        float last_time = storage ? storage->GetFloat(id, -1000.0f) : -1000.0f;
        bool is_recently_copied = (now >= last_time) && ((now - last_time) < duration);

        const char* copy_label = is_recently_copied ? "Copied!" : normal_label;

        // Size calculation: ensure width accommodates "Copied!" so layout never jitters
        ImVec2 normal_label_sz = CalcTextSize(normal_label);
        ImVec2 copied_label_sz = CalcTextSize("Copied!");
        float max_label_w = ImMax(normal_label_sz.x, copied_label_sz.x);

        float default_w = icon_w + icon_gap + max_label_w + ImMax(14.0f * scale, style.FramePadding.x * 2.0f);
        float default_h = ImMax(font_sz + 4.0f * scale, font_sz + style.FramePadding.y * 2.0f);
        ImVec2 btn_sz = CalcItemSize(size, default_w, default_h);

        const ImVec2 pos = window->DC.CursorPos;
        const ImRect bb(pos, ImVec2(pos.x + btn_sz.x, pos.y + btn_sz.y));
        ItemSize(btn_sz, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;

        bool hovered = false, held = false;
        bool clicked = ButtonBehavior(bb, id, &hovered, &held);

        if (clicked)
        {
            if (text_to_copy)
            {
                SetClipboardText(text_to_copy);
            }
            if (storage)
            {
                storage->SetFloat(id, now);
            }
            is_recently_copied = true;
#if defined(IMGUI_TOOLTIP_EXT_H)
            TimedTooltip::ClearActiveTooltip();
#endif
        }

        if (p_is_copied)
        {
            *p_is_copied = is_recently_copied;
        }

        if (hovered)
        {
            SetMouseCursor(ImGuiMouseCursor_Hand);
            const char* tip_text = is_recently_copied
                ? "**Copied!**\n\nContents copied to clipboard."
                : "**Copy to Clipboard**\n\nCopy contents to the OS clipboard.";
            Detail::AttachCopyButtonTooltip(tip_text, hovered);
        }

        // Render Actionable Button Visuals
        ImDrawList* dl = window->DrawList;
        ImU32 btn_bg, btn_border, btn_text_col, icon_col;
        if (is_recently_copied)
        {
            btn_bg = IM_COL32(28, 92, 45, 230);
            btn_border = IM_COL32(45, 140, 70, 240);
            btn_text_col = IM_COL32(180, 250, 195, 255);
            icon_col = IM_COL32(180, 250, 195, 255);
        }
        else if (held)
        {
            btn_bg = IM_COL32(65, 70, 84, 255);
            btn_border = IM_COL32(95, 105, 125, 255);
            btn_text_col = IM_COL32(240, 243, 250, 255);
            icon_col = IM_COL32(220, 230, 245, 255);
        }
        else if (hovered)
        {
            btn_bg = IM_COL32(52, 56, 68, 240);
            btn_border = IM_COL32(85, 95, 115, 240);
            btn_text_col = IM_COL32(235, 240, 248, 255);
            icon_col = IM_COL32(210, 220, 235, 240);
        }
        else
        {
            btn_bg = IM_COL32(40, 42, 50, 210);
            btn_border = IM_COL32(65, 70, 82, 180);
            btn_text_col = IM_COL32(195, 202, 215, 230);
            icon_col = IM_COL32(165, 175, 192, 210);
        }

        float btn_rounding = (style.FrameRounding > 0.0f) ? style.FrameRounding : (3.5f * scale);
        dl->AddRectFilled(bb.Min, bb.Max, btn_bg, btn_rounding);
        dl->AddRect(bb.Min, bb.Max, btn_border, btn_rounding, 0, 1.0f * scale);

        // Layout icon & text centered inside button:
        ImVec2 cur_text_sz = CalcTextSize(copy_label);
        bool show_text = (btn_sz.x >= icon_w + icon_gap + cur_text_sz.x + 4.0f * scale);
        float total_inner_w = show_text ? (icon_w + icon_gap + cur_text_sz.x) : icon_w;
        float start_inner_x = bb.Min.x + (btn_sz.x - total_inner_w) * 0.5f;
        float icon_x = start_inner_x;
        float icon_y = bb.Min.y + (btn_sz.y - 10.0f * scale) * 0.5f;

        // Draw Icon:
        if (is_recently_copied)
        {
            // Checkmark vector icon
            ImVec2 p1(icon_x, icon_y + 5.5f * scale);
            ImVec2 p2(icon_x + 3.2f * scale, icon_y + 8.5f * scale);
            ImVec2 p3(icon_x + 9.0f * scale, icon_y + 1.5f * scale);
            dl->AddLine(p1, p2, icon_col, 1.5f * scale);
            dl->AddLine(p2, p3, icon_col, 1.5f * scale);
        }
        else
        {
            // Classic overlapping double-sheet copy icon
            // Back page:
            ImVec2 b_min(icon_x + 2.5f * scale, icon_y);
            ImVec2 b_max(icon_x + 9.5f * scale, icon_y + 7.5f * scale);
            dl->AddRect(b_min, b_max, icon_col, 1.0f * scale, 0, 1.0f * scale);

            // Front page:
            ImVec2 f_min(icon_x, icon_y + 2.5f * scale);
            ImVec2 f_max(icon_x + 7.0f * scale, icon_y + 10.0f * scale);
            dl->AddRectFilled(f_min, f_max, btn_bg, 1.0f * scale); // mask out back sheet
            dl->AddRect(f_min, f_max, icon_col, 1.0f * scale, 0, 1.0f * scale);
        }

        // Draw Label:
        if (show_text)
        {
            float text_x = start_inner_x + icon_w + icon_gap;
            float text_y = bb.Min.y + (btn_sz.y - font_sz) * 0.5f;
            dl->AddText(ImVec2(text_x, text_y), btn_text_col, copy_label);
        }

        return clicked;
    }

    IMGUI_API inline bool CopyButton(const char* str_id, 
                                     const char* text_to_copy, 
                                     float duration = 1.5f, 
                                     const ImVec2& size = ImVec2(0, 0))
    {
        return CopyButtonEx(str_id, text_to_copy, duration, size, nullptr);
    }

    IMGUI_API inline bool CopyButton(const char* str_id, 
                                     const std::string& text_to_copy, 
                                     float duration = 1.5f, 
                                     const ImVec2& size = ImVec2(0, 0))
    {
        return CopyButtonEx(str_id, text_to_copy.c_str(), duration, size, nullptr);
    }

    IMGUI_API inline bool CopyLastItemButtonEx(const char* str_id, 
                                               float duration = 1.5f, 
                                               const ImVec2& size = ImVec2(0, 0), 
                                               bool* p_is_copied = nullptr)
    {
        const char* text_to_copy = nullptr;

#if defined(IMGUI_CONTRIB_INTERNAL_INCLUDED) || defined(IMGUI_INTERNAL_H)
        ImGuiContext& g = *GImGui;
        ImGuiID prev_id = g.LastItemData.ID;

        if (prev_id != 0)
        {
            if (ImGuiInputTextState* state = GetInputTextState(prev_id))
            {
                if (state->TextA.Data)
                    text_to_copy = state->TextA.Data;
            }
            else if (g.InputTextState.ID == prev_id && g.InputTextState.TextA.Data)
            {
                text_to_copy = g.InputTextState.TextA.Data;
            }
            else if (g.InputTextDeactivatedState.ID == prev_id && g.InputTextDeactivatedState.TextA.Data)
            {
                text_to_copy = g.InputTextDeactivatedState.TextA.Data;
            }
        }
#endif

        return CopyButtonEx(str_id, text_to_copy, duration, size, p_is_copied);
    }

    IMGUI_API inline bool CopyLastItemButton(const char* str_id, 
                                             float duration = 1.5f, 
                                             const ImVec2& size = ImVec2(0, 0))
    {
        return CopyLastItemButtonEx(str_id, duration, size, nullptr);
    }

    // Semantic Intent Aliases
    IMGUI_API inline bool CopyBufferButtonEx(const char* str_id, const char* text_to_copy, float duration = 1.5f, const ImVec2& size = ImVec2(0, 0), bool* p_is_copied = nullptr)
    {
        return CopyButtonEx(str_id, text_to_copy, duration, size, p_is_copied);
    }
    IMGUI_API inline bool CopyBufferButton(const char* str_id, const char* text_to_copy, float duration = 1.5f, const ImVec2& size = ImVec2(0, 0))
    {
        return CopyButtonEx(str_id, text_to_copy, duration, size, nullptr);
    }
    IMGUI_API inline bool CopyBufferButton(const char* str_id, const std::string& text_to_copy, float duration = 1.5f, const ImVec2& size = ImVec2(0, 0))
    {
        return CopyButtonEx(str_id, text_to_copy.c_str(), duration, size, nullptr);
    }

    IMGUI_API inline bool CopyTextFieldButtonEx(const char* str_id, float duration = 1.5f, const ImVec2& size = ImVec2(0, 0), bool* p_is_copied = nullptr)
    {
        return CopyLastItemButtonEx(str_id, duration, size, p_is_copied);
    }
    IMGUI_API inline bool CopyTextFieldButton(const char* str_id, float duration = 1.5f, const ImVec2& size = ImVec2(0, 0))
    {
        return CopyLastItemButtonEx(str_id, duration, size, nullptr);
    }

} // namespace ImGui

#endif // IMGUI_COPY_BUTTON_H
#endif // !CONTRIB_DISABLE_COPY_BUTTON
