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

// Fine-grained opt-outs & style defaults within imgui_mouse_wheel_controls.h:
#define CONTRIB_DISABLE_GET_MOUSE_WHEEL       // Disables GetMouseWheel* primitives
#define CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL    // Disables AdjustItemOnMouseWheel()
#define CONTRIB_DISABLE_CYCLE_MOUSE_WHEEL     // Disables CycleItemOnMouseWheel()
#define CONTRIB_DISABLE_PERCENT_SLIDER        // Disables PercentSlider() & SliderPercent()
#define CONTRIB_DISABLE_SLIDER_PERCENT        // Disables only the SliderPercent() alias
#define CONTRIB_DISABLE_MOUSE_WHEEL_COMBO     // Disables MouseWheelCombo & ComboWithReset
#define IMGUI_CONTRIB_DISABLE_RESET_UNDO      // Disables 1-click Reset-with-Undo (restore previous value)

// Optional compile-time icon & chevron style customization:
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL      ICON_FA_UNDO     // Custom primary reset icon (default: vector ↺)
#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT  ICON_FA_REDO     // Custom restore/redo icon (default: vector ↻)
#define IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE ImGui::ComboArrowStyle_RightWhenClosed // Combo chevron polarity
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

// 1. Standard switch with clickable label on the right:
ImGui::ToggleButton("Expert Mode", &s_ExpertMode);

// 2. Direct reference syntax (no & required):
ImGui::ToggleButton("Expert Mode", s_ExpertMode);

// 3. Compact switch without visible label (uses ID only):
ImGui::ToggleButton("##hud_switch", &s_HudActive);

// 4. Direct dual-string per-state tooltips (Off tooltip, On tooltip):
ImGui::ToggleButton("HUD Overlay", &s_HudActive,
                    "HUD is currently **OFF**.\n- Click to enable overlay.",
                    "HUD is currently **ON**.\n- Click to disable overlay.");

// 5. Array-based per-state tooltips ({ Off tooltip, On tooltip }):
static const char* const kTransitionTooltips[] = {
    "Smooth transitions are **OFF** (instant snap).\n- Click or scroll **up** on the switch to enable.",
    "Smooth transitions are **ON** (animated).\n- Click or scroll **down** on the switch to disable."
};
ImGui::ToggleButton("Smooth Transitions", &s_SmoothTransitions, ImVec2(0, 0), kTransitionTooltips);

