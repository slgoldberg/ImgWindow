# ImGui Contributions in ImgWindow (`contrib/`)

Welcome to the `contrib/` directory of **ImgWindow**. This directory houses ImgWindow's bundled library of community-contributed **Dear ImGui (`ImGui::`) extensions and immediate-mode widgets** for X-Plane plugins.

---

## 🎯 Why `contrib/`? (From Debug Menus to Cockpit Instruments)

Pure Dear ImGui is fundamentally an engine designed for game-dev debug menus and developer panels. When flight sim plugin authors try to build user-facing products with it, they immediately hit friction:

* **Tooltips follow the mouse, flicker, and fall off the screen** when moving near the simulator window edges.
* **Help text in tooltips is limited to plain ASCII strings** without headers, bullet lists, bold emphasis, or documentation links.
* **Toggles are clunky checkboxes** rather than tactile, cockpit-grade switches with clear dual-state affordances.
* **Sliders and combos don't respond naturally to the mouse scroll-wheel**, making fine tuning feel clumsy compared to real avionics knobs.
* **There is no standard "revert to default" pattern**, so users fear tweaking settings lest they lose their calibrated values.

Look at the spectrum of UI tasks a flight sim plugin author faces, and how `contrib/` covers them today:

| Common Cockpit UI Task | Vanilla Dear ImGui Friction | `contrib/` Solution |
| :--- | :--- | :--- |
| **Interactive Help & Manuals** | `SetTooltip()` follows the mouse cursor, flickers, and clips outside simulator window boundaries. Limited to plain ASCII strings. | **`ImGui::TimedTooltip` & `ImGui::MD`**<br>Stationary anchor after initial delay, low-water wheel reset, smart cursor-avoidance and screen-edge clamping, and full Markdown rendering. |
| **Two-Position Switches** | Clunky `[x]` checkboxes with no visual state distinction or dual-label tooltips. | **`ImGui::ToggleButton`**<br>Animated physical rocker/toggle switches with font-relative scaling, custom palettes, and automatic dual-state tooltips (`"Off Hint", "On Hint"`). |
| **Avionics Dials & Adjusters** | Requires click-and-drag. The mouse wheel does nothing, requiring awkward custom tracking. | **`ImGui::MouseWheelSlider` / `MouseWheelDrag` / `MouseWheelCombo`**<br>Natural mouse-wheel stepping with configurable increments and fine-tune modifier keys. |
| **Fearless Settings Tweaking** | No standard "revert to default" pattern; users hesitate to adjust complex tuning parameters. | **`ImGui::AdjustItemOnMouseWheel` & Reset Chasers**<br>Inline 1-click restore-to-default icon chaser with instant undo history. |
| **External Docs & Web Links** | Raw text URLs or awkward styled buttons with default arrow cursors. | **`ImGui::ButtonLink`, `TextLink`, `MenuItemLink`**<br>Native OS hand cursors (`ImGuiMouseCursor_Hand`), hover underlines, and one-click browser launch via `XPLMOpenURL`. |


---

## 🚀 Quick Start: Just `#include "contrib.h"`

Using `contrib/` requires zero setup or boilerplate. Simply include the master header:

```cpp
#include "contrib.h"
```

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

## Ecosystem Organization

To keep contributions modular and easy to navigate, the library is organized into two primary categories:

```
ImgWindow/
├── contrib.h                              # Master concentrator (-> contrib/includes.h)
└── contrib/
    ├── includes.h                         # Master roll-up for all extensions & widgets
    ├── contrib-global.h                   # Opt-in global namespace aliases
    ├── extensions.h                       # Concentrator for all extensions (-> extensions/includes.h)
    ├── widgets.h                          # Concentrator for all widgets    (-> widgets/includes.h)
    │
    ├── extensions/                        # Feature-complete ImGui::<Package>:: subsystems
    │   ├── TimedTooltip/                  # Stationary & delayed tooltip engine with auto-bounds clamping
    │   └── Markdown/                      # Markdown parser & renderer with dynamic font scaling & tags
    │
    └── widgets/                           # Lightweight, immediate-mode ImGui:: UI controls
        ├── imgui_toggle_buttons.h         # Animated toggle switches & segmented button pairs
        ├── imgui_mouse_wheel_controls.h   # Sliders, drags, combos & chasers with mouse-wheel support
        ├── imgui_clickable_links.h        # ButtonLink, TextLink, MenuItemLink & AttachItemCursor affordances
        ├── imgui_badges.h                 # Standalone C++ status badges, pills, and 3D keycaps (<kbd>)
        └── imgui_spinners.h               # Procedural activity spinners & radial countdown dials
```

