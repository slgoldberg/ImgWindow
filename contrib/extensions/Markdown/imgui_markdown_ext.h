// ============================================================================
// ImgWindow Extension: Markdown
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// Original Upstream: Juliette Foucaut (@juliettef) & Doug Binks (@dougbinks)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Contributors:
//   - Steven L. Goldberg: Lookahead line-wrapping, typographic delimiter
//     protection, hanging indents, dynamic font scaling, pre-calculation,
//     and parameterized styling tags.
// ============================================================================

#if !defined(CONTRIB_DISABLE_MARKDOWN) && \
    !defined(IMGUI_CONTRIB_DISABLE_MARKDOWN) && \
    !defined(IMGUI_DISABLE_CONTRIB_MARKDOWN) && \
    !defined(IMGUI_DISABLE_MARKDOWN) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_MARKDOWN)
#ifndef IMGUI_MARKDOWN_EXT_H
#define IMGUI_MARKDOWN_EXT_H

#include "imgui.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <cctype>
#include <cstdio>
#include <cstring>
#include "imgui_markdown.h"

namespace ImGui {

    // -------------------------------------------------------------------------
    // ImGui Markdown Extensions (Tags & Styling Widgets)
    // -------------------------------------------------------------------------
    
    // 1. Config Registry
    inline const MarkdownConfig*& GetDefaultMarkdownConfig() {
        static const MarkdownConfig* s_DefaultConfig = nullptr;
        return s_DefaultConfig;
    }

    inline void SetDefaultMarkdownConfig(const MarkdownConfig* config) {
        GetDefaultMarkdownConfig() = config;
    }

    inline const MarkdownConfig*& GetActiveMarkdownConfig() {
        static const MarkdownConfig* s_ActiveConfig = nullptr;
        return s_ActiveConfig;
    }

