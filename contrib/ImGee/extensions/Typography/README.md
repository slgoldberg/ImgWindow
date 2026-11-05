# ImGui Contrib Typography & StyledText Extension

The **Typography** extension provides semantic font hierarchy management and a lightweight, inline rich text engine for Dear ImGui within the **ImgWindow** framework.

Unlike full Markdown documents (which enforce paragraph margins, headers, block indentation, and thematic breaks), the **Inline StyledText Engine** is designed for tight cockpit UI elements: data readouts, status pills, tables, buttons, and mixed-style sentences where continuous baseline alignment is paramount.

---

## 🚀 Key Features

1. **Semantic Font Hierarchy (`ImFontRole`)**:
   Eliminates hardcoded font pointers (`m_pHeaderFont->...`). Plugins register fonts by semantic role (`Title`, `Header`, `Body`, `Small`, `Mono`), and widgets request fonts by role.
2. **Commutative Scoped Font Guard (`ScopedFontRole`)**:
   Zero-leak RAII guard that safely pushes a semantic font role and pops it upon exiting the scope.
3. **Inline StyledText Engine (`ImGui::StyledText`)**:
   Renders mixed bold, italic, monospace, colored text, badges, pills, keyboard keycaps, and images within a single immediate-mode call.
4. **Continuous Vertical Baseline Alignment**:
   Inline badges, pills, `<kbd>` keycaps, and icons automatically compute font metrics (`Font->Ascent`, `Font->Descent`) to align their visual center with the surrounding text baseline, completely eliminating vertical text jitter.
5. **Universal Procedural Vector Icons**:
   Full integration with `imgui_icons.h`—supports `<icon caret-up />`, emoji shortcodes (`:gear:`), raw Unicode glyphs (`▲`, `↺`), and named memory textures.
6. **Layout Pre-Measurement (`ImGui::CalcTextStyledSize`)**:
   Calculates exact rendered dimensions in pixels without drawing to the screen.

---

## 1. Semantic Font Roles (`imgui_font_roles.h`)

### The Problem
Traditional Dear ImGui plugins pass raw `ImFont*` pointers across classes or query global singletons (`m_Fonts[2]`). If a specific font size or style isn't loaded, code either asserts or crashes.

### The Solution: Semantic Roles & 4-Tier Fallback
Font roles decouple your layout code from font assets:

```cpp
#include "contrib/extensions/Typography/imgui_font_roles.h"

// Available Roles:
enum class ImFontRole {
    Default,
    Body,
    Title,
    Header,
    Heading,
    Subheading,
    Caption,
    Small,
    Mono,
    Code,
    Large,
    Bold,
    Italic
};
```

#### Registering Roles in `ImgFontAtlas`
During plugin startup, bind loaded fonts to semantic roles:

```cpp
void InitializeFonts() {
    auto atlas = ImgWindow::sFontAtlas;
    ImFont* fontRegular = atlas->AddFontFromFileTTF("fonts/Roboto-Regular.ttf", 15.0f);
    ImFont* fontBold    = atlas->AddFontFromFileTTF("fonts/Roboto-Bold.ttf", 15.0f);
    ImFont* fontHeader  = atlas->AddFontFromFileTTF("fonts/Roboto-Bold.ttf", 20.0f);
    ImFont* fontMono    = atlas->AddFontFromFileTTF("fonts/SourceCodePro-Regular.ttf", 14.0f);

    atlas->SetFontRole(ImFontRole::Body,   fontRegular);
    atlas->SetFontRole(ImFontRole::Bold,   fontBold);
    atlas->SetFontRole(ImFontRole::Header, fontHeader);
    atlas->SetFontRole(ImFontRole::Mono,   fontMono);
}
```

#### Scoped Font Guards (`ScopedFontRole`)
Use RAII guards to push and pop font roles safely:

```cpp
{
    ScopedFontRole headerGuard(ImFontRole::Header);
    ImGui::Text("Avionics Status & Navigation");
} // Restores previous font automatically

{
    ScopedFontRole monoGuard(ImFontRole::Mono);
    ImGui::Text("RAW NMEA: $GPGGA,123519,4807.038,N,01131.000,E*47");
}
```

---

## 2. Inline StyledText Engine (`imgui_styled_text.h`)

`ImGui::StyledText` parses lightweight XML-style tags and renders continuous text flows across lines.

### Basic Usage

```cpp
#include "contrib/extensions/Typography/imgui_styled_text.h"

// Inline bold, italic, and color:
ImGui::StyledText("Transponder is <color=green>ACTIVE</color> on <b>XPDR1</b>.");

// Wrapped text:
ImGui::StyledTextWrapped("Notice: Route clearance is <i>pending</i> from <color=#3498db>ATC Control</color>.");
```

### Supported Tag Catalog