### 1. Packages & Extensions (`contrib/extensions/`)
Feature-rich additions that manage internal state or background configuration (`ImGui::Markdown::`, `ImGui::TimedTooltip::`). See &rarr; [contrib/extensions/README.md](extensions/README.md).

### 2. Reusable Widgets (`contrib/widgets/`)
Lightweight, immediate-mode UI controls that live directly in the root `ImGui::` namespace (`ImGui::ToggleButton`, `ImGui::MouseWheelSlider`, `ImGui::ButtonLink`). See &rarr; [contrib/widgets/README.md](widgets/README.md).

---

## ⚙️ Configuration Reference: Opt-Out Flags

Everything in `contrib/` is **active and ready by default**. If your project happens to define a conflicting identifier, define any of the following macros *before* including `contrib.h` to selectively disable individual features:

| Macro Directive | Type | Default | Description |
| :--- | :---: | :---: | :--- |
| **`CONTRIB_ENABLE_GLOBAL_NAMESPACES`** | **Opt-In** | *Off* | Publishes `Markdown::`, `MD::`, and `TimedTooltip::` to global namespace. |
| **`CONTRIB_DISABLE_EXTENSIONS`** | **Opt-Out** | *Off* | Excludes all extensions (`contrib/extensions/`). |
| **`CONTRIB_DISABLE_WIDGETS`** | **Opt-Out** | *Off* | Excludes all contributed widgets (`contrib/widgets/`). |
| **`CONTRIB_DISABLE_TIMED_TOOLTIP`** | **Opt-Out** | *Off* | Excludes the `TimedTooltip` extension. |
| **`CONTRIB_DISABLE_MARKDOWN`** | **Opt-Out** | *Off* | Excludes the `Markdown` extension. |
| **`CONTRIB_DISABLE_TEXT_MD`** | **Opt-Out** | *Off* | Excludes root `ImGui::TextMD` / `TextWrappedMD` aliases. |
| **`CONTRIB_DISABLE_TOGGLE_BUTTONS`** | **Opt-Out** | *Off* | Excludes `ToggleButton` and `ToggleButtons`. |
| **`CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL`**| **Opt-Out** | *Off* | Excludes mouse-wheel toggling on `ToggleButton`. |
| **`CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS`**| **Opt-Out** | *Off* | Excludes mouse-wheel sliders, drags, cyclers, and combos. |
| **`CONTRIB_DISABLE_CLICKABLE_LINKS`** | **Opt-Out** | *Off* | Excludes `ButtonLink`, `TextLink`, `MenuItemLink`, and `AttachItemCursor`. |
| **`CONTRIB_DISABLE_BADGES`**          | **Opt-Out** | *Off* | Excludes standalone `Badge`, `Pill`, and `Kbd` widgets. |
| **`CONTRIB_DISABLE_SPINNERS`**        | **Opt-Out** | *Off* | Excludes `Spinner`, `RadialTimer`, and `CountdownTimer`. |
| **`CONTRIB_DISABLE_PERCENT_SLIDER`** | **Opt-Out** | *Off* | Excludes `PercentSlider` and `SliderPercent`. |
| **`CONTRIB_DISABLE_MOUSE_WHEEL_COMBO`** | **Opt-Out** | *Off* | Excludes `MouseWheelCombo` and `ComboWithReset`. |
| **`IMGUI_CONTRIB_DISABLE_RESET_UNDO`** | **Opt-Out** | *Off* | Disables 1-click Reset-with-Undo on reset buttons. |

*(Note: Prefixes `IMGUI_CONTRIB_DISABLE_*`, `IMGUI_DISABLE_CONTRIB_*`, and legacy `IMGUI_DISABLE_EXTRA_*` are also honored automatically.)*

---

## How to Contribute

We welcome community contributions! Whether porting an existing open-source widget or introducing a new UI component:

1. **Choose the Category:** Determine if your code is a standalone **Widget** (a single control in `widgets/`) or a full **Extension** (a package in `extensions/`).
2. **Namespace Appropriately:** Follow the namespacing rules above.
3. **Keep Dependencies Minimal:** Header-only or clean single-file implementations are strongly preferred.
4. **Document Your Work:** Include a dedicated `README.md` in your extension folder detailing usage, API signatures, and code examples.
5. **Open a PR:** Submit a Pull Request against the repository with a description and sample screenshot or code snippet.