    // 2. Color Helper for Built-in Styling Tags
    inline bool ParseMarkdownColor(const std::string& raw_str, ImVec4& out_col) {
        if (raw_str.empty()) return false;

        std::string str = raw_str;
        for (char& c : str) c = (char)tolower((unsigned char)c);

        // Strip optional key= prefix (e.g. color=0xFFA500, col=orange, bg=#FF0000, text=white)
        if (str.rfind("bgcolor=", 0) == 0)       str = str.substr(8);
        else if (str.rfind("bgcol=", 0) == 0)    str = str.substr(6);
        else if (str.rfind("bg=", 0) == 0)       str = str.substr(3);
        else if (str.rfind("fgcolor=", 0) == 0)  str = str.substr(8);
        else if (str.rfind("fgcol=", 0) == 0)    str = str.substr(6);
        else if (str.rfind("fg=", 0) == 0)       str = str.substr(3);
        else if (str.rfind("color=", 0) == 0)    str = str.substr(6);
        else if (str.rfind("col=", 0) == 0)      str = str.substr(4);
        else if (str.rfind("text=", 0) == 0)     str = str.substr(5);
        if (str.size() >= 2 && ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\''))) {
            str = str.substr(1, str.size() - 2);
        }
        if (str.empty()) return false;
        
        // Named UI colors
        if (str == "red")     { out_col = ImVec4(0.95f, 0.25f, 0.25f, 1.0f); return true; }
        if (str == "green")   { out_col = ImVec4(0.25f, 0.85f, 0.35f, 1.0f); return true; }
        if (str == "blue")    { out_col = ImVec4(0.30f, 0.65f, 1.0f,  1.0f); return true; }
        if (str == "yellow")  { out_col = ImVec4(1.0f,  0.85f, 0.20f, 1.0f); return true; }
        if (str == "orange")  { out_col = ImVec4(1.0f,  0.647f, 0.0f, 1.0f); return true; } // 0xFFA500
        if (str == "purple" || str == "violet") { out_col = ImVec4(0.60f, 0.25f, 0.90f, 1.0f); return true; }
        if (str == "cyan")    { out_col = ImVec4(0.20f, 0.85f, 0.95f, 1.0f); return true; }
        if (str == "magenta") { out_col = ImVec4(0.90f, 0.30f, 0.90f, 1.0f); return true; }
        if (str == "white")   { out_col = ImVec4(1.0f,  1.0f,  1.0f,  1.0f); return true; }
        if (str == "black")   { out_col = ImVec4(0.0f,  0.0f,  0.0f,  1.0f); return true; }
        if (str == "gray" || str == "grey") { out_col = ImVec4(0.60f, 0.60f, 0.60f, 1.0f); return true; }
        if (str == "gold")    { out_col = ImVec4(1.0f,  0.84f, 0.0f,  1.0f); return true; }
        if (str == "dark")    { out_col = ImVec4(0.18f, 0.18f, 0.22f, 1.0f); return true; }
        if (str == "light")   { out_col = ImVec4(0.90f, 0.90f, 0.92f, 1.0f); return true; }
        if (str == "danger")  { out_col = ImVec4(0.85f, 0.15f, 0.15f, 1.0f); return true; }
        if (str == "warning") { out_col = ImVec4(0.95f, 0.75f, 0.10f, 1.0f); return true; }
        if (str == "success") { out_col = ImVec4(0.15f, 0.70f, 0.25f, 1.0f); return true; }
        if (str == "info")    { out_col = ImVec4(0.20f, 0.55f, 0.90f, 1.0f); return true; }

        // Hex formats: #RRGGBB, #RRGGBBAA, 0xRRGGBB, 0xRRGGBBAA
        size_t start = 0;
        if (str[0] == '#') {
            start = 1;
        } else if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            start = 2;
        } else if (isxdigit((unsigned char)str[0])) {
            start = 0;
        } else {
            return false;
        }

        std::string hex = str.substr(start);
        if (hex.length() == 6) {
            unsigned int val = 0;
            if (sscanf(hex.c_str(), "%x", &val) == 1) {
                out_col.x = ((val >> 16) & 0xFF) / 255.0f;
                out_col.y = ((val >> 8) & 0xFF) / 255.0f;
                out_col.z = (val & 0xFF) / 255.0f;
                out_col.w = 1.0f;
                return true;
            }
        } else if (hex.length() == 8) {
            unsigned long long val = 0;
            if (sscanf(hex.c_str(), "%llx", &val) == 1) {
                out_col.x = ((val >> 24) & 0xFF) / 255.0f;
                out_col.y = ((val >> 16) & 0xFF) / 255.0f;
                out_col.z = ((val >> 8) & 0xFF) / 255.0f;
                out_col.w = (val & 0xFF) / 255.0f;
                return true;
            }
        }
        return false;
    }

    // 3. Custom Markdown Styling Tags Pre-Processor
    struct MarkdownTagStyle {
        std::string clean_text;
        std::string color_param; // Legacy single-color parameter
        std::string bg_color;    // Background color override or primary color
        std::string fg_color;    // Foreground / text color override
        bool is_bold = false;
        bool is_italic = false;
        bool is_mono = false;
    };

    inline MarkdownTagStyle ParseMarkdownTagStyle(const std::string& inner_text, const std::string& param) {
        MarkdownTagStyle style;
        style.clean_text = inner_text;

        auto strip_quotes = [](std::string s) -> std::string {
            if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
                return s.substr(1, s.size() - 2);
            }
            return s;
        };

        // 1. Check if param contains 'bold', 'italic', 'mono', or color tokens
        std::string p = param;
        for (char& c : p) {
            if (c == ',' || c == ';') c = ' ';
        }
        size_t pos = 0;
        bool found_explicit_style = false;
        while (pos < p.length()) {
            while (pos < p.length() && (p[pos] == ' ' || p[pos] == '\t')) ++pos;
            if (pos >= p.length()) break;
            size_t end = pos;
            while (end < p.length() && p[end] != ' ' && p[end] != '\t') ++end;
            std::string token = p.substr(pos, end - pos);
            std::string t = token;
            for (char& c : t) c = (char)tolower((unsigned char)c);
            if (t == "bold" || t == "b" || t == "font=bold" || t == "style=bold") {
                style.is_bold = true;
                found_explicit_style = true;
            } else if (t == "italic" || t == "italics" || t == "i" || t == "font=italic" || t == "style=italic") {
                style.is_italic = true;
                found_explicit_style = true;
            } else if (t == "mono" || t == "monospace" || t == "code" || t == "m" || t == "font=mono" || t == "style=mono") {
                style.is_mono = true;
                found_explicit_style = true;
            } else {
                if (t.rfind("fgcolor=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(8));
                } else if (t.rfind("fgcol=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(6));
                } else if (t.rfind("fg=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(3));
                } else if (t.rfind("text=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(5));
                } else if (t.rfind("bgcolor=", 0) == 0) {
                    style.bg_color = strip_quotes(token.substr(8));
                } else if (t.rfind("bgcol=", 0) == 0) {
                    style.bg_color = strip_quotes(token.substr(6));
                } else if (t.rfind("bg=", 0) == 0) {
                    style.bg_color = strip_quotes(token.substr(3));
                } else if (t.rfind("color=", 0) == 0) {
                    std::string val = strip_quotes(token.substr(6));
                    if (style.bg_color.empty()) style.bg_color = val;
                    else style.fg_color = val;
                } else if (t.rfind("col=", 0) == 0) {
                    std::string val = strip_quotes(token.substr(4));
                    if (style.bg_color.empty()) style.bg_color = val;
                    else style.fg_color = val;
                } else {
                    // Bare token (e.g. "green", "#ff0000", "danger")
                    std::string val = strip_quotes(token);
                    if (!val.empty()) {
                        if (style.bg_color.empty()) {
                            style.bg_color = val;
                        } else if (style.fg_color.empty()) {
                            style.fg_color = val;
                        }
                    }
                }
            }
            pos = end;
        }
        if (!style.bg_color.empty()) {
            style.color_param = style.bg_color;
        } else if (!style.fg_color.empty()) {
            style.color_param = style.fg_color;
        } else if (!found_explicit_style && !param.empty()) {
            style.color_param = strip_quotes(param);
            style.bg_color = style.color_param;
        }

        // 2. Normalize any UTF-8 non-breaking spaces (\xC2\xA0, e.g. macOS Option+Space)
        // to standard ASCII ' ' (0x20) so fonts baking only 7-bit ASCII (0x20..0x7E)
        // still measure and render the exact advance width of a space glyph:
        for (size_t i = 0; i + 1 < style.clean_text.length(); ++i) {
            if ((unsigned char)style.clean_text[i] == 0xC2 &&
                (unsigned char)style.clean_text[i + 1] == 0xA0) {
                style.clean_text.replace(i, 2, " ");
            }
        }

        // 3. Locate non-whitespace core to check for markdown bold/italic/code syntax
        // (**text**, __text__, *text*, _text_, `text`) WITHOUT discarding intentional
        // leading/trailing padding spaces (e.g., "<pill> " ICON_FA_RETWEET " </pill>"):
        size_t start_idx = 0;
        while (start_idx < style.clean_text.length() && style.clean_text[start_idx] == ' ') ++start_idx;
        size_t end_idx = style.clean_text.length();
        while (end_idx > start_idx && style.clean_text[end_idx - 1] == ' ') --end_idx;

        std::string lead_spaces = style.clean_text.substr(0, start_idx);
        std::string core = style.clean_text.substr(start_idx, end_idx - start_idx);
        std::string trail_spaces = style.clean_text.substr(end_idx);

        if ((core.length() >= 4 && core.rfind("**", 0) == 0 && core.compare(core.length() - 2, 2, "**") == 0) ||
            (core.length() >= 4 && core.rfind("__", 0) == 0 && core.compare(core.length() - 2, 2, "__") == 0))
        {
            style.is_bold = true;
            core = core.substr(2, core.length() - 4);
        }
        else if ((core.length() >= 2 && core.front() == '*' && core.back() == '*') ||
                 (core.length() >= 2 && core.front() == '_' && core.back() == '_'))
        {
            style.is_italic = true;
            core = core.substr(1, core.length() - 2);
        }
        else if (core.length() >= 2 && core.front() == '`' && core.back() == '`')
        {
            style.is_mono = true;
            core = core.substr(1, core.length() - 2);
        }
        style.clean_text = lead_spaces + core + trail_spaces;

        return style;
    }

    inline void PushMarkdownTagFont(const MarkdownConfig* config, const MarkdownTagStyle& style) {
        if (!config) config = GetActiveMarkdownConfig();
        if (!config) config = GetDefaultMarkdownConfig();
        if (style.is_mono && config && config->monoFont) {
            ImGui::PushFont(config->monoFont);
            return;
        }
        if (!config || !config->formatCallback) return;
        if (style.is_bold) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 2; // Bold
            config->formatCallback(info, true);
        } else if (style.is_italic) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 1; // Italic
            config->formatCallback(info, true);
        }
    }

    inline void PopMarkdownTagFont(const MarkdownConfig* config, const MarkdownTagStyle& style) {
        if (!config) config = GetActiveMarkdownConfig();
        if (!config) config = GetDefaultMarkdownConfig();
        if (style.is_mono && config && config->monoFont) {
            ImGui::PopFont();
            return;
        }
        if (!config || !config->formatCallback) return;
        if (style.is_bold) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 2; // Bold
            config->formatCallback(info, false);
        } else if (style.is_italic) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 1; // Italic
            config->formatCallback(info, false);
        }
    }

    using CustomTagCallback = std::function<void(const std::string& inner_text, const std::string& param)>;

    inline void EnsureTagFitsOnLine(float required_width) {
        const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
        if (config && config->wrapWidth < 0.0f) return;
        float lineStartX = ImGui::GetCurrentWindow()->Pos.x + ImGui::GetCurrentWindow()->DC.Indent.x;
        float cursorX = ImGui::GetCursorScreenPos().x;
        float avail = (config && config->wrapWidth > 0.0f) ? (config->wrapWidth - (cursorX - lineStartX)) : ImGui::GetContentRegionAvail().x;
        float scale = (ImGui::GetFontSize() > 0.0f) ? (ImGui::GetFontSize() / 14.0f) : 1.0f;
        // Only wrap down if we are mid-line (cursor is not at the start) and the tag doesn't fit on this line:
        if ((cursorX - lineStartX) > (10.0f * scale) && required_width > avail) {
            ImGui::NewLine();
        }
    }

    inline std::unordered_map<std::string, CustomTagCallback>& GetCustomMarkdownTags() {
        static std::unordered_map<std::string, CustomTagCallback> s_CustomTags;
        static bool s_InitializedDefaults = false;
        if (!s_InitializedDefaults) {
            s_InitializedDefaults = true;
#ifndef IMGUI_DISABLE_MARKDOWN_DEFAULT_TAGS
            // Built-in Tag 1: <color=...>, <col=...>
            auto colorHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                EnsureTagFitsOnLine(text_size.x);

                ImVec4 col(1.0f, 1.0f, 1.0f, 1.0f);
                std::string col_str = !style.fg_color.empty() ? style.fg_color : (!style.color_param.empty() ? style.color_param : style.bg_color);
                bool has_col = ParseMarkdownColor(col_str, col);
                if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);

                bool no_wrap = (config && config->wrapWidth < 0.0f);
                float avail_w = (config && config->wrapWidth > 0.0f) ? config->wrapWidth : ImGui::GetContentRegionAvail().x;
                if (no_wrap || text_size.x <= avail_w) {
                    ImGui::TextUnformatted(style.clean_text.c_str());
                } else {
                    float wrap_pos = (config && config->wrapWidth > 0.0f) ? (ImGui::GetCursorPos().x + config->wrapWidth) : 0.0f;
                    ImGui::PushTextWrapPos(wrap_pos);
                    ImGui::TextUnformatted(style.clean_text.c_str());
                    ImGui::PopTextWrapPos();
                }

                if (has_col) ImGui::PopStyleColor();

                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["color"] = colorHandler;
            s_CustomTags["col"]   = colorHandler;

            // Built-in Tag 2: <backdrop=...>, <bg=...>, <highlight=...>
            // Draws an exact filled rectangular pill behind the text bounding box with rounded corners
            auto backdropHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 bg_col(1.0f, 0.90f, 0.20f, 0.65f); // default soft yellow highlight
                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;
                ParseMarkdownColor(bg_str, bg_col);

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                float scale = (ImGui::GetFontSize() > 0.0f) ? (ImGui::GetFontSize() / 14.0f) : 1.0f;
                float pad_x = 3.0f * scale;
                float pad_y = 1.0f * scale;
                float rounding = 3.0f * scale;

                EnsureTagFitsOnLine(text_size.x + pad_x * 2.0f);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImU32 col_u32 = ImGui::GetColorU32(bg_col);
                // TrueType fonts include ~1px of diacritic headroom at pos.y while descenders ('p', 'g', 'y')
                // reach pos.y + text_size.y; starting at pos.y and extending +pad_y below centers the box optically:
                draw_list->AddRectFilled(
                    ImVec2(pos.x - pad_x, pos.y),
                    ImVec2(pos.x + text_size.x + pad_x, pos.y + text_size.y + pad_y),
                    col_u32, rounding
                );

                if (!style.fg_color.empty()) {
                    ImVec4 fg_col;
                    if (ParseMarkdownColor(style.fg_color, fg_col)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, fg_col);
                        ImGui::TextUnformatted(style.clean_text.c_str());
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::TextUnformatted(style.clean_text.c_str());
                    }
                } else {
                    // Auto-contrast text color if background is dark
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum < 0.35f) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        ImGui::TextUnformatted(style.clean_text.c_str());
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::TextUnformatted(style.clean_text.c_str());
                    }
                }

                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["backdrop"]  = backdropHandler;
            s_CustomTags["bg"]        = backdropHandler;
            s_CustomTags["highlight"] = backdropHandler;

            // Built-in Tag 3: <badge=...>, <pill=...>, <tag=...>
            // Renders a sleek pill/badge with colored background and contrasting text
            auto badgeHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 bg_col = ImVec4(0.25f, 0.25f, 0.30f, 1.0f); // default charcoal
                ImVec4 text_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;

                if (bg_str == "danger" || bg_str == "red") {
                    bg_col = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
                } else if (bg_str == "warning" || bg_str == "yellow") {
                    bg_col = ImVec4(0.95f, 0.75f, 0.10f, 1.0f);
                    text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                } else if (bg_str == "success" || bg_str == "green") {
                    bg_col = ImVec4(0.15f, 0.70f, 0.25f, 1.0f);
                } else if (bg_str == "info" || bg_str == "blue") {
                    bg_col = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
                } else if (!bg_str.empty()) {
                    ParseMarkdownColor(bg_str, bg_col);
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum > 0.65f) text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                }

                // If explicit foreground/text color is specified, override the automatic contrast color:
                if (!style.fg_color.empty()) {
                    ParseMarkdownColor(style.fg_color, text_col);
                }

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                float btn_width = text_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
                EnsureTagFitsOnLine(btn_width);

                bool has_ascii_text = false;
                for (char c : style.clean_text) {
                    if ((unsigned char)c > ' ' && (unsigned char)c < 0x80) {
                        has_ascii_text = true;
                        break;
                    }
                }

                if (has_ascii_text) {
                    // For text pills (with ascenders/descenders like 'p', 'g', 'y'), SmallButton's
                    // FramePadding.y = 0 leaves 0px below descenders and ~2px of diacritic headroom at top.
                    // Draw the rounded pill shifted +1px down so ascenders and descenders are balanced:
                    float scale = (ImGui::GetFontSize() > 0.0f) ? (ImGui::GetFontSize() / 14.0f) : 1.0f;
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    if (ImGui::GetCurrentWindow() && ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset > 0.0f) {
                        pos.y += ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset;
                    }
                    float rounding = ImMax(3.0f * scale, ImGui::GetStyle().FrameRounding);
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(pos.x, pos.y + 0.5f * scale),
                        ImVec2(pos.x + btn_width, pos.y + text_size.y + 1.0f * scale),
                        ImGui::GetColorU32(bg_col), rounding
                    );
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                    ImGui::SmallButton(style.clean_text.c_str());
                    ImGui::PopStyleColor(4);
                    ImGui::PopStyleVar();
                } else {
                    // Icon-only pill (e.g., FontAwesome glyph): keep standard SmallButton frame bounds
                    ImGui::PushStyleColor(ImGuiCol_Button, bg_col);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_col);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg_col);
                    ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                    ImGui::SmallButton(style.clean_text.c_str());
                    ImGui::PopStyleColor(4);
                }

                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["badge"] = badgeHandler;
            s_CustomTags["pill"]  = badgeHandler;
            s_CustomTags["tag"]   = badgeHandler;
            s_CustomTags["kbd"]   = badgeHandler;
            s_CustomTags["key"]   = badgeHandler;

            // Built-in Tag 4: <btn=...>
            s_CustomTags["btn"] = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);
                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                float btn_width = text_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
                EnsureTagFitsOnLine(btn_width);
                ImGui::SmallButton(style.clean_text.c_str());
                PopMarkdownTagFont(config, style);
            };

            // Built-in Tag 5: <mono>, <code>
            // Renders monospace code span with subtle rounded background pill
            auto monoCodeHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();

                ImFont* mono_font = (config && config->monoFont) ? config->monoFont : nullptr;
                if (mono_font) {
                    ImGui::PushFont(mono_font);
                } else {
                    PushMarkdownTagFont(config, style);
                }

                // Default: subtle translucent gray rounded pill behind code
                ImVec4 bg_col(0.5f, 0.5f, 0.5f, 0.24f);
                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;
                if (!bg_str.empty()) {
                    ParseMarkdownColor(bg_str, bg_col);
                }

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                float scale = (ImGui::GetFontSize() > 0.0f) ? (ImGui::GetFontSize() / 14.0f) : 1.0f;
                float pad_x = 3.0f * scale;
                float pad_y = 1.0f * scale;
                float rounding = 3.0f * scale;

                EnsureTagFitsOnLine(text_size.x + pad_x * 2.0f);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImU32 col_u32 = ImGui::GetColorU32(bg_col);
                draw_list->AddRectFilled(
                    ImVec2(pos.x - pad_x, pos.y),
                    ImVec2(pos.x + text_size.x + pad_x, pos.y + text_size.y + pad_y),
                    col_u32, rounding
                );

                if (!style.fg_color.empty()) {
                    ImVec4 fg_col;
                    if (ParseMarkdownColor(style.fg_color, fg_col)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, fg_col);
                        ImGui::TextUnformatted(style.clean_text.c_str());
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::TextUnformatted(style.clean_text.c_str());
                    }
                } else if (!bg_str.empty()) {
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum < 0.35f) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        ImGui::TextUnformatted(style.clean_text.c_str());
                        ImGui::PopStyleColor();
                    } else if (lum > 0.75f) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::TextUnformatted(style.clean_text.c_str());
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::TextUnformatted(style.clean_text.c_str());
                    }
                } else {
                    ImGui::TextUnformatted(style.clean_text.c_str());
                }

                if (mono_font) {
                    ImGui::PopFont();
                } else {
                    PopMarkdownTagFont(config, style);
                }
            };
            s_CustomTags["mono"] = monoCodeHandler;
            s_CustomTags["code"] = monoCodeHandler;
