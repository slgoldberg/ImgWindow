// ============================================================================
// ImgWindow Extension: TimedTooltip
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Contributors:
//   - Steven L. Goldberg: Stationary positioning, edge-docking, wiggle-latch,
//     hermetic style isolation, auto-wrapping, and Markdown synergy.
// ============================================================================

#define IMGUI_TOOLTIPS_EXT_H
#pragma once

#include "imgui.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace ImGui {
namespace TimedTooltip {

    struct TooltipConfig {
        int hover_delay_cycles = 65;      // Low Watermark: Cycles before appearing
        int max_display_cycles = 504;     // High Watermark: Cycles before auto-hiding
        int grace_period_cycles = 5;      // Forgiveness cycles for slipping off the widget
        float size_skew_multiplier = 0.5f;// Add extra cycles per pixel of height for massive manuals
        bool enable_wiggle_latch = true;  // Keep alive if mouse wiggles
        
        float default_wrap_width = 360.0f;// Target comfortable sticky-note width (360.0f; short text shrink-wraps)
        float min_wrap_width = 240.0f;    // Minimum constraint before forced truncation/overflow
        float max_wrap_width = 600.0f;    // Upper ceiling for wide manuals/tables
        ImVec2 padding = ImVec2(8.0f, 6.0f);                     // Compact internal padding around tooltip text
        ImVec2 item_spacing = ImVec2(4.0f, 0.0f);                // Tight vertical line pitch (matches native font leading)
        ImVec2 frame_padding = ImVec2(2.0f, 1.0f);               // Compact frame padding
        float indent_spacing = 20.0f;                            // Isolated standard indent spacing
        float bullet_spacing = 4.0f;                             // Gap between bullet glyph and text (default 4.0f)
        float font_scale = 0.0f;                                 // 0.0f = inherit ambient font scale; >0.0f = custom window font scale
        
