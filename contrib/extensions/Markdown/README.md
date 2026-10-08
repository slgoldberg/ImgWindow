# Markdown Extension

The **Markdown** extension provides rich Markdown parsing and rendering for Dear ImGui within ImgWindow, based on `juliettef/imgui_markdown` and heavily enhanced for X-Plane plugin user interfaces.

It allows you to embed rich text formatting, headers, links, lists, and custom interactive widgets directly into your ImGui windows, settings panels, and stationary tooltips.

---

## Architectural Enhancements over Upstream

* **Smart Line-Break Lookahead (Zero Mid-Word Breaks):**
  * Upstream `imgui_markdown` evaluates emphasis (`**bold**`, `*italic*`) and link tokens as isolated chunks, causing Dear ImGui's word-wrapper to sever words mid-syllable at line edges.
  * Our engine checks if an upcoming emphasized token would fit on the next line and pushes it down cleanly via lookahead, keeping words intact.
  * Typographic lookahead prevents orphaned opening delimiters (`"`, `'`, `(`, `[`, `{`) and severed contractions (`don't`) or hyphens (`system-level`).
* **Hanging Indents & Bullet List Typography:**
  * Replaces cramped upstream bullet spacing with scalable, comfortable breathing room (`bulletSpacing` defaulting to a clean 4px).
  * Mathematically maintains true hanging indents across wrapped multi-line list items and continuation lines, keeping all wrapped text cleanly aligned to the text column rather than under the bullet glyph.
  * Preserves leading space depth across emphasis chunks and links so nested sub-bullets never pop out of alignment.
  * Broadens list marker syntax to standard Markdown (`*`, `-`, `+`) with or without leading whitespace.
* **Layout Pre-Calculation (`CalcSize` / `CalcTextSize`):**
  * Exposes `ImGui::MD::CalcSize(...)` (aliased as `CalcTextSize`) to pre-measure exact wrapped multi-line Markdown dimensions without rendering to the screen—crucial for sizing OS floating windows, child frames, and stationary tooltips.
  * Defeats ImGui viewport culling by employing pass-unique offscreen window hashes.
* **Dynamic Font Scaling:**
  * Uses `ImGui::GetFontScale()` to cleanly honor parent window text zoom and high-DPI display scales.
* **Custom Tag Registration:**
  * Supports custom XML/HTML-style tags (e.g., `<btn>Label</btn>`, `<badge>OK</badge>`) that execute arbitrary C++ rendering callbacks inline with text flow.
* **Collision-Free Scope (`ImGui::MD::`):**
  * Scoped under `ImGui::MD::` to mirror standard ImGui vocabulary (`Text`, `TextWrapped`) without colliding with core ImGui functions.

---

## Quick Start

### 1. Include the Header
```cpp
#include "contrib/extensions/Markdown/imgui_markdown_ext.h"
```

### 2. Configure Default Styles (Optional)
During plugin startup, you can configure your Markdown appearance globally:

```cpp
ImGui::MarkdownConfig mdConfig;

// Configure header fonts (optional):
mdConfig.headingFormats[0] = { boldLargeFont, true };   // H1
mdConfig.headingFormats[1] = { boldNormalFont, true };  // H2
mdConfig.headingFormats[2] = { italicFont, false };     // H3

// Configure tight, clean bullet spacing:
mdConfig.bulletSpacing = 4.0f;

// Set globally for the extension:
ImGui::SetDefaultMarkdownConfig(&mdConfig);
```

### 3. Rendering Markdown in Windows
Use the clean, collision-free `ImGui::MD` namespace:

```cpp
// Standard inline Markdown block:
ImGui::MD::Text("### System Status\nEngine 1: **Normal** | Fuel: *Sufficient*");

// Markdown block with automatic wrapping matching available window width:
ImGui::MD::TextWrapped(
    "Check this option to enable **custom cursors**. "
    "Changes take effect immediately without requiring a simulator restart."
);
```

### 4. Rendering Markdown in Timed Tooltips
When combined with the companion **TimedTooltip** extension, Markdown tooltips are attached with a single line:

