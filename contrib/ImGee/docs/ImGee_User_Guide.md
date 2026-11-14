# ImGee: Avionics & Flight-Sim UI Toolkit for Dear ImGui
## Comprehensive Developer & User Manual (v1.0.0)

---

## 1. What ImGee Does For You (Executive Overview)

Stock `Dear ImGui` is a brilliant immediate-mode canvas engineered primarily for game-engine debuggers and developer overlays. The moment you try to ship a polished end-user application, cockpit instrument panel, or settings dialog with vanilla ImGui, you hit the same wall every developer hits:

1. **Missing Glyphs & Font Atlas Bloat:** Standard font atlases lack arrows, checkmarks, gears, and reset symbols (`↺`, `▲`, `▼`, `✓`, `⚙`, `✈`), rendering ugly missing-glyph boxes (`?`) unless you bake heavy custom icon fonts into VRAM.
2. **Hyperactive, Jittery Tooltips:** Stock `ImGui::SetTooltip()` flashes instantaneously when sweeping the mouse across controls, vibrates with every hand tremor, and clips off-screen near window edges.
3. **Inert Cursors & Scroll-Wheel Blindness:** Buttons and links leave the cursor on a dead `Arrow`; sliders, combos, and toggles ignore the mouse wheel (or worse, hijack window scrolling when closed); and users hesitate to tune settings because there is no 1-click "revert to default with undo."
4. **Flat, Labor-Intensive Text & Tables:** Mixing bold/monospace/colored inline text, status pills, or sortable/resizable tables requires dozens of lines of manual `ImDrawList` math and state boilerplate.

**`ImGee`** solves all of this in **a single `#include`**, with **zero `.cpp` files to compile** and **zero changes to your build system**:

* **Polite Stationary `TimedTooltip`s:** Dwell-verified, stationary sticky-note tooltips with viewport clamping, scroll dismissal, damped zoom scaling, and full Markdown formatting.
* **Tactile Avionics & Desktop Controls:** Mouse-wheel sliders, combos, and directional `SelectPulldown`s with "Lens UI" tooltip latching, 1-click Reset-with-Undo (`↺` / `↻`), 270° rotary dimmer knobs with `OFF` detents, animated rocker switches, non-blocking file dialogs, and active OS cursor affordances (`Hand`, `ResizeNS`, `ResizeEW`).
* **Rich Markdown, GFM Tables & Semantic Typography:** Interactive CommonMark with sortable/resizable GFM tables, async 3-tier image loading (memory, disk, and off-thread HTTP/HTTPS), `ImFontRole` semantic typography, baseline-aligned `StyledText`, status `<pill>` / `<badge>` / `<kbd>` capsules, curated themes (`Charcoal`, `Rusty`), and a zero-font procedural vector icon engine.

