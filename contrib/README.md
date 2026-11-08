# ImGui Contributions in ImgWindow (`contrib/` & `ImGee`)

Welcome to the `contrib/` directory of **ImgWindow**. This directory houses **ImGee**, ImgWindow's premier bundled suite of **Dear ImGui (`ImGui::`) extensions and immediate-mode avionics widgets** for X-Plane plugins.

---

## 🎯 Why ImGee? (From Debug Menus to Cockpit Instruments)

Pure Dear ImGui is fundamentally an engine designed for game-dev debug menus and developer panels. When flight sim plugin authors try to build user-facing products with it, they immediately hit friction:

* **Tooltips follow the mouse, flicker, and fall off the screen** when moving near the simulator window edges.
* **Help text in tooltips is limited to plain ASCII strings** without headers, bullet lists, bold emphasis, or documentation links.
* **Toggles are clunky checkboxes** rather than tactile, cockpit-grade switches with clear dual-state affordances.
* **Sliders and combos don't respond naturally to the mouse scroll-wheel**, making fine tuning feel clumsy compared to real avionics knobs.
* **There is no standard "revert to default" pattern**, so users fear tweaking settings lest they lose their calibrated values.

Look at the spectrum of UI tasks a flight sim plugin author faces, and how **ImGee** covers them today:

| Common Cockpit UI Task | Vanilla Dear ImGui Friction | ImGee Solution |
| :--- | :--- | :--- |
| **Interactive Help & Manuals** | `SetTooltip()` follows the mouse cursor, flickers, and clips outside simulator window boundaries. Limited to plain ASCII strings. | **`ImGui::TimedTooltip` & `ImGui::MD`**<br>Stationary anchor after initial delay, low-water wheel reset, smart cursor-avoidance and screen-edge clamping, and full Markdown rendering. |
| **Two-Position Switches** | Clunky `[x]` checkboxes with no visual state distinction or dual-label tooltips. | **`ImGui::ToggleButton`**<br>Animated physical rocker/toggle switches with font-relative scaling, custom palettes, and automatic dual-state tooltips (`"Off Hint", "On Hint"`). |
| **Avionics Dials & Adjusters** | Requires click-and-drag. The mouse wheel does nothing, requiring awkward custom tracking. | **`ImGui::MouseWheelSlider` / `MouseWheelDrag` / `MouseWheelCombo`**<br>Natural mouse-wheel stepping with configurable increments and fine-tune modifier keys. |
| **Fearless Settings Tweaking** | No standard "revert to default" pattern; users hesitate to adjust complex tuning parameters. | **`ImGui::AdjustItemOnMouseWheel` & Reset Chasers**<br>Inline 1-click restore-to-default icon chaser with instant undo history. |
| **One-Click Clipboard Copying** | Requires manual clipboard handling, lacks transient visual feedback, and twitches layout. | **`ImGui::CopyButton` & `CopyLastItemButton`**<br>Procedural vector icons (clipboard & checkmark), 1.5s feedback, anti-twitch sizing, and automatic chaser extraction from InputText. |
| **Interactive Data Tables** | Tables require dozens of manual `TableSetupColumn` / `TableHeadersRow` lines, manual sort callbacks, and custom sizing persistence. | **`ImGui::MD` CommonMark Tables**<br>Standard GFM table syntax with tristate column sorting, divider drag resizing, right-border width grip, A/B toggle memory, and 2-tier comment persistence (`<!-- table-preset: ... -->` / `<!-- table-state: ... -->`). |
| **Universal Vector Icons & Glyphs** | Missing glyph diamond boxes (``) when font atlases omit extended Unicode blocks (e.g. `↺`, `▲`, `▼`, `✓`, `⚙`). | **`ImGui::Icon` & Procedural Engine**<br>Zero-texture, zero-font procedural vector icon engine. Markdown `<icon ...>`, emoji shortcodes (`:gear:`), fallback link macros (`#[^](ICON_CARET_UP)`), and automatic raw Unicode glyph rescue. |
| **External Docs & Web Links** | Raw text URLs or awkward styled buttons with default arrow cursors. | **`ImGui::ButtonLink`, `TextLink`, `MenuItemLink`**<br>Native OS hand cursors (`ImGuiMouseCursor_Hand`), hover underlines, and one-click browser launch via `XPLMOpenURL`. |


---

## 🚀 Quick Start: Just `#include "contrib.h"` (or `#include "contrib/imgee.h"`)

Using `contrib/` requires zero setup or boilerplate. Simply include the master header:

```cpp
#include "contrib.h"
```

