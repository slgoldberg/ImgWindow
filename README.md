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

## No longer supported (see original repository):

* `WavFile` - Simple PCM Wavfile loader as used by XSB 1.4 onwards.
* `XOGLUtils` - an old OpenGL2 binding library for libxplanemp1.
* `XSquawkBox` support - these libraries were needed for it, but it's not
the goal of this `ImgWindow`-focused project to support `XSquawkBox` anymore.

---

## Call for Testers! (September, 2026)

We are actively looking for developers to test the new Panel Graphics bridging support across different platforms (Windows, Mac, Linux) and build environments. If you maintain a plugin that uses `ImgWindow`, please try dropping in this latest update and running it in legacy OpenGL mode *and* native Panel Graphics mode. Report any issues, blown-out colors, or build failures on the issue tracker!

---

## Modern Panel Graphics Support (Vulkan / Metal)

`ImgWindow` now features support for X-Plane's modern **Panel Graphics API** (introduced in the XPLM v4.4 SDK). This allows your plugin to render UI natively through X-Plane's Vulkan/Metal graphics pipeline, completely bypassing legacy OpenGL context bridges.  **However**, you do not need to _build_ with the `XPLM440` SDK enabled in order to _use_ Panel Graphics!  (Read on for more details on how `ImgWindow` will automatically detect whether it has been loaded into X-Plane v12.4.4b1 or later, in which case it will dynamically bind to the v4.4 SDK from an earlier SDK, so that -- if you're on an older version of X-Plane -- it can fall back to `OpenGL` instead!)

The transition to the ImGui-friendly XPLM Panel Graphics API brings **improved performance**, and **future-proofs** your plugin -- but modern graphics APIs are strictly asynchronous and highly unforgiving of legacy OpenGL paradigms. If you are opting into this rendering path, **you must strictly adhere to the new lifecycle rules below, if you enable Panel Graphics support when building with `ImgWindow`.**

### Backwards Compatibility & The Opt-In Strategy

The most important feature of this update is **100% backwards compatibility**. In the first case, legacy plugins (e.g., LiveTraffic) that are already using an older version of `ImgWindow` should be able to just drop this new version in and use it *as-is*, without any changes to their code -- even if they are still using ImGui v1.8x!

**"Opportunistic" Panel Graphics support is an explicit opt-in** using the `-DIMGWINDOW_USE_PANEL_GRAPHICS` flag in your build rules (as a compiler flag when building the files needed for `ImgWindow`). Because X-Plane 12's Panel Graphics **requires** ImGui's modern font atlas rendering, your build options depend entirely on which version of ImGui you are using:

#### 1. Older ImGui (Pre-v1.92.x)

If your project uses an older version of ImGui, **do not** define `-DIMGWINDOW_USE_PANEL_GRAPHICS`. It will generate a build error. Legacy ImGui versions lack the self-managed font atlas required for asynchronous Vulkan/Metal uploads.

| ImGui Version | `IMGWINDOW_USE_PANEL_GRAPHICS` Defined? | SDK Version | Resulting Rendering Backend |
| --- | --- | --- | --- |
| **< 1.92.x** | ❌ No | Any (XPLM300+) | **Legacy OpenGL** (Works perfectly on XP11 & XP12) |
| **< 1.92.x** | ✅ Yes | Any | **Build Error** (Incompatible with legacy ImGui font API) |

#### 2. Newer ImGui (v1.92.x or later)

If you have updated to ImGui 1.92.x or newer, you unlock the ability to opt into Panel Graphics.

Setting the `-DIMGWINDOW_USE_PANEL_GRAPHICS` macro acts as a "Prefer Panel Graphics" flag. If the user loads your plugin into X-Plane 12.4.4+ (which contains the XPLM v4.4 SDK, even if you didn't explicitly enable it!), all your ImGui graphics and fonts will render directly via Vulkan/Metal via Panel Graphics' custom support for ImGui drawing. If the user of such a binary built without *requiring* `XPLM440` loads your plugin into an older version of X-Plane v12 -- or even as far back as X-Plane v11.10 -- `ImgWindow` will gracefully and automatically **fall back to standard OpenGL** on all such platforms.

| ImGui Version | `IMGWINDOW_USE_PANEL_GRAPHICS` Defined? | SDK Version | Resulting Rendering Backend |
| --- | --- | --- | --- |
| **>= 1.92.x** | ❌ No | Any | **Legacy OpenGL** (Works perfectly on XP11 & XP12) |
| **>= 1.92.x** | ✅ Yes | `<= XPLM430` | **Panel Graphics** natively on XP12.4.4+ (Runs on XP11 & XP12 via **auto-fallback to OpenGL** on older XP versions!) |
| **>= 1.92.x** | ✅ Yes | `>= XPLM440` | **Panel Graphics** natively on XP12.4.4+ (Plugin **will not load** on older versions of X-Plane!) |

**CMake Example to enable Panel Graphics:**

```cmake
add_definitions(-DIMGWINDOW_USE_PANEL_GRAPHICS)

#add_definitions(-DXPLM440=1)	# OPTIONAL (restricts plugins to v12.4.4b1+!)
# Note: we recommend *against* defining `XPLM440`, unless you absolutely require
# panel graphics or other features from the v4.4 SDK in other code! (Because
# ImgWindow doesn't need it! It will bind to it dynamically if it's available
# if you define `IMGWINDOW_USE_PANEL_GRAPHICS` above. This means "use panel
# graphics if available in the currently-running version of X-Plane.)
```

---

### Important ⚠️ GOLDEN RULE of Panel Graphics: No Instantiation in Draw Callbacks

In legacy OpenGL, it was common practice to "lazily instantiate" windows directly inside a draw callback (e.g., `if (!myWindow) myWindow = new ImgWindow(...)`). **Under Vulkan/Metal, this will instantly crash X-Plane with an abort trap if it is called from within any draw callback!**

Modern rendering relies on tightly controlled GPU command buffers. You absolutely cannot allocate or destroy GPU textures (which `ImgWindow` must do upon creation and destruction) while a render pass is actively recording.

**The Solution: Use Latches and the Flight Loop**
Separate your *intent* to show a window from the actual *execution* of its creation.

1. When your plugin decides a window needs to open, set a boolean flag (e.g., `g_wants_alert_window = true`).
2. Inside a standard **Flight Loop Callback** (which runs safely on the main thread outside of the render pass), check that flag, instantiate the `ImgWindow`, and clear the flag.

*Note: Calling `setVisibility(false)` inside a draw callback remains perfectly safe, as this only flips an internal ImGui state flag and does not destroy GPU resources.*

It's worth noting that, internally, `ImgWindow` actually does this for you for loading new textures on the fly, on demand!  Whereas the `OpenGL` path through `ImgWindow` will (and should!) load textures in the draw callback, whenever a rendering path goes through panel graphics, `ImgWindow` maintains its own shared font atlas "dirty-bit" checking flight-loop callback, to load any missing glyphs every simulator frame. (This does, unfortunately, mean you may see some flashing of text in the first frame or two after the initial load, but .. that's just how it is.) Though it's of limited use, `ImgWindow` does provide some simplistic "bake delay" functionality, as described next, to help improve visual polish:

### Visual Polish: Texture Bake Delay (Ghosting)

Because X-Plane 12's VRAM texture uploads are asynchronous under Vulkan/Metal, heavy windows with complex font atlases may exhibit visual jitter, or texture pop-in, for the first few frames as the GPU bakes the new glyphs in the background.

To mitigate this, `ImgWindow` includes an optional **Texture Bake Delay** that can be set when an `ImgWindow` instance is created.

By default, this feature is **OFF**. Transient windows like momentary alerts or popup logs will render immediately (0-frame latency) so data remains perfectly synced with the user's action. However, for large, complex windows where visual polish is more important than millisecond latency, you can opt-in to ghosting, though your mileage may vary, so no guarantees are made that this will actually make a big difference.

To enable this for a given instance of `ImgWindow` or a derived class, simply insert a call to the setter for the bake delay, **immediately after** constructing the window (within the same cycle; if you defer this, it may not have any effect).  For example, if you have a specific derived class for which you always want it, then you could put this within the body of your derived class` constructor:

```cpp
// Holds the window transparent for 2 frames (default) upon creation
myHeavyWindow->setTextureBakeDelay(true); 

// Or specify a custom frame delay for exceptionally heavy textures
myHeavyWindow->setTextureBakeDelay(true, 4);
```

The reason this feature is configured via a setter, and not via a constructor parameter, is that (a) it's not a core semantic element of the `ImgWindow` class (it's just a practical adjustment knob you can use), and (b) the constructor is already bloated, and adding this would further that bloat, when the bake delay is really only useful for windows with largely varied font styling in a single visible region. (I.e., in cases where the incremental baking of the textures creates a visual effect that is off-putting to end users.)

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

This file was last updated in *September, 2026* by Steven L. Goldberg.