### Ecosystem Relationship: `ImGee` vs. `ImgWindow` & `contrib/`
* **`ImGee`** is a **100% generic, standalone-ready Dear ImGui UI toolkit** (architected to live in its own dedicated GitHub repository alongside Lua's **LuiGee** and potential future language bindings). Nothing in ImGee's core requires X-Plane.
* **`ImgWindow`** is the X-Plane C++ windowing and dynamic Panel Graphics (Vulkan/Metal + OpenGL) framework.
* **`ImgWindow/contrib/`** is `ImgWindow`'s general **community third-party container** -- a shared home for optional add-on libraries that complement `ImgWindow` or Dear ImGui. Today, `ImGee` is bundled inside `contrib/ImGee/` (with convenience header `contrib/imgee.h`) so `ImgWindow` users get it out of the box, while remaining cleanly isolated from `ImgWindow` core and any future third-party community packages added to `contrib/`.

```
ImgWindow/
├── ImgWindow.h / .cpp                     # Core X-Plane Window & Panel Graphics Bridge
├── ImgFontAtlas.h / .cpp                  # Shared Multi-Window Font Atlas Service
├── contrib.h                              # ImgWindow Community Umbrella Header (-> contrib/includes.h)
└── contrib/                               # Third-Party & Community Offerings Container
    ├── imgee.h                            # Convenience Forwarder (-> ImGee/imgee.h)
    └── ImGee/                             # The ImGee UI Toolkit (Standalone-Ready Package)
        ├── imgee.h                        # Canonical ImGee Master Header & Version Contract
        ├── extensions.h                   # Subsystem Roll-Up (-> extensions/includes.h)
        ├── widgets.h                      # Subsystem Roll-Up (-> widgets/includes.h)
        ├── imgee-global.h                 # Opt-in Global Scope Aliases (Markdown::, TimedTooltip::)
        ├── LICENSE                        # Open-Source License (BSD 3-Clause)
        ├── docs/                          # ImGee User Guide & UX Design Manifesto (MD + PDF)
        ├── extensions/                    # Flagship Subsystems (TimedTooltip, Markdown, Typography)
        ├── widgets/                       # Immediate-Mode Controls, File Dialogs & Vector Icons
        └── themes/                        # Curated Studio & Cockpit Palettes (Charcoal, Rusty)
```

---

## 2. Structural Architecture: The Two Categories of Content

Every component within ImGee is delivered strictly as a **single, self-contained C++ header file (`.h` only)**. There are zero `.cpp` compilation units to add to your build system, zero static or dynamic link dependencies, and zero order-of-inclusion restrictions. Each header contains its own header guards and self-contained forward declarations; you can `#include` ImGee headers in any order without circular dependency hazards.

ImGee separates its code into two distinct architectural categories:

### 2.1 Flagship Extensions (`extensions/`)
Extensions are feature-complete, heavyweight subsystems that solve deep architectural limitations of stock Dear ImGui. Unlike simple widgets, extensions maintain internal or frame-to-frame state, manage timing delays, parse complex syntax trees, or manage font registries.

ImGee provides **three flagship extensions** that form the core value of the package:
* **`TimedTooltip` (`imgui_tooltip_ext.h`):** Replaces volatile, cursor-following vanilla `ImGui::SetTooltip()` with stationary, delay-timed tooltips featuring intelligent display boundary clamping, cursor avoidance, and low-water scroll dismissal.
* **`Markdown` (`imgui_markdown_ext.h`):** Replaces flat plain ASCII with interactive CommonMark, full GFM tables with tristate column sorting and drag-resizing, parameterized styling tags (`<badge>`, `<pill>`, `<icon>`), and custom vector button faces (`ButtonMD`).
* **`Typography` (`imgui_font_roles.h` & `imgui_styled_text.h`):** A 9-slot semantic font role registry (`ImFontRole`), RAII scoped font guards (`ScopedFontRole`), and the `StyledText` engine for inline baseline-aligned sizing with zero Markdown document overhead.

> [!IMPORTANT]
> **The Flagship Core:** We consider `TimedTooltip`, `Markdown`, and `Typography` the **flagship foundation** of the entire ImGee suite. We strongly believe every flight simulation plugin should use these modern subsystems instead of the stock or open-source versions from which they derive.

### 2.2 Cockpit & Avionics Widgets (`widgets/`) & Curated Themes (`themes/`)
Widgets are lightweight, zero-dependency, immediate-mode flight-deck controls (`imgui_knobs.h`, `imgui_toggle_buttons.h`, `imgui_mouse_wheel_controls.h`, `imgui_badges.h`, `imgui_popup_menus.h`, `imgui_file_dialog.h`, `imgui_spinners.h`, etc.), paired with curated studio and cockpit color palettes (`themes/imgui_styles.h`). They hook directly into Dear ImGui's ambient context (`GImGui`, ID stack, `ImGuiStorage`, and active window draw lists) and live directly in the root `ImGui::` namespace to feel completely native.

* **Completely Independent:** Even if a project chooses not to install or enable any of the three extensions, all widgets in `widgets/` remain 100% usable and self-sufficient.
* **Zero Mandatory State:** Widgets execute strictly within the immediate-mode paradigm, drawing antialiased vector geometry directly to `ImDrawList`.

![Example using many of the ImGee features described herein: In-cockpit scratchpad with live Markdown, interactive toolbar, status annunciators, and vector icons](images/fig_cockpit_scratchpad.png)

---

## 3. Common Design Standards & Ecosystem Philosophy

ImGee follows strict architectural patterns established by the broader Dear ImGui community:

### 3.1 Omar Cornut's Ecosystem Standard (Single Point of Discovery)
In official Dear ImGui extensions and ecosystem projects (such as `implot`, `imgui_club`, and `imnodes`), Omar Cornut explicitly advocates extending `namespace ImGui { ... }`.
* **Discoverability & Autocomplete:** Modern C++ developers rely heavily on IDE autocomplete. When typing `ImGui::`, developers immediately discover every capability available across the toolkit in one place.
* **Zero Cognitive Overhead:** Developers are not forced to memorize artificial root namespaces (such as `extgui::` or `imgee::`) or manage scattered `using namespace` directives.

### 3.2 Second-Level Sub-Namespaces for Subsystems
While extending `namespace ImGui` is essential, placing heavy packages directly into the root risks colliding with future Dear ImGui core functions. ImGee cleanly encapsulates its extensions within **second-level sub-namespaces**:
* `ImGui::TimedTooltip::`
* `ImGui::Markdown::` (with official shorthand alias `ImGui::MD::`)
* `ImGui::Typography::`

Sub-namespaces allow extensions to reuse Dear ImGui's standard vocabulary without ambiguity:
```cpp
ImGui::TimedTooltip::Text("Hover-delayed stationary tooltip.");
ImGui::MD::TextWrapped("Configure your **avionics** below.");
```

### 3.3 Convenient Root Shorthands & Global Scope
* Shorthands like `ImGui::TextMD(...)` and `ImGui::ButtonMD(...)` are exposed in the root `ImGui::` namespace.
* Defining `#define IMGEE_ENABLE_GLOBAL_NAMESPACES` publishes `Markdown::`, `MD::`, and `TimedTooltip::` directly to the global scope for maximum brevity.

### 3.4 Working with Immediate-Mode Widgets in C++
Developers consuming ImGee widgets should expect standard Dear ImGui immediate-mode conventions:
* **Caller-Owned State:** Controls accept pointers to caller-owned primitives (e.g. `float* value`, `bool* open`). ImGee never allocates heap memory for persistent values unless explicitly requested.
* **Ambient Context:** Functions implicitly draw into the currently active `ImGuiWindow` and utilize Dear ImGui's ambient style and font stacks.
* **ID Stack Safety:** Push and Pop IDs (`ImGui::PushID` / `ImGui::PopID`) apply seamlessly across all ImGee controls, rotary knobs, and reset buttons.

### 3.5 The ImGee UX Design Manifesto (Human Factors Standard)
For the full architectural philosophy, ergonomics theory, and interaction rules governing ImGee, see the companion document:
* **[The ImGee UX Design Manifesto](ImGee_UX_Design_Manifesto.md)** (`ImGee_UX_Design_Manifesto.pdf`):
  * **Part I: The Architectural Foundation (The Framework's Promise):**
    * **Flight Deck Cognitive Reality:** Protecting Expert Mode (muscle memory & peripheral vision); eliminating Thinking Mode interruptions.
    * **The Zero-Inert Law:** Mandatory active cursor affordances (`Hand`, `ResizeNS`, `ResizeEW`, `TextInput`) for all interactive elements.
    * **First-Gesture Discoverability:** Natural list ordering (+1 on scroll down) and eliminating initial-gesture false negatives.
    * **Monotonic vs. Categorical Dialectic:** Strict rules for clamping (`wrap = false`) vs. circular wrapping (`wrap = true`).
    * **The Lens UI Principle:** Modal focus latching with > 6 px hysteresis during open-menu wheel navigation.
    * **Container Pass-Through:** Closed pulldowns pass scroll events through to parent scrollable containers.
    * **Stationary, Polite Assistance (TimedTooltips):** Why stock Dear ImGui tooltips fail (instant-flash strobing, mouse-bound jitter, target occlusion) and why stationary, dwell-verified TimedTooltips prevail.
    * **Reversibility & Non-Destructive Defaults:** 1-click restore (`↺`) with instant undo history.
  * **Part II: The Developer's Covenant (Promises to the End User):**
    * The Seven Sacred Promises: Banish legacy tooltips, match data physics, honor natural list order, eliminate ambiguous cursor pixels, prevent scroll traps, offer 1-click restore, and prioritize semantic scanning hierarchy.
  * **Appendix A: Canonical Verification Checklists:** Turnkey compliance checklists for framework architects and downstream plugin developers.

---

## 4. Generic Dear ImGui Core & Multi-Language Bindings

While developed and battle-tested within the X-Plane flight simulation ecosystem, **ImGee core is 100% generic Dear ImGui**. Nothing in the C++ toolkit inherently requires X-Plane or any specific host environment. Any application, game engine, or simulator utilizing Dear ImGui can consume ImGee directly.

### 4.1 LuiGee: Lua Bindings for FlyWithLua
For Lua developers, ImGee's flight-deck capabilities are brought directly into the Lua scripting environment as **LuiGee**.
* **Broad Simulator Support:** Available for ImGui-based windows in FlyWithLua across all X-Plane versions from **X-Plane 11.20 through the latest X-Plane 12+ releases and into the future**.
* **Zero-Boilerplate Scripting:** Exposes cockpit switches, knobs, vector icons, and timed tooltips to Lua developers with idiomatic syntax.

### 4.2 Community & Other Language Bindings
We are actively considering bindings for other environments and programming languages (such as **Python**, **Rust**, and **C#**). If developers in the wider Dear ImGui ecosystem are interested in bringing ImGee to other platforms or developing language wrappers, we warmly welcome community discussions and collaboration!

---

## 5. Integration Models & Include Paths

One of the central design principles of **ImGee** is immediacy: you do not need to initialize subsystems, configure factories, or add `.cpp` files to your build scripts. Everything is configured and ready out of the box in a single `#include`.

### 5.1 Recommended Front Door: Including ImGee Directly
When consuming **ImGee** inside a project that bundles `ImgWindow` as a Git submodule (e.g., in `third-party/ImgWindow/` or `src/ImgWindow/`), include the **ImGee package header** directly:

```cpp
// Recommended when the parent folder of ImgWindow/ is on your -I include path:
#include "ImgWindow/contrib/imgee.h"

// Or if ImgWindow/ itself is on your -I include path (e.g. imgui4xp):
#include "contrib/imgee.h"
```

*(Note: The canonical inner suite header `#include "ImgWindow/contrib/ImGee/imgee.h"` is 100% equivalent. Always use exact casing -- `contrib/imgee.h` or `contrib/ImGee/imgee.h` -- so your code compiles cleanly on case-sensitive Linux filesystems.)*

### 5.2 Standalone ImGee Repository / Submodule Usage
When **ImGee** is submoduled directly from its own standalone Git repository (outside of `ImgWindow`):

```cpp
// Via submodule directory:
#include "ImGee/imgee.h"

// Or if ImGee/ itself is on your -I include path:
#include "imgee.h"
```

### 5.3 A-La-Carte Subsystem Inclusion
ImGee is architecturally modular. If your application only wants the immediate-mode widgets (rotary knobs, sliders, toggles, pulldowns) or only wants the flagship extensions (`TimedTooltip`, `Markdown`, `Typography`), you can include either half independently:

```cpp
// Option A: Include only extensions (TimedTooltip, Markdown, Typography):
#include "ImgWindow/contrib/ImGee/extensions.h"   // (or "ImGee/extensions.h" standalone)

// Option B: Include only cockpit widgets (Knobs, Toggles, Wheel Sliders, Badges, Icons):
#include "ImgWindow/contrib/ImGee/widgets.h"      // (or "ImGee/widgets.h" standalone)
```

### 5.4 About `ImgWindow/contrib.h` (The Community Umbrella Header)
In `ImgWindow` distributions, you may also see `#include "ImgWindow/contrib.h"` (or `#include "contrib.h"`).

What is the difference between `contrib/imgee.h` and `contrib.h`?
* **`contrib/imgee.h`** includes **strictly ImGee** -- nothing else.
* **`contrib.h`** is `ImgWindow`'s general **community container umbrella header**: it pulls in `ImGee` today *plus* any additional third-party community offerings that may be merged into `ImgWindow/contrib/` in the future.

If your goal is specifically to use **ImGee**, `#include "ImgWindow/contrib/imgee.h"` (or `"contrib/imgee.h"`) makes your dependency explicit and future-proof.

### 5.5 Ready by Default Out of the Box
Every capability described in this manual is **100% active and enabled out of the box**:
* **Procedural Vector Icons** resolve automatically without font atlas baking.
* **Stationary Timed Tooltips** eliminate cursor jitter and support rich Markdown out of the box.
* **Cockpit Input Affordances** (sliders with reset/undo buttons, rotary knobs, animated toggles, directional pulldowns) compile cleanly into `ImGui::`.
* **Formatted Markdown, GFM Tables & 3-Tier Images** render interactive documentation with zero external build dependencies (and bridge automatically to `ImgWindow` texture management whenever `ImgWindow.h` is present).

*(For projects with strict binary constraints or symbol conflicts, granular `IMGEE_DISABLE_*` opt-outs are documented in [Appendix A](#appendix-a-preprocessor-opt-out-configuration--build-matrix)).*

---

## 6. Orientation Roadmap & User Manual Directory

To guide developers through the suite, this manual is structured into dedicated, modular reference sections:

* **Part I: Flagship Extensions Suite**
  * **[TimedTooltip Extension](#7-flagship-extension-stationary-timed-tooltips-imgui_tooltip_exth)**: Stationary delayed tooltips, screen-edge clamping, and scroll dismissal.
  * **[Markdown Extension](#8-flagship-extension-rich-markdown-engine--gfm-tables-imgui_markdown_exth)**: CommonMark rendering, GFM tables with tristate sorting and drag-resizing, 3-tier images, and custom tags.
  * **[Typography Extension](#9-foundation-extension-semantic-typography--font-roles-imgui_font_rolesh--imgui_styled_texth)**: `ImFontRole` 9-slot registry, scoped font guards, and baseline-aligned `StyledText`.
* **Part II: Cockpit Widgets & Themes Catalog**
  * **[Procedural Vector Icons](#101-procedural-vector-icon-engine--glyphs-imgui_iconsh)**: Zero-texture vector engine and `IconButton` / `ButtonIcon`.
  * **[Avionics Rotary Knobs](#102-avionics-rotary-knobs--dimmer-dials-imgui_knobsh)**: 270° potentiometers with ticks, readout, 2-step `OFF` detents, and 1-click reset.
  * **[Mouse-Wheel Sliders with Reset](#103-mouse-wheel-sliders--reset-chasers-imgui_mouse_wheel_controlsh)**: Sliders and combos with natural wheel stepping and 1-click Reset-with-Undo.
  * **[Status Badges & Pills](#104-badges-pills--annunciators-imgui_badgesh)**: Badges, pills, named-icon annunciators (`PillNamed`), and `<kbd>` keycaps.
  * **[Tactile Animated Toggles](#105-tactile-animated-toggle-switches-imgui_toggle_buttonsh)**: Cockpit rocker switches with dual-state tooltips.
  * **[Radar Spinners & Countdown Dials](#106-procedural-activity-spinners--countdown-dials-imgui_spinnersh)**: Activity spinners and approach countdown timers.
  * **[Directional Popups & SelectPulldown](#107-directional-popups-item-anchoring--selectpulldown-imgui_popup_menush)**: Directional blossoming popups, headroom queries, and 1-line dataref selectors with Lens UI.
  * **[In-Cockpit File Dialogs & Pickers](#108-in-cockpit-file-dialogs--toolbar-pickers-imgui_file_dialogh)**: Non-blocking VR-safe `OpenFileDialog`, `SaveFileDialog`, and inline `FilePickerCombo`.
  * **[Curated Studio & Cockpit Themes](#109-curated-studio--cockpit-themes-imgui_stylesh)**: Turnkey `Charcoal` and `Rusty` palettes replacing Dear ImGui's stock neon blue.
* **Part III: Reference & Standards**
  * **[Appendix A: Preprocessor Opt-Out Configuration](#appendix-a-preprocessor-opt-out-configuration--build-matrix)**: Granular `IMGEE_DISABLE_*` switches for binary size and symbol isolation.
  * **Companion Standard: [The ImGee UX Design Manifesto](ImGee_UX_Design_Manifesto.md)** (`ImGee_UX_Design_Manifesto.pdf`): Canonical human factors and interaction engineering tenets.

---

## 7. Flagship Extension: Stationary Timed Tooltips (`imgui_tooltip_ext.h`)

Vanilla tooltips follow the cursor frame-by-frame like a bee, creating visual distraction, resetting scroll state, and clipping off-screen when near monitor borders. ImGee elevates Dear ImGui tooltips to be stationary, anchored, and cursor-aware:

```cpp
// 1. Plain text stationary tooltip
ImGui::Button("Flaps 15");
ImGui::TimedTooltip::Text("Takeoff flap detent. Generates additional lift with moderate drag.");

// 2. Full Markdown stationary tooltip
ImGui::Button("GPWS");
ImGui::TimedTooltip::TextMD(
    "### Ground Proximity Warning System\n"
    "* **Terrain Ahead:** Pull up immediately.\n"
    "* **Glideslope:** Deviation exceeds 1.3 dots."
);
```

### Key Tooltip Affordances:
* **Initial Dwell Delay:** Prevents accidental popup flashing while sweeping the mouse across busy cockpit panels.
* **Stationary Anchor:** Freezes in position once visible, allowing effortless, relaxed reading.
* **Smart Screen Clamping:** Automatically nudges inside the simulator or host window boundaries.
* **Wheel-Defeat Dismissal:** Rolling the mouse wheel immediately closes the tooltip so it never blocks scrolling views.
* **Font Scaling & Damped Compression:** Automatically dampens font zoom (using a non-linear curve with a 0.90f floor and 1.65f ceiling) so tooltips stay readable without ballooning into unwieldy screen-filling rectangles on ultra-wide or 4K monitors.

![Stationary Timed Tooltip: Anti-jitter hover tooltip with dwell timeout and embedded status pills](images/fig_timed_tooltips.png)

---

## 8. Flagship Extension: Rich Markdown Engine & GFM Tables (`imgui_markdown_ext.h`)

The ImGee Markdown engine transforms plain text strings into fully interactive cockpit documentation:

```cpp
const char* doc = R"(
# Flight Operations Manual
The **Autopilot Flight Director System** (AFDS) manages aircraft trajectory.

### Status Annunciations
| Mode | Status | Armed | Minimum Speed |
| :--- | :---: | :---: | :--- |
| **LNAV** | <badge-success>ENGAGED</badge-success> | Yes | `V2 + 15` |
| **VNAV** | <badge-warning>ARMED</badge-warning> | Yes | `250 KTS` |
| **ILS** | <badge>STANDBY</badge> | No | `VREF + 5` |

> [!NOTE]
> Ensure barometric altimeters are set to standard pressure (29.92 IN HG / 1013 HPA) when passing transition altitude.
)";

ImGui::Markdown::Text(doc);
```

### Key Markdown Affordances:
* **Tristate Column Sorting:** Click any column header to cycle Ascending → Descending → Default sort.
* **Interactive Drag Sizing:** Adjust column dividers directly with instant mouse feedback.
* **Persistent Presets:** Save and restore column widths via inline comment tags (`<!-- table-state: ... -->`).
* **Code Block Copy Buttons:** Shaded code blocks automatically include one-click clipboard copying with 1.5s visual feedback.

![GFM Markdown Tables: Resizable, sortable avionics tables with inline status badges](images/fig_gfm_tables.png)

![Fenced Code Containers: Shaded charcoal blocks with language chips and 1-click clipboard copy](images/fig_code_blocks.png)

### 8.1 Declarative Semantic Style Tags & Macro Aliases

Rather than writing verbose C++ code or nesting raw push/pop color commands, developers can define custom semantic Markdown tags dynamically during plugin startup:

#### 1. Semantic Style Tags (`RegisterMarkdownStyleTag`)
Map custom tags to composite typography styles combining colors, weights, fonts, and background capsules:

```cpp
// Register declarative styles during plugin initialization:
ImGui::RegisterMarkdownStyleTag("cam", "green bold mono");
ImGui::RegisterMarkdownStyleTag("viewmode", "mono bg=#88888840");
ImGui::RegisterMarkdownStyleTag("alert", "bg=#ff4444 fg=#ffffff bold");
```

Use them seamlessly inside any Markdown text:
```cpp
ImGui::MD::Text("Set camera to <cam>Heading</cam> mode or switch to <viewmode>Circle view</viewmode>.");
```

#### 2. Macro Template Tag Aliases (`RegisterMarkdownTagAlias`)
Register reusable tag macros that expand into arbitrary Markdown syntax with `%s` placeholder substitution:

```cpp
// Register tag templates:
ImGui::RegisterMarkdownTagAlias("opt", "<pill #444444 fg=#ffffff>%s</pill>");
ImGui::RegisterMarkdownTagAlias("callsign", "**`%s`**");
ImGui::RegisterMarkdownTagAlias("hotkey", "<kbd>%s</kbd>");
```

When evaluated, `<opt>Circle view</opt>` expands into `<pill #444444 fg=#ffffff>Circle view</pill>`, complete with background capsule geometry and baseline alignment.

#### 3. Hex Alpha Transparency & Auto-Contrast
* **Alpha Channels:** Color specifications support 4-digit (`#RGBA`) and 8-digit (`#RRGGBBAA`) hex codes (e.g. `bg=#88888840` or `bg=#8884` for a 25% translucent charcoal pill).
* **Auto-Contrast:** In capsule tags (`<badge>`, `<pill>`), bare color tokens default to the background color (`bg_color`), and the text foreground color is automatically chosen based on perceived luminance (white text on dark capsules, dark text on bright capsules). Explicit `fg=` or `text=` overrides this auto-calculation when desired.
* **Font Role Integration:** Style tags automatically pull loaded fonts from the `ImFontRole` registry:
  * `mono` / `monospace` → Slot 5 (`Monospace`) with fallback to Slot 7 (`MonospaceMedium`)
  * `bold` → Slot 3 (`Bold`) with fallback to Slot 4 (`BoldLarge`)
  * `italic` → Slot 6 (`Italic`) with fallback to Slot 8 (`BoldItalic`)

---

## 9. Foundation Extension: Semantic Typography & Font Roles (`imgui_font_roles.h` & `imgui_styled_text.h`)

Cockpit user interfaces require strict typographic hierarchy: primary instruments need large, legible digits; captions and annunciator legends require micro-typography; and datarefs require clear monospaced alignment.

ImGee provides two complementary typography systems:
1. **Semantic Font Roles (`ImGui::ImFontRole`):** A 9-slot canonical font registry (Slots 0..8, plus semantic `Heading1`..`Heading3` and `Tiny` roles) linking logical design roles to loaded rasterized fonts with automatic 4-tier fallbacks.
2. **Inline Styled Text Engine (`ImGui::StyledText`):** Lightweight XML-style tag parsing with baseline alignment, formatted variadic `printf` support, and zero Markdown document overhead.

### 9.1 The `ImGui::ImFontRole` Registry

```cpp
namespace ImGui {
    enum class ImFontRole {
        Regular = 0,        // Slot 0: Primary interface font (e.g. 14px)
        Title,              // Slot 1: Section headers, dialog titles (e.g. 16px)
        Small,              // Slot 2: Captions, annotations, secondary subtext (e.g. 12px)
        Bold,               // Slot 3: Strong emphasis, table headers (e.g. Bold 14px)
        BoldLarge,          // Slot 4: Hero banners, major readouts (e.g. Bold 16px)
        Monospace,          // Slot 5: Datarefs, callsigns, frequencies (e.g. Mono 14px)
        Italic,             // Slot 6: Citations, hints, metadata (e.g. Italic 14px)
        MonospaceMedium,    // Slot 7: Numeric avionics readouts, dials (e.g. Mono Medium 15px)
        BoldItalic,         // Slot 8: Strong soft emphasis (e.g. Bold-Italic 14px)
        Heading1,           // Markdown H1 banner title (falls back to BoldLarge -> Title)
        Heading2,           // Markdown H2 section header (falls back to Title -> Bold)
        Heading3,           // Markdown H3 subsection header (falls back to Bold)
        Tiny,               // Micro subtext, footnotes (falls back to Small)
        Count
    };
}
```

Fonts loaded into slots `0..8` of `ImGui::GetIO().Fonts` map automatically, or you can bind any `ImFont*` explicitly via `ImGui::SetFontRole(...)` and apply roles safely with RAII `ImGui::ScopedFontRole`:

```cpp
// Register explicitly during initialization:
ImGui::SetFontRole(ImGui::ImFontRole::MonospaceMedium, avionicsMonoFont);

// Push & pop safely via RAII guard during rendering:
{
    ImGui::ScopedFontRole guard(ImGui::ImFontRole::Title);
    ImGui::TextUnformatted("Flight Management System");
}
```

### 9.2 Inline Styled Text (`ImGui::StyledText`)
When you want rich typography without building a full CommonMark AST, `ImGui::StyledText` parses XML-style tags (`<b>`, `<i>`, `<mono>`, `<huge>`, `<small>`, `<color>`, `<badge>`, `<pill>`) inline with continuous baseline alignment.

It supports full `printf`-style formatted variadic arguments out of the box with zero runtime boilerplate:

```cpp
// 1. Variadic formatted text:
ImGui::StyledText("ALT: <huge>%d</huge> <small>%s</small> | SPD: <mono>%.1f</mono> <small>KTS</small>",
                  altitude, altUnits, airspeed);

// 2. Dynamic status pills with computed values:
ImGui::StyledText("Status: <pill #2ecc71>%s</pill> | VOR: <mono>%s</mono>",
                  statusName, identStr);

// 3. Unformatted pointer pairs:
ImGui::StyledTextUnformatted(bufStart, bufEnd);
```

---

## 10. Cockpit & Avionics Widgets Showcase

Standard Dear ImGui controls are built for developer tools and debug menus. ImGee extends ImGui with a complete suite of physical, tactile avionics widgets designed for real cockpit ergonomics.

### 10.1 Procedural Vector Icon Engine & Glyphs (`imgui_icons.h`)

Standard Dear ImGui plugins often suffer from missing glyphs when attempting to display symbols like reset arrows (`↺`), sort carets (`▲`, `▼`), checkmarks (`✓`), gears (`⚙`), or airplanes (`✈`). 

ImGee includes a **zero-texture, zero-font procedural vector engine** (`imgui_icons.h`). It submits antialiased geometric primitives (lines, arcs, bezier curves, triangles) directly to Dear ImGui's active `ImDrawList`.

```
┌─────────────────────────────────────────────────────────────┐
│                 ImGui::Icon("name", size)                   │
└──────────────────────────────┬──────────────────────────────┘
                               │
                ┌──────────────┴──────────────┐
                ▼                             ▼
       Has Font / Texture?          Missing from Font?
                │                             │
    ┌───────────┴───────────┐                 │
    ▼                       ▼                 ▼
┌─────────────────┐     ┌─────────────────┐ ┌─────────────────┐
│ Custom Texture  │     │ Icon Font Glyph │ │ Vector Engine   │
│ (Memory Image)  │     │ (FontAtlas)     │ │ (100% Batched   │
│                 │     │                 │ │  ImDrawList)    │
└─────────────────┘     └─────────────────┘ └─────────────────┘
```

#### Vector Push-Buttons: `IconButton` & `ButtonIcon`
ImGee provides turnkey button helpers that render procedural vector icons without font dependencies:

```cpp
// 1. Square Icon-Only Button with native hand cursor and tooltip
if (ImGui::IconButton("##ResetHdg", "undo", ImVec2(0, 0), "Reset Heading to 360°")) {
    s_Heading = 360.0f;
}

// 2. Icon + Text Button (Icon on Left)
if (ImGui::ButtonIcon("Settings", "gear")) {
    OpenSettingsDialog();
}

// 3. Text + Icon Button (Icon on Right)
if (ImGui::ButtonIcon("Next Waypoint", "caret-right", ImVec2(160, 0), /*icon_on_right=*/true)) {
    AdvanceWaypoint();
}
```

![Procedural Vector Icons: Aircraft enroute symbols, checks, gears, and Unicode rescues in live telemetry](images/fig_vector_icons.png)

---

### 10.2 Avionics Rotary Knobs & Dimmer Dials (`imgui_knobs.h`)
Standard sliders feel unnatural for cockpit instruments. `ImGui::Knob` implements an avionics-grade potentiometer:

```
          . - ~ ~ - .
      . '    .---.    ' .
    /       /     \       \
   :       |   *   |       :   <- 270° "Omega" Arc
    \       \     /       /       with Open Deadband at 6 o'clock
      . '    '---'    ' .
          ' -  *  - '
             OFF
```

#### Features:
1. **270-Degree "Omega" Arc:** An open deadband at 6 o'clock immediately communicates an interactive dial.
2. **Tactile Two-Step Detente Switch:** Counter-clockwise rotation dims smoothly down to minimum operating value (e.g., 5%). A deliberate extra mouse-wheel click or drag snaps across the detente gap into the powered-down `"OFF"` state. Rotating clockwise from `"OFF"` snaps instantly back to minimum active power.
3. **Scroll-Wheel Primary:** Hover and roll the wheel with fine-tuning modifiers (`Shift` for 10x, `Alt`/`Ctrl` for 0.1x).

```cpp
static float s_InstrumentLight = 0.75f;

// 1. Standard Knob with perimeter ticks and value readout below:
ImGui::Knob("Panel Light", &s_InstrumentLight, 0.0f, 1.0f, "%.2f",
            0.0f /* auto radius */, 0.01f /* wheel step */,
            ImGuiKnobFlags_ShowTicks | ImGuiKnobFlags_ValueBelow);

// 2. Composite Knob with 1-Click Reset Chaser (default comes right after &value):
static const float s_DefaultLight = 0.80f;
ImGui::KnobWithReset("Backlight", &s_InstrumentLight, s_DefaultLight,
                     0.0f, 1.0f, "%.0f%%");
```

![Avionics Rotary Knobs: 270-degree potentiometers with tick marks, readout, and 1-click reset](images/fig_rotary_knobs.png)

---

### 10.3 Mouse-Wheel Sliders & Reset Chasers (`imgui_mouse_wheel_controls.h`)

Settings tweaking in flight simulations is fraught with user anxiety: if an author tweaks an aircraft trim or graphic setting, they fear losing their calibrated value.

ImGee solves this with **Inline 1-Click Restore-to-Default Chasers** (with automatic 1-click Undo/Redo A/B comparison):

```cpp
static float s_Gamma = 2.2f;
static const float s_DefaultGamma = 2.2f;

// Renders slider with natural mouse-wheel stepping and an abutting ↺ / ↻ button.
// Clicking resets to s_DefaultGamma; clicking again swaps back to your previous value!
ImGui::SliderWithReset("Display Gamma", &s_Gamma, s_DefaultGamma, 1.0f, 3.0f, "%.2f");
```

![Sliders with Reset & Wheel Stepping: Heading, altitude, and vertical speed sliders with hover tooltip](images/fig_sliders_reset.png)

---

### 10.4 Badges, Pills & Annunciators (`imgui_badges.h`)

Avionics interfaces require prominent visual annunciations for modes, cautions, and operational states:

```cpp
// 1. Standard Status Pill (auto-selects high-contrast dark/light foreground text)
ImGui::Pill("ONLINE", ImVec4(0.1f, 0.8f, 0.3f, 1.0f));

// 2. Icon + Text Capsule or Unary Annunciator (using vector icon name or registered texture)
ImGui::PillNamed("plane", "AP ENG", ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
ImGui::SameLine();
ImGui::PillNamed("warning", ImVec4(0.9f, 0.3f, 0.2f, 1.0f)); // Icon-only unary capsule

// 3. Keyboard Shortcut Keycap
ImGui::Text("AP Disconnect:");
ImGui::SameLine();
ImGui::Kbd("Z");
```

![Status Pills and Badges: Modern status indicators, annunciators, and keycaps](images/fig_pills_and_badges.png)

---

### 10.5 Tactile Animated Toggle Switches (`imgui_toggle_buttons.h`)

Replaces desktop form checkboxes with physical cockpit toggles featuring smooth thumb slide animations:

```cpp
static bool s_StrobeLights = true;
if (ImGui::ToggleButton("Strobe Lights", &s_StrobeLights)) {
    UpdateElectricalBus();
}
```

![Tactile Animated Toggle Switches: Dual-state rocker toggles with inline status pill](images/fig_toggle_switches.png)

---

### 10.6 Procedural Activity Spinners & Countdown Dials (`imgui_spinners.h`)

Procedural radar sweeps and radial timers for approach fixes:

```cpp
// 1. Radar Spinner (Indicates active datalink connection)
ImGui::Spinner("##RadarSweep", 12.0f, 2.5f, IM_COL32(59, 130, 246, 255));

// 2. Radial Timer (Approach countdown)
static float s_LegFraction = 0.65f;
ImGui::RadialTimer("Approach Leg", s_LegFraction, 14.0f, 3.0f, IM_COL32(234, 179, 8, 255));
```

![Procedural Activity Spinners: Radar spinners, fuel burn gauges, and approach countdown timers](images/fig_spinners_and_dials.png)

### 10.7 Directional Popups, Item Anchoring & SelectPulldown (`imgui_popup_menus.h`)

Provides precision item-anchored popup placement, upward blossoming for bottom-docked status bars, and compact 1-line dataref selectors with Lens UI mouse-wheel interaction:

* **Item-Anchored Popups (`OpenPopupAtItem` & `BeginAnchoredPopup`)**:
  Positions dropdowns relative to the preceding item's bounding box (`GetItemRectMin`/`Max`). Supports `ImGuiPopupAnchor_BelowItem` (standard dropdown) or `ImGuiPopupAnchor_AboveItem` (blossoms upward for bottom bars and docks) with font-scale invariance and muscle-memory toggle protection.
* **1-Line Dataref Selector (`SelectPulldown` / `SelectPulldownMD`)**:
  - **Scroll Pass-Through When Closed**: Ignores mouse-wheel events while closed so parent scrollable windows never accidentally change settings during scroll navigation.
  - **"Lens UI" Selection-Latched Tooltips**: When spinning the mouse wheel inside an open pulldown menu, per-item tooltips dynamically latch onto the newly selected item (`*current_item`) instead of remaining frozen over whichever row the stationary cursor rests on. Deliberate mouse movement (`> 6 px`) cleanly breaks the latch and reverts to standard spatial hover.
  - **Predictable Wheel Stepping & Circular Wrapping**: Supports optional `bool wrap = false` and `bool natural_list_order = true` parameters. When `wrap = true`, spinning past boundaries wraps smoothly; `natural_list_order = true` increments down the list on downward wheel rotation (`clicks < 0.0f`).
  - **Automatic Focus & Scroll-Into-View**: Navigated items receive `SetItemDefaultFocus()` and scroll into view (`SetScrollHereY()`) in tall menus.
  - **Dynamic Cursor Cues**: Hand cursor (`ImGuiMouseCursor_Hand`) on trigger button, resize cursor (`ImGuiMouseCursor_ResizeNS`) over open options.
  - **Rich Markdown Tooltip Aliases**: Call `ImGui::SelectPulldownMD(...)` or `ImGui::MD::SelectPulldown(...)` with rich Markdown tooltips (`TimedTooltip::MD::TextProxy`).

```cpp
// 1. Standard Downward Pulldown with Lens UI tooltips and circular wrap:
static int s_SelectedCam = 1;
const char* const kCamModes[] = { "Circle", "Spot", "Tower", "Cockpit" };
const char* const kCamTips[]  = { "**Circle**\nFollow aircraft", "**Spot**\nFixed distance chase", "**Tower**\nAirport tower view", "**Cockpit**\nPilot 3D cockpit" };
if (ImGui::SelectPulldownMD("##CameraMode", &s_SelectedCam, kCamModes, 4, kCamTips, 130.0f, ImGuiPopupAnchor_BelowItem, ImGuiPopupAlign_Left, true /* wrap */)) {
    UpdateCamera(s_SelectedCam);
}

// 2. Upward Blossoming Pulldown (Status Bars & Bottom Docks):
static int s_PowerMode = 0;
const char* const kPowerModes[] = { "Battery", "APU", "External Power" };
if (ImGui::SelectPulldown("##PowerMode", &s_PowerMode, kPowerModes, 3, nullptr, 140.0f, ImGuiPopupAnchor_AboveItem)) {
    UpdatePowerSource(s_PowerMode);
}
```

---

### 10.8 In-Cockpit File Dialogs & Toolbar Pickers (`imgui_file_dialog.h`)

Native OS file dialogs block the main render thread, stall Vulkan/Metal drivers, and are invisible inside VR headsets. ImGee provides pure C++11, non-blocking, in-viewport file browsers using native POSIX (`opendir`/`readdir`) and Win32 (`FindFirstFileW`) APIs with zero shell/console spawning:

```cpp
// 1. Compact inline toolbar file switcher:
static std::string s_ActiveChecklist = "b738_normal.md";
if (ImGui::FilePickerCombo("Checklist", &s_ActiveChecklist, "Resources/checklists", ".md;.txt")) {
    LoadChecklist(s_ActiveChecklist);
}

// 2. Full in-window Open / Save modals (with breadcrumb bar, search filter & overwrite guard):
static bool s_ShowOpen = false;
static std::string s_ChosenPath;
if (ImGui::Button("Load Flight Plan...")) s_ShowOpen = true;
if (ImGui::OpenFileDialog("Select Flight Plan", &s_ShowOpen, &s_ChosenPath, "Output/FMS plans", ".fms;.pln")) {
    ImportFlightPlan(s_ChosenPath);
}
```

---

### 10.9 Curated Studio & Cockpit Themes (`themes/imgui_styles.h`)

Stock `ImGui::StyleColorsDark()` uses a harsh 2015 electric-blue developer palette. ImGee includes refined, high-contrast palettes engineered for legible cockpit typography:

```cpp
ImGui::ApplyDefaultTheme();   // Applies Charcoal Slate (canonical ImGee default)
ImGui::StyleColorsCharcoal(); // Understated studio graphite & cool platinum accents
ImGui::StyleColorsRusty();    // Warm aviation bronze / terracotta with ivory text
```

---

## Appendix A: Preprocessor Opt-Out Configuration & Build Matrix

> [!NOTE]
> **Zero-Overhead Opt-In Architecture:** Core `ImgWindow` never includes `ImGee` (or anything in `contrib/`). If your project only needs standard windowing, simply `#include "ImgWindow.h"` -- zero ImGee code is compiled into your binary, and no opt-out defines are necessary.
>
> The `IMGEE_DISABLE_*` directives below are only needed if you include ImGee (`#include "ImgWindow/contrib/imgee.h"`, `#include "ImGee/imgee.h"`, or `#include "ImgWindow/contrib.h"`) and wish to selectively exclude specific subsystems or widgets.

### A.1 Master Suite Toggle
| Preprocessor Define | Scope | Description |
| :--- | :--- | :--- |
| `IMGEE_DISABLE_ALL` | Suite | Excludes the entire ImGee suite when including `imgee.h` or umbrella headers. |

### A.2 Subsystem & Feature Opt-Outs
| Macro Directive | Description |
| :--- | :--- |
| `IMGEE_DISABLE_EXTENSIONS` | Excludes `Typography`, `Markdown`, and `TimedTooltip`. |
| `IMGEE_DISABLE_WIDGETS` | Excludes all immediate-mode widgets in `widgets/`. |
| `IMGEE_DISABLE_THEMES` | Excludes `StyleColorsCharcoal` / `StyleColorsRusty` in `themes/`. |
| `IMGEE_DISABLE_TIMED_TOOLTIP` | Excludes stationary delayed tooltip engine. |
| `IMGEE_DISABLE_MARKDOWN` | Excludes Markdown parser, GFM tables, and callouts. |
| `IMGEE_DISABLE_TYPOGRAPHY` | Excludes `ImFontRole` and `ImGui::StyledText`. |
| `IMGEE_DISABLE_ICONS` | Excludes procedural vector icon rendering engine. |
| `IMGEE_DISABLE_ICON_BUTTON` | Excludes `IconButton` and `ButtonIcon` push buttons. |
| `IMGEE_DISABLE_KNOBS` | Excludes avionics rotary knobs, dials, and detents. |
| `IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS` | Excludes wheel sliders, drags, combos, and reset buttons. |
| `IMGEE_DISABLE_BADGES` | Excludes status badges, pills, `PillNamed`, and keycaps. |
| `IMGEE_DISABLE_POPUP_MENUS` | Excludes directional anchored popups (`AboveItem`) and `SelectPulldown`. |
| `IMGEE_DISABLE_TOGGLE_BUTTONS` | Excludes animated physical toggle switches. |
| `IMGEE_DISABLE_CLICKABLE_LINKS` | Excludes OS hand cursor links (`ButtonLink`, `TextLink`). |
| `IMGEE_DISABLE_COPY_BUTTON` | Excludes `CopyButton` and `CopyLastItemButton`. |
| `IMGEE_DISABLE_FILE_DIALOG` | Excludes `FilePickerCombo`, `OpenFileDialog`, and `SaveFileDialog`. |
| `IMGEE_DISABLE_SPINNERS` | Excludes activity spinners and countdown timer dials. |
| `IMGEE_DISABLE_STB_IMAGE` | Disables ImGee's isolated built-in `stb_image` decoder. |
| `IMGEE_EXTERNAL_STB_IMAGE` | Optional compile-speed optimization: skips inline decoder compilation so you can compile `imgee_stb_image.cpp` once in large multi-TU projects. |
| `IMGEE_ENABLE_GLOBAL_NAMESPACES` | Publishes `Markdown::`, `MD::`, and `TimedTooltip::` to global scope. |

> [!NOTE]
> **Backward-Compatible Alias Bridge:** For projects written against earlier pre-v1.0 builds, legacy `CONTRIB_*` switches are automatically mapped to their canonical `IMGEE_*` counterparts with zero build warnings.

---
*Document Version: 1.0.0 • Author: Steven L. Goldberg (@slgoldberg) and the Rogers • ImGee Suite*