// Optional: Customize switch active track & thumb colors globally:
ImGui::SetDefaultToggleButtonColors(ImVec4(0.26f, 0.59f, 0.98f, 1.0f)); // Active track color
// Or via ImU32:
ImGui::SetDefaultToggleButtonColors(ABCIM_COL32(HILITE_COLOR));
```

* **Features:**
  * Frame-rate independent sliding animation powered by `io.DeltaTime` and stored per-widget in `ImGuiStorage`.
  * **Flexible Types & Syntax:** Accepts `bool*`, `int*`, `bool&`, or `int&` (`0` = Off, non-zero = On).
  * **Global Theme Integration (`SetDefaultToggleButtonColors`):** Easily set the switch track/thumb colors globally via `ImVec4` or `ImU32`. Defaults to your theme's active highlight color (`ImGuiCol_ButtonActive`).
  * **Built-In Clamped Mouse-Wheel Support (`allow_mouse_wheel = true`):** Spinning the scroll wheel up/right over the pill switch turns it **On** and clamps there; spinning down/left turns it **Off** and clamps there (never wraps). Hovering over the text label does *not* capture the mouse wheel, preventing accidental toggles when scrolling windows.
  * **Per-State Tooltips:** Supports both direct dual-string parameters (`off_tooltip`, `on_tooltip`) and 2-element string arrays (`{ off_tooltip, on_tooltip }`) automatically rendered via `TimedTooltip::TextMDProxy` (or standard `SetTooltip` fallback).
  * **Responsive Auto-Scaling:** Switch dimensions scale naturally with the ambient font size (0.78x ambient frame height for standard switches, 0.60x for compact switches), looking crisp on standard, Retina, and 4K displays, with support for explicit `ImVec2 size` overrides.
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
// 1. Attach wheel-cycling to a popup-menu opener button with an East-West scroll cursor:
if (ImGui::ButtonLink(kDensityLabels[s_DensityIdx])) {
    ImGui::OpenPopup("##density_menu");
}
if (ImGui::CycleItemOnMouseWheel(&s_DensityIdx, 5 /* item_count */, false /* wrap */,
                                 true /* natural_list_order */, true /* consume */,
                                 ImGuiMouseCursor_ResizeEW)) {
    ApplyDensity(s_DensityIdx);
}

// 2. Standalone wheel-cycled ButtonLink paired with an abutting ResetButton:
//    Evaluating `(is_mod || can_restore) && static_cast<bool>(btn)` resets/restores on click
//    when actionable, and automatically falls back to click-to-toggle tooltip when idle at default!
const bool is_mod      = (s_DensityIdx != 2);
const bool can_restore = (!is_mod && s_LastDensityIdx != 2);
auto btn = ImGui::ButtonLink(kDensityLabels[s_DensityIdx], ImVec2(110.0f, 0.0f));
bool btn_clicked = (is_mod || can_restore) && static_cast<bool>(btn);
if (ImGui::CycleItemOnMouseWheel(&s_DensityIdx, 5, false, true, true, ImGuiMouseCursor_ResizeEW)) {
    if (s_DensityIdx == 2) s_LastDensityIdx = 2;
}
ImGui::SameLine(0.0f, 0.0f);
bool rst_clicked = ImGui::ResetButton("##density_reset", is_mod, 0.0f, true /* abutting */, nullptr, can_restore);
if (btn_clicked || rst_clicked) {
    if (is_mod) { s_LastDensityIdx = s_DensityIdx; s_DensityIdx = 2; }
    else if (can_restore) { s_DensityIdx = s_LastDensityIdx; }
    ImGui::TimedTooltip::ClearActiveTooltip();
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

#### E. Composite Sliders & Drag Boxes with Reset & Undo (`SliderWithReset` / `PercentSliderWithReset` / `DragWithReset`)
Full-featured compound controls (overloaded for `float*` and `int*`, also aliased as `ResettableSlider`, `ResettablePercentSlider`, and `ResettableDrag`) with built-in mouse-wheel support, a flush abutting vector undo/reset button (`0`-gap seam), smart width reservation, hover isolation, active input lockout, and **Reset-with-Undo** (1-click A/B toggle between default and previous value).
* Sits cleanly in a single line: `[ Control Track/Box | ↺ / ↻ ]  Label Text`
* **"Reset with Undo" (1-Click A/B Comparison):**
  * When modified (`*v != v_default`), clicking the Reset button saves the current non-default value (`last_val`), resets `*v = v_default`, and flips the button into **Restore/Redo Mode** (`↻` or `IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT`).
  * Clicking the button again in Restore mode swaps `*v` right back to `last_val`, letting users click back and forth effortlessly to A/B compare their custom setting against the default!
  * **Automatic Session Memory + Optional Persistent Pointer (`v_last`):** Works automatically with **zero extra arguments** via internal per-ID session memory (`##reset_undo`), or accepts an optional trailing `float* v_last = nullptr` / `int* v_last = nullptr` pointer if you want to persist the pre-reset value in your own variable/dataref across sessions.
* **Automatic 3-State Default Tooltips:** When `reset_tooltip` is `nullptr`, automatically formats the default and previous values using the control's `format` string:
  * **Modified:** `"Click to revert to default (85%)."`
  * **After Reset (Restore Mode):** `"At default (85%). Click to restore previous (42%)."`
  * **At Default (Idle):** `"Already at default (85%)."` (Clicking the dimmed button or right-side label immediately toggles the status/help `TimedTooltip` ON/OFF!)
* **Dual Icon Configuration (`#define` or Runtime Setter) & Vector Fallback:**
  * With zero font configuration, draws crisp, DPI-scaled procedural vector arrows directly in `ImDrawList`: counter-clockwise **`↺`** for Reset, and clockwise **`↻`** for Restore-Previous.
  * Or configure custom FontAwesome / icon-font strings at compile time (`#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL ICON_FA_UNDO` and `#define IMGUI_CONTRIB_DEFAULT_RESET_LABEL_ALT ICON_FA_REDO` / `IMGUI_CONTRIB_DEFAULT_REDO_LABEL`) or at runtime via `ImGui::SetDefaultResetButtonLabel(reset_label, restore_label)`. Button width is automatically sized across both glyphs so the control never shifts by 1px when toggling states.
