// ============================================================================
// ImGui Contrib: Opt-In Global Namespace Convenience Aliases
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// By default, all ImgWindow Contrib extensions live strictly under ImGui::
// to preserve pristine namespace hygiene (e.g. ImGui::TimedTooltip,
// ImGui::Markdown, ImGui::MD).
//
// Including this header (or defining CONTRIB_ENABLE_GLOBAL_NAMESPACES before
// including contrib/includes.h or contrib.h) publishes top-level convenience
// aliases into the global namespace for developers who prefer minimal boilerplate:
//
//   TimedTooltip::Text(...)   --> ImGui::TimedTooltip::Text(...)
//   Markdown::Text(...)       --> ImGui::Markdown::Text(...)
//   MD::Text(...)             --> ImGui::MD::Text(...)
// ============================================================================

#pragma once

#ifndef IMGUI_CONTRIB_GLOBAL_H
#define IMGUI_CONTRIB_GLOBAL_H

#include "includes.h"

#if !defined(CONTRIB_DISABLE_GLOBAL_NAMESPACES) && \
    !defined(IMGUI_CONTRIB_DISABLE_GLOBAL_NAMESPACES)

#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(CONTRIB_DISABLE_GLOBAL_TIMED_TOOLTIP)
namespace TimedTooltip = ImGui::TimedTooltip;
#endif

#if defined(IMGUI_MARKDOWN_EXT_H) && !defined(CONTRIB_DISABLE_GLOBAL_MARKDOWN)
namespace Markdown = ImGui::Markdown;
namespace MD       = ImGui::Markdown;
#endif

#endif // !CONTRIB_DISABLE_GLOBAL_NAMESPACES

#endif // IMGUI_CONTRIB_GLOBAL_H
