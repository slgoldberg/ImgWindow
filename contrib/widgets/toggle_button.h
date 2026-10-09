// ============================================================================
// ImgWindow Contrib Widget: ToggleButton
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Modern binary toggle controls for Dear ImGui:
// 1. ImGui::ToggleButton: Animated pill toggle switch with sliding circle thumb
//    (iOS / Material design style) and automatic hand cursor on hover.
// 2. ImGui::ToggleButtons: Segmented dual-button pair for explicit binary states.
// ============================================================================

#pragma once

#ifndef IMGUI_DISABLE_EXTRA_TOGGLE_BUTTON
#ifndef IMGUI_EXTRA_TOGGLE_BUTTON_H
#define IMGUI_EXTRA_TOGGLE_BUTTON_H

#include "imgui.h"
#include <algorithm>
#include <cstring>

namespace ImGui {

    // -------------------------------------------------------------------------
    // 1. Animated Toggle Switch (Pill Track + Sliding Thumb)
    // -------------------------------------------------------------------------
    // Renders a smooth, resolution-independent toggle pill.
    // If 'label' contains visible text before '##', the label is drawn to the
    // right of the toggle switch, matching standard ImGui::Checkbox semantics.
    // Clicking either the switch or label toggles the state and returns true.
    inline bool ToggleButton(const char* label, bool* v, const ImVec2& size_arg = ImVec2(0.0f, 0.0f))
    {
        if (!v || !label) return false;

        ImVec2 p = GetCursorScreenPos();
        ImDrawList* draw_list = GetWindowDrawList();
        const ImGuiStyle& style = GetStyle();

        // Proportional sizing derived from ambient frame height:
        float height = (size_arg.y > 0.0f) ? size_arg.y : GetFrameHeight();
        float width  = (size_arg.x > 0.0f) ? size_arg.x : (height * 1.62f);
        float radius = height * 0.5f;

        PushID(label);
        bool clicked = InvisibleButton("##toggle_switch", ImVec2(width, height));
        bool hovered = IsItemHovered();
        bool held    = IsItemActive();
        PopID();

        if (clicked) {
            *v = !(*v);
        }

        if (hovered) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        // Animate sliding thumb smoothly between 0.0f (off) and 1.0f (on)
        ImGuiStorage* storage = GetStateStorage();
        ImGuiID id = GetID(label);
        float* p_t = storage ? storage->GetFloatRef(id, *v ? 1.0f : 0.0f) : nullptr;
        float target = *v ? 1.0f : 0.0f;
        if (p_t) {
            float speed = 12.0f; // animation rate
            float dt = GetIO().DeltaTime;
            if (*p_t < target) {
                *p_t = std::min(target, *p_t + speed * dt);
            } else if (*p_t > target) {
                *p_t = std::max(target, *p_t - speed * dt);
            }
        }
        float t = p_t ? *p_t : target;

        // Render Track (Pill Background)
        ImVec4 col_off = style.Colors[ImGuiCol_FrameBg];
        ImVec4 col_on  = style.Colors[ImGuiCol_HeaderActive];
        if (col_on.w <= 0.0f) col_on = style.Colors[ImGuiCol_ButtonActive];

        ImVec4 track_vec = ImVec4(
            col_off.x + (col_on.x - col_off.x) * t,
            col_off.y + (col_on.y - col_off.y) * t,
            col_off.z + (col_on.z - col_off.z) * t,
            col_off.w + (col_on.w - col_off.w) * t
        );
        ImU32 track_col = GetColorU32(track_vec);
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), track_col, radius);

        if (style.FrameBorderSize > 0.0f) {
            draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), GetColorU32(ImGuiCol_Border), radius, 0, style.FrameBorderSize);
        }

        // Render Sliding Circular Thumb
        float thumb_radius = radius - 2.5f;
        if (thumb_radius < 3.0f) thumb_radius = 3.0f;
        float x_min = p.x + radius;
        float x_max = p.x + width - radius;
        float thumb_x = x_min + (x_max - x_min) * t;
        float thumb_y = p.y + radius;

        ImU32 thumb_col = GetColorU32(held ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
        draw_list->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius, thumb_col);

        // Render Optional Label Text (Matches ImGui::Checkbox semantics)
        const char* id_pos = std::strstr(label, "##");
        bool has_visible_text = (id_pos != label) && (label[0] != '\0');
        if (has_visible_text) {
            SameLine(0.0f, style.ItemInnerSpacing.x);
            if (id_pos) {
                TextUnformatted(label, id_pos);
            } else {
                TextUnformatted(label);
            }
            if (IsItemHovered()) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
                if (IsItemClicked()) {
                    *v = !(*v);
                    clicked = true;
                }
            }
        }

        return clicked;
    }

    // -------------------------------------------------------------------------
    // 2. Segmented Dual-State Toggle Buttons
    // -------------------------------------------------------------------------
    // Side-by-side joined button pair for explicit binary states.
    // Highlights the active button and toggles the target variable.
    inline bool ToggleButtons(const char* str_id, int* v, const char* label_off, const char* label_on, const ImVec2& button_size = ImVec2(0.0f, 0.0f))
    {
        if (!v || !label_off || !label_on) return false;

        PushID(str_id);
        bool pressed = false;
        bool is_on = (*v != 0);

        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, GetStyle().ItemSpacing.y));

        // Off Button
        if (is_on) {
            PushStyleColor(ImGuiCol_Button, GetStyle().Colors[ImGuiCol_FrameBg]);
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
        } else {
            PushStyleColor(ImGuiCol_Button, GetStyle().Colors[ImGuiCol_ButtonActive]);
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_Text]);
        }
        if (Button(label_off, button_size)) {
            if (is_on) {
                *v = 0;
                pressed = true;
            }
        }
        if (IsItemHovered()) SetMouseCursor(ImGuiMouseCursor_Hand);
        PopStyleColor(2);

        SameLine(0.0f, 0.0f);

        // On Button
        if (is_on) {
            PushStyleColor(ImGuiCol_Button, GetStyle().Colors[ImGuiCol_ButtonActive]);
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_Text]);
        } else {
            PushStyleColor(ImGuiCol_Button, GetStyle().Colors[ImGuiCol_FrameBg]);
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
        }
        if (Button(label_on, button_size)) {
            if (!is_on) {
                *v = 1;
                pressed = true;
            }
        }
        if (IsItemHovered()) SetMouseCursor(ImGuiMouseCursor_Hand);
        PopStyleColor(2);

        PopStyleVar();
        PopID();

        return pressed;
    }

    inline bool ToggleButtons(const char* str_id, bool* v, const char* label_off, const char* label_on, const ImVec2& button_size = ImVec2(0.0f, 0.0f))
    {
        if (!v) return false;
        int val = *v ? 1 : 0;
        bool res = ToggleButtons(str_id, &val, label_off, label_on, button_size);
        if (res) *v = (val != 0);
        return res;
    }

} // namespace ImGui

#endif // IMGUI_EXTRA_TOGGLE_BUTTON_H
#endif // IMGUI_DISABLE_EXTRA_TOGGLE_BUTTON
