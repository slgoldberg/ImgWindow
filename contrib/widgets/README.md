# ImGui Extra Widgets

This directory contains lightweight, reusable, immediate-mode **Widgets** extending Dear ImGui within the **ImgWindow** framework.

Unlike **Extensions** (which manage their own complex background subsystems, registries, and configuration structures), **Widgets** are self-contained immediate-mode controls. They hook directly into Dear ImGui's ambient context (`GImGui`, ID stack, `ImGuiStorage`, and window draw lists) and live directly in the root `ImGui::` namespace to feel completely native.

---

## Architecture & Inclusion Strategy

### 1. Root `ImGui::` Namespacing
All widgets in this directory are injected directly into `namespace ImGui { ... }`. You call `ImGui::ToggleButton(...)`, `ImGui::SliderWithReset(...)`, `ImGui::PercentSlider(...)`, and `ImGui::ButtonLink(...)` just like Dear ImGui's built-in `ImGui::Checkbox` or `ImGui::SliderFloat`.

### 2. The 4-Tier Inclusion Model

| Pattern | Usage | Benefit |
| :--- | :--- | :--- |
| **Master Contrib Roll-Up** | `#include "ImgWindow/contrib.h"` *(or `"contrib/includes.h"`)* | Includes **all** extensions (`TimedTooltip`, `Markdown`) and contributed widgets in guaranteed dependency order. |
| **Widgets Roll-Up** | `#include "ImgWindow/contrib/widgets.h"` *(or `"contrib/widgets/includes.h"`)* | Instant access to the entire widget collection in one line. |
| **A-la-Carte** | `#include "ImgWindow/contrib/widgets/imgui_mouse_wheel_controls.h"` | Include only the specific control family you need. |
| **Opt-Out Macros** | `#define CONTRIB_DISABLE_<FEATURE>` | Zero conflict if your project already defines a conflicting identifier. |

#### Opt-Out Configuration Flags
If your project defines a function with the same name, define any of the following canonical macros *before* including `contrib.h` or `widgets.h` (`IMGUI_CONTRIB_DISABLE_*`, `IMGUI_DISABLE_CONTRIB_*`, and legacy `IMGUI_DISABLE_EXTRA_*` macro names are also honored automatically):

```cpp
#define CONTRIB_DISABLE_TOGGLE_BUTTONS        // Disables ToggleButton & ToggleButtons
#define CONTRIB_DISABLE_TOGGLE_MOUSE_WHEEL    // Disables mouse-wheel support on ToggleButton & ToggleButtons
#define CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS  // Disables all wheel sliders, drags, cyclers & combos
#define CONTRIB_DISABLE_CLICKABLE_LINKS       // Disables ButtonLink, TextLink & AttachItemCursor

// Fine-grained opt-outs within imgui_mouse_wheel_controls.h:
#define CONTRIB_DISABLE_GET_MOUSE_WHEEL       // Disables GetMouseWheel* primitives
#define CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL    // Disables AdjustItemOnMouseWheel()
#define CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL     // Disables CycleItemOnMouseWheel()
#define CONTRIB_DISABLE_PERCENT_SLIDER        // Disables PercentSlider() & SliderPercent()
#define CONTRIB_DISABLE_SLIDER_PERCENT        // Disables only the SliderPercent() alias
#define CONTRIB_DISABLE_MOUSE_WHEEL_COMBO     // Disables MouseWheelCombo & ComboWithReset
```

---

## Widget Catalog

### 1. Toggle Controls (`imgui_toggle_buttons.h`)

#### A. Animated Toggle Switch (`ImGui::ToggleButton`)
A modern, iOS / Material-design rounded pill switch with a smoothly sliding circular thumb. Accepts both `bool*` and `int*` state variables.

```cpp
#include "contrib/widgets/imgui_toggle_buttons.h"

static bool s_ExpertMode = false;
static int  s_SmoothTransitions = 1;

// Standard switch with clickable label on the right:
ImGui::ToggleButton("Expert Mode", &s_ExpertMode);

// Compact switch without visible label (uses ID only):
ImGui::ToggleButton("##hud_switch", &s_HudActive);

// Switch with built-in per-state tooltips ({ Off tooltip, On tooltip }):
static const char* const kTransitionTooltips[] = {
    "Smooth transitions are **OFF** (instant snap).\n- Click or scroll **up** on the switch to enable.",
    "Smooth transitions are **ON** (animated).\n- Click or scroll **down** on the switch to disable."
};
ImGui::ToggleButton("Smooth Transitions", &s_SmoothTransitions, ImVec2(0, 0), kTransitionTooltips);
```

