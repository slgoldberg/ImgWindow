# ImGee: Avionics & Flight-Sim UI Toolkit for Dear ImGui
## Comprehensive Developer & User Manual (v2.4.0)

---

## 1. Executive Architecture & Design Philosophy

`Dear ImGui` is an immediate-mode graphical user interface library built fundamentally for game-engine debugging, tool development, and developer panels. When flight simulator plugin developers attempt to build production-grade avionics interfaces, settings windows, or interactive kneeboards with stock Dear ImGui, they immediately encounter several fundamental architectural gaps:

1. **Missing Glyphs and Font Atlas Friction:** Standard font atlases omit extended Unicode mathematical and symbol blocks (such as `↺`, `▲`, `▼`, `✓`, `⚙`, or `✈`), resulting in missing-glyph diamond boxes (`?` or placeholder squares) unless heavy custom icon fonts are baked into VRAM.
2. **Volatile Tooltips:** Vanilla `ImGui::SetTooltip()` follows the mouse cursor frame-by-frame, continuously flickering, resetting scroll state, and clipping off-screen when hovering near flight simulator window borders.
3. **Clunky Input Affordances:** Checkboxes look like desktop form controls rather than physical cockpit switches; sliders do not respond naturally to the mouse scroll wheel; and there is no universal "revert to calibrated default" pattern for complex tuning parameters.
4. **Rigid Text Rendering:** Stock text rendering lacks inline semantic sizing, baseline alignment, and formatted Markdown tables with column resizing and persistent sorting.

**`ImGee`** is an immediate-mode avionics and flight-simulation UI toolkit designed to bridge these gaps. Bundled as the premier package within the `ImgWindow` `contrib/` ecosystem, ImGee turns Dear ImGui into a cockpit-grade design system with zero mandatory dependencies, zero runtime allocations in hot loops, and complete backward compatibility.

```
ImgWindow/
├── contrib.h                              # Master Concentrator (-> contrib/includes.h)
└── contrib/
    ├── includes.h                         # Level-1 Container Package Switchboard
    │
    ├── ImGee/                             # The ImGee Flight-Deck UI Suite
    │   ├── imgee.h                        # Canonical ImGee Master Header & Opt-Out Toggles
    │   ├── imgee-global.h                 # Opt-in Global Scope Aliases
    │   ├── docs/
    │   │   └── ImGee_User_Guide.md        # Comprehensive User Manual (this document)
    │   ├── extensions/                    # Subsystems (Typography, Markdown, TimedTooltip)
    │   └── widgets/                       # Immediate-mode flight controls & vector icons
    │
    ├── extensions/                        # Zero-breakage Forwarding Headers
    └── widgets/                           # Zero-breakage Forwarding Headers
```

---

## 2. Integration & Inclusion Models

### 2.1 The Front Door: `contrib.h`
For standard plugins, integration requires a single line:

```cpp
#include "contrib.h"
```

This single include opens the master package switchboard (`contrib/includes.h`), which automatically pulls in `contrib/ImGee/imgee.h`. All widgets and extensions are registered under the standard `ImGui::` namespace.

### 2.2 Direct Suite Inclusion: `contrib/ImGee/imgee.h`
If your project manages multiple vendor packages or prefers strict package isolation, you can include ImGee directly:

```cpp
#include "contrib/ImGee/imgee.h"
```

### 2.3 Container-Level vs. Package-Level Opt-Out Switches
ImGee provides a hierarchical two-tier preprocessor configuration matrix:

