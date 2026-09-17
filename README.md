# ImgWindow: ImGui wrapper for X-Plane Modern XPLM Window API with font support

The sources in this repository are shared with the greater X-Plane developer
community in the hope that it may save somebody a headache some day.

This repository ONLY contains the files needed for the `ImgWindow` class
and the `ImgFontAtlas` wrapper for binding to ImGui v1.84 through v1.92.x,
and has been updated to completely hide the core changes in ImGui v1.92.x
to provide simple migration from older versions of ImGui vis-a-vis fonts
and keyboard API changes in ImGui, requiring no code changes for either.

This was originally the public XSquawkBox Public (xsb_public) repository,
which contained several components, including ImgWindow and its dependencies.

This is the new home, forked from Chris Collins' original repository by
Steven L. Goldberg (slgoldberg), and focused down to just ImgWindow and
ImgFontAtlas, with some added sample code including various font examples.

If users need the other public sources from `xsb_public`, please see the
original repository from which this was forked. Going forward, please
submit any Pull Requests to slgoldberg on *this* repository to contribute
improvements back for ImgWindow or ImgFontAtlas.

## Licensing Note

New sources are released under the BSD 3-Clause license, following on to
the exact license terms provided for the ImgWindow and related sources
carried forward herein.

There are no other licensed dependencies included, as this repository is
solely focused on providing developers a way to use ImgWindow to bring
ImGui to XPLM Modern Windows -- and/or to the modern X-Plane **Panel Graphics
API**, available in the XPLM v4.4 SDK (starting with X-Plane v12.4.4).

## Prerequisites

Components in this library assume the availability of the X-Plane XPLM3 or
later SDK, and rely on the developer including these files within their own
build projects for any X-Plane plugins that use ImGui, which must also be
installed (no less than ImGui v1.84 WIP, and currently no more than ImGui
v1.92.x). Later versions may work fine, but no guarantees are made.

## Components in this Repository