#endif
        }
        return s_CustomTags;
    }

    // Registration APIs (supporting both 2-arg parameterized and 1-arg simple callbacks):
    inline void RegisterMarkdownTag(const std::string& tag_name, std::function<void(const std::string& inner_text, const std::string& param)> callback) {
        GetCustomMarkdownTags()[tag_name] = callback;
    }

    inline void RegisterMarkdownTag(const std::string& tag_name, std::function<void(const std::string& inner_text)> callback) {
        RegisterMarkdownTag(tag_name, [callback](const std::string& text, const std::string&) {
            callback(text);
        });
    }

    // Backward-compatibility aliases
    inline void RegisterMarkdownWidget(const std::string& tag_name, std::function<void(const std::string& inner_text, const std::string& param)> callback) {
        RegisterMarkdownTag(tag_name, callback);
    }
    inline void RegisterMarkdownWidget(const std::string& tag_name, std::function<void(const std::string& inner_text)> callback) {
        RegisterMarkdownTag(tag_name, callback);
    }

    // 4. Core Markdown Extension Engine (Unified Single-Pass Pipeline)
    inline void MarkdownExt(const char* markdown_text, size_t markdown_len, const MarkdownConfig* config_override = nullptr) {
        if (!markdown_text) return;
        const MarkdownConfig* config = config_override ? config_override : GetDefaultMarkdownConfig();
        static const MarkdownConfig s_FallbackConfig;
        if (!config) {
            config = &s_FallbackConfig;
        }

        MarkdownConfig local_config = *config;
        const MarkdownConfig* prev_active = GetActiveMarkdownConfig();
        GetActiveMarkdownConfig() = &local_config;

        local_config.tagCallback = [](const MarkdownTagData& data) {
            std::string tag_name(data.tagName, data.tagNameLength);
            std::string tag_param(data.tagParam ? data.tagParam : "", data.tagParamLength);
            std::string inner_text(data.innerText, data.innerTextLength);
            
            const MarkdownConfig* prev = GetActiveMarkdownConfig();
            if (data.config) GetActiveMarkdownConfig() = data.config;

            auto& tags = GetCustomMarkdownTags();
            auto it = tags.find(tag_name);
            if (it != tags.end()) {
                it->second(inner_text, tag_param);
            } else {
                ImGui::TextUnformatted(inner_text.c_str());
            }

            GetActiveMarkdownConfig() = prev;
        };

        Markdown::Render(markdown_text, markdown_len, local_config);

        GetActiveMarkdownConfig() = prev_active;
    }

    inline void MarkdownExt(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        MarkdownExt(markdown_text.c_str(), markdown_text.length(), config_override);
    }

} // namespace ImGui

