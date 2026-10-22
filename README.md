# `ImgWindow v2`: ImGui wrapper for X-Plane Modern XPLM Window API with font support

The sources in this repository are shared with the greater X-Plane developer
community in the hope that it may save somebody a headache some day.

This was originally the public XSquawkBox Public (`xsb_public`) repository, which contained several components, including `ImgWindow` and its dependencies. But it has been heavily expanded to add some critically important improvements for using **Dear ImGui** in C++ X-Plane plugins. 

Here are the major additions to the framework since it was forked:
---

+ **Full support for major Dear ImGui API changes** (while keeping ImGui v1.8x compatibility):
  
  - **`ImGui v1.90` Keyboard event processing:**<br>
  This is a completely transparent rewrite under the hood. XPLM Window API's keyboard handling remains identical from your plugin's perspective, but it fully supports ImGui's v1.90 keyboard IO refactor. You still get full "editing shortcuts" (Ctrl+C/V, Shift+arrow selections) working perfectly across Windows, Linux, and macOS.
  
    * _**Note:**_<br>
	_The "legacy" keyboard processing logic is **still supported** in `ImgWindow` for ImGui v1.8x! The framework **automatically** detects which version of ImGui you are using (based on `imgui.h` defining it in `IMGUI_VERSION_NUM`). As long as your version of ImGui contains this definition, and sets it correctly, `ImgWindow` will still support it with no changes needed vis-à-vis the original `ImgWindow` repository!_
  
  - **`ImGui v1.92` Font Atlas changes:**<br>
  The new `ImFontAtlas` features (like dynamic loading) are fully supported. Crucially, the `ImgFontAtlas` wrapper painstakingly preserves the _exact_ same "shared atlas" semantics that earlier versions used. You can still construct a single shared atlas at plugin-enable time, and it will fill in missing glyphs automatically.
  
    _**N.B.:** We did this so your code shouldn't have to change at all. We intentionally hide ImGui's new per-window atlas management (which would needlessly complicate 98% of X-Plane plugins). ImgWindow just auto-detects your ImGui version at compile time and does the heavy lifting for you._

+ **A dynamic "Panel Graphics Bridge":**<br>
  If you're using ImGui v1.92+, you can finally take advantage of the modern X-Plane 12.4.4+ **Panel Graphics API** (Vulkan/Metal) _without_ forcing your plugin to require `XPLM440=1`!
  
> [!NOTE]
> This is a **critical build choice!**
> If you just define `IMGWINDOW_USE_PANEL_GRAPHICS` in your build, the framework automatically uses dynamic bindings to render via Panel Graphics on XP12.4.4+, but gracefully falls back to legacy OpenGL on older simulators. If you define `XPLM440=1` instead, you will hard-break backwards compatibility with older versions of X-Plane and OpenGL. Don't do that unless you genuinely only want to support the bleeding edge. Read the [Panel Graphics Migration Guide](docs/Panel-Graphics-Migration.md) for the gory details.

+ **Right-drag window moving & drag state detection:**<br>
An opt-in feature allowing the framework to move in-simulator windows automatically if the user right-clicks and drags anywhere on the window (saving them from hunting for a tiny drag region). Callers can also query `IsWindowMoving()` during their `buildInterface()` loop to detect when the user is actively dragging or resizing the window (e.g. to pause heavy computations or graph updates during movement).

+ **Cursor invalidation on blur:**<br>
Automatically invalidates the cursor position when the window loses focus, ensuring it updates correctly when the user clicks back in.

+ **Optional Per-Window Custom ImGui Cursors:**<br>
Opt-in support for dynamic ImGui mouse cursors. If enabled, the plugin can change the mouse cursor based on what ImGui is hovering over (e.g., an I-beam over text inputs, or a pointing finger over buttons). These were always part of the ImGui API, but ImgWindow used to ignore them. Now, it ensures the cursor is properly handed off between X-Plane and ImGui.

+ And more...

---

