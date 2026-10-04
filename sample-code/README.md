# Sample code and fonts for use with `ImgWindow v2`

This folder contains example implementations and resources to help developers set up custom font atlases for `ImgWindow`-powered plugins, as well as critical examples for proper lifecycle teardown.

> [!NOTE]
> This README is really only to document the contents of the `sample-code` directory, as well as the contained `fonts` folder. For more details about how to configure your font atlas and manage the overall `ImgWindow` repository within your project, see also:
>   * the main [Top-Level README](../README.md), and/or
>   * the [Basic Usage Guide](../docs/Basic-Usage-Guide.md)
>   * the [Advanced Architecture Patterns Guide](../docs/Advanced-Architecture-Patterns.md)

## Contents

1. `InitializeImGui.cpp`: A sample global initialization file demonstrating how to set up a shared font atlas for all `ImgWindow` instances, including loading multiple font weights (Roboto Regular, Bold, Italic, Mono) and merging custom glyph subsets of FontAwesome icons with tailored baseline offsets.
   - **When to run this:** Usually during your `XPluginStart` or `XPluginEnable` lifecycle events.

2. `TeardownImGui.cpp`: A critical teardown function showing how to safely clean down and reset the shared atlas for dynamic resolution/scaling changes or plugin reloading. It demonstrates how to reset the `ImgWindow::sFontAtlas` shared pointer (to prevent Linux heap corruption on reload) and enforce `ImgWindow::Shutdown()` (to flush deferred Panel Graphics Vulkan textures and prevent VRAM leaks).
   - **When to run this:** Always run this during your `XPluginDisable` or `XPluginStop` lifecycle events.

3. `fonts/`: A directory containing example font files converted from Google Fonts into compressed `.inc` arrays for use with `ImgFontAtlas`.

---

## ⚠️ Important Note on Repository Scope

This repository is strictly designed to act as a **lightweight git submodule** containing only the core files needed for `ImgWindow` and `ImgFontAtlas` integration (similarly to how Dear ImGui itself is packaged). 

This repository is **not** a complete, buildable standalone X-Plane plugin. If you are looking for a fully structured boilerplate project that incorporates `ImgWindow` as a submodule complete with CMake files, Docker cross-compilation helpers, and working plugin templates, please visit Bill Good's **[imgui4xp](https://github.com/sparker256/imgui4xp)** repository.
