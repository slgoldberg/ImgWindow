# ImGui Extensions

This directory contains standalone, feature-complete **Extensions** (Packages) for Dear ImGui within ImgWindow.

---

## What is an Extension?

In ImgWindow, an **Extension** is a self-contained package providing substantial functionality beyond standard UI widgets. Extensions typically:
* Maintain internal or persistent state between frames.
* Require custom configuration structs or setup steps.
* Encapsulate complex algorithms (e.g., text parsing, layout pre-calculation, delayed timers).
* May integrate or adapt third-party libraries.

### Namespacing & API Consistency
To prevent collisions while keeping the API intuitive, **all functions and types in an extension are scoped inside a second-level namespace**. 

Crucially, sub-namespaces allow us to **mirror Dear ImGui's standard vocabulary** (`Text`, `TextWrapped`, etc.) rather than inventing arbitrary new verbs:

```cpp
namespace ImGui {
    namespace <PackageName> {
        // Functions mirror familiar ImGui patterns
    }
}
```

*Example:*
```cpp
// Familiar verbs within clean, collision-free scopes:
// (Note: To prevent collision with the core ImGui::Markdown() function, the Markdown extension uses `ImGui::MD`):
ImGui::MD::TextWrapped("Configure your **settings** below.");
ImGui::TimedTooltip::TextMD("Current mode: *%s*", modeStr);
```

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