| Preprocessor Define | Scope | Description |
| :--- | :--- | :--- |
| `CONTRIB_DISABLE_ALL` | Container | Completely disables the entire `contrib/` folder. Zero contrib code compiled. |
| `IMGEE_DISABLE_ALL` (or `CONTRIB_IMGEE_DISABLE_ALL`) | Package | Excludes the entire ImGee suite while allowing other contrib packages. |
| `IMGEE_DISABLE_EXTENSIONS` | Subsystem | Excludes Typography, Markdown, and TimedTooltip. |
| `IMGEE_DISABLE_WIDGETS` | Subsystem | Excludes all rotary knobs, sliders, badges, toggles, and popups. |
| `IMGEE_DISABLE_ICONS` | Feature | Excludes the procedural vector icon rendering engine. |
| `IMGEE_DISABLE_KNOBS` | Feature | Excludes avionics rotary knobs and dimmer dials. |
| `IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS` | Feature | Excludes wheel sliders, drags, combos, and reset chasers. |
| `IMGEE_DISABLE_BADGES` | Feature | Excludes badges, status pills, annunciators, and keycaps. |
| `IMGEE_DISABLE_POPUP_MENUS` | Feature | Excludes directional anchored popups and pulldowns. |
| `IMGEE_ENABLE_GLOBAL_NAMESPACES` | Namespaces | Exposes `Markdown::`, `MD::`, and `TimedTooltip::` to global scope. |

> [!NOTE]
> **Legacy Alias Bridge:** For projects written against earlier versions of ImgWindow, legacy switches like `CONTRIB_DISABLE_WIDGETS`, `CONTRIB_DISABLE_BADGES`, and `CONTRIB_DISABLE_MARKDOWN` are automatically mapped to their canonical `IMGEE_*` counterparts with zero build warnings.

---

## 3. Procedural Vector Icon Engine & Glyphs

Standard Dear ImGui plugins often suffer from missing glyphs when attempting to display symbols like reset arrows (`↺`), sort carets (`▲`, `▼`), checkmarks (`✓`), gears (`⚙`), or airplanes (`✈`). 

ImGee includes a **zero-texture, zero-font procedural vector engine** (`imgui_icons.h`). It submits antialiased geometric primitives (lines, arcs, bezier curves, triangles) directly to Dear ImGui's active `ImDrawList`.

```
                    ┌─────────────────────────┐
                    │ Raw String or Unicode   │
                    │ ("undo", ":gear:", "↺") │
                    └────────────┬────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │ NormalizeIconName()     │
                    │ (Canonical Identifier)  │
                    └────────────┬────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         │                       │                       │
┌────────▼────────┐     ┌────────▼────────┐     ┌────────▼────────┐
│ Custom Texture  │     │ Icon Font Glyph │     │ Vector Engine   │
│ (Memory Image)  │     │ (FontAtlas)     │     │ (100% Batched   │
│                 │     │                 │     │  ImDrawList)    │
└─────────────────┘     └─────────────────┘     └─────────────────┘
```

### 3.1 Icon Catalog & Normalization
The engine understands dozens of canonical aliases, FontAwesome names, emoji shortcodes, and raw UTF-8 glyphs:

* **Reset / Undo:** `"undo"`, `":undo:"`, `"ICON_FA_UNDO"`, `"reset"`, `"\xE2\x86\xBA"` (`↺`)
* **Redo / Restore:** `"redo"`, `":redo:"`, `"\xE2\x86\xBB"` (`↻`)
* **Carets & Navigation:** `"caret-up"` (`▲`), `"caret-down"` (`▼`), `"caret-left"` (`◀`), `"caret-right"` (`▶`)
* **Actions:** `"check"` (`✓`, `✔`), `"cross"` (`✗`, `✘`), `"plus"` (`+`), `"minus"` (`-`)
* **Cockpit / Utility:** `"gear"` / `"settings"` (`⚙`), `"plane"` / `"aircraft"` (`✈`), `"info"` (`ℹ`), `"warning"` (`⚠`), `"star"` (`★`), `"search"` (`🔍`)

### 3.2 Submitting Inline Vector Icons
Use `ImGui::Icon()` to draw an icon inline with current text:

```cpp
// Auto-sizes to 90% of active font size, aligned to text baseline
ImGui::Text("Calibrated Heading:");
ImGui::SameLine();
ImGui::Icon("plane");

// Explicit size (24x24 px) with custom status color
ImVec4 warnCol(1.0f, 0.75f, 0.1f, 1.0f);
ImGui::Icon("warning", 24.0f, &warnCol);
```

### 3.3 Vector Push-Buttons: `IconButton` & `ButtonIcon`
ImGee provides turnkey button helpers that render procedural vector icons without font dependencies:

```cpp
// 1. Square Icon-Only Button with native hand cursor and tooltip
if (ImGui::IconButton("##ResetHdg", "undo", ImVec2(0, 0), "Reset Heading to 360°")) {
    s_Heading = 360.0f;
}

// 2. Icon + Text Button (Icon on Left)
if (ImGui::ButtonIcon("Settings", "gear")) {
    OpenSettingsDialog();
}

// 3. Text + Icon Button (Icon on Right)
if (ImGui::ButtonIcon("Next Waypoint", "caret-right", ImVec2(160, 0), /*icon_on_right=*/true)) {
    AdvanceWaypoint();
}
```

---

## 4. Semantic Typography & Inline Sized Text

Cockpit user interfaces require strict typographic hierarchy: primary instruments need large, legible digits; captions and annunciator legends require micro-typography; and datarefs require clear monospaced alignment.

ImGee provides two complimentary typography systems:
1. **Semantic Font Roles (`ImFontRole`):** An 8-slot registry linking logical design roles to loaded rasterized fonts with automatic hierarchical fallbacks.
2. **Inline Styled Text Engine (`ImGui::StyledText`):** Lightweight XML-style tag parsing with baseline alignment and zero Markdown document overhead.

### 4.1 The `ImFontRole` Registry

```cpp
enum class ImFontRole {
    Regular = 0,        // Slot 0: Primary interface font (e.g. 14px)
    Title,              // Slot 1: Section headers, dialog titles (e.g. 16px)
    Small,              // Slot 2: Captions, annotations, secondary subtext (e.g. 12px)
    Bold,               // Slot 3: Strong emphasis, table headers (e.g. Bold 14px)
    BoldLarge,          // Slot 4: Hero banners, major readouts (e.g. Bold 16px)
    Monospace,          // Slot 5: Datarefs, callsigns, frequencies (e.g. Mono 14px)
    Italic,             // Slot 6: Citations, hints, metadata (e.g. Italic 14px)
    MonospaceMedium,    // Slot 7: Numeric avionics readouts (e.g. Mono Medium 15px)
    Tiny                // Micro subtext, annunciator legends (e.g. 10px / fallback to Small)
};
```

#### Direct C++ Typography Helpers
Instead of manually pushing and popping fonts from ImGui's font stack, use convenient direct helpers:

```cpp
ImGui::TitleText("Primary Flight Display Configuration");
ImGui::SmallText("Sub-channel frequency: %0.2f MHz", s_Freq);
ImGui::TinyText("ANNUNCIATOR BUS VOLTAGE OK");
ImGui::MonoText("sim/cockpit2/radios/actuators/nav1_frequency_hz");
```

### 4.2 Inline Sized Text Tags (`ImGui::StyledText`)

`ImGui::StyledText` allows embedding sizing, bolding, colors, and badges directly inside text strings:

```cpp
ImGui::StyledText("Altimeter: <huge color=#38BDF8>29.92</huge> <small>IN HG</small>");
ImGui::StyledText("Engine 1: <badge-success>NORMAL</badge-success> | EGT: <mono>645°C</mono>");
```