```cpp
#include "contrib/extensions/TimedTooltip/imgui_tooltips_ext.h"
#include "contrib/extensions/Markdown/imgui_markdown_ext.h"

ImGui::Button("Accessibility Mode");
ImGui::TimedTooltip::TextMD(
    "### Accessibility Options\n"
    "Enables **larger fonts** and high-contrast styling across all plugin chrome.\n\n"
    "* Fast toggle: `Ctrl+Shift+A`\n"
    "* Requires zero reload."
);
```

---

## Configuration Reference (`ImGui::MarkdownConfig`)

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `bulletSpacing` | `float` | `0.0f` (`4.0f` active) | Horizontal gap in pixels between the bullet circle glyph and the following text. |
| `headingFormats` | `HeadingFormat[3]` | `{}` | Font pointer and separator line toggle for H1, H2, and H3 headers. |
| `linkCallback` | `LinkCallback` | `nullptr` | Invoked when the user clicks a markdown `[link](url)`. |
| `tooltipCallback` | `TooltipCallback` | `nullptr` | Invoked when hovering a link with tooltip description. |
| `imageCallback` | `ImageCallback` | `nullptr` | Invoked to render inline markdown `![alt](url)` images. |
| `formatCallback` | `FormatCallback` | `defaultMarkdownFormatCallback` | Invoked when entering/exiting format blocks (headers, lists, bold, italic). |

### Per-Call Configuration Overrides
Every rendering function in `ImGui::MD` accepts an optional `const MarkdownConfig*` pointer if a specific widget requires unique fonts, callbacks, or bullet spacing:

```cpp
ImGui::MarkdownConfig customConfig = *ImGui::GetDefaultMarkdownConfig();
customConfig.bulletSpacing = 6.0f;

ImGui::MD::Text(markdownString, &customConfig);
```

---

## Layout Pre-Calculation (`CalcTextSize`)

When building stationary tooltips or sizing dynamic OS windows to fit a block of documentation, you need to know the rendered height *before* drawing:

```cpp
ImGuiID widgetId = ImGui::GetID("doc_section");
float wrapWidth = 400.0f; // Constrain to 400px wide
float fontScale = 0.0f;   // 0.0f = inherit ambient scale

// Pre-calculates exact ImVec2 dimensions without rendering to the screen:
ImVec2 requiredSize = ImGui::MD::CalcTextSize(widgetId, 1, docString, wrapWidth, fontScale);

// Use the size to configure window bounds or child frames:
ImGui::SetNextWindowSize(requiredSize);
```

---

## Custom Inline Tags (`RegisterMarkdownWidget`)

You can register custom interactive widgets that blend seamlessly into the text stream using tag syntax:

```cpp
// Register a custom tag:
ImGui::RegisterMarkdownWidget("btn", [](const std::string& inner_text) {
    if (ImGui::SmallButton(inner_text.c_str())) {
        OnButtonClicked(inner_text);
    }
});

// Render text containing the custom tag:
ImGui::MD::Text("Click <btn>Arm Speedbrake</btn> before descending.");
```

---

## API Summary

```cpp
namespace ImGui {

    // Global Configuration & Tag Registry:
    void SetDefaultMarkdownConfig(const MarkdownConfig* config);
    const MarkdownConfig*& GetDefaultMarkdownConfig();
    void RegisterMarkdownWidget(const std::string& tag_name, CustomTagCallback callback);

    namespace MD {
        // Standard Text Rendering:
        void Text(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr);
        void TextWrapped(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr);

        // Backward-Compatibility Aliases:
        void TextMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr);
        void TextWrappedMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr);

        // Layout Pre-Measurement:
        ImVec2 CalcSize(ImGuiID id, int pass, const std::string& text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr);
        ImVec2 CalcTextSize(ImGuiID id, int pass, const std::string& text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr);
        ImVec2 CalcMarkdownSize(ImGuiID id, int pass, const std::string& text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr);

        // Interactive Button with Tooltip:
        bool ButtonMD(const std::string& label, const std::string& tooltip_md = "", float tooltip_wrap_width = 400.0f, bool tooltip_allowed = true);
    }
}
```
