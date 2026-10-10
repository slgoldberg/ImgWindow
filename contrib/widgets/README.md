# ImGui Extra Widgets

This directory contains lightweight, reusable, immediate-mode **Widgets** extending Dear ImGui within the **ImgWindow** framework.

Unlike **Extensions** (which manage their own complex background subsystems, registries, and configuration structures), **Widgets** are self-contained immediate-mode controls. They hook directly into Dear ImGui's ambient context (`GImGui`, ID stack, `ImGuiStorage`, and window draw lists) and live directly in the root `ImGui::` namespace to feel completely native.

---

## Architecture & Inclusion Strategy

### 1. Root `ImGui::` Namespacing
All widgets in this directory are injected directly into `namespace ImGui { ... }`. You call `ImGui::ToggleButton(...)`, `ImGui::SliderPercent(...)`, and `ImGui::ButtonLink(...)` just like Dear ImGui's built-in `ImGui::Checkbox` or `ImGui::SliderFloat`.

### 2. The 3-Tier Inclusion Model

| Pattern | Usage | Benefit |
| :--- | :--- | :--- |
| **Master Roll-Up** | `#include "contrib/widgets/imgui_extra_widgets.h"` | Instant access to the entire widget collection in one line. |
| **A-la-Carte** | `#include "contrib/widgets/toggle_button.h"` | Include only the specific control family you need. |
| **Opt-Out Macros** | `#define IMGUI_DISABLE_EXTRA_<WIDGET>` | Zero conflict if your project already defines a conflicting identifier. |

#### Opt-Out Configuration Flags
If your project defines a function with the same name, define any of the following macros *before* including `imgui_extra_widgets.h`:

```cpp
#define IMGUI_DISABLE_EXTRA_TOGGLE_BUTTON  // Disables ToggleButton & ToggleButtons
#define IMGUI_DISABLE_EXTRA_WHEEL_SLIDER   // Disables AdjustOnItemMouseWheel & Wheel Sliders
#define IMGUI_DISABLE_EXTRA_CLICKABLE_LINK // Disables ButtonLink, TextLink & Link affordances
#include "contrib/widgets/imgui_extra_widgets.h"
```

---

## Widget Catalog

### 1. Toggle Controls (`toggle_button.h`)

#### A. Animated Toggle Switch (`ImGui::ToggleButton`)
A modern, iOS / Material-design rounded pill switch with a smoothly sliding circular thumb.

```cpp
#include "contrib/widgets/toggle_button.h"

static bool s_ExpertMode = false;

// Standard switch with label on the right:
ImGui::ToggleButton("Expert Mode", &s_ExpertMode);

// Compact switch without visible label (uses ID only):
ImGui::ToggleButton("##hud_switch", &s_HudActive);
```

* **Features:**
  * Frame-rate independent sliding animation powered by `io.DeltaTime` and stored per-widget in `ImGuiStorage`.
  * Sizing automatically derives from `ImGui::GetFrameHeight()` (resolution-independent) or accepts explicit `ImVec2 size`.
  * Automatic hand pointer cursor (`ImGuiMouseCursor_Hand`) on hover.
  * Theme integration: uses `ImGuiCol_FrameBg` (off), `ImGuiCol_HeaderActive` / `ImGuiCol_ButtonActive` (on), and `ImGuiCol_SliderGrab` (thumb).

#### B. Segmented Dual-Button Pair (`ImGui::ToggleButtons`)
Joined side-by-side buttons for clear binary selections (`Off` / `On`, `Opaque` / `Transparent`).

```cpp
static int s_WindowStyle = 0; // 0 = Opaque, 1 = Transparent

if (ImGui::ToggleButtons("##style_toggle", &s_WindowStyle, "Opaque", "Transparent")) {
    ApplyStyleChange();
}
```

---

### 2. Mouse-Wheel Sliders (`wheel_slider.h`)

#### A. Standalone Helper (`ImGui::AdjustOnItemMouseWheel`)
Attaches to the preceding ImGui widget via `ImGui::IsItemHovered()`, interprets vertical and horizontal mouse-wheel clicks, and clamps boundaries. 
* **Option C Cursor Precision**: Switches the cursor to East-West resize arrows (`ImGuiMouseCursor_ResizeEW`) strictly when hovering over the physical slider track, preserving standard arrow cursors over text labels.

```cpp
#include "contrib/widgets/wheel_slider.h"

// Attach mouse-wheel adjustment to any standard ImGui slider:
ImGui::SliderFloat("Text Zoom", &fontZoom, 0.75f, 1.75f);
ImGui::AdjustOnItemMouseWheel(&fontZoom, 0.75f, 1.75f, 0.01f);
```

#### B. Integrated Wheel Sliders (`SliderFloatWithWheel` / `SliderPercentWithWheel`)
Drop-in replacements for standard sliders that respond to mouse-wheel scrolling out of the box.

```cpp
static float s_Heading = 180.0f;
ImGui::SliderFloatWithWheel("Heading", &s_Heading, 0.0f, 360.0f, "%.0f°", 0, 1.0f);

static float s_Opacity = 0.85f;
ImGui::SliderPercentWithWheel("Window Opacity", &s_Opacity, 0.1f, 1.0f);
```

#### C. Composite Reset Sliders (`ResetSliderFloatWithWheel` / `ResetSliderPercentWithWheel`)
Full-featured compound sliders with an abutting vector undo/reset button, smart width reservation, and dual-tooltip support.
* Sits cleanly in a single line: `[ Slider Track ] [ ↺ Reset ]  Label Text`
* Automatically dims the reset button when already at default value, and lights up when modified.
* Features an antialiased vector counter-clockwise undo glyph drawn directly in `ImDrawList` (zero font dependencies).

```cpp
static float s_Heading = 180.0f;

// 1-line slider with built-in mouse-wheel and one-click reset to 180°:
ImGui::ResetSliderFloatWithWheel("Heading", &s_Heading, 180.0f /* default */, 0.0f, 360.0f, "%.0f°");

static float s_Opacity = 0.85f;
ImGui::ResetSliderPercentWithWheel("Opacity", &s_Opacity, 0.85f /* default */, 0.1f, 1.0f);
```

---

### 3. Clickable Link Affordances (`clickable_link.h`)

In standard Dear ImGui, buttons and clickable elements keep the default OS arrow cursor. The `clickable_link.h` header eliminates the boilerplate of writing `if (IsItemHovered()) SetMouseCursor(ImGuiMouseCursor_Hand);` on every interactive item.

```cpp
#include "contrib/widgets/clickable_link.h"

// Standard button with hand cursor on hover:
if (ImGui::ButtonLink("Save Flight Plan")) {
    SavePlan();
}

// Inline hyperlinked text (styled like an active link with hand cursor):
if (ImGui::TextLink("Read Pilot Handbook")) {
    OpenDocumentation();
}

// Checkboxes and radio buttons with hand cursor:
ImGui::CheckboxLink("Enable Autothrottle", &s_Autothrottle);
ImGui::RadioButtonLink("NAV 1", &s_NavSource, 1);
```

#### Standalone Cursor Affordance Helper:
```cpp
// Attach hand cursor to any custom drawn element:
ImGui::Image(textureId, ImVec2(64, 64));
ImGui::ShowLinkCursorOnHover();
```

---

## Credits & Upstream Lineage

* **Author & Architect**: Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))
* **SliderPercent Upstream Inspiration**: Birger Hoppe ([LiveTraffic](https://github.com/bhoppe/LiveTraffic))
* **License**: BSD 3-Clause (see [LICENSE](../../LICENSE))