#### Supported Inline Tags:
* **Sizing Tags:** `<tiny>`, `<small>`, `<regular>`, `<big>`, `<large>`, `<huge>`
* **Style Tags:** `<b>` / `<strong>`, `<i>` / `<em>`, `<mono>` / `<code>`, `<h1>`, `<h2>`, `<h3>`
* **Color Tag:** `<color=#RRGGBB>Text</color>` or `<color=#RGB>`
* **Embedded Badges & Pills:** `<badge>Text</badge>`, `<badge-danger>FAIL</badge-danger>`, `<pill>VFR</pill>`
* **Keycaps:** `<kbd>Ctrl+Alt+P</kbd>`
* **Inline Vector Icons:** `<icon name=gear size=14 color=#94A3B8 />`
* **Named Memory Images:** `<image name=logo width=32 height=32 />`

> [!TIP]
> **Sub-pixel Baseline Alignment:** When mixing `<huge>` numbers with `<small>` units on the same line, ImGee automatically calculates font ascent offsets, keeping the bottoms of all glyphs locked to a common visual baseline.

---

## 5. Flight-Deck Avionics Widgets

### 5.1 Avionics Rotary Knobs & Dimmer Dials (`imgui_knobs.h`)
Standard sliders feel unnatural for cockpit instruments. `ImGui::Knob` implements an avionics-grade potentiometer:

```
          . - ~ ~ - .
      . '    .---.    ' .
    /       /     \       \
   :       |   *   |       :   <- 270° "Omega" Arc
    \       \     /       /       with Open Deadband at 6 o'clock
      . '    '---'    ' .
          ' -  *  - '
             OFF
```

#### Features:
1. **270-Degree "Omega" Arc:** An open deadband at 6 o'clock immediately communicates an interactive dial.
2. **Tactile Two-Step Detente Switch:** Counter-clockwise rotation dims smoothly down to minimum operating value (e.g., 5%). A deliberate extra mouse-wheel click or drag snaps across the detente gap into the powered-down `"OFF"` state. Rotating clockwise from `"OFF"` snaps instantly back to minimum active power.
3. **Scroll-Wheel Primary:** Hover and roll the wheel with fine-tuning modifiers (`Shift` for 10x, `Alt`/`Ctrl` for 0.1x).

```cpp
static float s_InstrumentLight = 0.75f;

// 1. Standard Knob with perimeter ticks
ImGui::Knob("Panel Light", &s_InstrumentLight, 0.0f, 1.0f, 
            ImGuiKnobFlags_ShowTicks | ImGuiKnobFlags_ValueBelow);

// 2. Composite Knob with Reset Chaser
static const float s_DefaultLight = 0.80f;
ImGui::KnobWithReset("Backlight", &s_InstrumentLight, 0.0f, 1.0f, 
                     s_DefaultLight, "%.0f%%");
```

---

### 5.2 Mouse-Wheel Sliders & Reset Chasers (`imgui_mouse_wheel_controls.h`)

Settings tweaking in flight simulations is fraught with user anxiety: if an author tweaks an aircraft trim or graphic setting, they fear losing their calibrated value.

ImGee solves this with **Inline 1-Click Restore-to-Default Chasers**:

```cpp
static float s_Gamma = 2.2f;
static const float s_DefaultGamma = 2.2f;

// Renders slider with natural mouse-wheel stepping and a ↺ button that appears
// whenever s_Gamma differs from s_DefaultGamma. Clicking restores default instantly.
ImGui::SliderWithReset("Display Gamma", &s_Gamma, 1.0f, 3.0f, s_DefaultGamma, "%.2f");
```

#### Universal Mouse-Wheel Adjuster: `AdjustItemOnMouseWheel`
Want to add scroll-wheel support and a reset chaser to any stock Dear ImGui widget? Use `AdjustItemOnMouseWheel`:

```cpp
static float s_Volume = 0.70f;
ImGui::SliderFloat("Master Volume", &s_Volume, 0.0f, 1.0f);
ImGui::AdjustItemOnMouseWheel(&s_Volume, 0.01f, 0.0f, 1.0f, 0.70f);
```

