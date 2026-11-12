# ImGee: Avionics & Flight-Sim UI Toolkit for Dear ImGui
**Version 1.0.0 • Architecture & Developer Reference**

**ImGee** is a specialized immediate-mode avionics and flight-simulation UI design toolkit built directly on top of [Dear ImGui](https://github.com/ocornut/imgui). Bundled as the premier package within the `ImgWindow` `contrib/` ecosystem (and architected to stand independently as a self-contained open-source project), ImGee equips flight simulator plugin developers with cockpit-grade widgets, procedural vector icons, semantic typography, and interactive Markdown.

---

## 📖 Documentation & Guides

* **[Comprehensive ImGee User Manual (Markdown)](docs/ImGee_User_Guide.md)**: Full API reference, code examples, typography tables, and architectural details.
* **[Comprehensive ImGee User Manual (PDF)](docs/ImGee_User_Guide.pdf)**: Complete 60-page publication-grade PDF manual with running headers, dynamic pagination, and high-DPI in-cockpit figures.

---

## 🏛️ Architecture: The Two Categories of Content

Every component within ImGee is delivered strictly as a **single, self-contained C++ header file (`.h` only)**. There are zero `.cpp` compilation units to add to your build system, zero static or dynamic link dependencies, and zero order-of-inclusion restrictions. You can `#include` ImGee headers in any order without circular dependency hazards.

ImGee is divided into two distinct architectural categories:

```
contrib/ImGee/
├── extensions/          # Heavyweight, stateful subsystems (The Flagships)
└── widgets/             # Lightweight, immediate-mode cockpit controls (.h only)
```

### 1. Flagship Extensions (`extensions/`)
Extensions are feature-complete, heavyweight subsystems that solve deep architectural limitations of stock Dear ImGui. Unlike simple widgets, extensions maintain internal or frame-to-frame state, manage timing delays, parse complex syntax trees, or manage font registries.

ImGee provides **three flagship extensions** that form the core value of the package:
* **`TimedTooltip` (`imgui_tooltip_ext.h`):** Replaces volatile, cursor-following vanilla `ImGui::SetTooltip()` with stationary, delay-timed tooltips featuring intelligent display boundary clamping, cursor avoidance, and low-water scroll dismissal.
* **`Markdown` (`imgui_markdown_ext.h`):** Replaces flat plain ASCII with interactive CommonMark, full GFM tables with tristate column sorting and drag-resizing, parameterized styling tags (`<badge>`, `<pill>`, `<icon>`), and custom vector button faces (`ButtonMD`).
* **`Typography` (`imgui_typography_ext.h`):** An 8-slot semantic font role registry (`ImFontRole`), RAII scoped font guards, and the `StyledText` engine for inline baseline-aligned sizing with zero Markdown document overhead.

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

### 1. Why "contrib.h"? (ImgWindow Ecosystem Context)
In this distribution, **ImGee is provided alongside `ImgWindow v2` as a premier community contribution** housed inside the `contrib/` directory. This architectural relationship explains why the top-level all-in-one concentrator header is named `contrib.h`.

### 2. Submodule Include Paths: Recommended Front Door
In production environments, developers integrate `ImgWindow` as a **Git submodule** (e.g., in `third-party/ImgWindow/` or `libs/ImgWindow/`). Standard project build rules (CMake, Makefiles, Xcode, Visual Studio) typically add the directory *containing* `ImgWindow` to the compiler's include search paths (`-I`), rather than adding the inner `ImgWindow` repository root directly.

Because of this standard submodule topology, typing `#include "contrib.h"` will fail unless the build system specifically includes the inner repository folder.

**The safest, most standard, and recommended all-in-one inclusion is:**

```cpp
#include "ImgWindow/contrib.h"
```

*(Note: If your build system explicitly adds the `ImgWindow` directory directly to your `-I` compiler flags, `#include "contrib.h"` works identically).*

### 3. Direct Package Integration: ImGee Suite Only
If you wish to include *only* the ImGee suite -- bypassing the container switchboard or avoiding potential future third-party packages in `contrib/` -- you can include ImGee directly using any of our symmetrical package headers:

```cpp
// Option A: Canonical top-level contrib forwarder (Recommended):
#include "ImgWindow/contrib/imgee.h"

// Option B: PascalCase case-insensitive alias:
#include "ImgWindow/contrib/ImGee.h"

// Option C: Inner suite master header (or folder roll-up):
#include "ImgWindow/contrib/ImGee/imgee.h"
#include "ImgWindow/contrib/ImGee/includes.h"
```

### 4. A-La-Carte Subsystem Inclusion
ImGee is architecturally decoupled. If your plugin only requires flight-deck widgets or only requires extensions:

```cpp
// Option A: Include only extensions (TimedTooltip, Markdown, Typography):
#include "ImgWindow/contrib/ImGee/extensions.h"

// Option B: Include only cockpit widgets (Knobs, Toggles, Wheel Sliders, Badges, Icons):
#include "ImgWindow/contrib/ImGee/widgets.h"
```

> [!NOTE]
> **Legacy Backward-Compatibility Shims:** Legacy headers like `contrib/extensions.h` and `contrib/widgets.h` exist quietly solely to avoid breaking earlier code, forwarding into `ImGee`. New plugins should always use the canonical `ImgWindow/contrib/ImGee/...` paths.

### 5. Standalone / Independent Submodule Usage
When ImGee is spun out into its own dedicated open-source Git repository and submoduled directly into a host application:

```cpp
// Direct root include:
#include "imgee.h"

// Or via submodule path:
#include "ImGee/imgee.h"
```

Everything is configured and ready by default out of the box -- no subsystem initialization, factory registration, or frame hooks required.

---

## 🧩 Subsystems & Module Structure

```
contrib/ImGee/
├── imgee.h                        # Canonical ImGee Master Header & Version Contract
├── includes.h                     # Folder-level Roll-Up Header (-> imgee.h)
├── extensions.h                   # Subsystem Roll-Up Header (-> extensions/includes.h)
├── widgets.h                      # Subsystem Roll-Up Header (-> widgets/includes.h)
├── imgee-global.h                 # Opt-in Global Scope Aliases (Markdown::, TimedTooltip::)
├── LICENSE                        # Open-Source License (BSD 3-Clause)
├── docs/
│   ├── ImGee_User_Guide.md        # Comprehensive User Manual (Source)
│   ├── ImGee_User_Guide.pdf       # 61-Page Compiled Publication-Grade Manual
│   └── images/                    # High-DPI in-cockpit figures and screenshots
├── extensions/                    # Stateful Subsystems (The Flagships)
│   ├── README.md                  # Extensions Architecture & Omar Cornut's Standard
│   ├── includes.h                 # Extensions Roll-Up Header
│   ├── TimedTooltip/              # Stationary & delayed tooltip engine with edge clamping
│   ├── Markdown/                  # CommonMark GFM tables, tristate sorting & styled tags
│   └── Typography/                # Font roles (ImFontRole) & inline StyledText
└── widgets/                       # Immediate-Mode Flight Controls (.h only)
    ├── README.md                  # Complete Widgets Catalog Reference
    ├── includes.h                 # Widgets Roll-Up Header
    ├── imgui_icons.h              # Zero-texture procedural vector icon engine & ImGui::Icon
    ├── imgui_knobs.h              # Avionics rotary knobs, dimmer dials & 2-step OFF detents
    ├── imgui_mouse_wheel_controls.h # Mouse-wheel sliders, combos & inline reset chasers
    ├── imgui_badges.h             # Status badges, pills, unary annunciators & <kbd>
    ├── imgui_popup_menus.h        # Directional popups (AboveItem) & SelectPulldown
    ├── imgui_toggle_buttons.h     # Tactile animated physical toggle switches
    ├── imgui_spinners.h           # Procedural radar sweeps & radial countdown timers
    ├── imgui_clickable_links.h    # OS hand cursor links (ButtonLink, TextLink)
    ├── imgui_copy_button.h        # Clipboard copy buttons with vector icons & feedback
    └── imgui_file_dialog.h        # Non-blocking modal-free in-sim file dialogs
```

---

## ⚙️ Configuration Reference: Opt-Out Flags

Every feature in ImGee is enabled by default. To disable specific components, define any of the following macros *before* including `imgee.h` (or `contrib.h`):

| Canonical Macro Directive | Legacy Alias | Description |
| :--- | :--- | :--- |
| **`IMGEE_DISABLE_ALL`** | `CONTRIB_IMGEE_DISABLE_ALL` | Disables the entire ImGee suite. |
| **`IMGEE_DISABLE_EXTENSIONS`** | `CONTRIB_DISABLE_EXTENSIONS` | Excludes all extensions (`TimedTooltip`, `Markdown`, `Typography`). |
| **`IMGEE_DISABLE_WIDGETS`** | `CONTRIB_DISABLE_WIDGETS` | Excludes all flight-deck widgets in `widgets/`. |
| **`IMGEE_DISABLE_TIMED_TOOLTIP`** | `CONTRIB_DISABLE_TIMED_TOOLTIP` | Excludes stationary delayed tooltip engine. |
| **`IMGEE_DISABLE_MARKDOWN`** | `CONTRIB_DISABLE_MARKDOWN` | Excludes Markdown parser, GFM tables, and callouts. |
| **`IMGEE_DISABLE_TYPOGRAPHY`** | `CONTRIB_DISABLE_TYPOGRAPHY` | Excludes `ImFontRole` and `ImGui::StyledText`. |
| **`IMGEE_DISABLE_ICONS`** | `CONTRIB_DISABLE_ICONS` | Excludes procedural vector icon engine and `ImGui::Icon`. |
| **`IMGEE_DISABLE_KNOBS`** | `CONTRIB_DISABLE_KNOBS` | Excludes avionics rotary knobs, dials, and detents. |
| **`IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS`** | `CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS` | Excludes mouse-wheel sliders, drags, combos, and reset buttons. |
| **`IMGEE_DISABLE_BADGES`** | `CONTRIB_DISABLE_BADGES` | Excludes status badges, pills, annunciators, and keycaps. |
| **`IMGEE_DISABLE_TOGGLE_BUTTONS`** | `CONTRIB_DISABLE_TOGGLE_BUTTONS` | Excludes animated physical toggle switches. |
| **`IMGEE_DISABLE_POPUP_MENUS`** | `CONTRIB_DISABLE_POPUP_MENUS` | Excludes directional popups (`AboveItem`) and pulldowns. |
| **`IMGEE_DISABLE_CLICKABLE_LINKS`** | `CONTRIB_DISABLE_CLICKABLE_LINKS` | Excludes OS hand cursor links (`ButtonLink`, `TextLink`). |
| **`IMGEE_DISABLE_SPINNERS`** | `CONTRIB_DISABLE_SPINNERS` | Excludes activity spinners and countdown timer dials. |
| **`IMGEE_ENABLE_GLOBAL_NAMESPACES`** | `CONTRIB_ENABLE_GLOBAL_NAMESPACES` | Publishes `Markdown::`, `MD::`, and `TimedTooltip::` to global scope. |

*(Note: Legacy `CONTRIB_DISABLE_*` flags are automatically mapped via an internal alias bridge in `imgee.h` with zero build warnings.)*

---

## 📄 License & Attribution

ImGee is released under the **BSD 3-Clause License**. See [LICENSE](LICENSE) for full details.  
Copyright © 2026 Steven L. Goldberg (`@slgoldberg`) and the Rogers.
