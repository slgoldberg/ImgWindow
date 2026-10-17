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

## Ecosystem Organization

To keep contributions modular and easy to include at any level of granularity, the ecosystem is organized into two primary categories with uniform **`includes.h`** roll-up headers, top-level concentrators, and opt-in global convenience:

```
ImgWindow/
├── contrib.h                              # Top-level concentrator (-> contrib/includes.h)
└── contrib/
    ├── includes.h                         # Master roll-up for all extensions & widgets
    ├── contrib-global.h                   # Opt-in top-level global namespace aliases
    ├── extensions.h                       # Concentrator for all extensions (-> extensions/includes.h)
    ├── widgets.h                          # Concentrator for all widgets    (-> widgets/includes.h)
    │
    ├── extensions/                        # Feature-complete ImGui::<Package>:: subsystems
    │   ├── includes.h                     # Roll-up for all extensions (order-independent)
    │   ├── TimedTooltip/                  # Stationary & delayed tooltip engine with auto-bounds clamping
    │   └── Markdown/                      # Markdown parser & renderer with dynamic font scaling & tags
    │
    └── widgets/                           # Lightweight, immediate-mode ImGui:: UI controls
        ├── includes.h                     # Roll-up for all contributed widgets
        ├── imgui_toggle_buttons.h         # Animated toggle switches & segmented button pairs
        ├── imgui_mouse_wheel_controls.h   # Sliders, drags, combos & chasers with mouse-wheel support
        └── imgui_clickable_links.h        # ButtonLink, TextLink, MenuItemLink & AttachItemCursor affordances
```

---

## 🏛️ Namespacing Philosophy: Accommodating All Developer Preferences