* **Seamless Abutting Frame Borders:** When abutting an active control (`abutting == true`), `ResetButton` automatically suppresses redundant internal frame borders (`FrameBorderSize > 0.0f`), ensuring the control track and reset button share a single clean outer frame without an unwanted vertical box seam between them.

```cpp
static float s_Heading = 180.0f;
static int   s_Fov     = 80;

// Optional: configure custom Reset & Restore/Redo icons globally (or via #define before #include):
// ImGui::SetDefaultResetButtonLabel(ICON_FA_UNDO, ICON_FA_REDO);

// 1-line sliders (float* or int*) with built-in mouse-wheel and Reset-with-Undo:
ImGui::SliderWithReset("Heading", &s_Heading, 180.0f /* default */, 0.0f, 360.0f, "%.0f°");
ImGui::SliderWithReset("FOV",     &s_Fov,     80     /* default */, 40,   120,    "%d°");

static float s_Opacity = 0.85f;
ImGui::PercentSliderWithReset("Opacity", &s_Opacity, 0.85f /* default */, 0.1f, 1.0f);

static float s_PitchOffset = 0.0f;
ImGui::DragWithReset("Pitch Offset", &s_PitchOffset, 0.0f /* default */, 0.5f /* speed */, -45.0f, 45.0f, "%.1f°");
```

#### F. Turnkey Mouse-Wheel Combos (`MouseWheelCombo` / `ComboWithReset`)
Dropdown combo boxes that work like standard `ImGui::Combo` when clicked, and support live mouse-wheel cycling both when **closed** and while the dropdown popup menu is **open**!
* **Split Preview vs. Chevron `[▼]` Zones:**
  * **Preview Text Zone (`[box_min.x .. box_max.x - GetFrameHeight()]`):** Shows `hover_cursor` (`ImGuiMouseCursor_ResizeNS` by default) and cycles items on mouse-wheel without falsely highlighting the right-hand chevron button.
  * **Chevron Button Zone (`[box_max.x - GetFrameHeight() .. box_max.x]`):** Shows **`ImGuiMouseCursor_Hand`**, highlights strictly when the chevron itself is hovered (or when the popup is open), and **ignores mouse-wheel scrolling** so clicking/hovering the opener button feels like a dedicated dropdown button.
* **Configurable Chevron Arrow Polarity (`ImGui::ComboArrowStyle`):**
  Configure globally via `#define IMGUI_CONTRIB_DEFAULT_COMBO_ARROW_STYLE` or `ImGui::SetDefaultComboArrowStyle(...)`:
  * `ImGui::ComboArrowStyle_DownWhenClosed` (`0`, default): `▼` when closed $\rightarrow$ `▲` when open (or `▼` if the popup flipped upward near the bottom of the screen).
  * `ImGui::ComboArrowStyle_RightWhenClosed` (`1`, ABC disclosure style): `▶` when closed $\rightarrow$ `▼` (or `▲` if opened upward) when open.
  * `ImGui::ComboArrowStyle_UpWhenClosed` (`2`): `▲` when closed $\rightarrow$ `▼` (or `▲` if opened upward) when open.
  * `ImGui::ComboArrowStyle_StaticDown` (`3`): Vanilla ImGui's static `▼`.
* **`ComboWithReset`** (also aliased as `ResettableCombo`) pairs the mouse-wheel combo with the flush abutting `↺` / `↻` reset-with-undo button (with automatic session memory or optional `int* last_item = nullptr` persistence).
* **Per-Item Tooltips (`item_tooltips[]`) & Open-Menu Wheel Tooltip Latch ("Lens UI"):** Pass an optional array of tooltip strings (plain text or Markdown when `contrib.h` is active) to attach a distinct tooltip to each item in the open dropdown list. When the user spins the mouse wheel while the dropdown menu is open, tooltip targeting automatically latches onto the **newly wheel-selected item** (`*current_item`) rather than whichever row the stationary mouse cursor happens to be parked over, and seamlessly returns to normal mouse hover targeting when the mouse moves (`> 6px`).

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

