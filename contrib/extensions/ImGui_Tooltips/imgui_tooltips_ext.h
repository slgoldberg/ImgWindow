#define IMGUI_TOOLTIPS_EXT_H
#pragma once

#include "imgui.h"
#include <unordered_map>
#include <cmath>

namespace ImGui {
namespace V2 {

    struct TooltipConfig {
        int hover_delay_cycles = 65;      // Low Watermark: Cycles before appearing
        int max_display_cycles = 504;     // High Watermark: Cycles before auto-hiding
        int grace_period_cycles = 5;      // Forgiveness cycles for slipping off the widget
        float size_skew_multiplier = 0.5f;// Add extra cycles per pixel of height for massive manuals
        bool enable_wiggle_latch = true;  // Keep alive if mouse wiggles
    };

    inline const TooltipConfig*& GetDefaultTooltipConfig() {
        static const TooltipConfig* s_DefaultConfig = nullptr;
        return s_DefaultConfig;
    }
    inline void SetDefaultTooltipConfig(const TooltipConfig* config) {
        GetDefaultTooltipConfig() = config;
    }

    struct TooltipState {
        int hover_cycles = 0;
        int unhover_grace_cycles = 0;
        int display_cycles = 0;
        ImVec2 last_mouse_pos = ImVec2(-1, -1);
        ImVec2 locked_pos = ImVec2(-1, -1);
        ImVec2 last_size = ImVec2(0, 0);
        bool is_displayed = false;
        bool is_cooldown = false; // Prevents reappearing if they remain perfectly still after auto-hide
    };

        inline ImGuiID& GetActiveTooltipID() {
        static ImGuiID s_ActiveTooltip = 0;
        return s_ActiveTooltip;
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
        }
    }


    inline bool BeginStationaryTooltipProxy(ImGuiID hash_id, bool is_hovered, const TooltipConfig* config_override = nullptr) {
        TooltipState& state = GetTooltipStateMap()[hash_id];
        
        const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
        static const TooltipConfig fallback_config;
        if (!config) config = &fallback_config;

        // 1. Hover Latch (Low Watermark & Grace Period)
        if (is_hovered) {
            state.unhover_grace_cycles = 0;
            if (!state.is_cooldown) state.hover_cycles++;
        } else {
            if (state.is_displayed) {
                state.unhover_grace_cycles++;
                if (state.unhover_grace_cycles > config->grace_period_cycles) {
                    state.is_displayed = false;
                    if (GetActiveTooltipID() == hash_id) GetActiveTooltipID() = 0;
                    state.hover_cycles = 0;
                    state.is_cooldown = false;
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

            // 3. Render Stationary Window
            ImGui::SetNextWindowPos(state.locked_pos);
            char window_name[32];
            snprintf(window_name, sizeof(window_name), "##TT_%08X", hash_id);
            bool open = ImGui::Begin(window_name, nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
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
        ImGui::End();
    }

    inline void EndStationaryTooltip(const char* str_id) {
        EndStationaryTooltip(ImGui::GetID(str_id));
    }

} // namespace V2
} // namespace ImGui
