# Markdown Extension

The **Markdown** extension provides rich Markdown parsing and rendering for Dear ImGui within ImgWindow, based on `juliettef/imgui_markdown` and heavily enhanced for X-Plane plugin user interfaces.

It allows you to embed rich text formatting, headers, links, lists, and custom interactive widgets directly into your ImGui windows, settings panels, and stationary tooltips.

> **Author / Maintainer:** Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))  
> **Original Upstream:** Juliette Foucaut ([@juliettef](https://github.com/juliettef)) & Doug Binks ([@dougbinks](https://github.com/dougbinks))  
> **License:** BSD 3-Clause (see [ImgWindow LICENSE](../../../README.md#licensing-note))

---

## Architectural Enhancements over Upstream

* **Smart Line-Break Lookahead (Zero Mid-Word Breaks):**
  * Upstream `imgui_markdown` evaluates emphasis (`**bold**`, `*italic*`) and link tokens as isolated chunks, causing Dear ImGui's word-wrapper to sever words mid-syllable at line edges.
  * Our engine checks if an upcoming emphasized token would fit on the next line and pushes it down cleanly via lookahead, keeping words intact.
  * Typographic lookahead prevents orphaned opening delimiters (`"`, `'`, `(`, `[`, `{`) and severed contractions (`don't`) or hyphens (`system-level`).
  * Conforms to CommonMark punctuation rules: standard punctuation (`/`, `(`, `[`, `{`, `-`, `:`, etc.) can immediately precede emphasis tokens (e.g., `#2/**"Cerise"**` or `(**Important**)`), while preserving strict intra-word identifier safety (`variable_name`).
* **Hanging Indents & Bullet List Typography:**
  * Replaces cramped upstream bullet spacing with scalable, comfortable breathing room (`bulletSpacing` defaulting to a clean 4px).
  * Mathematically maintains true hanging indents across wrapped multi-line list items and continuation lines, keeping all wrapped text cleanly aligned to the text column rather than under the bullet glyph.
  * Preserves leading space depth across emphasis chunks and links so nested sub-bullets never pop out of alignment.
  * Broadens list marker syntax to standard Markdown (`*`, `-`, `+`) with or without leading whitespace, with strict token isolation so bullet glyphs never hijack inline styling into rogue italic runs.
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

---

## Markdown Styling Tags (`<tag=param>` / `<tag param>`)

The extension includes a built-in pre-processor that allows you to embed rich, inline styling tags and interactive widgets directly within Markdown text without breaking standard Markdown formatting rules:

```markdown
This option is <color=red>highly experimental</color>!
Please verify <backdrop=yellow>active route</backdrop> before flight.
System status: <badge=danger>FAULT</badge> | <badge=success>ONLINE</badge>
```

### 1. Flexible Tag Syntax

Tags support both HTML-style attributes and key-value formats:
- **Assignment format:** `<tag=red>text</tag>`, `<backdrop=yellow>text</backdrop>`
- **Space-separated format:** `<tag red>text</tag>`, `<color #00FFCC>text</color>`
- **Multi-parameter format:** `<tag=red,bold>text</tag>`, `<tag red bold>text</tag>`

### 2. Bold and Italic Styling Inside Tags

You can style tag text in two natural ways:

#### A. Markdown Syntax Inside the Tag (Recommended)
Place standard Markdown delimiters directly inside the tag content:
```markdown
Check <tag red>**accessibility**</tag> or options.
Please review <backdrop=yellow>**critical checklist**</backdrop> before departure.
Click <btn>*more info*</btn> for details.
```
* The engine detects inner Markdown wrappers (`**bold**`, `__bold__`, `*italic*`, `_italic_`), strips the wrapper characters so asterisks never render literally, and pushes the active bold/italic font.
* For `<backdrop>`, the font is pushed *before* measuring text width, ensuring the pill bounding box fits the bold glyphs.

#### B. Tag Parameter Flags
You can also specify `bold` (or `b`) and `italic` (or `i`) directly in the tag attributes:
```markdown
Check <tag red bold>accessibility</tag> or options.
Check <tag=red,bold>accessibility</tag> or options.
<btn bold>Acknowledge</btn>
```

> [!NOTE]
> **Why `**<tag>...</tag>**` is not supported:**
> In Markdown and CommonMark, formatting delimiters wrap text spans rather than embedding interactive widgets across boundary tokens. Nesting formatting inside the tag (`<tag>**text**</tag>`) or on the tag (`<tag bold>text</tag>`) follows standard HTML/XML element structure and guarantees correct word wrapping and draw-list grouping.

### 3. Inline Spacing & Punctuation Flow

Inline tags integrate cleanly into paragraph and sentence flow:
* **Preserved Spaces:** When followed by a space, such as `<tag red>badge</tag> or options`, the engine preserves the space character so text flows naturally without cramped or "eaten" spacing.
* **Flush Punctuation:** When followed by punctuation without space, such as `<tag red>badge</tag>, options`, the punctuation docks directly flush against the tag border.

### 4. Pre-Supplied Built-in Tags (Ready Out-of-the-Box)

The following tags are pre-registered and active by default:

| Tag | Syntax | Description |
| :--- | :--- | :--- |
| **`color` / `col`** | `<color=NAME_OR_HEX>text</color>` | Renders `text` in the specified color. Supports UI names (`red`, `green`, `blue`, `yellow`, `orange`, `cyan`, `magenta`, `white`, `black`, `gray`, `gold`) or hex (`#RRGGBB`, `#RRGGBBAA`, `0xRRGGBB`). |
| **`backdrop` / `highlight`** | `<backdrop=COLOR>text</backdrop>` | Renders a rounded background rectangle ("pill") behind the exact text bounding box. Because of lookahead wrapping, backdrops never get severed across lines. Auto-contrasts text color if the background is dark. |
| **`badge` / `pill` / `tag`** | `<badge=VARIANT>text</badge>` | Renders a sleek pill badge. Variants include `danger` (red), `warning` (yellow), `success` (green), `info` (blue), or any custom hex color. |
| **`btn`** | `<btn=ID>label</btn>` | Renders an inline, clickable `ImGui::SmallButton`. |

*(To disable default tags, define `#define IMGUI_DISABLE_MARKDOWN_DEFAULT_TAGS` before including the header).*

### 5. Custom Tag Registration (`RegisterMarkdownTag`)

You can register your own custom styling tags or widgets with either 1-argument or 2-argument (parameterized) callbacks:

```cpp
// Register a parameterized tag: <status=critical>Engine 1</status>
ImGui::RegisterMarkdownTag("status", [](const std::string& text, const std::string& param) {
    ImVec4 col = (param == "critical") ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
});

// Simple single-argument tag: <notice>Check checklists</notice>
ImGui::RegisterMarkdownTag("notice", [](const std::string& text) {
    ImGui::BulletText("%s", text.c_str());
});
```

*(Note: `ImGui::RegisterMarkdownWidget` is preserved as a direct backward-compatible alias).*

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

---

## Appendix: Real-World Example (Settings & Reference Manual)

Below is a complete, copy-pasteable real-world example illustrating how the **Markdown** and **TimedTooltip** extensions combine to create a searchable settings panel with custom tags, nested list items, and rich stationary tooltips:

```cpp
#include "contrib/extensions/Markdown/imgui_markdown_ext.h"
#include "contrib/extensions/TimedTooltip/imgui_tooltips_ext.h"

void RenderSettingsPanel()
{
    ImGui::Begin("Plugin Settings & Reference");

    // 1. Rich Header and Overview with Custom Badges
    ImGui::MD::TextWrapped(
        "### Welcome to Avionics Manager\n"
        "Configure your autopilot, navigation modes, and <backdrop=yellow>**expert features**</backdrop> below. "
        "Hover over any setting or <tag red>tag</tag> for operational manuals."
    );

    ImGui::Separator();

    // 2. Multi-Line List with Nested Bullets, Badges, and Punctuation Delimiters
    ImGui::MD::TextWrapped(
        "* <tag red>modes</tag>: _Click_ on active tags above to filter the feature catalog.\n"
        "  * _Important_: Filter queries use exact \"<tag yellow>substring</tag>\" matching.\n"
        "  * Toggle back to preset: `__#1__` / \"**Default Nav**\" <pill green>Active</pill>.\n"
        "* <tag blue>autopilot</tag>: Controls automated lateral and vertical flight guidance.\n"
        "  * Supported modes: **LNAV**, **VNAV**, and **FLCH**.\n"
        "  * Disconnect shortcut: `Ctrl+Shift+D`."
    );

    ImGui::Spacing();

    // 3. Interactive Widgets with Rich Markdown Timed Tooltips
    if (ImGui::Button("Reset to Defaults")) {
        // Reset logic...
    }
    ImGui::TimedTooltip::TextMD(
        "### Reset Configuration\n"
        "Restores all plugin settings back to factory defaults.\n\n"
        "* **Reversible:** A backup of your current setup is saved automatically.\n"
        "* Active theme: #3/\"**Black & White**\" <pill green>PG</pill>.\n"
        "* *Click to confirm.*"
    );

    ImGui::End();
}
```

