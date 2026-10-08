# TimedTooltip Extension

The **TimedTooltip** extension provides an advanced, stationary tooltip engine designed specifically for Dear ImGui within X-Plane plugins. 

Unlike standard Dear ImGui tooltips which move with the mouse and can flicker, obstruct UI elements, or run off-screen, `TimedTooltip` locks in place, calculates dynamic viewport boundaries, preserves custom interactive cursors, isolates its styling hermetically from parent windows, and supports both plain-text and rich Markdown content.

---

## Key Features

* **Configurable Delay & Lifetime:**
  * **Low Watermark (Hover Delay):** Prevents tooltip spam by waiting for the user to pause over a widget before showing.
  * **High Watermark (Auto-Dismiss):** Automatically hides tooltips after a readable duration, with adaptive time-scaling for taller/multi-line text.
  * **Grace Period:** Forgives micro-slips off the target widget so users don't lose their place.
  * **Wiggle Latch:** Micro-mouse movements reset the auto-hide timer so users can continue reading long descriptions.
* **Hermetic Style Isolation:**
  * Pushes and pops its own `WindowPadding`, `ItemSpacing`, `FramePadding`, and `IndentSpacing`. Outer parent window style modifications (such as `ItemSpacing.x = 0` in menu bars or compressed button pads) can never distort tooltip typography, and tooltip styles never leak back out.
  * Precise vertical line pitch (`item_spacing.y = 0.0f`) preserves the exact 15px native font leading in X-Plane, preventing vertical bloat and screen overrun.
* **Smart Bounds Clamping & Docking:**
  * **Upward Growth:** If a multi-line tooltip would clip off the bottom of the screen, it automatically grows upwards, clamping cleanly above the display floor.
  * **Leftward Shift:** If a tooltip approaches the right screen border, it automatically shifts leftwards.
  * **Adaptive Wrap:** Shrink-wraps tightly around concise text while wrapping paragraphs at a readable width (80% of available viewport width by default).
* **Interactive Cursor Protection:**
  * Preserves and restores custom widget cursors (e.g., `ImGuiMouseCursor_Hand`) so hovering interactive elements remains intuitive.
* **Global Mutex:**
  * Automatically suppresses competing tooltips across disparate translation units, child windows, and widget hierarchies.
* **Plain Text & Markdown Support:**
  * Native plain-text via `Text(...)` and rich Markdown via `TextMD(...)`.

---

## Quick Start

### 1. Include the Header
```cpp
#include "contrib/extensions/TimedTooltip/imgui_tooltips_ext.h"
```

### 2. Basic Usage (Plain Text)
Attach a timed tooltip directly after drawing any ImGui widget:

```cpp
if (ImGui::Button("Save Flight Plan")) {
    SavePlan();
}
// Automatically attaches to the preceding widget:
ImGui::TimedTooltip::Text("Saves the active route to disk as an .fms file.");
```

### 3. Markdown Tooltip
If using the companion **Markdown** extension, include `imgui_markdown_ext.h` and call `TextMD`:

```cpp
#include "contrib/extensions/TimedTooltip/imgui_tooltips_ext.h"
#include "contrib/extensions/Markdown/imgui_markdown_ext.h"

ImGui::Button("Autopilot Nav");
ImGui::TimedTooltip::TextMD("Arms **LNAV** mode. *Requires active GPS waypoint.*");
```

---

## Configuration (`ImGui::TimedTooltip::Config`)

The `TooltipConfig` (aliased as `ImGui::TimedTooltip::Config`) controls timing, dimensions, typography, and colors.

### Configuration Fields Reference

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `hover_delay_cycles` | `int` | `65` | Frame cycles the mouse must hover before the tooltip appears (~1.0s at 60fps). |
| `max_display_cycles` | `int` | `504` | Frame cycles before the tooltip auto-dismisses. Set negative to disable auto-hide. |
| `grace_period_cycles`| `int` | `5` | Allowable frame cycles off the widget before cancelling hover state. |
| `size_skew_multiplier`| `float` | `0.5f` | Extra cycles added to display timeout per pixel of vertical height. |
| `enable_wiggle_latch`| `bool` | `true` | When `true`, subtle mouse movements inside the tooltip reset the dismiss timer. |
| `default_wrap_width` | `float` | `360.0f` | Target comfortable sticky-note width (~50-60 chars). Short text shrink-wraps tightly to its natural size. |
| `min_wrap_width`     | `float` | `240.0f` | Minimum width boundary before forced truncation/overflow. |
| `max_wrap_width`     | `float` | `600.0f` | Maximum width ceiling for wide documentation/manuals. |
| `padding`            | `ImVec2` | `(8.0f, 6.0f)` | Internal window padding around the tooltip contents. |
| `item_spacing`       | `ImVec2` | `(4.0f, 0.0f)` | Item spacing between lines. `y = 0.0f` matches native 15px font leading. |
| `frame_padding`      | `ImVec2` | `(2.0f, 1.0f)` | Internal padding for frame elements inside the tooltip. |
| `indent_spacing`     | `float` | `20.0f` | Standard indent spacing between nested list levels. |
| `bullet_spacing`     | `float` | `4.0f` | Gap between bullet glyph and following text. |
| `font_scale`         | `float` | `0.0f` | Window font scale multiplier (`0.0f` = inherit ambient font scale; `>0.0f` = custom scale). |
| `bg_color`           | `ImVec4` | Yellow note | Window and popup background color (`#FFF299`). |
| `border_color`       | `ImVec4` | Dark yellow | Tooltip border outline color (`#CCBF66`). |
| `text_color`         | `ImVec4` | Jet black | Tooltip text and separator color (`#000000`). |

