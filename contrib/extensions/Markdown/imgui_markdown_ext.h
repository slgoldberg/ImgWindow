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

#pragma once

#include "imgui.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <cctype>
#include <cstdio>
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
    inline bool ParseMarkdownColor(const std::string& str, ImVec4& out_col) {
        if (str.empty()) return false;
        
        // Named UI colors
        if (str == "red")     { out_col = ImVec4(0.95f, 0.25f, 0.25f, 1.0f); return true; }
        if (str == "green")   { out_col = ImVec4(0.25f, 0.85f, 0.35f, 1.0f); return true; }
        if (str == "blue")    { out_col = ImVec4(0.30f, 0.65f, 1.0f,  1.0f); return true; }
        if (str == "yellow")  { out_col = ImVec4(1.0f,  0.85f, 0.20f, 1.0f); return true; }
        if (str == "orange")  { out_col = ImVec4(1.0f,  0.55f, 0.15f, 1.0f); return true; }
        if (str == "cyan")    { out_col = ImVec4(0.20f, 0.85f, 0.95f, 1.0f); return true; }
        if (str == "magenta") { out_col = ImVec4(0.90f, 0.30f, 0.90f, 1.0f); return true; }
        if (str == "white")   { out_col = ImVec4(1.0f,  1.0f,  1.0f,  1.0f); return true; }
        if (str == "black")   { out_col = ImVec4(0.0f,  0.0f,  0.0f,  1.0f); return true; }
        if (str == "gray" || str == "grey") { out_col = ImVec4(0.60f, 0.60f, 0.60f, 1.0f); return true; }
        if (str == "gold")    { out_col = ImVec4(1.0f,  0.84f, 0.0f,  1.0f); return true; }
        if (str == "dark")    { out_col = ImVec4(0.18f, 0.18f, 0.22f, 1.0f); return true; }
        if (str == "light")   { out_col = ImVec4(0.90f, 0.90f, 0.92f, 1.0f); return true; }

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
        std::string color_param;
        bool is_bold = false;
        bool is_italic = false;
    };

    inline MarkdownTagStyle ParseMarkdownTagStyle(const std::string& inner_text, const std::string& param) {
        MarkdownTagStyle style;
        style.clean_text = inner_text;

        // 1. Check if param contains 'bold', 'italic', or style flags
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
            if (t == "bold" || t == "b") {
                style.is_bold = true;
                found_explicit_style = true;
            } else if (t == "italic" || t == "italics" || t == "i") {
                style.is_italic = true;
                found_explicit_style = true;
            } else {
                if (style.color_param.empty()) {
                    style.color_param = token;
                } else {
                    style.color_param += " " + token;
                }
            }
            pos = end;
        }
        if (style.color_param.empty() && !found_explicit_style) {
            style.color_param = param;
        }

        // 2. Strip leading/trailing whitespace in clean_text
        size_t start_idx = 0;
        while (start_idx < style.clean_text.length() && style.clean_text[start_idx] == ' ') ++start_idx;
        size_t end_idx = style.clean_text.length();
        while (end_idx > start_idx && style.clean_text[end_idx - 1] == ' ') --end_idx;
        std::string trimmed = style.clean_text.substr(start_idx, end_idx - start_idx);

        // 3. Check if inner_text itself has markdown bold/italic syntax (**text**, __text__, *text*, _text_)
        if ((trimmed.length() >= 4 && trimmed.rfind("**", 0) == 0 && trimmed.compare(trimmed.length() - 2, 2, "**") == 0) ||
            (trimmed.length() >= 4 && trimmed.rfind("__", 0) == 0 && trimmed.compare(trimmed.length() - 2, 2, "__") == 0))
        {
            style.is_bold = true;
            trimmed = trimmed.substr(2, trimmed.length() - 4);
        }
        else if ((trimmed.length() >= 2 && trimmed.front() == '*' && trimmed.back() == '*') ||
                 (trimmed.length() >= 2 && trimmed.front() == '_' && trimmed.back() == '_'))
        {
            style.is_italic = true;
            trimmed = trimmed.substr(1, trimmed.length() - 2);
        }
        while (trimmed.length() > 0 && trimmed.front() == ' ') trimmed.erase(0, 1);
        while (trimmed.length() > 0 && trimmed.back() == ' ') trimmed.pop_back();
        style.clean_text = trimmed;

        return style;
    }

    inline void PushMarkdownTagFont(const MarkdownConfig* config, const MarkdownTagStyle& style) {
        if (!config) config = GetActiveMarkdownConfig();
        if (!config) config = GetDefaultMarkdownConfig();
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

                ImVec4 col(1.0f, 1.0f, 1.0f, 1.0f);
                if (ParseMarkdownColor(style.color_param, col)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, col);
                    ImGui::TextUnformatted(style.clean_text.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(style.clean_text.c_str());
                }

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
                ParseMarkdownColor(style.color_param, bg_col);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                float pad_x = 3.0f;
                float pad_y = 1.0f;

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImU32 col_u32 = ImGui::GetColorU32(bg_col);
                draw_list->AddRectFilled(
                    ImVec2(pos.x - pad_x, pos.y - pad_y),
                    ImVec2(pos.x + text_size.x + pad_x, pos.y + text_size.y + pad_y),
                    col_u32, 3.0f
                );

                // Auto-contrast text color if background is dark
                float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                if (lum < 0.35f) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    ImGui::TextUnformatted(style.clean_text.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(style.clean_text.c_str());
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

                if (style.color_param == "danger" || style.color_param == "red") {
                    bg_col = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
                } else if (style.color_param == "warning" || style.color_param == "yellow") {
                    bg_col = ImVec4(0.95f, 0.75f, 0.10f, 1.0f);
                    text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                } else if (style.color_param == "success" || style.color_param == "green") {
                    bg_col = ImVec4(0.15f, 0.70f, 0.25f, 1.0f);
                } else if (style.color_param == "info" || style.color_param == "blue") {
                    bg_col = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
                } else {
                    ParseMarkdownColor(style.color_param, bg_col);
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum > 0.65f) text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, bg_col);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg_col);
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                ImGui::SmallButton(style.clean_text.c_str());
                ImGui::PopStyleColor(4);

                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["badge"] = badgeHandler;
            s_CustomTags["pill"]  = badgeHandler;
            s_CustomTags["tag"]   = badgeHandler;

            // Built-in Tag 4: <btn=...>
            s_CustomTags["btn"] = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);
                ImGui::SmallButton(style.clean_text.c_str());
                PopMarkdownTagFont(config, style);
            };
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
    inline void MarkdownExt(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        const MarkdownConfig* config = config_override ? config_override : GetDefaultMarkdownConfig();
        if (!config) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[MarkdownExt Error: No MarkdownConfig Set!]");
            return;
        }

        const MarkdownConfig* prev_active = GetActiveMarkdownConfig();
        GetActiveMarkdownConfig() = config;

        MarkdownConfig local_config = *config;
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

        Markdown(markdown_text.c_str(), markdown_text.length(), local_config);

        GetActiveMarkdownConfig() = prev_active;
    }

    namespace MD {

        // -------------------------------------------------------------------------
        // Standard Widget API (mirroring standard ImGui vocabulary)
        // -------------------------------------------------------------------------

        inline void Text(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            ImGui::BeginGroup();
            MarkdownExt(markdown_text, config_override);
            ImGui::EndGroup(); 
        }

        inline void TextWrapped(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, config_override);
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
            
            if (tooltip_allowed && !tooltip_markdown.empty() && ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                #if IMGUI_VERSION_NUM >= 18989
                ImGui::BeginChild("##md_tt", ImVec2(tooltip_wrap_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
#else
                ImGui::BeginChild("##md_tt", ImVec2(tooltip_wrap_width, 0.0f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
#endif
                Text(tooltip_markdown);
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
                    Text(markdown_text);
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
            
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f);
            
            ImGui::SetNextWindowPos(ImVec2(-10000.0f, -10000.0f));
            ImGui::Begin(measure_name, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
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

} // namespace MD

} // namespace ImGui

#ifdef IMGUI_TOOLTIPS_EXT_H
namespace ImGui {
namespace TimedTooltip {

    inline bool TextMDProxyV(ImGuiID id, bool is_hovered, const TooltipConfig* config_override, const char* fmt, va_list args) {
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
        
        bool displayed = false;
        if (BeginStationaryTooltipProxy(id, is_hovered, config_override)) {
            displayed = true;
            char buffer[4096];
            vsnprintf(buffer, sizeof(buffer), fmt, args);

            const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
            static const TooltipConfig fallback_config;
            if (!config) config = &fallback_config;
            
            TooltipState& state = GetTooltipStateMap()[id];
            
            // 1. Audit and sanitize wrap constraints against screen boundaries
            float max_screen_w = ImMax(100.0f, ImGui::GetIO().DisplaySize.x - 40.0f);
            float min_w = config->min_wrap_width;
            float max_w = config->max_wrap_width;
            if (min_w < 50.0f) min_w = 50.0f;
            if (max_w < min_w) max_w = min_w;
            if (max_w > max_screen_w) max_w = max_screen_w;
            if (min_w > max_w) min_w = max_w;

            float target_w = (config->default_wrap_width > 0.0f) ? config->default_wrap_width : 360.0f;
            float wrap_limit = ImClamp(target_w, min_w, max_w);
            if (wrap_limit > max_screen_w) wrap_limit = max_screen_w;

            // Prepare MarkdownConfig with custom bullet_spacing
            MarkdownConfig local_md;
            const MarkdownConfig* base_md = GetDefaultMarkdownConfig();
            if (base_md) local_md = *base_md;
            if (config->bullet_spacing > 0.0f) local_md.bulletSpacing = config->bullet_spacing;

            if (state.perfect_size.x == 0.0f) {
                // 1. Dry-run infinitely wide to see how small the text naturally is
                ImVec2 raw_size = MD::CalcMarkdownSize(id, 1, std::string(buffer), 0.0f, config->font_scale, &local_md);
                
                // 2. Shrink-wrap tightly around small text, or wrap if it exceeds our wrap limit!
                float final_width = raw_size.x;
                if (final_width > wrap_limit) {
                    final_width = wrap_limit;
                }
                if (final_width > max_screen_w) final_width = max_screen_w;
                
                // ADD EPSILON TO PREVENT MID-WORD BREAKS!
                final_width += 2.0f;
                
                // 3. Do one final dry run with the perfect width to calculate the wrapped vertical height!
                state.perfect_size = MD::CalcMarkdownSize(id, 2, std::string(buffer), final_width, config->font_scale, &local_md);
                state.perfect_size.x = final_width; 
                state.perfect_size.y += 4.0f;  // Prevent 1-pixel shift scrollbars when buttons are clicked!

                // Immediate position clamp on Frame 1:
                ImVec2 display_size = ImGui::GetIO().DisplaySize;
                ImVec2 render_pos = state.locked_pos;
                float expected_w = state.perfect_size.x + config->padding.x * 2.0f;
                float expected_h = state.perfect_size.y + config->padding.y * 2.0f;
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
            MD::Text(std::string(buffer), &local_md);
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
        return TextMDProxyV(ImGui::GetItemID(), ImGui::IsItemHovered(), config_override, fmt, args);
    }

    // Proxy Timed Tooltips with Markdown (accept explicit ID and explicit is_hovered state):
    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(const char* str_id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(ImGui::GetID(str_id), is_hovered, nullptr, fmt, args);
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

} // namespace TimedTooltip

} // namespace ImGui
#endif