| Tag | Example Syntax | Description |
| :--- | :--- | :--- |
| **Small** | `<small>reduced text</small>` | Applies `ImFontRole::Small` (Slot 2, typically 12px / -2px) |
| **Tiny** | `<tiny>micro text</tiny>` | Applies `ImFontRole::Tiny` (10px micro font, or fallback to `Small`) |
| **Big / Large** | `<big>larger text</big>` or `<large>...</large>` | Applies `ImFontRole::Title` (Slot 1, typically 16px / +2px) |
| **Huge** | `<huge>hero text</huge>` | Applies `ImFontRole::BoldLarge` (Slot 4, 16px+ bold) |
| **Regular** | `<regular>normal</regular>` or `<sans>...</sans>` | Applies `ImFontRole::Regular` (Slot 0 default body font) |
| **Bold** | `<b>bold text</b>` or `<strong>...</strong>` | Applies `ImFontRole::Bold` (Slot 3) |
| **Italic** | `<i>italic text</i>` or `<em>...</em>` | Applies `ImFontRole::Italic` (Slot 6) |
| **Monospace** | `<mono>code</mono>` or `<code>code</code>` | Applies `ImFontRole::Monospace` (Slot 5) |
| **Color** | `<color=red>warning</color>` or `<color=#ffaa00>caution</color>` | Applies foreground text color (named or hex) |
| **Headings** | `<h1>Banner</h1>`, `<h2>Section</h2>`, `<h3>Subsection</h3>` | Applies Heading1..3 / Title / Bold font roles |
| **Badges** | `<badge=primary>v1.14</badge>` | Inline rounded badge pill |
| **Pills** | `<pill color=#2ecc71>ARMED</pill>` | Interactive status pill with background color |
| **Keys** | `<kbd>Ctrl+C</kbd>` | 3D-styled keyboard keycap |
| **Textures** | `<img name="my_logo" width=24 height=24 />` | In-memory texture registered via `ImgWindow::RegisterMemoryImage` |
| **Vector Icons**| `<icon caret-up />`, `:gear:`, `▲` | Procedural vector icon from `imgui_icons.h` |

---

## 3. Direct Semantic Text Helpers (`imgui_font_roles.h`)

For standard Dear ImGui UI layouts where XML tags are not needed, direct wrapper functions render text immediately with semantic font roles:

```cpp
#include "contrib/extensions/Typography/imgui_font_roles.h"

// Formatted and unformatted small text (Slot 2, 12px):
ImGui::SmallText("Altitude: %.0f ft", altitude);
ImGui::SmallTextUnformatted("Captions and secondary subtext");

// Micro tiny text (Slot 2 fallback or dedicated 10px):
ImGui::TinyText("v1.0.2 [Beta]");
ImGui::TinyTextUnformatted("FAA Advisory Circular AC 90-66B");

// Title / header text (Slot 1, 16px):
ImGui::TitleText("Flight Plan: %s -> %s", dep, dest);

// Monospace text (Slot 5):
ImGui::MonoText("ICAO: %s | SQUAWK: %04d", callsign, squawk);
```

---

## 3. In-Memory Image & Icon Integration

The StyledText engine directly integrates with `ImgWindow`'s in-memory texture service and procedural vector icons.

### In-Memory Textures (`<img ... />`)
```cpp
// Static embedded PNG array:
static const unsigned char g_BeaconPng[] = { 0x89, 0x50, 0x4e, 0x47, ... };

// 1. Register texture:
int w = 0, h = 0;
ImgWindow::RegisterMemoryImage("avionics_beacon", g_BeaconPng, sizeof(g_BeaconPng), &w, &h);

// 2. Render inline with text:
ImGui::StyledText("Beacon: <img name='avionics_beacon' width=20 height=20 /> ONLINE");

// 3. Embedded inside an icon pill:
ImGui::StyledText("Transponder: <pill icon=avionics_beacon color=#2ecc71>ACTIVE</pill>");

// 4. Standalone unary annunciator:
ImGui::StyledText("Master Warning: <pill icon=avionics_beacon color=#e74c3c />");
```

### Procedural Vector Icons (`<icon ... />`)
Zero-font, zero-texture vector icons automatically align with the text baseline:

```cpp
// Explicit icon tag:
ImGui::StyledText("Navigation: <icon caret-up /> Climb to FL350");

// Emoji shortcodes:
ImGui::StyledText("Avionics: :gear: System nominal :check: Verified");

// Raw Unicode rescue:
ImGui::StyledText("Heading: ▲ 180° | Pitch: ▼ -2.5° | Revert: ↺");
```

---

## 4. Pre-Measurement (`CalcTextStyledSize`)

When calculating window bounds, stationary tooltip geometry, or layout columns, compute the exact size without rendering:

```cpp
ImVec2 size = ImGui::CalcTextStyledSize("Transponder: <pill color=green>ACTIVE</pill>", 300.0f);
// size.x = total rendered width in pixels
// size.y = total rendered height in pixels
```

---

## 5. Opt-Out Configuration

If your project defines conflicting identifiers, define the following before including `contrib.h` or `extensions.h`:

```cpp
#define CONTRIB_DISABLE_TYPOGRAPHY    // Disables Typography and StyledText extension
```
