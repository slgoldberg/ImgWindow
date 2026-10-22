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

#pragma once

#if !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGUI_DISABLE_CONTRIB_TIMED_TOOLTIP) && \
    !defined(IMGUI_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_TIMED_TOOLTIP)
#ifndef IMGUI_TOOLTIP_EXT_H
#define IMGUI_TOOLTIP_EXT_H

#include "imgui.h"
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>

namespace ImGui {
namespace TimedTooltip {
    using std::min;
    using std::max;

    struct TooltipConfig {
        int hover_delay_cycles = 25;      // Low Watermark: Cycles before appearing (~0.4s at 60fps)
        int max_display_cycles = 504;     // High Watermark: Cycles before auto-hiding
        int grace_period_cycles = 12;     // Forgiveness cycles for slipping off the widget
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
        ImFont* font = nullptr;                                  // Custom font for tooltip overlay window (if nullptr, uses ambient)
        
        ImVec4 bg_color = ImVec4(1.0f, 0.95f, 0.6f, 1.0f);      // Yellow Sticky Note (100% opaque)
        ImVec4 border_color = ImVec4(0.8f, 0.75f, 0.4f, 1.0f);   // Slightly darker border
        ImVec4 text_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);      // Black text

        void SetBgColor(ImU32 col) { bg_color = ColorConvertU32ToFloat4(col); }
        void SetBorderColor(ImU32 col) { border_color = ColorConvertU32ToFloat4(col); }
        void SetTextColor(ImU32 col) { text_color = ColorConvertU32ToFloat4(col); }
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

    // Resolves the active font scale: explicit config->font_scale override if set,
    // otherwise the ambient font scale from the calling parent window:
    inline float GetActiveFontScale(const TooltipConfig* config = nullptr) {
        if (config && config->font_scale > 0.0f) {
            return config->font_scale;
        }
        float font_size = ImGui::GetFontSize();
        if (font_size > 0.0f) {
            return font_size / 14.0f;
        }
        return ImGui::GetIO().FontGlobalScale;
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
        bool font_pushed = false;
        int last_frame_updated = 0;
        ImGuiContext* last_context_updated = nullptr;
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
        for (auto& kv : GetTooltipStateMap()) {
            if (kv.second.is_displayed || kv.second.hover_cycles > 0) {
                kv.second.is_displayed = false;
                kv.second.hover_cycles = 0;
                kv.second.unhover_grace_cycles = 0;
                kv.second.is_cooldown = false;
                kv.second.perfect_size = ImVec2(0, 0);
            }
        }
    }