* `ImgWindow` and `ImgFontAtlas` - Wrappers for the [dear imgui](https://github.com/ocornut/imgui) Immediate Mode GUI library
* **Dynamic X-Plane 12.4.4+ Panel Graphics support (Vulkan/Metal)** with automatic legacy OpenGL fallback.
* `ImgPanelGraphics` - Dynamic runtime symbol proxy namespace for zero-dependency backward compatibility.

## No longer supported (see original repository):

* `WavFile` - Simple PCM Wavfile loader as used by XSB 1.4 onwards.
* `XOGLUtils` - an old OpenGL2 binding library for libxplanemp1.
* `XSquawkBox` support - these libraries were needed for it, but it's not
the goal of this `ImgWindow`-focused project to support `XSquawkBox` anymore.

---

## Community Testing & Status (September, 2026)

The new dynamic Panel Graphics bridge has been verified stable across **Windows, macOS (Metal), and Linux (Vulkan)** in both legacy OpenGL fallback mode and native XPLM 4.4 Panel Graphics mode. 

If you maintain a plugin that uses `ImgWindow`, you can safely drop in this update to modernize your rendering pipeline. We continue to welcome developer feedback, edge-case testing, and contributions via the issue tracker and pull requests!

---

## Modern Panel Graphics Support (Vulkan / Metal)

`ImgWindow` features full, production-ready support for X-Plane's modern **Panel Graphics API** (introduced in the XPLM v4.4 SDK / X-Plane 12.4.4+). This allows your plugin to render UI natively through X-Plane's Vulkan/Metal graphics pipeline, bypassing legacy OpenGL completely.

The transition to native **Panel Graphics** brings substantially **improved rendering performance**, eliminates OpenGL context overhead, and **future-proofs** your plugin against the eventual deprecation of OpenGL. 

### Why Use `ImgWindow` for Panel Graphics?
* **Zero-Downtime Backward Compatibility:** With our dynamic bridge (`ImgPanelGraphics`), a single binary will run on modern Vulkan/Metal on X-Plane 12.4.4+ while seamlessly falling back to OpenGL on X-Plane 11.10 through 12.4.3. You do **not** need to build separate plugin binaries or force users to update their simulator.
* **Internal Lifecycle & Atlas Safeguards:** The framework automatically manages the shared font atlas across multi-window environments, guards against Vulkan null-descriptor pipeline crashes, and coordinates background atlas rebuilds outside of drawing callbacks.

### ⚠️ Strict Architectural Rules for Plugin Developers
While `ImgWindow` makes rendering seamless, modern graphics APIs are strictly asynchronous and highly unforgiving of legacy OpenGL paradigms. If your plugin loads custom UI textures or manages windows dynamically, you must adhere to three fundamental rules:

1. **Main-Thread GPU Allocations Only:** All calls to `ImgPanelGraphics::CreateTexture()` must execute on X-Plane's main serialization thread. Background worker threads can decode files (`stbi_load`), but raw pixel buffers must be dispatched back to the main thread before allocating GPU memory.
2. **Deferred Texture Destruction:** Vulkan/Metal draw calls are deferred and queued. Calling `ImgPanelGraphics::DestroyTexture()` synchronously the moment a UI screen closes will destroy memory while the GPU is still drawing it, triggering an instant `SIGSEGV`. Plugins must defer texture cleanup by 2–3 flight loop cycles via a garbage collection queue.
3. **Mandatory 4-Channel RGBA Buffers:** Panel Graphics strictly requires 32-bit RGBA image buffers. Loading 3-channel RGB images will cause instant memory overrun crashes in the Vulkan driver.

👉 **[Read the Panel Graphics Migration Guide](docs/Panel-Graphics-Migration.md)** for complete CMake build configurations, step-by-step migration examples for `ImGui::Image()`, and architectural guides on avoiding invalid texture crashes.

---

## Using this repository (as a "submodule" in your own project)

This repository *only* contains the `ImgWindow` and the `ImgFontAtlas` classes (headers and C++ implementations) and some ancillary files including some fonts you can include in your sources and add to your ImGui environment with ImgFontAtlas.

This can be included as a "submodule" (see below) within your X-Plane plugin sources, along with a submodule for the latest [dear imgui](https://github.com/ocornut/imgui) sources.

This is *not* a complete, "buildable" X-Plane sample plugin, though some limited sample code is provided in the `sample-code` directory, to assist with basic configuration of a shared font atlas with custom-loaded fonts, which are actually included in the `sample-code/fonts` directory as well.

## `imgui4xp`: A full sample plugin using `ImgWindow`

For a *much* more robust project containing a full sample X-Plane plugin with C++ sources to demonstrate exactly how to set up and use `ImgWindow` and `ImGui` for the user interface, while also demonstrating how to set up `Docker` for cross-compilation and how to use `Cmake`, see Bill Good's project, **[imgui4xp](https://github.com/sparker256/imgui4xp)**.

The [imgui4xp](https://github.com/sparker256/imgui4xp) project also demonstrates how to incorporate `ImgWindow` using this repository as a **git submodule** within your own project.

For example, let's say you want to have `ImgWindow` within your own source code tree under a directory at the top-level titled, "`third-party`". If you already have a subfolder containing an older version of `ImgWindow` called, "`ImgWindow`", you can simply delete that from the project, and re-add it as a submodule:

```bash
% cd /path/to/project_sources
% git rm -rf third-party/ImgWindow             # (ONLY if it already existed!)
% git commit -m "Remove embedded ImgWindow code to replace with submodule"
```

To set up the "submodule" connection within your project:

```bash
% cd /path/to/project_sources
% git submodule add [https://github.com/slgoldberg/ImgWindow](https://github.com/slgoldberg/ImgWindow) third-party/ImgWindow
% git commit -m "Replace embedded ImgWindow from slgoldberg's fork"
```

**Users** of your repository will need to pull the sources into that submodule directory once they clone or fork it locally:

```bash
% cd /path/to/cloned_project        
% git submodule update --init --recursive
```

To update the submodule any time:

```bash
% cd /path/to/project_sources/third-party/ImgWindow
% git pull                                       
% cd /path/to/project_sources
% git add third-party/ImgWindow
% git commit -m "Update embedded ImgWindow from slgoldberg's fork"
```

## Contributing to this project

Pull Requests (PRs) are welcome, though it's usually better to start by contacting the owner (Steve Goldberg) via private message (PM) to the [X-Plane.org forum](https://forums.x-plane.org). Send a private message to `@slgoldberg` on that forum, and introduce yourself and explain what you're hoping to accomplish.

In general, to contribute code here, the best place to start is by "`fork`ing" this `ImgWindow` repository, which lets you make changes locally which makes it trivial for owner(s) of the forked repository to see your proposed changes even before you submit a formal Pull Request (PR).

### Forking, testing, and managing changes in advance of a Pull Request:

To create your own "`fork`" of `ImgWindow` to start the process, simply click the "**Fork**" button on the main [web page](https://github.com/slgoldberg/ImgWindow) for [`ImgWindow` at github.com](https://github.com/slgoldberg/ImgWindow), then use your favorite method to clone that locally.

Once you have your local fork, do all your work *within* that local fork. There are two paths forward from here:

1. **Standalone changes.**
If you _only_ want to make minor changes, such as fix typos or add a line or two, then make all your changes in place and push them back to your `master` branch on the upstream repository (i.e., on GitHub).
2. **Testing and iterating on changes in context with your plugin using `ImgWindow`.**
This assumes your plugin's source code defines `ImgWindow` as a "`submodule`". First, change your plugin's source project by **renaming** your `ImgWindow` directory so that it can readily be "redirected" to different targets:

```bash
% cd /path/to/plugin_project
% git mv third-party/ImgWindow third-party/ImgWindow_GITHUB
% ln -s third-party/ImgWindow_GITHUB third-party/ImgWindow
% git add third-party/ImgWindow    
% git commit -m "Add layer of indirection using symbolic link to ImgWindow"
```

Finally, for testing, you start by making changes to your `ImgWindow` clone project, within *its* source tree. Update your `third-party/ImgWindow` symbolic link to point to *this* directory where you're working on your fork:

```bash
% cd /path/to/plugin_project
% rm third-party/ImgWindow   
% ln -s /path/to/ImgWindow_fork third-party/ImgWindow
```

After you make a change locally in the `ImgWindow` fork's local files, test those changes by switching over to your **plugin**'s project repository and build and run it in X-Plane!

Once you have a final, working set of changes to `ImgWindow`:

```bash
% cd /path/to/ImgWindow_fork
% git add .       
% git commit -m "Clear descriptions since the PR will show these"
% git push
```

---

### Errata / Missing Info?

If anything is wrong or missing from this README, please either fix it and send us a PR, or let us know.

This file was last updated in *September, 2026* by Steven L. Goldberg.
