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
    using CustomTagCallback = std::function<void(const std::string& inner_text, const std::string& param)>;

    inline std::unordered_map<std::string, CustomTagCallback>& GetCustomMarkdownTags() {
        static std::unordered_map<std::string, CustomTagCallback> s_CustomTags;
        static bool s_InitializedDefaults = false;
        if (!s_InitializedDefaults) {
            s_InitializedDefaults = true;
#ifndef IMGUI_DISABLE_MARKDOWN_DEFAULT_TAGS
            // Built-in Tag 1: <color=...>, <col=...>
            auto colorHandler = [](const std::string& inner_text, const std::string& param) {
                ImVec4 col(1.0f, 1.0f, 1.0f, 1.0f);
                if (ParseMarkdownColor(param, col)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, col);
                    ImGui::TextUnformatted(inner_text.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(inner_text.c_str());
                }
            };
            s_CustomTags["color"] = colorHandler;
            s_CustomTags["col"]   = colorHandler;

            // Built-in Tag 2: <backdrop=...>, <bg=...>, <highlight=...>
            // Draws an exact filled rectangular pill behind the text bounding box with rounded corners
            auto backdropHandler = [](const std::string& inner_text, const std::string& param) {
                ImVec4 bg_col(1.0f, 0.90f, 0.20f, 0.65f); // default soft yellow highlight
                ParseMarkdownColor(param, bg_col);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImVec2 text_size = ImGui::CalcTextSize(inner_text.c_str());
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
                    ImGui::TextUnformatted(inner_text.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(inner_text.c_str());
                }
            };
            s_CustomTags["backdrop"]  = backdropHandler;
            s_CustomTags["bg"]        = backdropHandler;
            s_CustomTags["highlight"] = backdropHandler;

            // Built-in Tag 3: <badge=...>, <pill=...>, <tag=...>
            // Renders a sleek pill/badge with colored background and contrasting text
            auto badgeHandler = [](const std::string& inner_text, const std::string& param) {
                ImVec4 bg_col = ImVec4(0.25f, 0.25f, 0.30f, 1.0f); // default charcoal
                ImVec4 text_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                if (param == "danger" || param == "red") {
                    bg_col = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
                } else if (param == "warning" || param == "yellow") {
                    bg_col = ImVec4(0.95f, 0.75f, 0.10f, 1.0f);
                    text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                } else if (param == "success" || param == "green") {
                    bg_col = ImVec4(0.15f, 0.70f, 0.25f, 1.0f);
                } else if (param == "info" || param == "blue") {
                    bg_col = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
                } else {
                    ParseMarkdownColor(param, bg_col);
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum > 0.65f) text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, bg_col);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg_col);
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                ImGui::SmallButton(inner_text.c_str());
                ImGui::PopStyleColor(4);
            };
            s_CustomTags["badge"] = badgeHandler;
            s_CustomTags["pill"]  = badgeHandler;
            s_CustomTags["tag"]   = badgeHandler;

            // Built-in Tag 4: <btn=...>
            s_CustomTags["btn"] = [](const std::string& inner_text, const std::string&) {
                ImGui::SmallButton(inner_text.c_str());
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

    // 4. Core String-Splitter Engine
    inline void MarkdownExt(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        const MarkdownConfig* config = config_override ? config_override : GetDefaultMarkdownConfig();
        if (!config) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[MarkdownExt Error: No MarkdownConfig Set!]");
            return;
        }

        size_t cursor = 0;
        auto& tags = GetCustomMarkdownTags();

        while (cursor < markdown_text.length()) {
            size_t tag_start = markdown_text.find("<", cursor);
            if (tag_start == std::string::npos) {
                Markdown(markdown_text.c_str() + cursor, markdown_text.length() - cursor, *config);
                break;
            }

            if (tag_start > cursor) {
                Markdown(markdown_text.c_str() + cursor, tag_start - cursor, *config);
            }

            size_t tag_end = markdown_text.find(">", tag_start);
            if (tag_end == std::string::npos) {
                Markdown(markdown_text.c_str() + tag_start, markdown_text.length() - tag_start, *config);
                break;
            }

            // Extract tag header and parse name vs parameter:
            std::string tag_header = markdown_text.substr(tag_start + 1, tag_end - tag_start - 1);
            std::string tag_name;
            std::string tag_param;

            size_t sep = tag_header.find_first_of("=:\t ");
            if (sep != std::string::npos) {
                tag_name = tag_header.substr(0, sep);
                size_t val_start = tag_header.find_first_not_of("=:\t \"'", sep);
                if (val_start != std::string::npos) {
                    size_t val_end = tag_header.find_last_not_of(" \"'");
                    tag_param = tag_header.substr(val_start, val_end - val_start + 1);
                }
            } else {
                tag_name = tag_header;
            }

            auto it = tags.find(tag_name);
            if (it != tags.end()) {
                std::string closing_tag = "</" + tag_name + ">";
                size_t closing_start = markdown_text.find(closing_tag, tag_end + 1);
                
                if (closing_start != std::string::npos) {
                    std::string inner_text = markdown_text.substr(tag_end + 1, closing_start - tag_end - 1);

                    // Flow inline with preceding text if not at start of line
                    if (tag_start > 0 && markdown_text[tag_start - 1] != '\n' && markdown_text[tag_start - 1] != '\r') {
                        ImGui::SameLine(0.0f, 0.0f);
                    }

                    it->second(inner_text, tag_param); // Execute Custom Styling Tag

                    cursor = closing_start + closing_tag.length();

                    // Flow inline with following text if not immediately followed by a newline
                    if (cursor < markdown_text.length() && markdown_text[cursor] != '\n' && markdown_text[cursor] != '\r') {
                        ImGui::SameLine(0.0f, 0.0f);
                    }
                    continue;
                }
            }

            // Not a registered tag, render as normal markdown text
            Markdown(markdown_text.c_str() + tag_start, tag_end - tag_start + 1, *config);
            cursor = tag_end + 1;
        }
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

        inline void DelayedTooltipMD(const std::string& markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
            if (is_allowed && ImGui::IsItemHovered()) {
                ImGuiID id = ImGui::GetItemID();
                ImGuiStorage* storage = ImGui::GetStateStorage();
                
                int hover_start = storage->GetInt(id, 0);
                if (hover_start == 0) {
                    hover_start = ImGui::GetFrameCount();
                    storage->SetInt(id, hover_start);
                }

                if (ImGui::GetFrameCount() - hover_start >= delay_frames) {
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

    inline void TextMDV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        bool is_hovered = ImGui::IsItemHovered();
        ImGuiMouseCursor previous_cursor = ImGui::GetMouseCursor();
        
        // BRUTE FORCE CURSOR: If the widget is hovered but the cursor is still Arrow, it means the widget's 
        // internal IsItemHovered() check failed or ran out of order. Since this is an interactive tooltippable 
        // widget, we violently force the Hand cursor!
        if (is_hovered && previous_cursor == ImGuiMouseCursor_Arrow) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            previous_cursor = ImGuiMouseCursor_Hand;
        }
        
        ImGuiID id = ImGui::GetItemID(); 
        
        if (BeginStationaryTooltipProxy(id, is_hovered, config_override)) {
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
    }

    // Markdown-enabled Timed Tooltip with per-call config override
    inline void TextMD(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextMDV(config, fmt, args);
        va_end(args);
    }

    // Markdown-enabled Timed Tooltip: mirrors standard ImGui vocabulary (ImGui::TimedTooltip::TextMD)
    // Automatically binds to the last drawn widget using ImGui::GetItemID()
    // Defers string formatting until the tooltip actually appears to save CPU!
    inline void TextMD(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextMDV(nullptr, fmt, args);
        va_end(args);
    }

    // Direct aliases for backward-compatibility
    inline void TimedTooltipMD(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextMDV(config, fmt, args);
        va_end(args);
    }

    inline void TimedTooltipMD(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextMDV(nullptr, fmt, args);
        va_end(args);
    }

} // namespace TimedTooltip

} // namespace ImGui
#endif