#ifndef IMGUI_LINK_CLICK_RESULT_DEFINED
#define IMGUI_LINK_CLICK_RESULT_DEFINED
namespace ImGui {
namespace Detail {
    struct LinkClickToggleState {
        ImGuiContext* context                 = nullptr;
        int           frame_count             = -1;
        ImGuiID       item_id                 = 0;
        ImVec2        item_min                = ImVec2(-1.0f, -1.0f);
        bool          toggle_mode             = false;
        bool          click_consumed          = false;
        bool          is_menu_item            = false;
        bool          menu_item_selected      = false;
        ImGuiContext* menu_active_context     = nullptr;
        int           menu_active_frame       = -1;
        bool          wheel_latched           = false;
        ImGuiContext* wheel_latch_context     = nullptr;
        int           wheel_latch_alive_frame = -1;
        ImVec2        wheel_latch_mouse_pos   = ImVec2(-1.0f, -1.0f);
    };

    inline LinkClickToggleState& GetLinkClickToggleState() {
        static LinkClickToggleState s_State;
        return s_State;
    }

    inline void RegisterItemTooltipToggle(bool is_toggle_eligible = true) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        ImGuiID cur_id = GetItemID();
        ImVec2 cur_min = GetItemRectMin();
        bool same_item = (st.context == cur_ctx) && (st.frame_count == cur_frame) &&
                         ((cur_id != 0 && st.item_id == cur_id) ||
                          (cur_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
        if (!same_item) {
            st.context            = cur_ctx;
            st.frame_count        = cur_frame;
            st.item_id            = cur_id;
            st.item_min           = cur_min;
            st.click_consumed     = false;
            st.is_menu_item       = false;
            st.menu_item_selected = false;
        }
        if (IsItemClicked()) {
            st.wheel_latched = false;
        }
        st.toggle_mode = st.click_consumed ? false : is_toggle_eligible;
    }

    inline void RegisterMenuItemSelection(bool is_selected, bool is_clicked = false, bool is_toggle_eligible = false) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        st.context             = cur_ctx;
        st.frame_count         = cur_frame;
        st.item_id             = GetItemID();
        st.item_min            = GetItemRectMin();
        st.click_consumed      = false;
        st.toggle_mode         = is_toggle_eligible;
        st.is_menu_item        = true;
        st.menu_item_selected  = is_selected;
        st.menu_active_context = cur_ctx;
        st.menu_active_frame   = cur_frame;
        if (is_clicked) {
            st.wheel_latched = false;
        }
    }

