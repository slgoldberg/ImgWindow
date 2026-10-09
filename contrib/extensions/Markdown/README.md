# Markdown Extension

The **Markdown** extension provides rich Markdown parsing and rendering for Dear ImGui within ImgWindow, based on `enkisoftware/imgui_markdown` and heavily enhanced for X-Plane plugin user interfaces.

It allows you to embed rich text formatting, headers, links, lists, and custom interactive widgets directly into your ImGui windows, settings panels, and stationary tooltips.

> **Author / Maintainer:** Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))  
> **Original Upstream:** Juliette Foucaut ([@juliettef](https://github.com/juliettef)) & Doug Binks ([@dougbinks](https://github.com/dougbinks)) — Repository: [enkisoftware/imgui_markdown](https://github.com/enkisoftware/imgui_markdown)  
> **License:** BSD 3-Clause (Extension Wrapper; see [ImgWindow LICENSE](../../../README.md#licensing-note)) & zlib (Upstream Parser)

---

## Architectural Overview: Foundation vs. Extension Wrapper

The Markdown system is structured as a clean two-layer architecture:

1. **Base Engine (`imgui_markdown.h`):**
   * Upstream parser originally created by Doug Binks and Juliette Foucaut ([`enkisoftware/imgui_markdown`](https://github.com/enkisoftware/imgui_markdown)).
   * Responsible for raw character scanning, CommonMark tokenization, AST-like block dispatch, and low-level C-style callbacks (`formatCallback`, `linkCallback`, `imageCallback`, `tooltipCallback`).
   * Our fork incorporates crucial low-level fixes for CommonMark punctuation lookahead and bullet list token isolation.

2. **ImgWindow Extension Wrapper (`imgui_markdown_ext.h`):**
   * High-level, ergonomic C++ layer designed specifically for X-Plane plugins and modern ImGui applications.
   * Encapsulates all functionality inside the second-level `ImGui::MD::` namespace (following Omar Cornut's standard `namespace ImGui` while eliminating global naming collisions).
   * Introduces an XML/HTML-style inline tag pre-processor (`<tag>`, `<color>`, `<backdrop>`, `<badge>`, `<btn>`) with custom tag callback registration via `ImGui::MD::RegisterTag`.
   * Solves Dear ImGui layout hurdles: typographic line-wrap lookahead (eliminating mid-word breaks across styles and links), mathematical hanging indents for wrapped multi-line bullets, and ambient font scale inheritance (`ImGui::GetFontScale()`).
   * Provides non-rendering layout measurement via `ImGui::MD::CalcTextSize` (defeating ImGui viewport culling to measure exact dimensions for floating windows and stationary tooltips).
   * Directly interfaces with the companion `TimedTooltip` extension to attach rich, multi-line Markdown tooltips with a single function call (`ImGui::TimedTooltip::TextMD`).

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

### 2. Configure Default Styles & Fonts (Optional)

You are responsible for loading your own fonts into your `ImgFontAtlas` during plugin startup (e.g., in `InitializeImGui()`). There is no runtime font synthesization in Dear ImGui—if you want Markdown to render distinct headers, bold text, or italics, you must load those fonts into your atlas.

As demonstrated in [`sample-code/InitializeImGui.cpp`](../../../sample-code/InitializeImGui.cpp), each sequential, non-merged font added increments an index in `ImGui::GetIO().Fonts->Fonts[i]`. We recommend defining macro aliases for these slots:

```cpp
// Example font slot definitions (from sample-code/InitializeImGui.cpp):
#define IM_FONT_SLOT(i)       ImGui::GetIO().Fonts->Fonts[i]
#define IM_NORMAL_FONT        IM_FONT_SLOT(0) // Regular base font (14px)
#define IMG_TITLE_FONT        IM_FONT_SLOT(1) // Large header font
#define IM_SMALLER_FONT       IM_FONT_SLOT(2) // Compact UI font
#define IM_BOLD_FONT          IM_FONT_SLOT(3) // Bold font
#define IM_BOLD_LARGER_FONT   IM_FONT_SLOT(4) // Bold title font
#define IM_MONO_NORMAL_FONT   IM_FONT_SLOT(5) // Monospace code font
#define IM_ITALIC_FONT        IM_FONT_SLOT(6) // Italic font
```

During plugin startup, configure your Markdown appearance globally using `ImGui::MD::Config`:

```cpp
ImGui::MD::Config mdConfig;

// Configure semantic fonts directly (no cryptic array indices!):
mdConfig.h1Font     = IM_BOLD_LARGER_FONT; // # H1
mdConfig.h2Font     = IMG_TITLE_FONT;      // ## H2
mdConfig.h3Font     = IM_BOLD_FONT;        // ### H3
mdConfig.boldFont   = IM_BOLD_FONT;        // **Bold** / __Bold__
mdConfig.italicFont = IM_ITALIC_FONT;      // *Italic* / _Italic_
mdConfig.monoFont   = IM_MONO_NORMAL_FONT; // `code` / <code> / <mono>

// Configure tight, clean bullet spacing:
mdConfig.bulletSpacing = 4.0f;

// Set globally for the extension:
ImGui::SetDefaultMarkdownConfig(&mdConfig);
```

> [!NOTE]
> **Graceful Font Fallback:**  
> If any font pointer is left as `nullptr`, the engine gracefully falls back:
> * `h1Font`, `h2Font`, `h3Font` fall back to `headingFormats[0..2]` (or the ambient window font).
> * `boldFont` falls back to `headingFormats[3]` (or the ambient window font).
> * `italicFont` falls back to `ImGuiCol_TextDisabled` (upstream's dimmed text color).
> * `monoFont` falls back to the ambient window font.

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
When combined with the companion **TimedTooltip** extension, rich multi-line Markdown tooltips are attached with a single line of code:

```cpp
#include "contrib/extensions/TimedTooltip/imgui_tooltip_ext.h"
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

## Configuration Reference (`ImGui::MD::Config` / `ImGui::MarkdownConfig`)

For clean namespace symmetry with `ImGui::TimedTooltip::Config`, the configuration struct is accessible as both `ImGui::MD::Config` and `ImGui::MarkdownConfig`.

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `boldFont` | `ImFont*` | `nullptr` | **[Semantic]** Font used for bold emphasis (`**bold**` / `__bold__`). (Falls back to `headingFormats[3].font`). |
| `italicFont` | `ImFont*` | `nullptr` | **[Semantic]** Font used for true italics (`*italic*` / `_italic_`). (Falls back to `ImGuiCol_TextDisabled` if null). |
| `monoFont` | `ImFont*` | `nullptr` | **[Semantic]** Font used for monospace code spans (`` `code` ``) and `<mono>` / `<code>` tags. (Falls back to ambient font if null). |
| `h1Font` | `ImFont*` | `nullptr` | **[Semantic]** Font used for H1 headers (`# Header`). (Falls back to `headingFormats[0].font`). |
| `h2Font` | `ImFont*` | `nullptr` | **[Semantic]** Font used for H2 headers (`## Header`). (Falls back to `headingFormats[1].font`). |
| `h3Font` | `ImFont*` | `nullptr` | **[Semantic]** Font used for H3 headers (`### Header`). (Falls back to `headingFormats[2].font`). |
| `bulletSpacing` | `float` | `0.0f` (`4.0f` active) | Horizontal gap in pixels between the bullet circle glyph and the following text. |
| `headingFormats` | `HeadingFormat[4]` | `{}` | Upstream array fallback for H1 (`[0]`), H2 (`[1]`), H3 (`[2]`), and Bold (`[3]`). |
| `linkCallback` | `LinkCallback` | `nullptr` | Upstream hook invoked when the user clicks a markdown `[link](url)`. |
| `tooltipCallback` | `TooltipCallback` | `nullptr` | Upstream hook invoked when hovering a link with tooltip description. |
| `imageCallback` | `ImageCallback` | `nullptr` | Upstream hook invoked to render inline markdown `![alt](url)` images. |
| `formatCallback` | `FormatCallback` | `defaultMarkdownFormatCallback` | Low-level upstream hook invoked when entering/exiting format blocks (headers, lists, bold, italic, code). |

> [!NOTE]
> **Do I Need to Implement the Callbacks?**  
> In 99% of UI tasks, **no**. The callbacks (`linkCallback`, `tooltipCallback`, `imageCallback`, and `formatCallback`) are low-level C-style hooks inherited directly from the upstream `enkisoftware/imgui_markdown` base engine.  
> 
> With the semantic font properties (`boldFont`, `italicFont`, `monoFont`, `h1Font`, etc.) and the high-level tag registration API (`ImGui::MD::RegisterTag`), you do not need to write custom format callbacks.

### Emphasis & Monospace Typography: Bold, Italics, and Code

Standard Markdown and CommonMark recognize distinct emphasis and code levels:
* **Level 1 Emphasis (`*italic*` or `_italic_`):** Traditionally rendered as italics (`<em>`).
* **Level 2 Emphasis (`**bold**` or `__bold__`):** Traditionally rendered as bold (`<strong>`).
* **Code Spans (`` `code` `` / `<code>` / `<mono>`):** Rendered in monospace font (`<code>`).

#### 1. Bold Emphasis (`**bold**` / `__bold__`)
Bold emphasis automatically uses **`mdConfig.boldFont`**. If set (e.g. `IM_BOLD_FONT`), all double-delimiter text renders in crisp bold. If left `nullptr`, it checks `headingFormats[3].font` before falling back to the ambient window font.

#### 2. True Italics (`*italic*` / `_italic_`)
In upstream `enkisoftware/imgui_markdown`, there was no field for italics, so single-delimiter text was given a shortcut: tinted with `ImGuiCol_TextDisabled` (a dimmed, half-opacity gray).

With our extension, simply assign your loaded italic font to **`mdConfig.italicFont`**:
```cpp
mdConfig.italicFont = IM_ITALIC_FONT;
```
When `italicFont` is assigned, `*hello*` and `_hello_` render in true, crisp italic typography rather than dimmed disabled gray. If left `nullptr`, it preserves upstream's disabled-color fallback for compatibility.

#### 3. Monospace Code Spans (`` `code` ``) and Code Tags (`<code>` / `<mono>`)
For code identifiers, hotkeys, file paths, and terminal syntax, assign your loaded monospace font to **`mdConfig.monoFont`**:
```cpp
mdConfig.monoFont = IM_MONO_NORMAL_FONT;
```

You have two complementary options for rendering monospace text:
* **Inline Backtick Spans (`` `code` ``):** Standard CommonMark inline backticks switch cleanly to `mdConfig.monoFont` while preserving the surrounding text color and sentence flow.
* **Code Tags (`<code>foobar</code>` or `<mono>foobar</mono>`):** Switches to `mdConfig.monoFont` and automatically draws a subtle, translucent rounded pill background (`IM_COL32(128, 128, 128, 60)`) behind the text bounding box, providing a sleek styled code badge (similar to GitHub and documentation sites). Accepts optional background tinting via parameter (e.g. `<code=#334455>foobar</code>`).

### Per-Call Configuration Overrides
Every rendering function in `ImGui::MD` accepts an optional `const MarkdownConfig*` pointer if a specific widget requires unique fonts, callbacks, or bullet spacing:

```cpp
ImGui::MD::Config customConfig = *ImGui::GetDefaultMarkdownConfig();
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
* **Automatic Line-Wrap Protection:** If an inline tag, button, or background pill exceeds the remaining horizontal space on the current line, the renderer automatically wraps down to the next line (`EnsureTagFitsOnLine`) to prevent tags from spilling past window borders.

### 4. Pre-Supplied Built-in Tags (Ready Out-of-the-Box)

The following tags are pre-registered and active by default:

| Tag | Syntax Options | Parameters & Variants | Description |
| :--- | :--- | :--- | :--- |
| **`color` / `col`** | `<color=COLOR>text</color>`<br>`<color COLOR>text</color>` | Named colors (`red`, `green`, `blue`, `yellow`, `orange`, `cyan`, `magenta`, `white`, `black`, `gray`, `gold`, `dark`, `light`) or hex (`#RGB`, `#RRGGBB`, `#RRGGBBAA`, `0xRRGGBB`). | Renders `text` in the specified color. |
| **`backdrop` / `highlight`** | `<backdrop=COLOR>text</backdrop>`<br>`<backdrop COLOR [bold] [italic]>text</backdrop>` | Accepts any named color or hex code. Optional attribute flags: `bold`, `italic`. | Renders a rounded background rectangle ("pill") behind the exact text bounding box. Backdrops auto-contrast text color against dark backgrounds and never split awkwardly across lines. |
| **`badge` / `pill` / `tag`** | `<badge=VARIANT>text</badge>`<br>`<badge VARIANT [bold] [italic]>text</badge>` | Semantic variants (`danger`, `warning`, `success`, `info`) or custom hex color. Optional flags: `bold`, `italic`. | Renders a sleek, rounded pill badge with thematic background and contrasting text. |
| **`btn`** | `<btn=ID>label</btn>`<br>`<btn ID>label</btn>` | Unique button ID or label identifier string. | Renders an inline, clickable `ImGui::SmallButton` directly within text flow. |
| **`mono` / `code`** | `<mono>text</mono>`<br>`<code [COLOR]>text</code>` | Accepts optional custom background color or hex parameter. | Renders `text` in monospace (`mdConfig.monoFont`) inside a subtle, translucent rounded background pill. |

*(To disable default tags, define `#define IMGUI_DISABLE_MARKDOWN_DEFAULT_TAGS` before including the header).*

#### Inner Markdown vs. Tag Attributes

It's helpful to distinguish how formatting reaches your tags:
1. **Inner Markdown (`<tag>**text**</tag>`):**  
   Works universally across **all** tags (both built-in tags and your own custom registered tags). If you write standard Markdown delimiters inside a tag, the engine automatically pushes the matching bold/italic font and strips the delimiter characters *before* calling the tag callback.
2. **Tag Attributes (`<tag param>text</tag>`):**  
   Built-in tags (`badge`, `backdrop`, etc.) inspect the opening tag's attribute string for colors and layout flags. When writing custom tags, the raw parameter string is passed directly into your callback as the second argument (`param`), allowing you to build arbitrary domain-specific markup (e.g., `<item id=42 count=3>Widget</item>`).

---

### 5. Custom Tag Registration (`ImGui::MD::RegisterTag`)

You can register your own custom styling tags or widgets using `ImGui::MD::RegisterTag` (or `ImGui::RegisterMarkdownTag`) with either 1-argument or 2-argument (parameterized) callbacks:

```cpp
// 1. Parameterized tag: <status=critical>Engine 1</status> or <status ok>Engine 2</status>
ImGui::MD::RegisterTag("status", [](const std::string& text, const std::string& param) {
    ImVec4 col = (param == "critical") ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
});

// 2. Simple single-argument tag: <notice>Check checklists</notice>
ImGui::MD::RegisterTag("notice", [](const std::string& text) {
    ImGui::BulletText("%s", text.c_str());
});
```

> [!TIP]
> **A Friendly Note on C++ Lambdas:**  
> If you are new to modern C++, the `[](const std::string& text, const std::string& param) { ... }` syntax is standard C++11 lambda notation for an inline anonymous function. The lambda receives whatever text was enclosed between `<tag>` and `</tag>` as `text`, and any parameters or attributes from the opening tag as `param`.

*(Note: `ImGui::RegisterMarkdownTag` and `ImGui::RegisterMarkdownWidget` are also preserved as direct backward-compatible aliases).*

---

## API Summary

```cpp
namespace ImGui {

    // Global Configuration & Tag Registry:
    void SetDefaultMarkdownConfig(const MarkdownConfig* config);
    const MarkdownConfig*& GetDefaultMarkdownConfig();
    void RegisterMarkdownTag(const std::string& tag_name, TagCallback callback);
    void RegisterMarkdownWidget(const std::string& tag_name, CustomTagCallback callback);

    namespace MD {
        // Configuration Alias:
        using Config = MarkdownConfig;

        // Custom Tag Registration (Convenience Aliases):
        void RegisterTag(const std::string& tag_name, std::function<void(const std::string& text, const std::string& param)> callback);
        void RegisterTag(const std::string& tag_name, std::function<void(const std::string& text)> callback);

        // Standard Text Rendering (Mirroring standard ImGui vocabulary):
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

## Appendix: Real-World Example & Comparison

Below is a complete real-world example illustrating how the **Markdown** and **TimedTooltip** extensions combine to create a rich, responsive settings and reference panel with custom tags, nested list items, and stationary tooltips.

### Side-by-Side: Why Use the Extensions?

To appreciate what the extensions handle behind the scenes, consider what is required to build a mixed-format settings card with highlight backdrops, colored status pills, bullet lists with hanging indents, and a stationary clamped tooltip:

#### The Hard Way: Vanilla Dear ImGui (Without Extensions)
```cpp
// -----------------------------------------------------------------------------
// VANILLA IMGUI: ~70+ lines of low-level boilerplate, manual geometry math,
// and state storage just for a single header, backdrop pill, and timed tooltip!
// -----------------------------------------------------------------------------
void RenderSettingsPanelVanilla()
{
    ImGui::Begin("Plugin Settings & Reference");

    // 1. Header: Manual font push, separator, and manual pop
    ImGui::PushFont(IMG_TITLE_FONT);
    ImGui::TextUnformatted("Welcome to Avionics Manager");
    ImGui::PopFont();
    ImGui::Separator();

    // 2. Mixed Bold Text + Background Highlight Pill:
    // In vanilla ImGui, you cannot wrap text across lines if you embed custom drawn rectangles.
    // You have to manually calculate draw list coordinates, text sizes, and padding:
    ImGui::TextUnformatted("Configure your autopilot, navigation modes, and ");
    ImGui::SameLine(0.0f, 0.0f);
    
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 text_sz = IM_BOLD_FONT->CalcTextSizeA(IM_BOLD_FONT->FontSize, FLT_MAX, 0.0f, "expert features");
    ImVec2 p1 = ImVec2(p0.x + text_sz.x + 8.0f, p0.y + text_sz.y + 2.0f);
    
    // Draw background pill manually:
    ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(255, 230, 60, 255), 4.0f);
    
    // Position text inside pill and draw in dark contrast color:
    ImGui::SetCursorScreenPos(ImVec2(p0.x + 4.0f, p0.y + 1.0f));
    ImGui::PushFont(IM_BOLD_FONT);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(20, 20, 20, 255));
    ImGui::TextUnformatted("expert features");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    
    // Resume following text:
    ImGui::SetCursorScreenPos(ImVec2(p1.x, p0.y));
    ImGui::TextUnformatted(" below.");

    ImGui::Spacing();

    // 3. Bullet List:
    // Vanilla ImGui::BulletText does NOT support hanging indents when text wraps.
    // Continuation lines wrap directly beneath the bullet circle instead of indenting cleanly.
    // Fixing this requires manual ImGui::Indent() / ImGui::Unindent() bookkeeping.
    ImGui::Bullet();
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::TextWrapped("autopilot: Controls automated flight guidance (LNAV, VNAV, FLCH).");

    ImGui::Spacing();

    // 4. Reset Button with Stationary Clamped Tooltip:
    // Vanilla ImGui::SetTooltip moves with the mouse, has no hover delay, and lacks screen clamping.
    // Building a stationary tooltip with a 1-second delay and viewport boundary clamping requires
    // querying frame counters, ImGuiStorage, and calculating display boundary math:
    ImGui::Button("Reset to Defaults");
    
    static int s_HoverFrames = 0;
    if (ImGui::IsItemHovered()) {
        s_HoverFrames++;
        if (s_HoverFrames > 60) { // ~1.0 second delay
            ImVec2 mousePos = ImGui::GetIO().MousePos;
            ImVec2 dispSize = ImGui::GetIO().DisplaySize;
            ImVec2 tipSize(260.0f, 80.0f); // Guessed height!
            
            // Manual screen boundary clamping:
            ImVec2 tipPos = ImVec2(mousePos.x, mousePos.y + 20.0f);
            if (tipPos.y + tipSize.y > dispSize.y) tipPos.y = mousePos.y - tipSize.y - 10.0f;
            if (tipPos.x + tipSize.x > dispSize.x) tipPos.x = dispSize.x - tipSize.x - 10.0f;
            
            ImGui::SetNextWindowPos(tipPos);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(255, 242, 153, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
            if (ImGui::BeginTooltip()) {
                ImGui::TextUnformatted("Restores all settings to factory defaults.");
                ImGui::EndTooltip();
            }
            ImGui::PopStyleColor(2);
        }
    } else {
        s_HoverFrames = 0;
    }

    ImGui::End();
}
```

---

#### The Clean Way: With ImgWindow Extensions (`MD` + `TimedTooltip`)
```cpp
// -----------------------------------------------------------------------------
// WITH EXTENSIONS: Clean, declarative, expressive code.
// The engine automatically handles dynamic font scaling, lookahead word wrapping,
// hanging bullet indents, draw-list pills, delay timers, and screen docking!
// -----------------------------------------------------------------------------
#include "contrib/extensions/Markdown/imgui_markdown_ext.h"
#include "contrib/extensions/TimedTooltip/imgui_tooltip_ext.h"

void RenderSettingsPanelExtensions()
{
    ImGui::Begin("Plugin Settings & Reference");

    // 1. Rich Header and Overview with Custom Backdrop Pill and Tags:
    ImGui::MD::TextWrapped(
        "### Welcome to Avionics Manager\n"
        "Configure your autopilot, navigation modes, and <backdrop=yellow>**expert features**</backdrop> below. "
        "Hover over any setting or <tag red>tag</tag> for operational manuals."
    );

    ImGui::Separator();

    // 2. Multi-Line List with Nested Bullets, Badges, and Automatic Hanging Indents:
    ImGui::MD::TextWrapped(
        "* <tag red>modes</tag>: _Click_ on active tags above to filter the feature catalog.\n"
        "  * _Important_: Filter queries use exact \"<tag yellow>substring</tag>\" matching.\n"
        "  * Toggle back to preset: `__#1__` / \"**Default Nav**\" <pill green>Active</pill>.\n"
        "* <tag blue>autopilot</tag>: Controls automated lateral and vertical flight guidance.\n"
        "  * Supported modes: **LNAV**, **VNAV**, and **FLCH**.\n"
        "  * Disconnect shortcut: `Ctrl+Shift+D`."
    );

    ImGui::Spacing();

    // 3. Interactive Widgets with Stationary Clamped Markdown Timed Tooltip:
    if (ImGui::Button("Reset to Defaults")) {
        // Reset logic...
    }
    // Automatically binds to the preceding button, manages hover cycle delays,
    // isolates styles, clamps to screen borders, and renders rich Markdown:
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