// Optional: use disclosure chevron polarity (▶ when closed -> ▼ when open):
// ImGui::SetDefaultComboArrowStyle(ImGui::ComboArrowStyle_RightWhenClosed);

// Standard mouse-wheel-enabled dropdown with per-item tooltips:
ImGui::MouseWheelCombo("Camera View", &s_CamMode, kCameraModes, IM_ARRAYSIZE(kCameraModes),
                       -1, false /* wrap */, kCameraModeTooltips);

// Compound mouse-wheel dropdown with abutting Reset-with-Undo (1 = Chase):
ImGui::ComboWithReset("Camera Mode", &s_CamMode, 1 /* default */,
                      kCameraModes, IM_ARRAYSIZE(kCameraModes),
                      -1, false /* wrap */, nullptr /* auto 3-state reset/restore tooltip */,
                      kCameraModeTooltips);
```

#### G. Mouse-Wheel Checkboxes (`MouseWheelCheckbox` / `ToggleItemOnMouseWheel` / `CycleItemOnMouseWheel(bool*)`)
Allows standard checkboxes to be toggled with the mouse wheel strictly when hovering over the **square `[✓]` box on the left** (`[box_min.x .. box_min.x + GetFrameHeight()]`), while hovering over the **text label** shows `ImGuiMouseCursor_Hand` (still clickable to toggle) and **ignores** the mouse wheel!
* By default (`wrap = false`), scrolling **up/right** checks the box (`true`) and scrolling **down/left** unchecks it (`false`), clamping cleanly at both ends (or pass `wrap = true` to flip on every wheel notch).
* Includes convenience overloads accepting `ImGuiMouseCursor_` directly after `bool* v` so you can customize or suppress the hover cursor (`ImGuiMouseCursor_Hand` or `ImGuiMouseCursor_None`) without passing intermediate boolean parameters.

```cpp
static bool s_AutoZoom = true;

// Option 1: Single-call MouseWheelCheckbox (with optional cursor override right after &v):
ImGui::MouseWheelCheckbox("Enable Auto-Zoom", &s_AutoZoom);
ImGui::MouseWheelCheckbox("Quiet Mode", &s_QuietMode, ImGuiMouseCursor_Hand);

// Option 2: 1-line chaser attached to standard ImGui::Checkbox:
ImGui::Checkbox("Expert Mode", &s_ExpertMode);
ImGui::ToggleItemOnMouseWheel(&s_ExpertMode, ImGuiMouseCursor_None); // or CycleItemOnMouseWheel(&s_ExpertMode)
```

---

### 3. Clickable Link & Item Cursor Affordances (`imgui_clickable_links.h`)

In standard Dear ImGui, buttons, menu items, and clickable text keep the default OS arrow cursor. The `imgui_clickable_links.h` header eliminates the boilerplate of writing `if (IsItemHovered()) SetMouseCursor(ImGuiMouseCursor_Hand);` on every interactive item, while providing automatic **click-to-toggle tooltip** ergonomics and **open-popup wheel-selection tooltip latching**.

* **Ambient Color Preservation on `ImGui::TextLink`:** In Dear ImGui 1.91+, the built-in `ImGui::TextLink(const char*)` forces `ImGuiCol_TextLink` (a bright sky-blue tint) with an underline. Our `imgui_clickable_links.h` overloads preserve your **ambient `ImGuiCol_Text` color** by default (or accept an explicit `const ImVec4* col_override` and `std::string`), while rendering a zero-frame clickable text item with a Hand cursor on hover (with Dear ImGui's native hyperlink preserved as `ImGui::NativeTextLink`).
* **Smart Click-to-Toggle Tooltips (`ImGui::LinkClickResult`):** `ButtonLink`, `SmallButtonLink`, `TextLink`, `TextLinkUnformatted`, `MenuItemLink`, and `MD::TextLink` return `ImGui::LinkClickResult`:
  * **Tested in `if (...)` (Action Link):** Evaluating `if (ImGui::ButtonLink("Save"))` invokes `operator bool()`, marking the click as handled by your action block and dismissing any open tooltip on click.
  * **Standalone Statement (Info / Help Link):** Calling `ImGui::TextLink("(i) Help");` without testing its return value leaves the click unconsumed, so clicking the widget immediately toggles a trailing `ImGui::TimedTooltip::Text` / `TextMD` ON and OFF (without closing any enclosing `BeginPopup`!).

```cpp
#include "contrib/widgets/imgui_clickable_links.h"

