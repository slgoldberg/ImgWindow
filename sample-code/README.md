# Placeholder README for ImgWindow sample code

This folder contains example implementations and resources to help developers set up custom font atlases for `ImgWindow`-powered plugins.

## Contents

1. `InitializeImGui.cpp`: A sample global initialization file demonstrating how to set up a shared font atlas for all `ImgWindow` instances, including loading multiple font weights (Roboto Regular, Bold, Italic, Mono) and merging custom glyph subsets of FontAwesome icons with tailored baseline offsets. It also includes the companion teardown function (`RemoveImGui()`) to safely clean down or reset the atlas for dynamic resolution/scaling changes.
2. `fonts/`: A directory containing example font files converted from Google Fonts into compressed `.inc` arrays for use with `ImgFontAtlas`.

---

## ⚠️ Important Note on Repository Scope

This repository is strictly designed to act as a **lightweight git submodule** containing only the core files needed for `ImgWindow` and `ImgFontAtlas` integration (similarly to how Dear ImGui itself is packaged). 

This repository is **not** a complete, buildable standalone X-Plane plugin. If you are looking for a fully structured boilerplate project that incorporates `ImgWindow` as a submodule complete with CMake files, Docker cross-compilation helpers, and working plugin templates, please visit Bill Good's **[imgui4xp](https://github.com/sparker256/imgui4xp)** repository.