#### Viewport Scroll Defeat
In stock Dear ImGui, rolling the mouse wheel over a slider inside a scrollable window causes the window to scroll awkwardly instead of adjusting the setting. ImGee features automatic **scroll defeat**: when hovering an interactive wheel control, parent window scrolling is cleanly inhibited.

---

### 5.3 Badges, Pills & Annunciators (`imgui_badges.h`)

ImGee provides semantic status indicators with automatic high-contrast text calculation:

```cpp
// 1. Semantic Badges
ImGui::Badge("ONLINE", ImGuiBadgeVariant_Success);
ImGui::SameLine();
ImGui::Badge("CAUTION", ImGuiBadgeVariant_Warning);
ImGui::SameLine();
ImGui::Badge("FAULT", ImGuiBadgeVariant_Danger);

// 2. Rounded Flight Status Pills
ImGui::Pill("AUTOPILOT: NAV", IM_COL32(37, 99, 235, 255));

// 3. Unary Flight-Deck Annunciator Pill (Flash / Invert on Master Warning)
ImGui::AnnunciatorPill("FIRE ENG 1", /*active=*/s_FireWarning, /*caution=*/false);

// 4. Keyboard Shortcut Caps
ImGui::Text("Toggle HUD:");
ImGui::SameLine();
ImGui::Kbd("Ctrl+Shift+H");
```

---

### 5.4 Directional Anchored Popups & Pulldowns (`imgui_popup_menus.h`)

Standard Dear ImGui popups appear at the mouse cursor position. For cockpit panels and status bars docked at the bottom of the screen, menus need to blossom **upward** relative to their launcher button.

```
       ┌────────────────────────┐
       │   Anchored Popup Menu  │
       │   - 118.10 MHz (TWR)   │
       │   - 121.90 MHz (GND)   │
       └───────────┬────────────┘
                   │
           ▲ AboveItem Anchor
           │
  ┌────────┴────────┐
  │ [Active Com 1]  │  <- Bottom Docked Status Bar
  └─────────────────┘
```

```cpp
if (ImGui::Button("COM 1 Frequency")) {
    ImGui::OpenPopupAtItem("ComPopup", ImGuiPopupAnchor_AboveItem);
}

if (ImGui::BeginAnchoredPopup("ComPopup", ImGuiPopupAnchor_AboveItem)) {
    if (ImGui::Selectable("118.10 MHz (Tower)")) SetCom1(118100);
    if (ImGui::Selectable("121.90 MHz (Ground)")) SetCom1(121900);
    ImGui::EndAnchoredPopup();
}
```

#### Muscle-Memory Toggle Safety
Clicking the opener button while an anchored popup is already open cleanly toggles it closed, eliminating the annoying double-open flicker present in vanilla ImGui menus.

---

### 5.5 Physical Animated Toggle Switches (`imgui_toggle_buttons.h`)

Replace generic checkboxes with tactile two-position switches:

```cpp
static bool s_PitotHeat = true;
// Renders an animated rocker switch with dual-state hover tooltips
ImGui::ToggleButton("Pitot Heat", &s_PitotHeat, "Heaters OFF (Risk of Icing)", "Heaters ON");
```

---

## 6. Rich Markdown Engine & GFM Tables

The ImGee Markdown engine (`imgui_markdown_ext.h`) transforms plain text strings into fully interactive documentation:

```cpp
const char* doc = R"(
# Flight Operations Manual
The **Autopilot Flight Director System** (AFDS) manages aircraft trajectory.

### Status Annunciations
| Mode | Status | Armed | Minimum Speed |
| :--- | :---: | :---: | :--- |
| **LNAV** | <badge-success>ENGAGED</badge-success> | Yes | `V2 + 15` |
| **VNAV** | <badge-warning>ARMED</badge-warning> | Yes | `250 KTS` |
| **ILS** | <badge>STANDBY</badge> | No | `VREF + 5` |

> [!NOTE]
> Ensure barometric altimeters are set to standard pressure (29.92 IN HG / 1013 HPA) when passing transition altitude.
)";

ImGui::Markdown::Text(doc);
```

