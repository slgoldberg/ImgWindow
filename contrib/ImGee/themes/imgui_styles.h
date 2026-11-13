// ============================================================================
// ImGee: Modern Studio & Cockpit Style Presets (imgui_styles.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone style presets for Dear ImGui that replace the stock 2015 electric
// blue default (ImGui::StyleColorsDark) with refined, flight-deck ready palettes.
//
// Usage:
//   ImGui::ApplyDefaultTheme();      // Applies Charcoal Slate
//   ImGui::StyleColorsCharcoal();    // Sleek modern slate/graphite
//   ImGui::StyleColorsRusty();       // Warm aviation bronze / terracotta (ABC classic)
// ============================================================================

#pragma once

#ifndef IMGUI_STYLES_H
#define IMGUI_STYLES_H

#include "imgui.h"

namespace ImGui {

/**
 * @brief Sleek modern Dark Slate / Charcoal theme.
 * Replaces the stock neon blue with understated graphite and slate tones,
 * pure white text, and high-contrast platinum/slate accents.
 */
inline void StyleColorsCharcoal(ImGuiStyle* dst = nullptr)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    // Base background colors - balanced, high-contrast studio slate
    colors[ImGuiCol_Text]                  = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.58f, 0.61f, 0.66f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.16f, 0.17f, 0.20f, 0.96f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.13f, 0.15f, 0.50f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.21f, 0.23f, 0.26f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.34f, 0.37f, 0.43f, 0.75f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frame backgrounds (input fields, checkboxes, combos)
    colors[ImGuiCol_FrameBg]               = ImVec4(0.22f, 0.24f, 0.28f, 0.85f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.30f, 0.33f, 0.39f, 0.90f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.36f, 0.40f, 0.47f, 1.00f);

    // Title bar & Menu bar
    colors[ImGuiCol_TitleBg]               = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.12f, 0.13f, 0.15f, 0.65f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.12f, 0.13f, 0.15f, 0.40f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.32f, 0.35f, 0.40f, 0.85f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.42f, 0.46f, 0.52f, 0.95f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.50f, 0.55f, 0.62f, 1.00f);

    // CheckMark & SliderGrab - crisp, high-visibility cool platinum accent
    colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.93f, 0.97f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.44f, 0.49f, 0.57f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.60f, 0.66f, 0.76f, 1.00f);

    // Buttons - distinct targets with clear contrast and bright hover feedback
    colors[ImGuiCol_Button]                = ImVec4(0.28f, 0.31f, 0.37f, 0.85f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.38f, 0.43f, 0.52f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.48f, 0.54f, 0.64f, 1.00f);

    // Headers (collapsing headers, selectables, tree nodes)
    colors[ImGuiCol_Header]                = ImVec4(0.28f, 0.31f, 0.37f, 0.75f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.38f, 0.43f, 0.52f, 0.85f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.46f, 0.52f, 0.62f, 1.00f);

    // Separators & Resize Grips
    colors[ImGuiCol_Separator]             = ImVec4(0.32f, 0.35f, 0.40f, 0.65f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.45f, 0.50f, 0.58f, 0.85f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.58f, 0.64f, 0.74f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.33f, 0.38f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.44f, 0.49f, 0.56f, 0.75f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.58f, 0.64f, 0.74f, 0.90f);

    // Tabs
    colors[ImGuiCol_Tab]                   = ImVec4(0.20f, 0.22f, 0.26f, 0.86f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.38f, 0.43f, 0.52f, 0.90f);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19090)
    colors[ImGuiCol_TabSelected]           = ImVec4(0.30f, 0.33f, 0.39f, 1.00f);
    colors[ImGuiCol_TabDimmed]             = ImVec4(0.14f, 0.15f, 0.18f, 0.90f);
    colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
#else
    colors[ImGuiCol_TabActive]             = ImVec4(0.30f, 0.33f, 0.39f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.14f, 0.15f, 0.18f, 0.90f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
#endif

    // Plots & Tables
    colors[ImGuiCol_PlotLines]             = ImVec4(0.75f, 0.80f, 0.88f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.95f, 0.96f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.55f, 0.62f, 0.72f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.75f, 0.82f, 0.92f, 1.00f);
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM >= 18000)
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.32f, 0.35f, 0.40f, 0.85f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.24f, 0.26f, 0.30f, 0.55f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.035f);
#endif

    // Text selection & Modal dimming
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.40f, 0.48f, 0.60f, 0.55f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.90f, 0.93f, 0.97f, 0.95f);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19140)
    colors[ImGuiCol_NavCursor]             = ImVec4(0.65f, 0.72f, 0.82f, 0.85f);