    inline bool KeepWheelLatchAlive(bool wheel_stepped) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        ImVec2 mouse_pos = GetIO().MousePos;
        if (wheel_stepped) {
            st.wheel_latched           = true;
            st.wheel_latch_context     = cur_ctx;
            st.wheel_latch_alive_frame = cur_frame;
            st.wheel_latch_mouse_pos   = mouse_pos;
            return false;
        } else if (st.wheel_latched && st.wheel_latch_context == cur_ctx) {
            float dx = mouse_pos.x - st.wheel_latch_mouse_pos.x;
            float dy = mouse_pos.y - st.wheel_latch_mouse_pos.y;
            if (dx > 6.0f || dx < -6.0f || dy > 6.0f || dy < -6.0f) {
                st.wheel_latched = false;
                return true;
            } else {
                st.wheel_latch_alive_frame = cur_frame;
            }
        }
        return false;
    }

    inline bool ResolveItemTooltipHover(ImGuiID raw_item_id, bool raw_hovered) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        if (!st.wheel_latched || st.wheel_latch_context != cur_ctx) {
            return raw_hovered;
        }
        ImVec2 mouse_pos = GetIO().MousePos;
        float dx = mouse_pos.x - st.wheel_latch_mouse_pos.x;
        float dy = mouse_pos.y - st.wheel_latch_mouse_pos.y;
        bool menu_still_open = (st.menu_active_context == cur_ctx) && (cur_frame - st.menu_active_frame <= 1);
        if ((!menu_still_open && cur_frame - st.wheel_latch_alive_frame > 2) ||
            dx > 6.0f || dx < -6.0f || dy > 6.0f || dy < -6.0f) {
            st.wheel_latched = false;
            return raw_hovered;
        }
        if (menu_still_open) {
            ImVec2 cur_min = GetItemRectMin();
            bool same_item = (st.context == cur_ctx) && (st.frame_count == cur_frame) &&
                             ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                              (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
            if (same_item && st.is_menu_item) {
                return st.menu_item_selected;
            }
            return false;
        }
        return raw_hovered;
    }

    inline void MarkLastItemClickConsumed() {
        LinkClickToggleState& st = GetLinkClickToggleState();
        if (st.context == GetCurrentContext() && st.frame_count == GetFrameCount()) {
            st.click_consumed = true;
            st.toggle_mode    = false;
        }
    }

    inline bool ShouldToggleTooltipOnClick(ImGuiID raw_item_id, bool is_item_active) {
        (void)is_item_active;
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImVec2 cur_min = GetItemRectMin();
        if (st.context == GetCurrentContext() && st.frame_count == GetFrameCount()) {
            if ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y)) {
                return st.toggle_mode;
            }
        }
        return false;
    }
} // namespace Detail

    struct LinkClickResult {
        bool clicked;

        constexpr LinkClickResult(bool is_clicked = false) noexcept : clicked(is_clicked) {}

        operator bool() const noexcept {
            Detail::MarkLastItemClickConsumed();
            return clicked;
        }
    };
} // namespace ImGui
#endif // IMGUI_LINK_CLICK_RESULT_DEFINED

namespace ImGui {

    namespace Markdown {

        using Config = MarkdownConfig;

        // Tag registration convenience aliases
        inline void RegisterTag(const std::string& tag_name, std::function<void(const std::string& inner_text, const std::string& param)> callback) {
            RegisterMarkdownTag(tag_name, callback);
        }
        inline void RegisterTag(const std::string& tag_name, std::function<void(const std::string& inner_text)> callback) {
            RegisterMarkdownTag(tag_name, callback);
        }

        namespace Detail {
            inline void RenderWithWrapMode(const char* text, size_t len, float default_wrap_width, const MarkdownConfig* config_override) {
                if (!text) return;
                const MarkdownConfig* base = config_override ? config_override : GetDefaultMarkdownConfig();
                static const MarkdownConfig s_FallbackConfig;
                if (!base) {
                    base = &s_FallbackConfig;
                }
                MarkdownConfig local_cfg = *base;
                // If config_override explicitly specifies a positive wrapWidth, honor it unless default_wrap_width > 0.0f:
                if (default_wrap_width > 0.0f) {
                    local_cfg.wrapWidth = default_wrap_width;
                } else if (!config_override || config_override->wrapWidth == 0.0f) {
                    if (default_wrap_width < 0.0f && ImGui::GetCurrentWindow() && ImGui::GetCurrentWindow()->DC.TextWrapPos >= 0.0f) {
                        // Honor active ImGui::PushTextWrapPos() stack just like standard ImGui::Text / TextUnformatted:
                        float wp = ImGui::GetCurrentWindow()->DC.TextWrapPos;
                        local_cfg.wrapWidth = (wp == 0.0f) ? 0.0f : ImMax(10.0f, wp - ImGui::GetCursorPos().x);
                    } else {
                        local_cfg.wrapWidth = default_wrap_width;
                    }
                }
                ImGui::BeginGroup();
                MarkdownExt(text, len, &local_cfg);
                ImGui::EndGroup();
            }
        } // namespace Detail