* **Features:**
  * Frame-rate independent sliding animation powered by `io.DeltaTime` and stored per-widget in `ImGuiStorage`.
  * Accepts either `bool*` or `int*` (`0` = Off, non-zero = On).
  * **Built-In Clamped Mouse-Wheel Support (`allow_mouse_wheel = true`):** Spinning the scroll wheel up/right over the pill switch turns it **On** and clamps there; spinning down/left turns it **Off** and clamps there (never wraps). Hovering over the text label does *not* capture the mouse wheel, preventing accidental toggles when scrolling windows.
  * **Per-State Tooltips (`item_tooltips`):** Optional 2-element string array `{ off_tooltip, on_tooltip }` automatically displays the tooltip for the active state via `TimedTooltip::TextMDProxy` (or standard `SetTooltip` fallback).
  * Sizing automatically derives from `ImGui::GetFrameHeight()` (resolution-independent) or accepts explicit `ImVec2 size`.
  * Automatic hand pointer cursor (`ImGuiMouseCursor_Hand` by default, customizable via `hover_cursor` or `IMGUI_EXTRA_TOGGLE_DEFAULT_CURSOR`).

#### B. Segmented Dual-Button Pair (`ImGui::ToggleButtons`)
Joined side-by-side buttons for explicit binary selections (`Off` / `On`, `Solid Opaque` / `Glass Translucent`). Accepts both `int*` and `bool*`.

```cpp
static int s_WindowStyle = 0; // 0 = Solid Opaque, 1 = Glass Translucent

// 1. Compact pair without a right-side label (starts with "##"):
if (ImGui::ToggleButtons("##style_toggle", &s_WindowStyle, "Solid Opaque", "Glass Translucent")) {
    ApplyStyleChange();
}

// 2. With clickable right-side title ("Surface Style") and wheel-latched hover index:
int hovered_idx = -1;
ImGui::ToggleButtons("Surface Style##style_pair", &s_WindowStyle, "Solid Opaque", "Glass Translucent",
                     ImVec2(130.0f, 0.0f), nullptr /* item_tooltips */, &hovered_idx);
```

* **Features:**
  * **Optional Right-Side Clickable Title:** Following standard Dear ImGui `"Title##id"` semantics, if `label` contains text before `"##"`, it is drawn to the right of the button pair, shows a Hand cursor on hover, toggles the state on click, and ignores the mouse wheel.
  * **Clamped Mouse-Wheel Toggling:** Scrolling up/right over either button selects `1` (`label_on`); scrolling down/left selects `0` (`label_off`).
  * **Per-Button Tooltips (`item_tooltips`) & Wheel-Selection Tooltip Latch (`out_hovered_item`):** When the user spins the mouse wheel over either button to change the state, tooltip targeting (and `*out_hovered_item`) automatically latches onto the newly selected button (`0` or `1`)—even if the stationary mouse cursor is still parked over the unselected button—until the mouse moves (`> 1.5px`).

---

### 2. Mouse-Wheel Sliders, Drag Boxes, Cyclers & Combos (`imgui_mouse_wheel_controls.h`)

#### A. Core Mouse-Wheel Primitives (`ImGui::GetMouseWheel` / `GetMouseWheelV` / `GetMouseWheelH` / `GetMouseWheelEither`)
Queries vertical (`GetMouseWheel` / `GetMouseWheelV`), horizontal (`GetMouseWheelH`), or either-axis (`GetMouseWheelEither`, alias `GetMouseWheelAny`) scroll-wheel deltas for the current frame. Passing `consume = true` zeroes out `io.MouseWheel` / `io.MouseWheelH` so parent scrollable windows do not scroll while adjusting a hovered control.

```cpp
float clicks = ImGui::GetMouseWheelEither(true /* consume event */);
```

#### B. Continuous Range Chaser (`ImGui::AdjustItemOnMouseWheel`)
Attaches to **any** preceding continuous range item (`SliderFloat`, `SliderInt`, `DragFloat`, `DragInt`, etc.) via `ImGui::IsItemHovered()`. Overloaded for both `float*` and `int*`, it interprets vertical/horizontal mouse-wheel clicks, consumes the wheel event, and clamps boundaries.
* **Track-Only Cursor Precision:** Switches the cursor to East-West resize arrows (`ImGuiMouseCursor_ResizeEW` by default) strictly when hovering over the physical control track/box, preserving standard arrow cursors over text labels.

