# ImGee: Avionics & Flight-Sim UI Toolkit for Dear ImGui

**ImGee** is an immediate-mode avionics and flight-simulation UI design toolkit built directly on top of [Dear ImGui](https://github.com/ocornut/imgui). Bundled as the flagship suite in `ImgWindow/contrib/ImGee/`, it equips flight sim plugin developers with cockpit-grade widgets, procedural vector icons, semantic typography, and interactive Markdown.

---

## 📖 Documentation & Guides

* **[Comprehensive ImGee User Manual](docs/ImGee_User_Guide.md)**: Full API reference, code examples, typography tables, and architectural details (publication-grade Markdown ready for PDF compilation).

---

## 🚀 Quick Start

### 1. Standard Integration (via `contrib.h`)
```cpp
#include "contrib.h"
```

### 2. Direct Package Integration (via `contrib/ImGee/imgee.h`)
```cpp
#include "contrib/ImGee/imgee.h"
```

---

## 🧩 Subsystems & Module Structure

```
contrib/ImGee/
├── imgee.h                        # Master package roll-up header & canonical IMGEE_* switches
├── imgee-global.h                 # Opt-in global namespace aliases (Markdown::, TimedTooltip::)
├── docs/
│   └── ImGee_User_Guide.md        # Comprehensive User & Developer Guide
├── extensions/
│   ├── TimedTooltip/              # Stationary & delayed tooltip engine with screen edge clamping
│   ├── Markdown/                  # CommonMark GFM tables, tristate column sorting & tags
│   └── Typography/                # Font roles (ImFontRole) & inline StyledText (<huge>..<tiny>)
└── widgets/
    ├── imgui_icons.h              # Zero-texture procedural vector icon engine & ImGui::Icon
    ├── imgui_knobs.h              # Avionics rotary knobs, dimmer dials & 2-step OFF detents
    ├── imgui_mouse_wheel_controls.h # Mouse-wheel sliders, combos & inline reset chasers
    ├── imgui_badges.h             # Status badges, pills, unary annunciators & <kbd>
    ├── imgui_popup_menus.h        # Directional popups (AboveItem) & SelectPulldown widgets
    ├── imgui_toggle_buttons.h     # Tactile animated physical toggle switches
    ├── imgui_spinners.h           # Procedural radar sweeps & radial countdown timers
    ├── imgui_clickable_links.h    # OS hand cursor links (ButtonLink, TextLink)
    ├── imgui_copy_button.h        # Clipboard copy buttons with vector icons & feedback
    └── imgui_file_dialog.h        # Non-blocking modal-free in-sim file dialogs
```

---

## ⚙️ Configuration & Opt-Out Flags

Every feature in ImGee is enabled by default. To disable specific components, define any of the following macros before including `imgee.h` (or `contrib.h`):

```cpp
// Disable entire ImGee suite:
#define IMGEE_DISABLE_ALL

// Subsystem switches:
#define IMGEE_DISABLE_EXTENSIONS
#define IMGEE_DISABLE_WIDGETS

// Feature switches:
#define IMGEE_DISABLE_ICONS
#define IMGEE_DISABLE_ICON_BUTTON
#define IMGEE_DISABLE_TYPOGRAPHY
#define IMGEE_DISABLE_MARKDOWN
#define IMGEE_DISABLE_TIMED_TOOLTIP
#define IMGEE_DISABLE_KNOBS
#define IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS
#define IMGEE_DISABLE_BADGES
#define IMGEE_DISABLE_POPUP_MENUS
#define IMGEE_DISABLE_TOGGLE_BUTTONS
#define IMGEE_DISABLE_CLICKABLE_LINKS
#define IMGEE_DISABLE_SPINNERS

// Opt-in global namespaces:
#define IMGEE_ENABLE_GLOBAL_NAMESPACES
```

*(Note: Legacy `CONTRIB_DISABLE_*` flags are automatically mapped via an internal alias bridge in `imgee.h`.)*