This repository is maintained by **Steven L. Goldberg (`slgoldberg`)**, and was forked from Chris Collins' original `xsb_public` repository. Going forward, this fork **only** contains the source files needed for `ImgWindow` and `ImgFontAtlas`. If you need the other legacy utility code from the original project -- specifically `WavFile` or `XOGLUtils` -- please see the original repository instead. Going forward, please submit any Pull Requests or other feedback, directly on **this** repository, which can be found at &lt;https&#8203;://github.com/slgoldberg/ImgWindow&gt;. **Read below** for detailed usage information, as well as an important migration guide for the new **Panel Graphics** support if you are planning to enable it for your plugin.

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
installed (compatible with Dear ImGui v1.78 WIP through v1.92+, with native support for the new `ImFontAtlas` API in ImGui v1.92+). Later versions may work fine, but no guarantees are made.

## Components in this Repository

* `ImgWindow` and `ImgFontAtlas` - Wrappers for the [Dear ImGui](https://github.com/ocornut/imgui) immediate mode GUI library.
* **Dynamic X-Plane 12.4.4+ Panel Graphics support** - Enable your build to render ImGui via **Panel Graphics** _or_ **OpenGL fallback**, depending on which version of X-Plane it is loaded on! (I.e., build a single backward-compatible binary for each target platform.)
  - _Note: this includes an option to build with the entire v4.4 SDK, **only** supporting Panel Graphics on X-Plane v12.4.4 and later (**without** backwards compatibility support for OpenGL rendering)._
* `ImgPanelGraphics::` - Dynamic runtime symbol proxy namespace for zero-dependency backward compatibility (!!). Details within.
* **Modern Window Geometry & Ergonomic Constructors (v2.2+)** - Eliminates inverted $Y$ boxel math and prevents off-screen window bugs via `WindowGeometry` and C++11 delegating constructors:
  - **Auto-Centered**: `ImgWindow(width, height)` auto-centers on the primary display with zero coordinate boilerplate.
  - **Top-Left Origin**: `WindowGeometry::FromTopLeft(x, y, w, h)` provides natural 2D desktop origin coordinates.
  - **Safe Out-of-Bounds Recovery**: `WindowGeometry::Sanitize(...)` audits saved window coordinates on startup, recovering windows from disconnected monitors or resolution shifts while preserving "poor-man's docking" margins.
  - **VR-Ready**: Automatically initializes VR window geometry and aspect ratio via `XPLMSetWindowGeometryVR`.
* **`contrib/` UI Suite** - Bundled flight-deck-grade Dear ImGui extensions and immediate-mode widgets (see [`contrib/README.md`](contrib/README.md)):
  - **`TimedTooltip`**: Polite, hover-delayed stationary tooltips that lock in place and auto-dismiss after reading.
  - **`Markdown` (`ImGui::Markdown` & `ImGui::MD`)**: Rich CommonMark parsing and rendering with dynamic font scaling, token lookahead, hanging bullet indents, and custom styling tags.
  - **Widgets**: Animated toggle switches (`ToggleButton`), mouse-wheel sliders/combos (`MouseWheelSlider`), 1-click resets with undo history, and clickable web/doc links (`ButtonLink`, `TextLink`).
  - **Turnkey Inclusion**: `#include "contrib.h"` activates the entire suite in one line, with 3-tier namespacing (sub-namespaces, root `ImGui::` shorthands, or opt-in global namespaces).

## No longer supported (see original repository):

* `WavFile` - Simple PCM Wavfile loader as used by XSB 1.4 onwards.
* `XOGLUtils` - an old OpenGL2 binding library for libxplanemp1.
* `XSquawkBox` support - these libraries were needed for it, but it's not
the goal of this `ImgWindow`-focused project to support `XSquawkBox` anymore.

---

## Production Status & Architecture

`ImgWindow v2` is production-ready and verified stable across **Windows, macOS (Metal), and Linux (Vulkan)**. It delivers a unified, zero-boilerplate bridge supporting native **XPLM 4.4 Panel Graphics** on modern X-Plane 12 while maintaining 100% backward compatibility with legacy **OpenGL** on earlier simulator releases.

If you maintain an existing plugin that uses `ImgWindow`, you can safely drop in this update to modernize your rendering pipeline. We continue to welcome developer feedback and contributions via the issue tracker and pull requests!

---

## 🗺️ How-To Guides ("TripTiks")

For quick, scenario-based walkthroughs with copy-pasteable CMake snippets and code:
* 🚀 **[TripTik 1: Get Started With ImgWindow](docs/how-to/Get-Started-With-ImgWindow.md)** — The 6-step starter guide for creating your first window, command-first dispatching, and choosing your build mode.
* 🛠️ **[TripTik 2: Support Panel Graphics](docs/how-to/Support-Panel-Graphics.md)** — The 10-minute migration checklist for updating existing v1 plugins.
* 🖼️ **[TripTik 3: Render Custom Textures](docs/how-to/Render-Custom-Textures.md)** — Using `ImgWindow::CreateTexture()` to draw images without raw OpenGL pipeline code.
* 🎓 **[Ground School](docs/how-to/Ground-School.md)** — Fundamental Dear ImGui immediate-mode principles in X-Plane.

---

## Getting Started: Basic Usage Model

If you are new to the framework or building a plugin from scratch, understanding how `ImgWindow` intercepts XPLM rendering and proxies it to ImGui is crucial. 

At a high level, using the framework requires only two core components:
1. **`ImgFontAtlas`**: A shared class that manages your fonts and safely bakes the texture to the GPU. You instantiate this once for your entire plugin.
2. **`ImgWindow`**: The base class you must subclass. By overriding the `buildInterface()` method, you define your standard ImGui UI layout. The framework handles the rest—intercepting mouse/keyboard events and generating the draw lists for X-Plane.

👉 **[Read the Basic Usage Guide](docs/Basic-Usage-Guide.md)** for a complete walkthrough of the architecture, code examples for creating your windows, and details on managing the shared font atlas.

---

## Advanced Architecture Patterns

Once your plugin begins scaling up to multiple windows, complex tooltips, and bespoke symbol fonts, you'll need a more robust architecture. We have compiled a masterclass document on advanced patterns used by large-scale production plugins.

👉 **[Read the Advanced Architecture Patterns Guide](docs/Advanced-Architecture-Patterns.md)** for detailed strategies on managing the ImGui lifecycle, `ImgFontAtlas` optimization (including FontAwesome VRAM reduction), and building smart auto-positioning widgets.

---

## Modern Panel Graphics Support (Vulkan / Metal)

`ImgWindow` features full, production-ready support for X-Plane's modern **Panel Graphics API** (introduced in the XPLM v4.4 SDK / X-Plane 12.4.4+). This allows your plugin to render UI natively through X-Plane's Vulkan/Metal graphics pipeline, bypassing legacy OpenGL completely.

The transition to native **Panel Graphics** brings substantially **improved rendering performance**, eliminates OpenGL context overhead, and **future-proofs** your plugin against the eventual deprecation of OpenGL. 

### Why Use `ImgWindow` for Panel Graphics?
* **Zero-Downtime Backward Compatibility:** With our dynamic bridge (`ImgPanelGraphics`), a single binary will run on modern Vulkan/Metal on X-Plane 12.4.4+ while seamlessly falling back to OpenGL on X-Plane 11 and earlier X-Plane 12 releases. You do **not** need to build separate plugin binaries or force users to update their simulator.
* **Internal Lifecycle & Atlas Safeguards:** The framework automatically manages the shared font atlas across multi-window environments and guards against Vulkan null-descriptor pipeline crashes.

### 🛠️ Developing with `ImgWindow` (X-Plane 12.4.4+)
Because Laminar Research relaxed earlier Panel Graphics phase restrictions in X-Plane 12.4.4, developing for modern Vulkan/Metal now feels exactly like legacy OpenGL! You can safely create and destroy textures or manage windows directly inside your draw callbacks without manually managing deferred flight-loops.

Just keep these two simple rules in mind:

1. **Use the Unified Texture API:** To abstract the complexity of supporting both Panel Graphics and the legacy OpenGL fallback, always use the unified `ImgWindow::CreateTexture()` and `ImgWindow::DeleteTexture()` wrappers instead of managing raw `GLuint` handles yourself. These are 100% safe to call synchronously on the main thread (including inside draw callbacks!).
2. **Mandatory 4-Channel RGBA Buffers:** Panel Graphics strictly requires 32-bit RGBA image buffers. Loading 3-channel RGB images will cause instant memory overrun crashes in the Vulkan driver.

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
% git submodule add https://github.com/slgoldberg/ImgWindow third-party/ImgWindow
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

This file was last updated in *October, 2026* by Steven L. Goldberg.
