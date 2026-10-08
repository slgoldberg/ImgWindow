# ImgWindow How-To Guides

This directory contains practical, scenario-based walkthroughs for integrating, configuring, and extending **ImgWindow** in X-Plane plugins.

Unlike API reference documentation, How-To guides are structured as step-by-step solutions to specific real-world challenges encountered when developing X-Plane user interfaces.

---

## Available Guides

| Guide | Description |
| :--- | :--- |
| [**Support-Panel-Graphics.md**](Support-Panel-Graphics.md) | How to migrate from legacy OpenGL rendering to modern X-Plane Panel Graphics (SDK 4.4+) while preserving backward compatibility for X-Plane 11 and older X-Plane 12 builds. |
| [**Render-Custom-Textures.md**](Render-Custom-Textures.md) | How to load, bind, and render custom textures, images, and avionics displays using `ImGui::Image` within an `ImgWindow`. |

---

## Contributing a How-To Guide

Have a pattern, workflow, or integration technique that other plugin developers would benefit from? Contributions are welcome!

### Guidelines for New Guides:
1. **Problem-Focused:** Frame the guide around a concrete goal (e.g. *"How to handle multi-monitor coordinate scaling"*, *"How to bind a custom font atlas"*).
2. **Actionable Steps:** Use clear, sequential headings (`Step 1: ...`, `Step 2: ...`) with working, minimal code snippets.
3. **Highlight Pitfalls:** Call out SDK version quirks, coordinate space mismatches (e.g., Vulkan/Metal Y-inversion), or threading restrictions using GitHub-style callouts (`> [!WARNING]`, `> [!NOTE]`).
4. **Link from this Index:** When submitting a PR with a new guide, add an entry to the table above.