    inline bool BeginStationaryTooltipProxy(ImGuiID hash_id, bool is_hovered, const TooltipConfig* config_override = nullptr) {
        ImGuiContext* cur_ctx = ImGui::GetCurrentContext();
        int cur_frame = ImGui::GetFrameCount();

        // Foreground-Widget Hover Priority:
        // In Dear ImGui, ImGui::IsItemHovered() only tests bounding-box overlap (HoveredRect),
        // so a later full-width background Selectable(SpanAllColumns) on the same line will also
        // report IsItemHovered() == true even when the mouse is over an earlier Button on that line!
        // The first widget in a given (ImGuiContext*, frame_count) that passes is_hovered == true
        // claims tooltip hover for the frame; any later overlapping widget yields automatically.
        static ImGuiContext* s_ClaimedHoverCtx   = nullptr;
        static int           s_ClaimedHoverFrame = -1;
        static ImGuiID       s_ClaimedHoverID    = 0;
        bool preempted_by_other_widget = false;
        if (is_hovered) {
            if (s_ClaimedHoverCtx != cur_ctx || s_ClaimedHoverFrame != cur_frame) {
                s_ClaimedHoverCtx   = cur_ctx;
                s_ClaimedHoverFrame = cur_frame;
                s_ClaimedHoverID    = hash_id;
            } else if (s_ClaimedHoverID != hash_id) {
                is_hovered = false;
                preempted_by_other_widget = true;
            }
        } else if (s_ClaimedHoverCtx == cur_ctx && s_ClaimedHoverFrame == cur_frame && s_ClaimedHoverID != hash_id) {
            preempted_by_other_widget = true;
        }

        TooltipState& state = GetTooltipStateMap()[hash_id];
        state.last_frame_updated = cur_frame; // Mark as alive this frame!
        state.last_context_updated = cur_ctx;
        
        // Garbage Collection & Immediate Preemption:
        // If another widget is now the foreground hovered widget, or if the active tooltip was
        // abandoned (e.g. its ImGuiID changed due to a label toggle), release the active tooltip immediately.
        ImGuiID active_id = GetActiveTooltipID();
        if (active_id != 0 && active_id != hash_id) {
            TooltipState& active_state = GetTooltipStateMap()[active_id];
            bool abandoned_in_same_ctx = (active_state.last_context_updated == cur_ctx &&
                                          active_state.last_frame_updated < cur_frame - 1);
            if (is_hovered || abandoned_in_same_ctx || !active_state.is_displayed) {
                active_state.is_displayed = false;
                active_state.hover_cycles = 0;
                active_state.unhover_grace_cycles = 0;
                active_state.is_cooldown = false;
                active_state.perfect_size = ImVec2(0, 0);
                GetActiveTooltipID() = 0;
            }
        }
        
        const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
        static const TooltipConfig fallback_config;
        if (!config) config = &fallback_config;

        ImGuiIO& io = GetIO();
        const bool is_mouse_wheel_active = (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f);
        const bool is_mouse_dragging = ImGui::IsMouseDragging(0) || ImGui::IsMouseDragging(1) || ImGui::IsMouseDragging(2);

        // 1. Hover Latch (Low Watermark & Grace Period)
        if (is_mouse_wheel_active || is_mouse_dragging) {
            // User input active (spinning wheel or dragging) -> slam low-water timer back to 0!
            state.hover_cycles = 0;
            state.unhover_grace_cycles = 0;
            if (state.is_displayed) {
                state.is_displayed = false;
                if (GetActiveTooltipID() == hash_id) GetActiveTooltipID() = 0;
                state.is_cooldown = false;
                state.perfect_size = ImVec2(0, 0);
            }
        } else if (is_hovered) {
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
                if (preempted_by_other_widget || state.unhover_grace_cycles > config->grace_period_cycles) {
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

        float scale = GetActiveFontScale(config);
        if (scale <= 0.0f) scale = 1.0f;

        // Trigger Display (With Global Mutex)
        if (!state.is_displayed && state.hover_cycles >= config->hover_delay_cycles) {
            if (GetActiveTooltipID() == 0 || GetActiveTooltipID() == hash_id) {
                GetActiveTooltipID() = hash_id; // CLAIM THE MUTEX!
                state.is_displayed = true;
                state.display_cycles = 0;
            // Lock anchor position (Offset slightly so the cursor doesn't cover the text)
            state.locked_pos = ImVec2(ImGui::GetMousePos().x + 15.0f * scale, ImGui::GetMousePos().y + 15.0f * scale);
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
/* [Roger: This is experimental: -Steve] */
/* [The goal being to force very short timeouts for just one line or two.] */
const float ssMult = min(max(0.125f, config->size_skew_multiplier), 0.875f);
const float ssFactor = 16.f * scale; // approx chunk sizes (vertical or horizontal)
if (state.last_size.y <= ssFactor)
    max_timeout = (int)(ssMult * (0.5f * (max_timeout) + state.last_size.x / ssFactor));
else if (state.last_size.y <= 2.f * ssFactor)
    max_timeout = (int)(ssMult * (0.67f * (max_timeout) + state.last_size.x / ssFactor));
else // only *add* to max timeout if it's not very short per above:
                    max_timeout += (int)(state.last_size.y * ssMult/*config->size_skew_multiplier*/);
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

            // 3. Render Stationary Window with automatic screen-edge clamping (flips above/left of cursor if clipping!)
            ImVec2 render_pos = state.locked_pos;
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            ImVec2 scaled_padding = ImVec2(config->padding.x * scale, config->padding.y * scale);
            float expected_h = (state.perfect_size.y > 0.0f) ? (state.perfect_size.y + scaled_padding.y * 2.0f) : state.last_size.y;
            float expected_w = (state.perfect_size.x > 0.0f) ? (state.perfect_size.x + scaled_padding.x * 2.0f) : state.last_size.x;

            // Ensure expected dimensions don't exceed the active display boundaries
            if (expected_w > display_size.x - 25.0f) expected_w = display_size.x - 25.0f;
            if (expected_h > display_size.y - 25.0f) expected_h = display_size.y - 25.0f;

            // Flip UPWARDS above cursor if tooltip would clip off the bottom of the screen!
            if (expected_h > 0.0f && render_pos.y + expected_h + 10.0f > display_size.y) {
                render_pos.y = state.last_mouse_pos.y - expected_h - 10.0f * scale;
                if (render_pos.y < 10.0f) render_pos.y = 10.0f;
            }

            // Shift LEFTWARDS (or flip left of cursor) if tooltip would clip off the right edge of the screen!
            if (expected_w > 0.0f && render_pos.x + expected_w + 10.0f > display_size.x) {
                float left_pos = state.last_mouse_pos.x - expected_w - 10.0f * scale;
                if (left_pos >= 10.0f) {
                    render_pos.x = left_pos;
                } else {
                    render_pos.x = display_size.x - expected_w - 15.0f;
                    if (render_pos.x < 10.0f) render_pos.x = 10.0f;
                }
            }

            ImGui::SetNextWindowPos(render_pos);
            char window_name[32];
            snprintf(window_name, sizeof(window_name), "##TT_%08X", hash_id);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, config->bg_color);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, config->bg_color);
            ImGui::PushStyleColor(ImGuiCol_Border, config->border_color);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scaled_padding);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(config->item_spacing.x * scale, config->item_spacing.y * scale));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(config->frame_padding.x * scale, config->frame_padding.y * scale));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, config->indent_spacing * scale);
            bool open = ImGui::Begin(window_name, nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            (void)open;
            if (scale != 1.0f) {
                ImGui::SetWindowFontScale(scale);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, config->text_color); // Pushed AFTER Begin to affect window contents!
            ImGui::PushStyleColor(ImGuiCol_Separator, config->text_color); // Ensures markdown horizontal lines match the text color!
            if (config->font) {
                ImGui::PushFont(config->font);
                state.font_pushed = true;
            } else {
                state.font_pushed = false;
            }
            // Once Begin() is called, always return true so caller invokes EndStationaryTooltip()
            // to properly close the window and pop styles, avoiding 1-frame titlebar blips on appearance.
            return true;
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
        if (state.font_pushed) {
            ImGui::PopFont();
            state.font_pushed = false;
        }
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

} // namespace TimedTooltip
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