        // -------------------------------------------------------------------------
        // Standard Widget API (mirroring standard ImGui vocabulary)
        // -------------------------------------------------------------------------
        // - MD::TextUnformatted / MD::Text: Do NOT auto-wrap by default (wrapWidth = -1.0f,
        //   single line per '\n') unless an explicit wrap_width > 0.0f, wrap_size.x > 0.0f,
        //   or config_override->wrapWidth > 0.0f is supplied.
        // - MD::TextWrapped: Automatically wraps to GetContentRegionAvail().x (wrapWidth = 0.0f),
        //   or to an explicit wrap_width / wrap_size.x if supplied.

        inline void TextUnformatted(const char* text, const char* text_end = nullptr, const MarkdownConfig* config_override = nullptr) {
            if (!text) return;
            size_t len = text_end ? (size_t)(text_end - text) : std::strlen(text);
            Detail::RenderWithWrapMode(text, len, -1.0f, config_override);
        }

        inline void TextUnformatted(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), -1.0f, config_override);
        }

        inline void Text(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), -1.0f, config_override);
        }

        inline void Text(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_width > 0.0f) ? wrap_width : -1.0f, config_override);
        }

        inline void Text(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_size.x > 0.0f) ? wrap_size.x : -1.0f, config_override);
        }

        inline void TextWrapped(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), 0.0f, config_override);
        }

        inline void TextWrapped(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_width > 0.0f) ? wrap_width : 0.0f, config_override);
        }

        inline void TextWrapped(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_size.x > 0.0f) ? wrap_size.x : 0.0f, config_override);
        }

        // Clickable Markdown Text Links (shows Hand cursor on hover; toggles TimedTooltip on click
        // unless the return value is tested in an `if (...)` block):
        inline LinkClickResult TextUnformattedLink(const char* text, const char* text_end = nullptr, const MarkdownConfig* config_override = nullptr) {
            TextUnformatted(text, text_end, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextUnformattedLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            return TextUnformattedLink(markdown_text.c_str(), markdown_text.c_str() + markdown_text.length(), config_override);
        }

#pragma push_macro("TextLink")
#undef TextLink
        inline LinkClickResult TextLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextLink(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, wrap_width, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextLink(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, wrap_size.x, config_override);
        }

        inline LinkClickResult TextLinkContrib(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, config_override);
        }

        inline LinkClickResult TextLinkContrib(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, wrap_width, config_override);
        }

        inline LinkClickResult TextLinkContrib(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, wrap_size.x, config_override);
        }
