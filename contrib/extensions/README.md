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
  ImGui::MD::TextWrapped("Configure your **settings** below.");
  ImVec2 size = ImGui::MD::CalcTextSize(id, 1, docString, wrapWidth);

  ImGui::TimedTooltip::TextMD("Current mode: *%s*", modeStr);
  ```
*(Note: To prevent collision with the core third-party `ImGui::Markdown(...)` free function, the Markdown extension's sub-namespace is cleanly abbreviated to `ImGui::MD::`).*

---

## Current Extensions

| Extension | Directory | C++ Namespace | Description |
| :--- | :--- | :--- | :--- |
| **Markdown** | [`Markdown/`](Markdown/) | `ImGui::MD::` | Markdown parser and renderer based on `juliettef/imgui_markdown`, enhanced with dynamic font scaling, token lookahead, true hanging indents, and rich parameterized styling tags (`<color>`, `<backdrop>`, `<badge>`). |
| **TimedTooltip** | [`TimedTooltip/`](TimedTooltip/) | `ImGui::TimedTooltip::` | Advanced stationary and delayed tooltip engine with automatic bounding box pre-calculation, viewport edge-clamping, and support for both plain-text and Markdown tooltips. |

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