Different flight sim developers have strong, differing philosophies when it comes to API ergonomics:
* Some developers prioritize **strict, collision-free sub-namespaces** to avoid polluting `ImGui::` and ensure 100% future-proof safety against upstream Dear ImGui updates.
* Other developers (like Sa'ar and many rapid-prototyping plugin authors) find second-level namespaces tedious to type and want **direct access under `ImGui::`** or even directly at **global scope**.

`contrib/` satisfies **both camps** without compromise through a 3-tier access model:

### Tier 1: Canonical Second-Level Sub-Namespaces (Production Best Practice)
Extensions live in clean, second-level sub-namespaces that mirror Dear ImGui's native vocabulary without colliding with core functions:
* `ImGui::Markdown::Text("...")` / `ImGui::Markdown::TextWrapped("...")`
* `ImGui::TimedTooltip::Text("...")` / `ImGui::TimedTooltip::TextUnformatted("...")`

### Tier 2: S-Tier Shorthands & Root `ImGui::` Convenience
* **`ImGui::MD::` Shorthand Alias:** `namespace MD = Markdown;` provides an official, zero-overhead shorthand:
  ```cpp
  ImGui::MD::Text("# Flight Deck"); // 100% identical to ImGui::Markdown::Text
  ```
* **Root `ImGui::` Widgets & Shorthands:** All widgets (`ToggleButton`, `MouseWheelSlider`, `PercentSlider`, `ButtonLink`) live directly in root `ImGui::`. Additionally, root Markdown shorthands matching native `ImGui::Text` are provided directly under `ImGui::`:
  ```cpp
  ImGui::TextMD("**Active** waypoint: %s", wpt);        // Shorthand for ImGui::Markdown::Text
  ImGui::TextWrappedMD("Weather briefing notes...");    // Shorthand for ImGui::Markdown::TextWrapped
  ImGui::TextMarkdown("Alternative spelling...");
  ```
* **Bidirectional Cross-Extension Bridges:** Call features from either direction:
  * `ImGui::TimedTooltip::Markdown::Text(...)` *or* `ImGui::TimedTooltip::MD::Text(...)`
  * `ImGui::Markdown::TimedTooltip::Text(...)` *or* `ImGui::MD::TimedTooltip::Text(...)`

### Tier 3: Opt-In Global Namespaces (Minimum Boilerplate)
For developers who want to bypass `ImGui::` entirely for extension calls, including [`contrib/contrib-global.h`](contrib-global.h) (or defining `#define CONTRIB_ENABLE_GLOBAL_NAMESPACES`) publishes top-level aliases to the global namespace:
```cpp
#define CONTRIB_ENABLE_GLOBAL_NAMESPACES
#include "contrib.h"

// Direct global calls (no ImGui:: required):
Markdown::Text("Status: **Optimal**");
MD::Text("Warning: *Check Fuel*");
TimedTooltip::Text("Stationary delayed tooltip");
```

---

## 🎛️ Configuration Matrix: Opt-In vs. Opt-Out

Everything in `contrib/` is **active and available by default** when you include `contrib.h`. You only define macros if you want to selectively disable features or opt into global aliases:

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
| **`CONTRIB_DISABLE_PERCENT_SLIDER`** | **Opt-Out** | *Off* | Excludes `PercentSlider` and `SliderPercent`. |
| **`CONTRIB_DISABLE_MOUSE_WHEEL_COMBO`** | **Opt-Out** | *Off* | Excludes `MouseWheelCombo` and `ComboWithReset`. |
| **`IMGUI_CONTRIB_DISABLE_RESET_UNDO`** | **Opt-Out** | *Off* | Disables 1-click Reset-with-Undo on reset buttons. |

*(Note: Prefixes `IMGUI_CONTRIB_DISABLE_*`, `IMGUI_DISABLE_CONTRIB_*`, and legacy `IMGUI_DISABLE_EXTRA_*` are also honored automatically).*

---

### 1. Packages & Extensions (`contrib/extensions/`)
* **What they are:** Feature-rich additions that maintain their own internal state, use configuration structs, or integrate external libraries.
* **Namespacing:** Primary canonical sub-namespace (`ImGui::Markdown::`, `ImGui::TimedTooltip::`), shorthand alias (`ImGui::MD::`), root convenience aliases (`ImGui::TextMD(...)`), and opt-in global namespaces (`contrib-global.h`).
* **Inclusion:**
  * **Everything in `contrib/`:** `#include "ImgWindow/contrib.h"` (or `"ImgWindow/contrib/includes.h"`)
  * **All extensions:** `#include "ImgWindow/contrib/extensions.h"` (or `"ImgWindow/contrib/extensions/includes.h"`)
  * **A-la-carte:** `#include "ImgWindow/contrib/extensions/TimedTooltip/imgui_tooltip_ext.h"`
* *See &rarr; [contrib/extensions/README.md](extensions/README.md) for full guidelines.*

### 2. Reusable Widgets (`contrib/widgets/`)
* **What they are:** Self-contained, immediate-mode UI controls and item chasers that feel like native Dear ImGui controls (e.g. toggle buttons, mouse-wheel sliders/drags/combos, clickable link affordances).
* **Namespacing:** Injected directly into the root `ImGui::` namespace (`ImGui::ToggleButton(...)`, `ImGui::MouseWheelSlider(...)`, `ImGui::AdjustItemOnMouseWheel(...)`, `ImGui::AttachItemCursor(...)`).
* **Inclusion Strategy:**
  * **Everything in `contrib/`:** `#include "ImgWindow/contrib.h"`
  * **All widgets:** `#include "ImgWindow/contrib/widgets.h"` (or `"ImgWindow/contrib/widgets/includes.h"`)
  * **A-la-carte:** `#include "ImgWindow/contrib/widgets/imgui_mouse_wheel_controls.h"`
  * **Opt-Out Guards:** Every extension, widget family, and chaser is wrapped in an `#ifndef CONTRIB_DISABLE_<FEATURE>` guard so you can selectively disable any feature before including `contrib.h`.
* *See &rarr; [contrib/widgets/README.md](widgets/README.md) for full guidelines.*

---

## How to Contribute

We welcome community contributions! Whether porting an existing open-source widget or introducing a new UI component:

1. **Choose the Category:** Determine if your code is a standalone **Widget** (a single control in `widgets/`) or a full **Extension** (a package in `extensions/`).
2. **Namespace Appropriately:** Follow the namespacing rules above.
3. **Keep Dependencies Minimal:** Header-only or clean single-file implementations are strongly preferred.
4. **Document Your Work:** Include a dedicated `README.md` in your extension folder detailing usage, API signatures, and code examples.
5. **Open a PR:** Submit a Pull Request against the repository with a description and sample screenshot or code snippet.