    // Return-value proxy for *Link widgets (ButtonLink, SmallButtonLink, TextLink, MD::TextLink):
    // Implicitly converts to bool when tested in `if (...)` or assigned to a bool, marking the
    // click as handled by the caller. When called as a standalone statement without testing
    // the return value, `operator bool()` is not called, allowing a trailing TimedTooltip
    // to automatically toggle ON/OFF when the widget is clicked!
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
namespace TimedTooltip {

    // Explicitly marks the preceding widget as a click-to-toggle tooltip target for this frame:
    inline void ToggleOnClick(bool enable = true) {
        ImGui::Detail::RegisterItemTooltipToggle(enable);
    }

    // Handles item click: dismisses if currently displayed; otherwise, if 'toggle_on_click'
    // is true (e.g. passive text, slider/drag/combo label, or unconsumed *Link widget),
    // immediately triggers tooltip display on click!
    inline void HandleTooltipClick(ImGuiID hash_id, bool is_clicked, bool toggle_on_click = false) {
        if (!is_clicked || hash_id == 0) return;
        TooltipState& state = GetTooltipStateMap()[hash_id];
        if (state.is_displayed) {
            state.is_displayed = false;
            if (GetActiveTooltipID() == hash_id) GetActiveTooltipID() = 0;
            state.hover_cycles = 0;
            state.unhover_grace_cycles = 0;
            state.is_cooldown = false;
            state.perfect_size = ImVec2(0, 0);
        } else if (toggle_on_click) {
            if (GetActiveTooltipID() != 0 && GetActiveTooltipID() != hash_id) {
                ClearActiveTooltip();
            }
            state.is_cooldown = false;
            state.unhover_grace_cycles = 0;
            state.hover_cycles = 1000000; // Satisfies hover_delay_cycles immediately in BeginStationaryTooltipProxy!
        } else {
            state.hover_cycles = 0;
            state.unhover_grace_cycles = 0;
            state.is_cooldown = false;
            state.perfect_size = ImVec2(0, 0);
        }
    }

    inline bool TextProxyV(ImGuiID id, bool is_hovered, const TooltipConfig* config_override, const char* fmt, va_list args, bool is_clicked = false, bool toggle_on_click = false) {
        ImGuiMouseCursor previous_cursor = ImGui::GetMouseCursor();

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
                id = ImGui::GetID(coord_key != 0 ? &coord_key : (const void*)(fmt ? fmt : "##timed_tt_anon"));
            }
        }

        HandleTooltipClick(id, is_clicked, toggle_on_click);