> [!IMPORTANT]
> **macOS Developers: Enable OS Clipboard (`IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS`)**  
> To enable one-click clipboard copying (`[Copy]` in code blocks) and text selection shortcuts (`Cmd+C`/`Cmd+V`) on macOS:
> 1. Add `#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS` to your `imconfig.h` (or pass it as a compiler flag).
> 2. Link `-framework ApplicationServices` in your project linker flags.  
> *(On Windows, clipboard works out of the box via `user32.lib`. On Linux, provide `MarkdownConfig::setClipboardTextFn` or set `ImGui::GetPlatformIO().Platform_SetClipboardTextFn`.)*

All widgets and extensions immediately feel like native Dear ImGui:

```cpp
// 1. Cockpit widgets live directly in ImGui:::
static bool s_HudActive = true;
ImGui::ToggleButton("HUD Active", &s_HudActive);

static float s_Brightness = 0.85f;
ImGui::MouseWheelSlider("Brightness", &s_Brightness, 0.0f, 1.0f, "%.0f%%");

// 2. Extensions mirror standard ImGui text & tooltip vocabulary:
ImGui::Markdown::Text("# Flight Deck\nTelemetry is **active**.");
ImGui::TimedTooltip::Text("Hover-delayed stationary tooltip.");
```

> [!TIP]
> **Forgiving Aliases ("It Just Works")**  
> Don't sweat memorizing rigid namespace rules. The library includes transparent aliases so whatever you intuitively reach for compiles cleanly:
> * Prefer brevity? **`ImGui::MD::Text(...)`** is an official shorthand alias for **`ImGui::Markdown::Text(...)`**.
> * Prefer root access? **`ImGui::TextMD(...)`** and **`ImGui::TextMarkdown(...)`** work directly under `ImGui::`.
> * Combining tooltips and markdown? **`ImGui::TimedTooltip::MD::Text`** and **`ImGui::MD::TimedTooltip::Text`** work identically.
> * Want to skip `ImGui::` completely? `#define CONTRIB_ENABLE_GLOBAL_NAMESPACES` before including `contrib.h` to use `Markdown::`, `MD::`, and `TimedTooltip::` directly in the global scope.

---

## Ecosystem Organization: Multi-Package Container Architecture

The `contrib/` directory serves as an extensible **community container** for Dear ImGui enhancements. Its flagship suite is **`ImGee`**, an avionics-grade toolkit designed specifically for flight-deck instruments and X-Plane plugin interfaces.

```
ImgWindow/
├── contrib.h                              # Master concentrator (-> contrib/includes.h)
└── contrib/
    ├── includes.h                         # Level-1 Container Switchboard (cascades to packages)
    │
    ├── ImGee/                             # Package 1: ImGee Avionics & UI Toolkit
    │   ├── imgee.h                        # Canonical ImGee master header & IMGEE_* switches
    │   ├── imgee-global.h                 # Opt-in global namespace aliases
    │   ├── docs/
    │   │   └── ImGee_User_Guide.md        # Comprehensive User Manual (Publication-ready)
    │   ├── extensions/                    # Subsystems (Typography, Markdown, TimedTooltip)
    │   │   ├── TimedTooltip/              # Stationary & delayed tooltip engine
    │   │   ├── Markdown/                  # GFM table parser, column sorter & text renderer
    │   │   └── Typography/                # Font roles (ImFontRole) & inline StyledText
    │   └── widgets/                       # Immediate-mode flight controls & vector icons
    │       ├── imgui_icons.h              # Zero-texture procedural vector icon engine
    │       ├── imgui_knobs.h              # Avionics rotary knobs, dimmer dials & detents
    │       ├── imgui_mouse_wheel_controls.h # Wheel sliders, combos & reset chasers
    │       ├── imgui_badges.h             # Status badges, pills, annunciators & <kbd>
    │       ├── imgui_popup_menus.h        # Directional popups (AboveItem) & pulldowns
    │       ├── imgui_toggle_buttons.h     # Tactile animated rocker/toggle switches
    │       ├── imgui_spinners.h           # Procedural radar sweeps & radial timers
    │       ├── imgui_clickable_links.h    # OS hand cursor links (XPLMOpenURL)
    │       ├── imgui_copy_button.h        # Vector copy buttons with 1.5s visual feedback
    │       └── imgui_file_dialog.h        # Non-blocking in-sim file dialogs
    │
    ├── extensions/                        # Backward-Compatible Forwarding Headers
    └── widgets/                           # Backward-Compatible Forwarding Headers
```

### 1. ImGee Suite (`contrib/ImGee/`)
The complete avionics and flight-sim UI design system. See &rarr; **[ImGee User Guide](ImGee/docs/ImGee_User_Guide.md)** and **[contrib/ImGee/README.md](ImGee/README.md)**.