#pragma pop_macro("TextLink")

        inline LinkClickResult TextWrappedLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            TextWrapped(markdown_text, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextWrappedLink(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            TextWrapped(markdown_text, wrap_width, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextWrappedLink(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            return TextWrappedLink(markdown_text, wrap_size.x, config_override);
        }

        // Backward-compatibility and convenience aliases
        inline void TextMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, config_override);
        }

        inline void TextWrappedMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            TextWrapped(markdown_text, config_override);
        }

        inline bool ButtonMD(const std::string& label, const std::string& tooltip_markdown = "", float tooltip_wrap_width = 400.0f, bool tooltip_allowed = true) {
            bool clicked = ImGui::Button(label.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            
            if (tooltip_allowed && !tooltip_markdown.empty() && ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                #if IMGUI_VERSION_NUM >= 18989
                ImGui::BeginChild("##md_tt", ImVec2(tooltip_wrap_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
#else
                ImGui::BeginChild("##md_tt", ImVec2(tooltip_wrap_width, 0.0f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
#endif
                TextWrapped(tooltip_markdown);
                ImGui::EndChild();
                ImGui::EndTooltip();
            }
            
            return clicked;
        }

        inline bool DelayedTooltipMD(const std::string& markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
            bool showing = false;
            if (is_allowed && ImGui::IsItemHovered()) {
                ImGuiID id = ImGui::GetItemID();
                ImGuiStorage* storage = ImGui::GetStateStorage();
                
                int hover_start = storage->GetInt(id, 0);
                if (hover_start == 0) {
                    hover_start = ImGui::GetFrameCount();
                    storage->SetInt(id, hover_start);
                }

                if (ImGui::GetFrameCount() - hover_start >= delay_frames) {
                    showing = true;
                    ImGui::BeginTooltip();
                    #if IMGUI_VERSION_NUM >= 18989
                    ImGui::BeginChild("##md_delay_tt", ImVec2(wrap_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
#else
                    ImGui::BeginChild("##md_delay_tt", ImVec2(wrap_width, 0.0f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
#endif
                    TextWrapped(markdown_text);
                    ImGui::EndChild();
                    ImGui::EndTooltip();
                }
            } else {
                ImGui::GetStateStorage()->SetInt(ImGui::GetItemID(), 0);
            }
            return showing;
        }


        // -------------------------------------------------------------------------
        // Layout Calculation API
        // -------------------------------------------------------------------------

        // Mathematically calculates the exact ImVec2 dimensions of a parsed markdown string 
        // WITHOUT rendering it to the screen. Perfect for pre-calculating X-Plane OS window boundaries!
        inline ImVec2 CalcMarkdownSize(ImGuiID hash_id, int pass, const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            ImVec2 calculated_size(0, 0);
            
            // Generate a perfectly unique window name for this specific tooltip and pass.
            // This mathematically guarantees ImGui can NEVER cache the window's position across frames,
            // which completely defeats ImGui's Viewport Culling optimization and prevents the 2-pixel tall bug!
            char measure_name[64];
            // By appending ImGui::GetFrameCount(), we guarantee the window name is unique for every single hover lifecycle!
            // This prevents ImGui from caching the -10000 position from a previous hover and culling it on subsequent hovers!
            snprintf(measure_name, sizeof(measure_name), "##md_measure_%08X_%d_%d", hash_id, pass, ImGui::GetFrameCount());
            
            float s = (font_scale > 0.0f) ? font_scale : 1.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f * s, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f * s, 1.0f * s));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f * s);
            
            ImGui::SetNextWindowPos(ImVec2(-10000.0f, -10000.0f));
            ImGui::Begin(measure_name, nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
            if (font_scale > 0.0f) {
                ImGui::SetWindowFontScale(font_scale);
            }
        
        // Feed it a massive constraint if 0.0f so it doesn't word-wrap infinitely to 1 char
        float child_width = (wrap_width > 0.0f) ? wrap_width : 99999.0f;
        // Pass 10000.0f Y to ensure we never clip vertically during the dry run.
        // We MUST NOT pass AlwaysAutoResize, or it will infinitely expand X and never word-wrap!
#if IMGUI_VERSION_NUM >= 18989
        ImGui::BeginChild("##md_measure_child", ImVec2(child_width, 10000.0f), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
#else
        ImGui::BeginChild("##md_measure_child", ImVec2(child_width, 10000.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
#endif
        
        ImGui::BeginGroup();
        MarkdownExt(markdown_text, md_config);
        ImGui::EndGroup();
        
        calculated_size = ImGui::GetItemRectSize();
        
        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar(3);
        
        return calculated_size;
    }

    inline ImVec2 CalcSize(ImGuiID hash_id, int pass, const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
        return CalcMarkdownSize(hash_id, pass, markdown_text, wrap_width, font_scale, md_config);
    }

    inline ImVec2 CalcTextSize(ImGuiID hash_id, int pass, const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
        return CalcMarkdownSize(hash_id, pass, markdown_text, wrap_width, font_scale, md_config);
    }

} // namespace Markdown

    // Canonical namespace alias: ImGui::MD is identical to ImGui::Markdown
    namespace MD = Markdown;

#if !defined(CONTRIB_DISABLE_TEXT_MD) && !defined(IMGUI_DISABLE_TEXT_MD) && !defined(CONTRIB_DISABLE_MARKDOWN_ROOT_ALIASES)
    // Top-level convenience aliases (matching ImGui::Text and ImGui::TextWrapped):
    inline void TextMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::Text(markdown_text, config_override);
    }

    inline void TextMD(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextUnformatted(markdown_text, nullptr, config_override);
    }

    inline void TextWrappedMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(markdown_text, config_override);
    }

    inline void TextWrappedMD(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(std::string(markdown_text ? markdown_text : ""), config_override);
    }

    // Top-level convenience alias for clickable Markdown text links:
    inline LinkClickResult TextMDLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        return Markdown::TextLink(markdown_text, config_override);
    }

    // Full-name root convenience aliases:
    inline void TextMarkdown(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::Text(markdown_text, config_override);
    }

    inline void TextMarkdown(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextUnformatted(markdown_text, nullptr, config_override);
    }

    inline void TextWrappedMarkdown(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(markdown_text, config_override);
    }

    inline void TextWrappedMarkdown(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(std::string(markdown_text ? markdown_text : ""), config_override);
    }

    inline LinkClickResult TextMarkdownLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        return Markdown::TextLink(markdown_text, config_override);
    }
#endif // !CONTRIB_DISABLE_TEXT_MD

} // namespace ImGui

#endif // IMGUI_MARKDOWN_EXT_H

// Order-independent TimedTooltip <-> Markdown bridge:
// Activated automatically regardless of whether imgui_tooltip_ext.h or
// imgui_markdown_ext.h was #included first!
#if defined(IMGUI_MARKDOWN_EXT_H) && defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED)
#define IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED
namespace ImGui {
namespace TimedTooltip {

    inline bool TextMDProxyV(ImGuiID id, bool is_hovered, const TooltipConfig* config_override, const char* fmt, va_list args, bool is_clicked = false, bool toggle_on_click = false) {
        ImGuiMouseCursor previous_cursor = ImGui::GetMouseCursor();
        
        // BRUTE FORCE CURSOR: If the widget is hovered but the cursor is still Arrow, it means the widget's 
        // internal IsItemHovered() check failed or ran out of order. Since this is an interactive tooltippable 
        // widget, we violently force the Hand cursor!
        if (is_hovered && previous_cursor == ImGuiMouseCursor_Arrow) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            previous_cursor = ImGuiMouseCursor_Hand;
        }
        
        if (id == 0) {
            // When id == 0 (e.g. after EndGroup() or custom layout), derive a unique ID
            // so multiple rows in loops calling DelayedTooltip("%s", text) do not collide!
            if (fmt && args) {
                va_list args_id;
                va_copy(args_id, args);
                char id_buf[256];
                vsnprintf(id_buf, sizeof(id_buf), fmt, args_id);
                va_end(args_id);
                if (id_buf[0] != '\0') {
                    id = ImGui::GetID(id_buf);
                }
            }
            if (id == 0) {
                // Secondary fallback: use screen position of current/last item
                ImVec2 item_min = ImGui::GetItemRectMin();
                int coord_key = (int)item_min.x ^ ((int)item_min.y << 16);
                id = ImGui::GetID(coord_key != 0 ? &coord_key : (const void*)(fmt ? fmt : "##timed_tt_anon_md"));
            }
        }

        HandleTooltipClick(id, is_clicked, toggle_on_click);
        
        bool displayed = false;
        if (BeginStationaryTooltipProxy(id, is_hovered, config_override)) {
            displayed = true;
            char buffer[4096];
            vsnprintf(buffer, sizeof(buffer), fmt, args);

            const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
            static const TooltipConfig fallback_config;
            if (!config) config = &fallback_config;
            
            TooltipState& state = GetTooltipStateMap()[id];
            
            float scale = TimedTooltip::GetActiveFontScale(config);
            if (scale <= 0.0f) scale = 1.0f;
            
            // 1. Audit and sanitize wrap constraints against screen boundaries
            float max_screen_w = ImMax(100.0f, ImGui::GetIO().DisplaySize.x - 40.0f);
            bool no_wrap = (config->default_wrap_width < 0.0f);
            float wrap_limit = max_screen_w;
            if (!no_wrap) {
                float min_w = config->min_wrap_width * scale;
                float max_w = config->max_wrap_width * scale;
                if (min_w < 50.0f * scale) min_w = 50.0f * scale;
                if (max_w < min_w) max_w = min_w;
                if (max_w > max_screen_w) max_w = max_screen_w;
                if (min_w > max_w) min_w = max_w;

                float target_w = (config->default_wrap_width > 0.0f) ? (config->default_wrap_width * scale) : (360.0f * scale);
                wrap_limit = ImClamp(target_w, min_w, max_w);
                if (wrap_limit > max_screen_w) wrap_limit = max_screen_w;
            }

            // Prepare MarkdownConfig with custom bullet_spacing
            MarkdownConfig local_md;
            const MarkdownConfig* base_md = GetDefaultMarkdownConfig();
            if (base_md) local_md = *base_md;
            if (config->bullet_spacing > 0.0f) local_md.bulletSpacing = config->bullet_spacing * scale;
            if (no_wrap) local_md.wrapWidth = -1.0f;

            if (state.perfect_size.x == 0.0f) {
                // 1. Dry-run infinitely wide to see how small the text naturally is
                ImVec2 raw_size = MD::CalcMarkdownSize(id, 1, std::string(buffer), 0.0f, scale, &local_md);
                
                if (no_wrap) {
                    state.perfect_size.x = raw_size.x + 2.0f * scale;
                    state.perfect_size.y = raw_size.y + 2.0f * scale;
                } else {
                    // 2. Shrink-wrap tightly around small text, or wrap if it exceeds our wrap limit!
                    float final_width = raw_size.x;
                    if (final_width > wrap_limit) {
                        final_width = wrap_limit;
                    }
                    if (final_width > max_screen_w) final_width = max_screen_w;
                    
                    // ADD EPSILON TO PREVENT MID-WORD BREAKS!
                    final_width += 2.0f * scale;
                    
                    // 3. Do one final dry run with the perfect width to calculate the wrapped vertical height!
                    state.perfect_size = MD::CalcMarkdownSize(id, 2, std::string(buffer), final_width, scale, &local_md);
                    state.perfect_size.x = final_width; 
                    state.perfect_size.y += 2.0f * scale;  // Symmetric 1px top + 1px bottom headroom for pills/backdrops
                }

                // Immediate position clamp on Frame 1:
                ImVec2 display_size = ImGui::GetIO().DisplaySize;
                ImVec2 render_pos = state.locked_pos;
                ImVec2 scaled_padding(config->padding.x * scale, config->padding.y * scale);
                float expected_w = state.perfect_size.x + scaled_padding.x * 2.0f;
                float expected_h = state.perfect_size.y + scaled_padding.y * 2.0f;
                if (expected_w > display_size.x - 25.0f) expected_w = display_size.x - 25.0f;
                if (expected_h > display_size.y - 25.0f) expected_h = display_size.y - 25.0f;
                if (expected_h > 0.0f && render_pos.y + expected_h + 10.0f > display_size.y) {
                    render_pos.y = display_size.y - expected_h - 15.0f;
                    if (render_pos.y < 10.0f) render_pos.y = 10.0f;
                }
                if (expected_w > 0.0f && render_pos.x + expected_w + 10.0f > display_size.x) {
                    render_pos.x = display_size.x - expected_w - 15.0f;
                    if (render_pos.x < 10.0f) render_pos.x = 10.0f;
                }
                ImGui::SetWindowPos(render_pos);
            }

#if IMGUI_VERSION_NUM >= 18989
            ImGui::BeginChild("##md_delay_tt", state.perfect_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
#else
            ImGui::BeginChild("##md_delay_tt", state.perfect_size, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
#endif
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f * scale);
            if (no_wrap) {
                MD::Text(std::string(buffer), &local_md);
            } else {
                MD::TextWrapped(std::string(buffer), &local_md);
            }
            ImGui::EndChild();
            EndStationaryTooltip(id);
        }
        
        // Violently restore the underlying widget's cursor if ImGui's Tooltip engine tried to defeat it!
        if (previous_cursor != ImGui::GetMouseCursor()) {
            ImGui::SetMouseCursor(previous_cursor);
        }
        return displayed;
    }

    inline bool TextMDV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        ImGuiID raw_id = ImGui::GetItemID();
        bool toggle = ImGui::Detail::ShouldToggleTooltipOnClick(raw_id, ImGui::IsItemActive());
        bool hovered = ImGui::Detail::ResolveItemTooltipHover(raw_id, ImGui::IsItemHovered());
        return TextMDProxyV(raw_id, hovered, config_override, fmt, args, ImGui::IsItemClicked(), toggle);
    }

    inline bool TextMDLinkV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        ImGuiID raw_id = ImGui::GetItemID();
        bool hovered = ImGui::Detail::ResolveItemTooltipHover(raw_id, ImGui::IsItemHovered());
        return TextMDProxyV(raw_id, hovered, config_override, fmt, args, ImGui::IsItemClicked(), true);
    }

    // Proxy Timed Tooltips with Markdown (accept explicit ID and explicit is_hovered state):
    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, config, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, nullptr, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(const char* str_id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(ImGui::GetID(str_id), is_hovered, nullptr, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDProxy(id, is_hovered, config, "%s", markdown_text.c_str());
    }

    inline bool TextMDProxy(const char* str_id, bool is_hovered, const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDProxy(ImGui::GetID(str_id), is_hovered, config, "%s", markdown_text.c_str());
    }

    // Markdown-enabled Timed Tooltip with per-call config override
    inline bool TextMD(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(config, fmt, args);
        va_end(args);
        return res;
    }

    // Markdown-enabled Timed Tooltip: mirrors standard ImGui vocabulary (ImGui::TimedTooltip::TextMD)
    // Automatically binds to the last drawn widget using ImGui::GetItemID()
    // Defers string formatting until the tooltip actually appears to save CPU!
    inline bool TextMD(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMD(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMD(config, "%s", markdown_text.c_str());
    }

    // Non-wrapping Unformatted Markdown Timed Tooltip (never auto-wraps; single line per '\n'):
    inline bool TextMDUnformatted(const char* markdown_text, const TooltipConfig* config = nullptr) {
        if (!markdown_text) return false;
        TooltipConfig local_cfg = config ? *config : GetCurrentTooltipConfig();
        local_cfg.default_wrap_width = -1.0f;
        return TextMD(&local_cfg, "%s", markdown_text);
    }

    inline bool TextMDUnformatted(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDUnformatted(markdown_text.c_str(), config);
    }

    // Explicit click-to-toggle Markdown Timed Tooltip chasers (ImGui::TimedTooltip::TextMDLink):
    inline bool TextMDLink(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDLinkV(config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMDLink(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDLinkV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMDLink(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDLink(config, "%s", markdown_text.c_str());
    }

    // Direct aliases for backward-compatibility
    inline bool TimedTooltipMD(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TimedTooltipMD(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    // Bidirectional Channel A: ImGui::TimedTooltip::Markdown:: & ImGui::TimedTooltip::MD::
    namespace Markdown {
        inline bool Text(const TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMD(config, "%s", markdown_text.c_str());
        }

        inline bool TextUnformatted(const char* markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDUnformatted(markdown_text, config);
        }

        inline bool TextUnformatted(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDUnformatted(markdown_text, config);
        }

#pragma push_macro("TextLink")
#undef TextLink
        inline bool TextLink(const TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDLink(config, "%s", markdown_text.c_str());
        }

        inline bool TextLinkContrib(const TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDLink(config, "%s", markdown_text.c_str());
        }
#pragma pop_macro("TextLink")
    } // namespace Markdown

    namespace MD = Markdown;

} // namespace TimedTooltip

namespace Markdown {
    // Bidirectional Channel B: ImGui::Markdown::TimedTooltip:: & ImGui::MD::TimedTooltip::
    namespace TimedTooltip {
        inline bool Text(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMD(config, "%s", markdown_text.c_str());
        }

        inline bool TextUnformatted(const char* markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDUnformatted(markdown_text, config);
        }

        inline bool TextUnformatted(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDUnformatted(markdown_text, config);
        }

#pragma push_macro("TextLink")
#undef TextLink
        inline bool TextLink(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDLink(config, "%s", markdown_text.c_str());
        }

        inline bool TextLinkContrib(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDLink(config, "%s", markdown_text.c_str());
        }
#pragma pop_macro("TextLink")
    } // namespace TimedTooltip
} // namespace Markdown

} // namespace ImGui
#endif // IMGUI_MARKDOWN_EXT_H && IMGUI_TOOLTIP_EXT_H

#endif // !IMGUI_DISABLE_MARKDOWN