```cpp
#include "contrib/widgets/imgui_mouse_wheel_controls.h"

// Attach mouse-wheel adjustment to any standard ImGui slider or drag box:
ImGui::SliderFloat("Text Zoom", &fontZoom, 0.75f, 1.75f);
ImGui::AdjustItemOnMouseWheel(&fontZoom, 0.75f, 1.75f, 0.01f);

ImGui::DragInt("Altitude", &alt, 10.0f, 0, 40000);
ImGui::AdjustItemOnMouseWheel(&alt, 0, 40000, 100.0f);
```

#### C. Discrete Choice Chaser (`ImGui::CycleItemOnMouseWheel`)
Attaches to **any** preceding widget (`Button`, `Text`, `Combo`, `Selectable`) to step an integer index forward or backward one item per scroll gesture. Overloaded for both `(int* idx, int item_count, ...)` and `(int* idx, int min_idx, int max_idx, ...)`.
* **Clamped by Default (`wrap = false`):** Stops cleanly at the first and last items so a fast flick of the scroll wheel never accidentally wraps from minimum to maximum.
* **Directional Hover Cursor:** Defaults to `ImGuiMouseCursor_ResizeNS` (`↕`) on hover, and can be overridden per-call (e.g., `ImGuiMouseCursor_ResizeEW` `↔` for horizontal button cycling or `ImGuiMouseCursor_Hand`).

```cpp
// Attach wheel-cycling directly to a button with an East-West scroll cursor:
if (ImGui::ButtonLink(kDensityLabels[s_DensityIdx])) {
    ImGui::OpenPopup("##density_menu");
}
if (ImGui::CycleItemOnMouseWheel(&s_DensityIdx, 5 /* item_count */, false /* wrap */,
                                 true /* natural_list_order */, true /* consume */,
                                 ImGuiMouseCursor_ResizeEW)) {
    ApplyDensity(s_DensityIdx);
}
```

#### D. Turnkey Mouse-Wheel Sliders & Drag Boxes (`MouseWheelSlider` / `PercentSlider` / `MouseWheelDrag`)
Drop-in replacements for standard sliders and drag boxes that respond to mouse-wheel scrolling out of the box. `MouseWheelSlider` and `MouseWheelDrag` are overloaded for both `float*` and `int*` so you never have to type `Float` or `Int` in the function name!

```cpp
static float s_Heading = 180.0f;
static int   s_Fov     = 80;
ImGui::MouseWheelSlider("Heading", &s_Heading, 0.0f, 360.0f, "%.0f°", 0, 1.0f);
ImGui::MouseWheelSlider("FOV",     &s_Fov,     40,   120,    "%d°",   0, 1.0f);

static float s_PitchOffset = 0.0f;
ImGui::MouseWheelDrag("Pitch Offset", &s_PitchOffset, 0.5f /* speed & default wheel step */, -45.0f, 45.0f, "%.1f°");

static float s_Opacity = 0.85f;
ImGui::PercentSlider("Window Opacity", &s_Opacity, 0.1f, 1.0f); // (alias: ImGui::SliderPercent)
```

#### E. Composite Sliders & Drag Boxes with Reset (`SliderWithReset` / `PercentSliderWithReset` / `DragWithReset`)
Full-featured compound controls (overloaded for `float*` and `int*`, also aliased as `ResettableSlider`, `ResettablePercentSlider`, and `ResettableDrag`) with built-in mouse-wheel support, a flush abutting vector undo/reset button (`0`-gap seam), smart width reservation, hover isolation, active input lockout, and dual-tooltip support.
* Sits cleanly in a single line: `[ Control Track/Box | ↺ ]  Label Text`
* Automatically dims the reset button when already at default value, and lights up when modified.
* Features an antialiased vector counter-clockwise undo glyph drawn directly in `ImDrawList` (zero font dependencies).

```cpp
static float s_Heading = 180.0f;
static int   s_Fov     = 80;

// 1-line sliders (float* or int*) with built-in mouse-wheel and one-click reset:
ImGui::SliderWithReset("Heading", &s_Heading, 180.0f /* default */, 0.0f, 360.0f, "%.0f°");
ImGui::SliderWithReset("FOV",     &s_Fov,     80     /* default */, 40,   120,    "%d°");

static float s_Opacity = 0.85f;
ImGui::PercentSliderWithReset("Opacity", &s_Opacity, 0.85f /* default */, 0.1f, 1.0f);

static float s_PitchOffset = 0.0f;
ImGui::DragWithReset("Pitch Offset", &s_PitchOffset, 0.0f /* default */, 0.5f /* speed */, -45.0f, 45.0f, "%.1f°");
```