---

## Usage Patterns: Global Setup vs. Per-Call Overrides

### Pattern A: Global Setup (Once at Plugin Startup)
For 95% of use cases, configure your desired tooltip personality once during plugin startup or window creation:

```cpp
// In your plugin initialization:
static ImGui::TimedTooltip::Config s_MyTooltipConfig;
s_MyTooltipConfig.hover_delay_cycles = 45;                           // Snappier hover delay
s_MyTooltipConfig.padding            = ImVec2(8.0f, 6.0f);           // Compact padding
s_MyTooltipConfig.item_spacing       = ImVec2(4.0f, 0.0f);           // Tight line pitch
s_MyTooltipConfig.bg_color           = ImVec4(1.0f, 0.95f, 0.6f, 0.95f); // Sticky note yellow

ImGui::TimedTooltip::SetDefaultTooltipConfig(&s_MyTooltipConfig);
```

Every subsequent call to `ImGui::TimedTooltip::Text(...)` and `ImGui::TimedTooltip::TextMD(...)` will automatically use this configuration without needing any per-call arguments.

### Pattern B: Per-Call Override (Instant & Dynamic)
If a specific widget needs unique styling—for example, a wider maximum width, distinct background color, or custom padding—you can fetch a copy of the active configuration using `ImGui::TimedTooltip::GetConfig()`, tweak the desired fields, and pass it directly to `Text` or `TextMD`:

```cpp
// Safely copies the active global config (or built-in defaults if no global config was set):
ImGui::TimedTooltip::Config wideConfig = ImGui::TimedTooltip::GetConfig();
wideConfig.max_wrap_width = 800.0f;                       // Allow wider manual formatting
wideConfig.bg_color       = ImVec4(0.15f, 0.15f, 0.20f, 0.95f); // Dark HUD styling
wideConfig.text_color     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

ImGui::Button("System Diagnostics");
// Pass custom configuration directly:
ImGui::TimedTooltip::TextMD(&wideConfig, "### System Status\n* Bus 1: **Normal**\n* Bus 2: **Normal**");
```

### Pattern C: Low-Level Custom Layout Blocks
For completely custom tooltip content containing arbitrary ImGui widgets, use `BeginStationaryTooltip` and `EndStationaryTooltip`:

```cpp
ImGui::Button("Engine Monitor");
if (ImGui::TimedTooltip::BeginStationaryTooltip("EngineMonitorTooltip")) {
    ImGui::Text("EGT: 640 C");
    ImGui::ProgressBar(0.75f, ImVec2(150, 15));
    ImGui::TimedTooltip::EndStationaryTooltip("EngineMonitorTooltip");
}
```

---

## Hermetic Style Isolation

In Dear ImGui, child windows and popups can inadvertently inherit active layout styles from their parent windows. If an outer menu bar or custom toolbar has set `ItemSpacing.x = 0.0f` or compressed `FramePadding` to pack buttons tightly, standard tooltips would inherit those distorted spacing values.

`TimedTooltip` isolates itself completely:
1. When opening a tooltip proxy, `BeginStationaryTooltipProxy` pushes:
   * `ImGuiStyleVar_WindowPadding` (`config->padding`)
   * `ImGuiStyleVar_ItemSpacing` (`config->item_spacing`)
   * `ImGuiStyleVar_FramePadding` (`config->frame_padding`)
   * `ImGuiStyleVar_IndentSpacing` (`config->indent_spacing`)
2. When closing, `EndStationaryTooltip` pops all four style variables in exact reverse order.

This guarantees that:
* **No outer window hacks leak inward:** Tooltips always render with crisp, predictable typography regardless of what the calling window modified.
* **No tooltip styles leak outward:** Calling windows retain their exact style state without needing defensive push/pop blocks.

---

## Screen Clamping & Docking

Tooltips automatically check the active display dimensions (`ImGui::GetIO().DisplaySize`):
* **Bottom Edge Protection:** If `render_pos.y + height` exceeds the screen floor, the anchor position automatically shifts upward so the entire tooltip remains visible on screen.
* **Right Edge Protection:** If `render_pos.x + width` exceeds the screen edge, the tooltip shifts leftwards to keep all content in view.

---

## API Summary

```cpp
namespace ImGui {
    namespace TimedTooltip {
        // High-level Plain Text (binds to previous ImGui item):
        void Text(const char* fmt, ...);
        void Text(const TooltipConfig* config, const char* fmt, ...);

        // High-level Markdown (requires imgui_markdown_ext.h):
        void TextMD(const char* fmt, ...);
        void TextMD(const TooltipConfig* config, const char* fmt, ...);

        // Configuration:
        using Config = TooltipConfig;
        using State  = TooltipState;
        void SetDefaultTooltipConfig(const Config* config);
        const Config*& GetDefaultTooltipConfig();
        const Config& GetCurrentTooltipConfig();
        Config GetConfig();

        // Low-level lifecycle controls:
        bool BeginStationaryTooltipProxy(ImGuiID id, bool is_hovered, const Config* cfg = nullptr);
        bool BeginStationaryTooltip(ImGuiID id, const Config* cfg = nullptr);
        bool BeginStationaryTooltip(const char* str_id, const Config* cfg = nullptr);
        void EndStationaryTooltip(ImGuiID id);
        void EndStationaryTooltip(const char* str_id);
        void ClearActiveTooltip();
    }
}
```