#else
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.65f, 0.72f, 0.82f, 0.85f);
#endif
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.08f, 0.09f, 0.10f, 0.60f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.08f, 0.09f, 0.10f, 0.65f);

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19100)
    // High-contrast, legible sky blue for hyperlinks
    colors[ImGuiCol_TextLink]              = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
#endif
}

/**
 * @brief Warm Aviation Bronze / Rusty theme.
 * Derived from the classic ABC "Rusty" preset; warm terracotta and bronze
 * tones with high-contrast warm ivory text.
 */
inline void StyleColorsRusty(ImGuiStyle* dst = nullptr)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.98f, 0.96f, 0.93f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.65f, 0.58f, 0.52f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.18f, 0.14f, 0.12f, 0.96f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.13f, 0.10f, 0.08f, 0.50f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.23f, 0.18f, 0.15f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.44f, 0.32f, 0.25f, 0.75f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]               = ImVec4(0.26f, 0.18f, 0.14f, 0.85f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.38f, 0.26f, 0.20f, 0.90f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.46f, 0.30f, 0.23f, 1.00f);

    colors[ImGuiCol_TitleBg]               = ImVec4(0.13f, 0.10f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.28f, 0.17f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.13f, 0.10f, 0.08f, 0.65f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.18f, 0.13f, 0.10f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.13f, 0.10f, 0.08f, 0.40f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.40f, 0.28f, 0.21f, 0.85f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.55f, 0.38f, 0.28f, 0.95f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.68f, 0.46f, 0.34f, 1.00f);

    colors[ImGuiCol_CheckMark]             = ImVec4(0.98f, 0.72f, 0.42f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.72f, 0.44f, 0.28f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.92f, 0.58f, 0.36f, 1.00f);

    colors[ImGuiCol_Button]                = ImVec4(0.36f, 0.24f, 0.18f, 0.85f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.50f, 0.32f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.62f, 0.38f, 0.27f, 1.00f);

    colors[ImGuiCol_Header]                = ImVec4(0.38f, 0.24f, 0.18f, 0.75f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.50f, 0.33f, 0.24f, 0.85f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.62f, 0.40f, 0.28f, 1.00f);

    colors[ImGuiCol_Separator]             = ImVec4(0.42f, 0.30f, 0.23f, 0.65f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.58f, 0.40f, 0.29f, 0.85f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.74f, 0.50f, 0.35f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.38f, 0.25f, 0.18f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.56f, 0.38f, 0.27f, 0.75f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.72f, 0.46f, 0.33f, 0.90f);

    colors[ImGuiCol_Tab]                   = ImVec4(0.26f, 0.17f, 0.13f, 0.86f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.50f, 0.33f, 0.24f, 0.90f);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19090)
    colors[ImGuiCol_TabSelected]           = ImVec4(0.38f, 0.25f, 0.19f, 1.00f);
    colors[ImGuiCol_TabDimmed]             = ImVec4(0.17f, 0.12f, 0.09f, 0.90f);
    colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.28f, 0.18f, 0.13f, 1.00f);
#else
    colors[ImGuiCol_TabActive]             = ImVec4(0.38f, 0.25f, 0.19f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.17f, 0.12f, 0.09f, 0.90f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.28f, 0.18f, 0.13f, 1.00f);
#endif

#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM >= 18000)
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.24f, 0.16f, 0.12f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.42f, 0.30f, 0.23f, 0.85f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.32f, 0.22f, 0.16f, 0.55f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 0.90f, 0.80f, 0.035f);
#endif

    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.62f, 0.36f, 0.22f, 0.55f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.98f, 0.72f, 0.42f, 0.95f);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19140)
    colors[ImGuiCol_NavCursor]             = ImVec4(0.92f, 0.58f, 0.36f, 0.85f);
#else
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.92f, 0.58f, 0.36f, 0.85f);
#endif
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.95f, 0.90f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.10f, 0.08f, 0.06f, 0.60f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.10f, 0.08f, 0.06f, 0.65f);

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19100)
    // Warm golden amber for hyperlinks
    colors[ImGuiCol_TextLink]              = ImVec4(0.98f, 0.72f, 0.42f, 1.00f);
#endif
}

/**
 * @brief Canonical default theme for ImGee applications.
 * Applies the Charcoal Slate style preset.
 */
inline void ApplyDefaultTheme(ImGuiStyle* dst = nullptr)
{
    StyleColorsCharcoal(dst);
}

} // namespace ImGui

#endif // IMGUI_STYLES_H