#### F. Turnkey Mouse-Wheel Combos (`MouseWheelCombo` / `ComboWithReset`)
Dropdown combo boxes that work like standard `ImGui::Combo` when clicked, and support live mouse-wheel cycling both when **closed** (hovering over the combo box frame) and while the dropdown popup menu is **open** (hovering over the open list or combo header)!
* **`ComboWithReset`** (also aliased as `ResettableCombo`) pairs the mouse-wheel combo with the flush abutting `↺` reset button to revert to `default_item` in one click.
* **Per-Item Tooltips (`item_tooltips[]`) & Open-Menu Wheel Tooltip Latch ("Lens UI"):** Pass an optional array of tooltip strings (plain text or Markdown when `contrib.h` is active) to attach a distinct tooltip to each item in the open dropdown list. When the user spins the mouse wheel while the dropdown menu is open, tooltip targeting automatically latches onto the **newly wheel-selected item** (`*current_item`) rather than whichever row the stationary mouse cursor happens to be parked over, and seamlessly returns to normal mouse hover targeting on the first movement of the mouse (`> 1.5px`).

```cpp
static const char* const kCameraModes[] = { "Cockpit", "Chase", "Circle", "Tower", "Ride-Along" };
static const char* const kCameraModeTooltips[] = {
    "**Cockpit View**: Inside the 3-D flight deck.",
    "**Chase View**: Locked behind the aircraft.",
    "**Circle View**: Free exterior orbit around the aircraft.",
    "**Tower View**: Stationary camera from the nearest ATC tower.",
    "**Ride-Along**: Wing-mount or fuselage camera position."
};
static int s_CamMode = 1; // Default: Chase (1)

// Standard mouse-wheel-enabled dropdown with per-item tooltips:
ImGui::MouseWheelCombo("Camera View", &s_CamMode, kCameraModes, IM_ARRAYSIZE(kCameraModes),
                       -1, false /* wrap */, kCameraModeTooltips);

// Compound mouse-wheel dropdown with abutting one-click reset to default (1 = Chase):
ImGui::ComboWithReset("Camera Mode", &s_CamMode, 1 /* default */,
                      kCameraModes, IM_ARRAYSIZE(kCameraModes),
                      -1, false /* wrap */, nullptr /* default reset tooltip */,
                      kCameraModeTooltips);
```

---

### 3. Clickable Link & Item Cursor Affordances (`imgui_clickable_links.h`)

In standard Dear ImGui, buttons and clickable elements keep the default OS arrow cursor. The `imgui_clickable_links.h` header eliminates the boilerplate of writing `if (IsItemHovered()) SetMouseCursor(ImGuiMouseCursor_Hand);` on every interactive item.

* **Ambient Color Preservation on `ImGui::TextLink`:** In Dear ImGui 1.91+, the built-in `ImGui::TextLink(const char*)` forces `ImGuiCol_TextLink` (a bright sky-blue tint) onto the text. Our `imgui_clickable_links.h` overloads preserve your **ambient `ImGuiCol_Text` color** by default (or accept an explicit `const ImVec4* text_color` override and `std::string`), while rendering a zero-frame clickable text item with a Hand cursor on hover.

```cpp
#include "contrib/widgets/imgui_clickable_links.h"

// Standard button with hand cursor on hover:
if (ImGui::ButtonLink("Save Flight Plan")) {
    SavePlan();
}

// Clickable text preserving ambient text color (or optional custom color):
if (ImGui::TextLink("A-Better-Camera Settings")) {
    ToggleHeaderMode();
}

// Checkboxes and radio buttons with hand cursor:
ImGui::CheckboxLink("Enable Autothrottle", &s_Autothrottle);
ImGui::RadioButtonLink("NAV 1", &s_NavSource, 1);
```

#### Universal Item Hover-Cursor Chaser (`ImGui::AttachItemCursor`):
```cpp
// Attach default Hand cursor to any preceding item:
ImGui::Image(textureId, ImVec2(64, 64));
ImGui::AttachItemCursor(); // (aliases: ShowItemCursor, SetItemCursor, ShowItemCursorOnHover, ShowLinkCursorOnHover)

// Or attach any specific ImGuiMouseCursor on hover:
ImGui::Button("Splitter");
ImGui::AttachItemCursor(ImGuiMouseCursor_ResizeEW);
```

---

## Credits & Upstream Lineage

* **Author & Architect**: Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))
* **SliderPercent Upstream Inspiration**: Birger Hoppe ([LiveTraffic](https://github.com/bhoppe/LiveTraffic))
* **License**: BSD 3-Clause (see [LICENSE](../../LICENSE))
