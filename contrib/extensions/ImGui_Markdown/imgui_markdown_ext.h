#pragma once

#include "imgui.h"
#include <string>
#include <functional>
#include <unordered_map>
#include "imgui_markdown.h"

namespace ImGui {

    // -------------------------------------------------------------------------
    // ImGui Markdown Extensions (Tags & Widgets)
    // -------------------------------------------------------------------------
    
    // 1. Config Registry
    inline const MarkdownConfig*& GetDefaultMarkdownConfig() {
        static const MarkdownConfig* s_DefaultConfig = nullptr;
        return s_DefaultConfig;
    }

    inline void SetDefaultMarkdownConfig(const MarkdownConfig* config) {
        GetDefaultMarkdownConfig() = config;
    }

    // 2. Custom Tag Pre-Processor
    using CustomTagCallback = std::function<void(const std::string& inner_text)>;

    inline std::unordered_map<std::string, CustomTagCallback>& GetCustomMarkdownTags() {
        static std::unordered_map<std::string, CustomTagCallback> s_CustomTags;
        return s_CustomTags;
    }

    inline void RegisterMarkdownWidget(const std::string& tag_name, CustomTagCallback callback) {
        GetCustomMarkdownTags()[tag_name] = callback;
    }

    // 3. Core String-Splitter Engine
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

            std::string tag_name = markdown_text.substr(tag_start + 1, tag_end - tag_start - 1);
            
            auto it = tags.find(tag_name);
            if (it != tags.end()) {
                std::string closing_tag = "</" + tag_name + ">";
                size_t closing_start = markdown_text.find(closing_tag, tag_end + 1);
                
                if (closing_start != std::string::npos) {
                    std::string inner_text = markdown_text.substr(tag_end + 1, closing_start - tag_end - 1);
                    it->second(inner_text); // Execute Custom Widget
                    cursor = closing_start + closing_tag.length();
                    continue;
                }
            }

            // Not a registered tag, render as normal markdown text
            Markdown(markdown_text.c_str() + tag_start, tag_end - tag_start + 1, *config);
            cursor = tag_end + 1;
        }
    }

    namespace V2 {

    // -------------------------------------------------------------------------
        // Standard Widget API
        // -------------------------------------------------------------------------

        inline void TextMD(const std::string& markdown_text) {
            ImGui::BeginGroup();
            MarkdownExt(markdown_text);
            ImGui::EndGroup(); 
        }

        inline void TextWrappedMD(const std::string& markdown_text) {
            TextMD(markdown_text);
        }

        inline bool ButtonMD(const std::string& label, const std::string& tooltip_markdown = "", float tooltip_wrap_width = 400.0f, bool tooltip_allowed = true) {
            bool clicked = ImGui::Button(label.c_str());
            
            if (tooltip_allowed && !tooltip_markdown.empty() && ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                #if IMGUI_VERSION_NUM >= 18989
                ImGui::BeginChild("##md_tt", ImVec2(tooltip_wrap_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoBackground);
#else
                ImGui::BeginChild("##md_tt", ImVec2(tooltip_wrap_width, 0.0f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
#endif
                TextMD(tooltip_markdown);
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
                    ImGui::BeginChild("##md_delay_tt", ImVec2(wrap_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoBackground);
#else
                    ImGui::BeginChild("##md_delay_tt", ImVec2(wrap_width, 0.0f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
#endif
                    TextMD(markdown_text);
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
        inline ImVec2 CalcMarkdownSize(const std::string& markdown_text, float wrap_width = 0.0f) {
        ImVec2 calculated_size(0, 0);
        
        ImGui::SetNextWindowPos(ImVec2(-10000.0f, -10000.0f));
        ImGui::Begin("##md_measure", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
        
        // Feed it a massive constraint if 0.0f so it doesn't word-wrap infinitely to 1 char
        float child_width = (wrap_width > 0.0f) ? wrap_width : 99999.0f;
#if IMGUI_VERSION_NUM >= 18989
        ImGui::BeginChild("##md_measure_child", ImVec2(child_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoBackground);
#else
        ImGui::BeginChild("##md_measure_child", ImVec2(child_width, 0.0f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
#endif
        
        ImGui::BeginGroup();
        MarkdownExt(markdown_text);
        ImGui::EndGroup();
        
        calculated_size = ImGui::GetItemRectSize();
        
        ImGui::EndChild();
        ImGui::End();
        
        return calculated_size;
    }

} // namespace V2

} // namespace ImGui

#ifdef IMGUI_TOOLTIPS_EXT_H
namespace ImGui {
namespace V2 {
    // Automatically binds to the last drawn widget using ImGui::GetItemID()
    // Defers string formatting until the tooltip actually appears to save CPU!
    inline void TimedTooltipMD(const char* fmt, ...) {
        ImGuiID id = ImGui::GetItemID(); 
        
        if (BeginStationaryTooltip(id)) {
            va_list args;
            va_start(args, fmt);
            char buffer[4096];
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);

            const TooltipConfig* config = GetDefaultTooltipConfig();
            static const TooltipConfig fallback_config;
            if (!config) config = &fallback_config;
            
            float wrap_width = config->default_wrap_width;
            if (wrap_width <= 0.0f) {
                float avail = ImGui::GetIO().DisplaySize.x - ImGui::GetWindowPos().x;
                wrap_width = avail * 0.8f;
                if (wrap_width < config->min_wrap_width) wrap_width = config->min_wrap_width;
                if (wrap_width > config->max_wrap_width) wrap_width = config->max_wrap_width;
            }

            TooltipState& state = GetTooltipStateMap()[id];
            if (state.perfect_size.x == 0.0f) {
                // 1. Dry-run infinitely wide to see how small the text naturally is
                ImVec2 raw_size = CalcMarkdownSize(std::string(buffer), 0.0f);
                
                // 2. Shrink-wrap tightly around small text, or clamp massive text to screen space
                float final_width = raw_size.x;
                if (final_width > wrap_width) {
                    final_width = wrap_width;
                    if (final_width < config->min_wrap_width) final_width = config->min_wrap_width;
                }
                
                // 3. Do one final dry run with the perfect width to calculate the wrapped vertical height!
                state.perfect_size = CalcMarkdownSize(std::string(buffer), final_width);
                state.perfect_size.x += 15.0f; // Add horizontal breathing room!
                state.perfect_size.y += 4.0f;  // Prevent 1-pixel shift scrollbars when buttons are clicked!
            }

#if IMGUI_VERSION_NUM >= 18989
            ImGui::BeginChild("##md_delay_tt", state.perfect_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#else
            ImGui::BeginChild("##md_delay_tt", state.perfect_size, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#endif
            TextMD(std::string(buffer));
            ImGui::EndChild();
            EndStationaryTooltip(id);
        }
    }
} // namespace V2
} // namespace ImGui
#endif
