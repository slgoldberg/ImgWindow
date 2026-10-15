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

To keep contributions modular and easy to include at any level of granularity, the ecosystem is organized into two primary categories with uniform **`includes.h`** roll-up headers and top-level concentrators:

```
ImgWindow/
├── contrib.h                              # Top-level concentrator (-> contrib/includes.h)
└── contrib/
    ├── includes.h                         # Master roll-up for all extensions & widgets
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

### 1. Packages & Extensions (`contrib/extensions/`)
* **What they are:** Feature-rich additions that maintain their own internal state, use configuration structs, or integrate external libraries.
* **Namespacing:** Scoped within a dedicated sub-namespace (`ImGui::<PackageName>::`, e.g. `ImGui::TimedTooltip::Text(...)`, `ImGui::MD::TextWrapped(...)`) to avoid polluting the root `ImGui::` namespace.
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