        ImVec4 bg_color = ImVec4(1.0f, 0.95f, 0.6f, 0.95f);      // Yellow Sticky Note
        ImVec4 border_color = ImVec4(0.8f, 0.75f, 0.4f, 1.0f);   // Slightly darker border
        ImVec4 text_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);      // Black text
    };

    inline const TooltipConfig*& GetDefaultTooltipConfig() {
        static const TooltipConfig* s_DefaultConfig = nullptr;
        return s_DefaultConfig;
    }
    inline void SetDefaultTooltipConfig(const TooltipConfig* config) {
        GetDefaultTooltipConfig() = config;
    }

    // Returns a reference to the active TooltipConfig:
    // If a global config was set via SetDefaultTooltipConfig(), returns that.
    // Otherwise, returns the built-in default configuration.
    inline const TooltipConfig& GetCurrentTooltipConfig() {
        const TooltipConfig* config = GetDefaultTooltipConfig();
        if (config) return *config;
        static const TooltipConfig s_BuiltinDefaultConfig;
        return s_BuiltinDefaultConfig;
    }

    // Direct convenience helper to get an editable copy of the current configuration:
    // e.g. TooltipConfig wideConfig = ImGui::TimedTooltip::GetConfig();
    inline TooltipConfig GetConfig() {
        return GetCurrentTooltipConfig();
    }

    struct TooltipState {
        int hover_cycles = 0;
        int unhover_grace_cycles = 0;
        int display_cycles = 0;
        ImVec2 last_mouse_pos = ImVec2(-1, -1);
        ImVec2 locked_pos = ImVec2(-1, -1);
        ImVec2 last_size = ImVec2(0, 0);
        ImVec2 perfect_size = ImVec2(0, 0);
        bool is_displayed = false;
        bool is_cooldown = false; // Prevents reappearing if they remain perfectly still after auto-hide
        int last_frame_updated = 0;
    };

        inline ImGuiID& GetActiveTooltipID() {
        // Use ImGui's global state storage to guarantee a single mutex across all Translation Units!
        // We use a hardcoded integer key (0x71701337) because ImGui::GetID() hashes based on the UI stack!
        // If we used GetID(), tooltips inside groups would generate different Mutex keys!
        return *(ImGuiID*)ImGui::GetStateStorage()->GetIntRef(0x71701337, 0);
    }

    inline std::unordered_map<ImGuiID, TooltipState>& GetTooltipStateMap() {
        static std::unordered_map<ImGuiID, TooltipState> s_TooltipStates;
        return s_TooltipStates;
    }

    // Forcefully dismisses the currently active tooltip (useful for UI state changes/loading screens)
    inline void ClearActiveTooltip() {
        ImGuiID active_id = GetActiveTooltipID();
        if (active_id != 0) {
            TooltipState& state = GetTooltipStateMap()[active_id];
            state.is_displayed = false;
            state.hover_cycles = 0;
            state.unhover_grace_cycles = 0;
            state.is_cooldown = false;
            GetActiveTooltipID() = 0;
            state.perfect_size = ImVec2(0,0);
        }
    }


    inline bool BeginStationaryTooltipProxy(ImGuiID hash_id, bool is_hovered, const TooltipConfig* config_override = nullptr) {
        TooltipState& state = GetTooltipStateMap()[hash_id];
        state.last_frame_updated = ImGui::GetFrameCount(); // Mark as alive this frame!
        
        // Garbage Collection: If the active tooltip was abandoned (e.g. its ImGuiID changed due to a label toggle), kill it!
        ImGuiID active_id = GetActiveTooltipID();
        if (active_id != 0 && active_id != hash_id) {
            TooltipState& active_state = GetTooltipStateMap()[active_id];
            if (active_state.last_frame_updated < ImGui::GetFrameCount() - 1) {
                ClearActiveTooltip();
            }
        }
        
        const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
        static const TooltipConfig fallback_config;
        if (!config) config = &fallback_config;

        // 1. Hover Latch (Low Watermark & Grace Period)
        if (is_hovered) {
            state.unhover_grace_cycles = 0;
            if (!state.is_cooldown) {
                state.hover_cycles++;
            } else {
                // If in cooldown, wiggling the mouse wakes it back up instantly!
                ImVec2 current_mouse = ImGui::GetMousePos();
                if (std::abs(current_mouse.x - state.last_mouse_pos.x) > 1.0f || std::abs(current_mouse.y - state.last_mouse_pos.y) > 1.0f) {
                    state.is_cooldown = false;
                    state.hover_cycles = config->hover_delay_cycles; // Instant pop!
                }
            }
        } else {
            if (state.is_displayed) {
                state.unhover_grace_cycles++;
                if (state.unhover_grace_cycles > config->grace_period_cycles) {
                    state.is_displayed = false;
                    if (GetActiveTooltipID() == hash_id) GetActiveTooltipID() = 0;
                    state.hover_cycles = 0;
                    state.is_cooldown = false;
                    state.perfect_size = ImVec2(0,0);
                }
            } else {
                state.hover_cycles = 0;
                state.is_cooldown = false;
            }
        }

        // Trigger Display (With Global Mutex)
        if (!state.is_displayed && state.hover_cycles >= config->hover_delay_cycles) {
            if (GetActiveTooltipID() == 0 || GetActiveTooltipID() == hash_id) {
                GetActiveTooltipID() = hash_id; // CLAIM THE MUTEX!
                state.is_displayed = true;
                state.display_cycles = 0;
            // Lock anchor position (Offset slightly so the cursor doesn't cover the text)
            state.locked_pos = ImVec2(ImGui::GetMousePos().x + 15.0f, ImGui::GetMousePos().y + 15.0f);
            state.last_mouse_pos = ImGui::GetMousePos();
            }
        }

        // 2. Display Latch & Wiggle Safety
        if (state.is_displayed) {
            ImVec2 current_mouse = ImGui::GetMousePos();
            
            if (config->enable_wiggle_latch) {
                // If they move the mouse more than a micro-pixel, reset the auto-hide timer!
                if (std::abs(current_mouse.x - state.last_mouse_pos.x) > 1.0f || 
                    std::abs(current_mouse.y - state.last_mouse_pos.y) > 1.0f) {
                    state.display_cycles = 0; 
                    state.last_mouse_pos = current_mouse;
                }
            }
            
            state.display_cycles++;
            
            // Calculate size-skewed High Watermark (skip if set to infinite/negative)
            if (config->max_display_cycles >= 0) {
                int max_timeout = config->max_display_cycles;
                if (state.last_size.y > 0) {
                    max_timeout += (int)(state.last_size.y * config->size_skew_multiplier);
                }
                
                // High Watermark: Auto-hide triggered
                if (state.display_cycles > max_timeout) {
                    state.is_displayed = false;
                    if (GetActiveTooltipID() == hash_id) GetActiveTooltipID() = 0; // RELEASE MUTEX
                    state.is_cooldown = true; // Prevent it from instantly popping back up!
                    state.hover_cycles = 0;
                    return false;
                }
            }

            // 3. Render Stationary Window with automatic screen-edge clamping (grows upwards if clipping bottom!)
            ImVec2 render_pos = state.locked_pos;
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            float expected_h = (state.perfect_size.y > 0.0f) ? (state.perfect_size.y + config->padding.y * 2.0f) : state.last_size.y;
            float expected_w = (state.perfect_size.x > 0.0f) ? (state.perfect_size.x + config->padding.x * 2.0f) : state.last_size.x;

            // Ensure expected dimensions don't exceed the active display boundaries
            if (expected_w > display_size.x - 25.0f) expected_w = display_size.x - 25.0f;
            if (expected_h > display_size.y - 25.0f) expected_h = display_size.y - 25.0f;

            // Grow UPWARDS if tooltip would clip off the bottom of the screen!
            if (expected_h > 0.0f && render_pos.y + expected_h + 10.0f > display_size.y) {
                render_pos.y = display_size.y - expected_h - 15.0f;
                if (render_pos.y < 10.0f) render_pos.y = 10.0f;
            }

            // Shift LEFTWARDS if tooltip would clip off the right edge of the screen!
            if (expected_w > 0.0f && render_pos.x + expected_w + 10.0f > display_size.x) {
                render_pos.x = display_size.x - expected_w - 15.0f;
                if (render_pos.x < 10.0f) render_pos.x = 10.0f;
            }

            ImGui::SetNextWindowPos(render_pos);
            char window_name[32];
            snprintf(window_name, sizeof(window_name), "##TT_%08X", hash_id);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, config->bg_color);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, config->bg_color);
            ImGui::PushStyleColor(ImGuiCol_Border, config->border_color);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, config->padding);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, config->item_spacing);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, config->frame_padding);
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, config->indent_spacing);
            bool open = ImGui::Begin(window_name, nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            if (config->font_scale > 0.0f) {
                ImGui::SetWindowFontScale(config->font_scale);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, config->text_color); // Pushed AFTER Begin to affect window contents!
            ImGui::PushStyleColor(ImGuiCol_Separator, config->text_color); // Ensures markdown horizontal lines match the text color!
            return open;
        }
        
        return false;
    }

    inline bool BeginStationaryTooltipProxy(const char* str_id, bool is_hovered, const TooltipConfig* config_override = nullptr) {
        return BeginStationaryTooltipProxy(ImGui::GetID(str_id), is_hovered, config_override);
    }

    inline bool BeginStationaryTooltip(ImGuiID hash_id, const TooltipConfig* config_override = nullptr) {
        return BeginStationaryTooltipProxy(hash_id, ImGui::IsItemHovered(), config_override);
    }

    inline bool BeginStationaryTooltip(const char* str_id, const TooltipConfig* config_override = nullptr) {
        return BeginStationaryTooltip(ImGui::GetID(str_id), config_override);
    }

    inline void EndStationaryTooltip(ImGuiID hash_id) {
        TooltipState& state = GetTooltipStateMap()[hash_id];
        
        // Capture size mathematically so the next frame can calculate the skew ratio!
        state.last_size = ImGui::GetWindowSize();
        ImGui::PopStyleColor(2); // Pop Text and Separator
        ImGui::End();
        ImGui::PopStyleVar(4);    // Pop WindowPadding, ItemSpacing, FramePadding, IndentSpacing
        ImGui::PopStyleColor(3); // Pop Window, Popup, Border
    }

    inline void EndStationaryTooltip(const char* str_id) {
        EndStationaryTooltip(ImGui::GetID(str_id));
    }

    using Config = TooltipConfig;
    using State = TooltipState;

    inline void TextV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        bool is_hovered = ImGui::IsItemHovered();
        ImGuiMouseCursor previous_cursor = ImGui::GetMouseCursor();

        if (is_hovered && previous_cursor == ImGuiMouseCursor_Arrow) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            previous_cursor = ImGuiMouseCursor_Hand;
        }

        ImGuiID id = ImGui::GetItemID();
        if (BeginStationaryTooltipProxy(id, is_hovered, config_override)) {
            const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
            static const TooltipConfig fallback_config;
            if (!config) config = &fallback_config;

            TooltipState& state = GetTooltipStateMap()[id];
            
            // 1. Audit and sanitize wrap constraints against screen boundaries
            float max_screen_w = std::max(100.0f, ImGui::GetIO().DisplaySize.x - 40.0f);
            float min_w = config->min_wrap_width;
            float max_w = config->max_wrap_width;
            if (min_w < 50.0f) min_w = 50.0f;
            if (max_w < min_w) max_w = min_w;
            if (max_w > max_screen_w) max_w = max_screen_w;
            if (min_w > max_w) min_w = max_w;

            float target_w = (config->default_wrap_width > 0.0f) ? config->default_wrap_width : 360.0f;
            float wrap_limit = std::clamp(target_w, min_w, max_w);
            if (wrap_limit > max_screen_w) wrap_limit = max_screen_w;

            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_limit);
            ImGui::TextV(fmt, args);
            ImGui::PopTextWrapPos();

            EndStationaryTooltip(id);
        }

        if (previous_cursor != ImGui::GetMouseCursor()) {
            ImGui::SetMouseCursor(previous_cursor);
        }
    }

    // Plain-text Timed Tooltip with per-call config override
    inline void Text(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextV(config, fmt, args);
        va_end(args);
    }

    // Plain-text Timed Tooltip: mirrors standard ImGui vocabulary (ImGui::TimedTooltips::Text)
    inline void Text(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextV(nullptr, fmt, args);
        va_end(args);
    }

} // namespace TimedTooltip

} // namespace ImGui