### 6.1 GFM Interactive Table Features
* **Tristate Column Sorting:** Click any column header to cycle Ascending $\rightarrow$ Descending $\rightarrow$ Default sort.
* **Interactive Drag Sizing:** Adjust column dividers directly.
* **Persistent Presets:** Save and restore column widths via inline comment tags (`<!-- table-state: ... -->`).
* **Code Block Copy Buttons:** Code blocks automatically include one-click clipboard copying with 1.5s visual feedback.

---

## 7. Stationary Anchored Tooltips (`imgui_tooltip_ext.h`)

Vanilla tooltips follow the cursor like a bee, creating visual distraction and clipping out-of-sim. ImGee provides stationary anchored tooltips:

```cpp
// 1. Plain text stationary tooltip
ImGui::Button("Flaps 15");
ImGui::TimedTooltip::Text("Takeoff flap detent. Generates additional lift with moderate drag.");

// 2. Full Markdown stationary tooltip
ImGui::Button("GPWS");
ImGui::TimedTooltip::TextMD(
    "### Ground Proximity Warning System\n"
    "* **Terrain Ahead:** Pull up immediately.\n"
    "* **Glideslope:** Deviation exceeds 1.3 dots."
);
```

### Key Tooltip Affordances:
* **Initial Delay:** Prevents accidental popup spam while sweeping the mouse across controls.
* **Stationary Anchor:** Freezes in position once visible, allowing effortless reading.
* **Smart Screen Clamping:** Automatically nudges inside the X-Plane main monitor boundaries.
* **Wheel-Defeat Dismissal:** Rolling the mouse wheel immediately closes the tooltip so it never blocks scrolling views.

---

## 8. Summary of Preprocessor Configuration Flags

Add any of these defines to your project or `imconfig.h` to tailor ImGee to your specific binary footprint requirements:

```cpp
// --- Master Opt-Out ---
#define CONTRIB_DISABLE_ALL                   // Exclude entire contrib container
#define IMGEE_DISABLE_ALL                     // Exclude entire ImGee suite

// --- Subsystem Opt-Outs ---
#define IMGEE_DISABLE_EXTENSIONS              // Exclude Typography, Markdown, Tooltips
#define IMGEE_DISABLE_WIDGETS                 // Exclude all cockpit widgets

// --- Granular Feature Opt-Outs ---
#define IMGEE_DISABLE_ICONS                   // Exclude procedural vector icon engine
#define IMGEE_DISABLE_ICON_BUTTON             // Exclude IconButton / ButtonIcon
#define IMGEE_DISABLE_TYPOGRAPHY              // Exclude Font Roles and StyledText
#define IMGEE_DISABLE_MARKDOWN                // Exclude Markdown parser and tables
#define IMGEE_DISABLE_TIMED_TOOLTIP           // Exclude TimedTooltip engine
#define IMGEE_DISABLE_KNOBS                   // Exclude avionics rotary knobs
#define IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS    // Exclude wheel sliders and combo controls
#define IMGEE_DISABLE_BADGES                  // Exclude badges, pills, and keycaps
#define IMGEE_DISABLE_POPUP_MENUS             // Exclude directional anchored popups
#define IMGEE_DISABLE_TOGGLE_BUTTONS          // Exclude animated toggle switches
#define IMGEE_DISABLE_CLICKABLE_LINKS         // Exclude OS link buttons and hand cursors
#define IMGEE_DISABLE_SPINNERS                // Exclude radar spinners and countdowns

// --- Opt-In Features ---
#define IMGEE_ENABLE_GLOBAL_NAMESPACES        // Publish Markdown:: and TimedTooltip:: to global scope
```

---
*Document Version: 2.4.0 • Author: Steven L. Goldberg (@slgoldberg) • ImgWindow Contrib Suite*