        bool displayed = false;
        if (BeginStationaryTooltipProxy(id, is_hovered, config_override)) {
            displayed = true;
            const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
            static const TooltipConfig fallback_config;
            if (!config) config = &fallback_config;

            TooltipState& state = GetTooltipStateMap()[id];
            (void)state;
            
            float scale = GetActiveFontScale(config);
            if (scale <= 0.0f) scale = 1.0f;

            // 1. Audit and sanitize wrap constraints against screen boundaries
            if (config->default_wrap_width < 0.0f) {
                // Explicit non-wrapping mode (single line per '\n'):
                ImGui::TextV(fmt, args);
            } else {
                float max_screen_w = std::max(100.0f, ImGui::GetIO().DisplaySize.x - 40.0f);
                float min_w = config->min_wrap_width * scale;
                float max_w = config->max_wrap_width * scale;
                if (min_w < 50.0f * scale) min_w = 50.0f * scale;
                if (max_w < min_w) max_w = min_w;
                if (max_w > max_screen_w) max_w = max_screen_w;
                if (min_w > max_w) min_w = max_w;

                float target_w = (config->default_wrap_width > 0.0f) ? (config->default_wrap_width * scale) : (360.0f * scale);
                float wrap_limit = (target_w < min_w) ? min_w : (target_w > max_w) ? max_w : target_w;
                if (wrap_limit > max_screen_w) wrap_limit = max_screen_w;

                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_limit);
                ImGui::TextV(fmt, args);
                ImGui::PopTextWrapPos();
            }

            EndStationaryTooltip(id);
        }

        if (previous_cursor != ImGui::GetMouseCursor()) {
            ImGui::SetMouseCursor(previous_cursor);
        }
        return displayed;
    }

    inline bool TextV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        ImGuiID raw_id = ImGui::GetItemID();
        bool toggle = ImGui::Detail::ShouldToggleTooltipOnClick(raw_id, ImGui::IsItemActive());
        bool hovered = ImGui::Detail::ResolveItemTooltipHover(raw_id, ImGui::IsItemHovered());
        return TextProxyV(raw_id, hovered, config_override, fmt, args, ImGui::IsItemClicked(), toggle);
    }

    inline bool TextLinkV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        ImGuiID raw_id = ImGui::GetItemID();
        bool hovered = ImGui::Detail::ResolveItemTooltipHover(raw_id, ImGui::IsItemHovered());
        return TextProxyV(raw_id, hovered, config_override, fmt, args, ImGui::IsItemClicked(), true);
    }

    // Proxy Timed Tooltips (accept explicit ID and explicit is_hovered state):
    inline bool TextProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextProxyV(id, is_hovered, config, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextProxy(ImGuiID id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextProxyV(id, is_hovered, nullptr, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextProxy(const char* str_id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextProxyV(ImGui::GetID(str_id), is_hovered, nullptr, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextProxy(ImGuiID id, bool is_hovered, const std::string& text, const TooltipConfig* config = nullptr) {
        return TextProxy(id, is_hovered, config, "%s", text.c_str());
    }

    inline bool TextProxy(const char* str_id, bool is_hovered, const std::string& text, const TooltipConfig* config = nullptr) {
        return TextProxy(ImGui::GetID(str_id), is_hovered, config, "%s", text.c_str());
    }

    // Plain-text Timed Tooltip with per-call config override
    inline bool Text(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextV(config, fmt, args);
        va_end(args);
        return res;
    }

    // Plain-text Timed Tooltip: mirrors standard ImGui vocabulary (ImGui::TimedTooltip::Text)
    inline bool Text(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool Text(const std::string& text, const TooltipConfig* config = nullptr) {
        return Text(config, "%s", text.c_str());
    }

    // Non-wrapping Unformatted Timed Tooltip (never auto-wraps; single line per '\n'):
    inline bool TextUnformatted(const char* text, const TooltipConfig* config = nullptr) {
        if (!text) return false;
        TooltipConfig local_cfg = config ? *config : GetCurrentTooltipConfig();
        local_cfg.default_wrap_width = -1.0f;
        return Text(&local_cfg, "%s", text);
    }

    inline bool TextUnformatted(const std::string& text, const TooltipConfig* config = nullptr) {
        return TextUnformatted(text.c_str(), config);
    }

    // Explicit click-to-toggle Timed Tooltip chasers (ImGui::TimedTooltip::TextLink):
#pragma push_macro("TextLink")
#undef TextLink
    inline bool TextLink(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextLinkV(config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextLink(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextLinkV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextLink(const std::string& text, const TooltipConfig* config = nullptr) {
        return TextLink(config, "%s", text.c_str());
    }

    inline bool TextLinkContrib(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextLinkV(config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextLinkContrib(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextLinkV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextLinkContrib(const std::string& text, const TooltipConfig* config = nullptr) {
        return TextLinkContrib(config, "%s", text.c_str());
    }
#pragma pop_macro("TextLink")

} // namespace TimedTooltip

} // namespace ImGui

#if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED)
#if defined(__has_include)
#if __has_include("../Markdown/imgui_markdown_ext.h")
#include "../Markdown/imgui_markdown_ext.h"
#elif __has_include("imgui_markdown_ext.h")
#include "imgui_markdown_ext.h"
#endif
#endif
#endif

#endif // IMGUI_TOOLTIP_EXT_H
#endif // !IMGUI_DISABLE_TIMED_TOOLTIP
