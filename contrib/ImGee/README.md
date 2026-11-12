# ImGee: Avionics & Flight-Sim UI Toolkit for Dear ImGui
**Version 1.0.0 • Architecture & Developer Reference**

**ImGee** turns stock [Dear ImGui](https://github.com/ocornut/imgui) from a game-debugger overlay into a tactile, ergonomic desktop and flight-deck UI system in **a single header `#include`** -- with zero `.cpp` files to compile and zero CMake changes.

Where vanilla Dear ImGui flashes cursor-chasing plain-text tooltips, ignores the mouse scroll-wheel, leaves interactive buttons on a dead `Arrow` cursor, and renders missing-glyph `?` boxes for Unicode symbols, **ImGee** gives you polite stationary `TimedTooltip`s, mouse-wheel sliders/combos/pulldowns with 1-click Reset-with-Undo (`↺`/`↻`), 270° rotary dimmer knobs, animated rocker switches, CommonMark + sortable GFM tables, async 3-tier images, semantic typography (`ImFontRole` & `StyledText`), curated themes (`Charcoal`, `Rusty`), and a zero-font procedural vector icon engine.

> **Ecosystem Note (`ImGee` vs. `ImgWindow/contrib/`):**  
> **ImGee** is 100% generic Dear ImGui with zero hard dependency on X-Plane, and is architected to stand as its own independent GitHub repository/submodule. In `ImgWindow v2.4.0`, ImGee is bundled inside `ImgWindow`'s general third-party community directory (`contrib/ImGee/`, with convenience forwarder `contrib/imgee.h`) so `ImgWindow` users get the full toolkit out of the box.

---

## 📖 Documentation & Guides

* **[Comprehensive ImGee User Manual (Markdown)](docs/ImGee_User_Guide.md)** • **[(PDF Manual)](docs/ImGee_User_Guide.pdf)**: Full API reference, copy-pasteable C++ examples, typography tables, and architectural details.
* **[The ImGee UX Design Manifesto (Markdown)](docs/ImGee_UX_Design_Manifesto.md)** • **[(PDF Standard)](docs/ImGee_UX_Design_Manifesto.pdf)**: Human-factors principles, cursor & wheel affordance laws, "Lens UI" focus latching, and developer checklists.

---

## 🏛️ Architecture: The Two Categories of Content

Every component within ImGee is delivered strictly as a **single, self-contained C++ header file (`.h` only)**. There are zero `.cpp` compilation units to add to your build system, zero static or dynamic link dependencies, and zero order-of-inclusion restrictions. You can `#include` ImGee headers in any order without circular dependency hazards.

ImGee is divided into two distinct architectural categories:

```
ImGee/
├── extensions/          # Heavyweight, stateful subsystems (TimedTooltip, Markdown, Typography)
├── widgets/             # Lightweight, immediate-mode controls & vector icons (.h only)
└── themes/              # Curated studio & cockpit palettes (Charcoal, Rusty)
```

### 1. Flagship Extensions (`extensions/`)
Extensions are feature-complete, heavyweight subsystems that solve deep architectural limitations of stock Dear ImGui. Unlike simple widgets, extensions maintain internal or frame-to-frame state, manage timing delays, parse complex syntax trees, or manage font registries.

ImGee provides **three flagship extensions** that form the core value of the package:
* **`TimedTooltip` (`imgui_tooltip_ext.h`):** Replaces volatile, cursor-following vanilla `ImGui::SetTooltip()` with stationary, delay-timed tooltips featuring intelligent display boundary clamping, cursor avoidance, and low-water scroll dismissal.
* **`Markdown` (`imgui_markdown_ext.h`):** Replaces flat plain ASCII with interactive CommonMark, full GFM tables with tristate column sorting and drag-resizing, async 3-tier image loading, parameterized styling tags (`<badge>`, `<pill>`, `<icon>`), and custom vector button faces (`ButtonMD`).
* **`Typography` (`imgui_font_roles.h` & `imgui_styled_text.h`):** A 9-slot semantic font role registry (`ImFontRole`), RAII scoped font guards (`ScopedFontRole`), and the `StyledText` engine for inline baseline-aligned sizing with zero Markdown document overhead.

> [!IMPORTANT]
> **The Flagship Core:** We consider `TimedTooltip`, `Markdown`, and `Typography` the **flagship foundation** of the entire ImGee suite. We strongly believe every flight simulation plugin should use these modern subsystems instead of the stock or open-source versions from which they derive.

### 2. Cockpit & Avionics Widgets (`widgets/`)
Widgets are lightweight, zero-dependency, immediate-mode flight-deck controls. They hook directly into Dear ImGui's ambient context (`GImGui`, ID stack, `ImGuiStorage`, and active window draw lists) and live directly in the root `ImGui::` namespace to feel completely native.

* **Completely Independent:** Even if a project chooses not to install or enable any of the three extensions, all widgets in `widgets/` remain 100% usable and self-sufficient.
* **Zero Mandatory State:** Widgets execute strictly within the immediate-mode paradigm, drawing antialiased vector geometry directly to `ImDrawList`.

---

## 📐 Common Design Standards & Namespacing

ImGee follows strict architectural patterns established by the broader Dear ImGui community:

### 1. Omar Cornut's Ecosystem Standard (Single Point of Discovery)
In official Dear ImGui extensions and ecosystem projects (such as `implot`, `imgui_club`, and `imnodes`), Omar Cornut explicitly advocates extending `namespace ImGui { ... }`.
* **Discoverability & Autocomplete:** Modern C++ developers rely heavily on IDE autocomplete. When typing `ImGui::`, developers immediately discover every capability available across the toolkit in one place.
* **Zero Cognitive Overhead:** Developers are not forced to memorize artificial root namespaces (such as `extgui::` or `imgee::`) or manage scattered `using namespace` directives.

### 2. Second-Level Sub-Namespaces for Subsystems
While extending `namespace ImGui` is essential, placing heavy packages directly into the root risks colliding with future Dear ImGui core functions. ImGee cleanly encapsulates its extensions within **second-level sub-namespaces**:
* `ImGui::TimedTooltip::`
* `ImGui::Markdown::` (with official shorthand alias `ImGui::MD::`)
* `ImGui::Typography::`

Sub-namespaces allow extensions to reuse Dear ImGui's standard vocabulary without ambiguity:
```cpp
ImGui::TimedTooltip::Text("Hover-delayed stationary tooltip.");
ImGui::MD::TextWrapped("Configure your **avionics** below.");
```

### 3. Convenient Root Shorthands & Global Scope
* Shorthands like `ImGui::TextMD(...)` and `ImGui::ButtonMD(...)` are exposed in the root `ImGui::` namespace.
* Defining `#define IMGEE_ENABLE_GLOBAL_NAMESPACES` publishes `Markdown::`, `MD::`, and `TimedTooltip::` directly to the global scope for maximum brevity.

---

## 🌐 Generic Dear ImGui Core & Multi-Language Bindings

While developed and battle-tested within the X-Plane flight simulation ecosystem, **ImGee core is 100% generic Dear ImGui**. Nothing in the C++ toolkit inherently requires X-Plane or any specific host environment. Any application, game engine, or simulator utilizing Dear ImGui can consume ImGee directly.

### 1. LuiGee: Lua Bindings for FlyWithLua
For Lua developers, ImGee's flight-deck capabilities are brought directly into the Lua scripting environment as **LuiGee**.
* **Broad Simulator Support:** Available for ImGui-based windows in FlyWithLua across all X-Plane versions from **X-Plane 11.20 through the latest X-Plane 12+ releases and into the future**.
* **Zero-Boilerplate Scripting:** Exposes cockpit switches, knobs, vector icons, and timed tooltips to Lua developers with idiomatic syntax.

### 2. Community & Other Language Bindings
We are actively considering bindings for other environments and programming languages (such as **Python**, **Rust**, and **C#**). If developers in the wider Dear ImGui ecosystem are interested in bringing ImGee to other platforms or developing language wrappers, we warmly welcome community discussions and collaboration!

---

## 🚀 Quick Start & Integration Models

### 1. Recommended Front Door: Including ImGee Directly
When consuming **ImGee** via an `ImgWindow` Git submodule (e.g., in `third-party/ImgWindow/` or `src/ImgWindow/`), include the **ImGee package header**:

```cpp
// Recommended when the parent folder of ImgWindow/ is on your -I include path:
#include "ImgWindow/contrib/imgee.h"

// Or if ImgWindow/ itself is on your -I include path (e.g. imgui4xp):
#include "contrib/imgee.h"
```

*(Note: `#include "ImgWindow/contrib/ImGee/imgee.h"` works identically. Always match exact filename casing so Linux builds succeed cleanly.)*

### 2. Standalone ImGee Repository / Submodule Usage
When ImGee is submoduled directly from its own dedicated Git repository:

```cpp
#include "ImGee/imgee.h"   // (or #include "imgee.h" if ImGee/ is on -I)
```

### 3. A-La-Carte Subsystem Inclusion
ImGee is architecturally decoupled. If your project only requires immediate-mode widgets or only requires extensions:

```cpp
// Option A: Include only extensions (TimedTooltip, Markdown, Typography):
#include "ImgWindow/contrib/ImGee/extensions.h"

// Option B: Include only cockpit widgets (Knobs, Toggles, Wheel Sliders, Badges, Icons):
#include "ImgWindow/contrib/ImGee/widgets.h"
```

### 4. About `ImgWindow/contrib.h` (Community Umbrella Header)
In `ImgWindow` distributions, `#include "ImgWindow/contrib.h"` (or `#include "contrib.h"`) serves as `ImgWindow`'s multi-package community umbrella header -- pulling in `ImGee` today plus any future community-submitted packages in `ImgWindow/contrib/`. Including `contrib/imgee.h` targets **strictly ImGee**.

Everything is configured and ready by default out of the box -- no subsystem initialization, factory registration, or frame hooks required.

---

## 🧩 Subsystems & Module Structure

```
ImGee/
├── imgee.h                        # Canonical ImGee Master Header & Version Contract
├── includes.h                     # Folder-level Roll-Up Header (-> imgee.h)
├── extensions.h                   # Subsystem Roll-Up Header (-> extensions/includes.h)
├── widgets.h                      # Subsystem Roll-Up Header (-> widgets/includes.h)
├── imgee-global.h                 # Opt-in Global Scope Aliases (Markdown::, TimedTooltip::)
├── imgee_stbi_internal.h          # Isolated namespace/symbol-prefixed PNG/JPG/GIF/BMP/TGA decoder
├── imgee_stb_image.cpp            # Optional single-TU build unit (for -DIMGEE_EXTERNAL_STB_IMAGE)
├── LICENSE                        # Open-Source License (BSD 3-Clause)
├── docs/
│   ├── ImGee_User_Guide.md        # Comprehensive User Manual (Source)
│   ├── ImGee_User_Guide.pdf       # Compiled Publication-Grade Manual
│   ├── ImGee_UX_Design_Manifesto.md  # Human Factors & Ergonomics Standard (Source)
│   ├── ImGee_UX_Design_Manifesto.pdf # Compiled UX Design Manifesto
│   └── images/                    # High-DPI in-cockpit figures and screenshots
├── extensions/                    # Stateful Subsystems (The Flagships)
│   ├── README.md                  # Extensions Architecture & Omar Cornut's Standard
│   ├── includes.h                 # Extensions Roll-Up Header
│   ├── TimedTooltip/              # Stationary & delayed tooltip engine with edge clamping
│   ├── Markdown/                  # CommonMark GFM tables, tristate sorting, 3-tier images & tags
│   └── Typography/                # Font roles (ImFontRole) & inline StyledText
├── widgets/                       # Immediate-Mode Flight Controls (.h only)
│   ├── README.md                  # Complete Widgets Catalog Reference
│   ├── includes.h                 # Widgets Roll-Up Header
│   ├── imgui_icons.h              # Zero-texture procedural vector icon engine & ImGui::Icon
│   ├── imgui_knobs.h              # Avionics rotary knobs, dimmer dials & 2-step OFF detents
│   ├── imgui_mouse_wheel_controls.h # Mouse-wheel sliders, combos & inline reset chasers
│   ├── imgui_badges.h             # Status badges, pills, unary annunciators & <kbd>
│   ├── imgui_popup_menus.h        # Directional popups (AboveItem) & SelectPulldown
│   ├── imgui_toggle_buttons.h     # Tactile animated physical toggle switches
│   ├── imgui_spinners.h           # Procedural radar sweeps & radial countdown timers
│   ├── imgui_clickable_links.h    # OS hand cursor links (ButtonLink, TextLink)
│   ├── imgui_copy_button.h        # Clipboard copy buttons with vector icons & feedback
│   └── imgui_file_dialog.h        # Non-blocking modal-free in-sim file dialogs
└── themes/                        # Curated Studio & Cockpit Palettes
    ├── includes.h                 # Themes Roll-Up Header
    └── imgui_styles.h             # StyleColorsCharcoal(), StyleColorsRusty(), ApplyDefaultTheme()
```

---

## ⚙️ Configuration Reference: Opt-Out Flags

Every feature in ImGee is enabled by default. To disable specific components, define any of the following macros *before* including `imgee.h`:

| Macro Directive | Description |
| :--- | :--- |
| **`IMGEE_DISABLE_ALL`** | Disables the entire ImGee suite. |
| **`IMGEE_DISABLE_EXTENSIONS`** | Excludes all extensions (`TimedTooltip`, `Markdown`, `Typography`). |
| **`IMGEE_DISABLE_WIDGETS`** | Excludes all flight-deck widgets in `widgets/`. |
| **`IMGEE_DISABLE_THEMES`** | Excludes curated palettes in `themes/`. |
| **`IMGEE_DISABLE_TIMED_TOOLTIP`** | Excludes stationary delayed tooltip engine. |
| **`IMGEE_DISABLE_MARKDOWN`** | Excludes Markdown parser, GFM tables, and callouts. |
| **`IMGEE_DISABLE_TYPOGRAPHY`** | Excludes `ImFontRole` and `ImGui::StyledText`. |
| **`IMGEE_DISABLE_ICONS`** | Excludes procedural vector icon engine and `ImGui::Icon`. |
| **`IMGEE_DISABLE_ICON_BUTTON`** | Excludes `IconButton` and `ButtonIcon` push buttons. |
| **`IMGEE_DISABLE_KNOBS`** | Excludes avionics rotary knobs, dials, and detents. |
| **`IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS`** | Excludes mouse-wheel sliders, drags, combos, and reset buttons. |
| **`IMGEE_DISABLE_BADGES`** | Excludes status badges, pills, `PillNamed`, and keycaps. |
| **`IMGEE_DISABLE_TOGGLE_BUTTONS`** | Excludes animated physical toggle switches. |
| **`IMGEE_DISABLE_POPUP_MENUS`** | Excludes directional popups (`AboveItem`) and `SelectPulldown`. |
| **`IMGEE_DISABLE_CLICKABLE_LINKS`** | Excludes OS hand cursor links (`ButtonLink`, `TextLink`). |
| **`IMGEE_DISABLE_COPY_BUTTON`** | Excludes `CopyButton` and `CopyLastItemButton`. |
| **`IMGEE_DISABLE_FILE_DIALOG`** | Excludes `FilePickerCombo`, `OpenFileDialog`, and `SaveFileDialog`. |
| **`IMGEE_DISABLE_SPINNERS`** | Excludes activity spinners and countdown timer dials. |
| **`IMGEE_DISABLE_STB_IMAGE`** | Disables ImGee's isolated built-in `stb_image` decoder. |
| **`IMGEE_EXTERNAL_STB_IMAGE`** | Skips inline decoder compilation so you can compile `imgee_stb_image.cpp` once. |
| **`IMGEE_ENABLE_GLOBAL_NAMESPACES`** | Publishes `Markdown::`, `MD::`, and `TimedTooltip::` to global scope. |

*(Note: Pre-v1.0 legacy `CONTRIB_*` defines are also mapped automatically in `imgee.h` for backward compatibility.)*

---

## 📄 License & Attribution

ImGee is released under the **BSD 3-Clause License**. See [LICENSE](LICENSE) for full details.  
Copyright © 2026 Steven L. Goldberg (`@slgoldberg`) and the Rogers.