### 2. Zero-Breakage Forwarding Shims (`contrib/extensions/`, `contrib/widgets/`)
To guarantee 100% backward compatibility for existing code and downstream CI pipelines, thin forwarding headers remain in `contrib/extensions/` and `contrib/widgets/`, routing cleanly to their canonical locations in `contrib/ImGee/`.

---

## ⚙️ Configuration Reference: Opt-Out Flags

The library provides a hierarchical two-tier configuration matrix:

### Tier 1: Master Container & Package Toggles
* **`CONTRIB_DISABLE_ALL`** (or `DISABLE_CONTRIB`, `IMGWINDOW_DISABLE_CONTRIB`): Completely shuts off the entire `contrib/` ecosystem. Zero contrib code compiled.
* **`IMGEE_DISABLE_ALL`** (or `CONTRIB_IMGEE_DISABLE_ALL`): Disables the entire ImGee suite while leaving the container active for future packages.

### Tier 2: ImGee Feature & Subsystem Toggles
Uncomment or pass any of these macros to selectively exclude individual ImGee subsystems or widgets:

| Canonical Macro Directive | Legacy Alias | Description |
| :--- | :--- | :--- |
| **`IMGEE_DISABLE_EXTENSIONS`** | `CONTRIB_DISABLE_EXTENSIONS` | Excludes all extensions (`Typography`, `Markdown`, `TimedTooltip`). |
| **`IMGEE_DISABLE_WIDGETS`** | `CONTRIB_DISABLE_WIDGETS` | Excludes all contributed widgets in `widgets/`. |
| **`IMGEE_DISABLE_ICONS`** | `CONTRIB_DISABLE_ICONS` | Excludes procedural vector icon engine and `ImGui::Icon`. |
| **`IMGEE_DISABLE_ICON_BUTTON`** | `CONTRIB_DISABLE_ICON_BUTTON` | Excludes `IconButton` and `ButtonIcon` push buttons. |
| **`IMGEE_DISABLE_TYPOGRAPHY`** | `CONTRIB_DISABLE_TYPOGRAPHY` | Excludes `ImFontRole` and `ImGui::StyledText`. |
| **`IMGEE_DISABLE_MARKDOWN`** | `CONTRIB_DISABLE_MARKDOWN` | Excludes Markdown parser, GFM tables, and callouts. |
| **`IMGEE_DISABLE_TIMED_TOOLTIP`** | `CONTRIB_DISABLE_TIMED_TOOLTIP` | Excludes stationary delayed tooltip engine. |
| **`IMGEE_DISABLE_KNOBS`** | `CONTRIB_DISABLE_KNOBS` | Excludes avionics rotary knobs, dials, and detents. |
| **`IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS`** | `CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS` | Excludes mouse-wheel sliders, drags, combos, and reset buttons. |
| **`IMGEE_DISABLE_BADGES`** | `CONTRIB_DISABLE_BADGES` | Excludes status badges, pills, annunciators, and keycaps. |
| **`IMGEE_DISABLE_POPUP_MENUS`** | `CONTRIB_DISABLE_POPUP_MENUS` | Excludes directional anchored popups (`AboveItem`) and pulldowns. |
| **`IMGEE_DISABLE_TOGGLE_BUTTONS`** | `CONTRIB_DISABLE_TOGGLE_BUTTONS` | Excludes animated physical toggle switches. |
| **`IMGEE_DISABLE_CLICKABLE_LINKS`** | `CONTRIB_DISABLE_CLICKABLE_LINKS` | Excludes OS hand cursor links (`ButtonLink`, `TextLink`). |
| **`IMGEE_DISABLE_SPINNERS`** | `CONTRIB_DISABLE_SPINNERS` | Excludes activity spinners and countdown timer dials. |
| **`IMGEE_ENABLE_GLOBAL_NAMESPACES`** | `CONTRIB_ENABLE_GLOBAL_NAMESPACES` | Publishes `Markdown::`, `MD::`, and `TimedTooltip::` to global scope. |

*(Note: Legacy `CONTRIB_DISABLE_*` flags are automatically mapped via an internal alias bridge in `contrib/ImGee/imgee.h` with zero build warnings.)*

---

## How to Contribute

We welcome community contributions! Whether porting an existing open-source widget or introducing a new UI component:

1. **Choose the Category:** Determine if your code is a standalone **Widget** (a single control in `widgets/`) or a full **Extension** (a package in `extensions/`).
2. **Namespace Appropriately:** Follow the namespacing rules above.
3. **Keep Dependencies Minimal:** Header-only or clean single-file implementations are strongly preferred.
4. **Document Your Work:** Include a dedicated `README.md` in your extension folder detailing usage, API signatures, and code examples.
5. **Open a PR:** Submit a Pull Request against the repository with a description and sample screenshot or code snippet.
