# ImGui Extensions

This directory contains standalone, feature-complete **Extensions** (Packages) for Dear ImGui within ImgWindow.

---

## What is an Extension?

In ImgWindow, an **Extension** is a self-contained package providing substantial functionality beyond standard UI widgets. Extensions typically:
* Maintain internal or persistent state between frames.
* Require custom configuration structs or setup steps.
* Encapsulate complex algorithms (e.g., text parsing, layout pre-calculation, delayed timers).
* May integrate or adapt third-party libraries.

### Namespacing Philosophy: Omar Cornut's Standard & Sub-Namespaces

A common question from newcomers is: *“Why not invent a separate root namespace like `extgui::`, `eximgui::`, or `extimgui::` to distinguish extension code?”*

The answer lies in **Dear ImGui's official architectural philosophy**:

#### 1. Omar Cornut's Ecosystem Standard (Single Point of Discovery)
In official Dear ImGui extensions and ecosystem projects (such as `implot`, `imgui_club`, and `imnodes`), Omar Cornut explicitly advocates extending `namespace ImGui { ... }`. 
* **Autocomplete & Discoverability:** Modern C++ developers rely heavily on IDE autocomplete. When a developer types `ImGui::`, they expect to discover every capability available within their GUI framework in one place.
* **Zero Cognitive Overhead:** Creating detached root namespaces (`extgui::`) forces developers to remember which feature lives in which foreign namespace, requires scattered `using namespace` declarations, and creates awkward syntax when passing native ImGui types across boundaries.

#### 2. The Power of Second-Level Sub-Namespaces
While extending `namespace ImGui` is essential, dumping heavy packages directly into the root namespace risks colliding with current or future Dear ImGui core functions. 

The elegant solution is **second-level sub-namespaces** (`ImGui::<PackageName>::`):

```cpp
namespace ImGui {
    namespace <PackageName> {
        // Functions mirror familiar ImGui patterns
    }
}
```

* **Zero Root Pollution:** The root `ImGui::` namespace stays clean, pristine, and safe from symbol collisions.
* **Mirroring Native ImGui Vocabulary:** Sub-namespaces allow us to reuse Dear ImGui's standard, battle-tested vocabulary (`Text`, `TextWrapped`, `CalcTextSize`) rather than inventing arbitrary or clunky new verbs (like `DrawMarkdownText` or `CalculateMarkdownDimensions`):
  ```cpp
  // Natural, native-feeling API design:
  ImGui::Markdown::TextWrapped("Configure your **settings** below.");
  ImGui::MD::TextWrapped("Configure your **settings** below."); // Zero-overhead shorthand alias!
  ImVec2 size = ImGui::Markdown::CalcTextSize(id, 1, docString, wrapWidth);

  // Bidirectional cross-extension bridges:
  ImGui::TimedTooltip::Markdown::Text("Current mode: *%s*", modeStr);
  ImGui::TimedTooltip::MD::Text("Current mode: *%s*", modeStr);
  ```

#### 3. Eradicating the Upstream Symbol Collision
In upstream `imgui_markdown.h`, the third-party library defined a free C-function `void ImGui::Markdown(...)`. In C++, having a function named `Markdown` directly in `namespace ImGui` makes declaring `namespace ImGui { namespace Markdown { ... } }` a fatal compiler symbol error.

In `ImgWindow`, we eliminated this trap by relocating upstream's raw rendering pipeline to:
```cpp
ImGui::Markdown::Render(markdown_text, length, config);
```
This frees up `namespace ImGui::Markdown` to be a pristine, first-class C++ namespace, prevents developers from accidentally invoking the raw unconfigured parser, and provides `namespace ImGui { namespace MD = Markdown; }` as an official shorthand!

#### 4. The 3 Tiers of Access (From Strict Scoping to Zero Boilerplate)
1. **Canonical**: `ImGui::Markdown::Text(...)` and `ImGui::TimedTooltip::Text(...)`
2. **Shorthands & Root Aliases**: `ImGui::MD::Text(...)`, `ImGui::TextMD(...)`, and `ImGui::TextMarkdown(...)` directly in root `ImGui::`.
3. **Opt-In Global Convenience**: Include [`contrib/contrib-global.h`](../contrib-global.h) (or define `#define CONTRIB_ENABLE_GLOBAL_NAMESPACES`) to use `Markdown::Text(...)`, `MD::Text(...)`, and `TimedTooltip::Text(...)` directly in the global scope.

---

## Current Extensions

| Extension | Directory | Canonical Namespace | Shorthand Alias | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Markdown** | [`Markdown/`](Markdown/) | `ImGui::Markdown::` | `ImGui::MD::` | Markdown parser and renderer based on `enkisoftware/imgui_markdown`, enhanced with dynamic font scaling, token lookahead, true hanging indents, and rich parameterized styling tags (`<color>`, `<backdrop>`, `<badge>`). |
| **TimedTooltip** | [`TimedTooltip/`](TimedTooltip/) | `ImGui::TimedTooltip::` | *(Root & Bridge)* | Advanced stationary and delayed tooltip engine with automatic bounding box pre-calculation, viewport edge-clamping, and support for both plain-text and Markdown tooltips. |

---

## Contribution Guidelines for Extensions

When creating or porting a new extension:

1. **Self-Contained Folder:** Place all headers, implementation files, and assets in a dedicated folder under `contrib/extensions/<ExtensionName>/`.
2. **Single Header Entrypoint:** Provide a primary header (e.g. `<extension_name>_ext.h` or `<extension_name>.h`) that consumers can include with one `#include` directive.
3. **No Mandatory Global State:** Avoid requiring global mutable state where possible. If state is needed, manage it within an explicit context struct or provide clean default fallbacks.
4. **ImGui Version Compatibility:** Gate version-specific ImGui calls using `#if IMGUI_VERSION_NUM >= ...` so the extension builds cleanly across supported Dear ImGui versions.
5. **Documentation:** Every extension folder must include a comprehensive `README.md` containing:
   * Architecture and features.
   * Minimal copy-pasteable usage example.
   * Configuration options and callbacks.
   * Any licensing or upstream credit.