// 1. Action button (tested in `if` -> executes action & dismisses tooltip on click):
if (ImGui::ButtonLink("Save Flight Plan")) {
    SavePlan();
}
ImGui::TimedTooltip::TextMD("Saves the active flight plan to disk.");

// 2. Standalone info link (return value ignored -> clicking toggles tooltip ON/OFF!):
ImGui::TextLinkUnformatted("(i) Works with scroll-wheel!");
ImGui::TimedTooltip::TextMD("Spin the **scroll-wheel** over this menu to step through options.");

// 3. Checkboxes, radio buttons, selectables, and menu items with hand cursor:
ImGui::CheckboxLink("Enable Autothrottle", &s_Autothrottle);
ImGui::RadioButtonLink("NAV 1", &s_NavSource, 1);
ImGui::SelectableLink("Row Item", isSelected);
ImGui::MenuItemLink("Compact Density", nullptr, isSelected);
```

#### Custom `BeginPopup` Menus with Mouse-Wheel Cycling & Per-Item Tooltips
When building custom `ImGui::BeginPopup` menus (with custom titles, separators, or footers), wrap the popup contents in `BeginGroup()` / `EndGroup()`, use `ImGui::MenuItem` (or `ImGui::MenuItemLink` for a Hand cursor on each row) with per-item `TimedTooltip::TextMD` calls, and attach `ImGui::CycleItemOnMouseWheel` after `EndGroup()`:

```cpp
if (ImGui::BeginPopup("#density_menu")) {
    ImGui::BeginGroup();
    if (ImGui::TextLink("Choose tracking list density:")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::Separator();

    for (int i = 4; i >= 0; --i) {
        bool isActive = (densityIdx == i);
        if (ImGui::MenuItem(menus[i], nullptr, isActive)) {
            gUserLinePadding = 4 + 2 * i;
        }
        ImGui::TimedTooltip::TextMD("Switch table spacing to **%s** density.", tipLabels[i]);
    }

    ImGui::Separator();
    ImGui::TextLinkUnformatted("  (i) Works with scroll-wheel!  ");
    ImGui::TimedTooltip::TextMD("Spin your scroll-wheel over this list to step through options.");
    ImGui::EndGroup();

    // Whole-menu mouse-wheel cycling (automatically latches per-item tooltips to `isActive`!):
    if (ImGui::CycleItemOnMouseWheel(&densityIdx, 0, 4, /*wrap=*/false, /*natural_list_order=*/false,
                                     /*consume=*/true, ImGuiMouseCursor_Hand)) {
        gUserLinePadding = 4 + 2 * densityIdx;
    }
    ImGui::EndPopup();
}
```
* **Automatic Wheel-Selection Tooltip Latch:** `ImGui::MenuItem` and `ImGui::MenuItemLink` register each item's `isActive` state. When the user spins the scroll wheel inside the open popup, `TimedTooltip` automatically switches from mouse-hover targeting to showing the tooltip for the **newly wheel-selected item** (`isActive`), returning to normal hover targeting as soon as the mouse moves (`> 6px`).

#### Universal Item Hover-Cursor Chaser (`ImGui::AttachItemCursor`):
```cpp
// Attach default Hand cursor to any preceding item:
ImGui::Image(textureId, ImVec2(64, 64));
ImGui::AttachItemCursor();

// Or attach any specific ImGuiMouseCursor on hover:
ImGui::Button("Splitter");
ImGui::AttachItemCursor(ImGuiMouseCursor_ResizeEW);
```

---

## Credits & Upstream Lineage

* **Author & Architect**: Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))
* **SliderPercent Upstream Inspiration**: Birger Hoppe ([LiveTraffic](https://github.com/bhoppe/LiveTraffic))
* **License**: BSD 3-Clause (see [ImgWindow LICENSE](../../README.md#licensing-note))
