## `ImgWindow` & Panel Graphics: Migration Guide

With the release of X-Plane 12.4.4b1, Laminar Research introduced the **Panel Graphics API** (XPLM v4.4), routing UI rendering through a modern Vulkan/Metal backend. 

`ImgWindow` now provides a **dynamic, backward-compatible bridge** to this new pipeline. You can inject ImGui directly into the modern Panel Graphics pipeline on XP12.4.4+ while simultaneously maintaining 100% backward compatibility with older OpenGL-based versions of X-Plane (XP11.10 through early XP12). 

There is no need to maintain two separate codebases or force your users to upgrade X-Plane. The framework detects the host simulator's capabilities at runtime and routes the ImGui draw data accordingly.

### 1. Build Configurations

You control how `ImgWindow` interacts with Panel Graphics entirely through CMake compiler definitions. 

| Build Flags | Rendering Pipeline | Simulator Compatibility |
| :--- | :--- | :--- |
| `-DIMGWINDOW_USE_PANEL_GRAPHICS` | **Dynamic Bridge (Recommended).** Uses Panel Graphics if XPLM 4.4 is detected at runtime. Falls back to legacy OpenGL otherwise. | **Maximum.** X-Plane 11.10+ through X-Plane 12.4.4+ |
| `-DIMGWINDOW_USE_PANEL_GRAPHICS` <br> `-DXPLM440=1` | **Strict Panel Graphics.** Forces modern rendering and strips the OpenGL fallback logic. | **Modern Only.** X-Plane 12.4.4 and newer. Will not load on older versions. |
| *(None)* | **Strict OpenGL.** Ignores Panel Graphics entirely and forces legacy OpenGL rendering. | **Standard.** X-Plane 11.10+ through current. |

**To enable the recommended Dynamic Bridge in CMake:**
```cmake
add_definitions(-DIMGWINDOW_USE_PANEL_GRAPHICS)
```
*(Note: Be sure to run a `make clean` or clear your CMake cache after altering these definitions to ensure precompiled headers are rebuilt correctly).*

### 2. Verifying the Pipeline

When your plugin initializes its first ImGui window, `ImgWindow` will log its routing decision to X-Plane's `Log.txt`. To verify that Panel Graphics is active, look for the following line:

> `ImgWindow: Rendering via XPLM v4.4 Panel Graphics`

### 3. Developer Caveats & Required Code Changes

While `ImgWindow` automatically handles the rendering pipeline abstraction, it **cannot** automatically translate custom OpenGL state managed by your plugin. If you use custom textures or spawn windows dynamically, you must account for the following constraints.

#### Caveat A: Custom Textures & `ImGui::Image()`
The Panel Graphics Vulkan/Metal backend has no knowledge of legacy OpenGL texture IDs. If your UI code generates custom textures via `glGenTextures()` and passes those raw GL integer IDs into `ImGui::Image()`, **X-Plane will instantly crash (CTD)** when Panel Graphics is enabled.

**The Fix:** You must conditionally branch your texture generation logic. `ImgWindow` provides a runtime state query method:

```cpp
bool ImgWindow::IsUsingPanelGraphics() const;
```

If you are migrating an older plugin, you can easily stub out crashing OpenGL textures using a macro until you are ready to rewrite your texture generation for Panel Graphics:
```cpp
// Example macro to hide legacy OpenGL textures when rendering ImGui using Panel Graphics:
#define HIDE_FROM_PG(x) if (!this->IsUsingPanelGraphics()) { x }

// Example usage in your ImGui user interface code:
HIDE_FROM_PG(
    ImGui::Image((void*)(intptr_t)myLegacyGLTextureId, ImVec2(100, 100));
)
```

#### Caveat B: No "Lazy" Window Creation in Draw Callbacks
Under the legacy OpenGL pipeline, it was technically possible (though ill-advised) to lazily instantiate a new XPLM Window (e.g., calling `XPLMCreateWindowEx`) from *within* an active drawing callback. 

**This is strictly forbidden under Panel Graphics.** Attempting to create a new window while the Panel Graphics rendering pipeline is mid-execution will trigger a hard assert in Laminar's engine and instantly crash the simulator.

**The Fix:** Window creation must happen outside of the draw cycle. If your UI logic determines a new window is needed during a draw callback, set a boolean flag (latch). Then, check that flag inside a standard XPLM Flight Loop callback (e.g., `xplm_FlightLoop_Phase_BeforeFlightModel`), create the window there, and clear the flag.

### 4. Visual Polish: Texture Bake Delay (Ghosting)

Because X-Plane 12's VRAM texture uploads are asynchronous under Vulkan/Metal, heavy windows with complex font atlases may exhibit visual jitter or texture pop-in for the first few frames as the GPU bakes the new glyphs in the background.

To mitigate this, `ImgWindow` includes an optional **Texture Bake Delay**. This feature holds the window entirely transparent for a specified number of frames immediately after creation, masking the asynchronous upload.

**YMMV (Your Mileage May Vary):** Depending on your hardware and the complexity of your font atlas, this delay may or may not make a visually significant difference. It is provided strictly as a tuning knob for developers trying to smooth out off-putting text flashing during initial window loads.

To enable the delay, call the setter **immediately after** constructing the window (within the same flight loop cycle). If you defer the call, it will have no effect.
(If you want such a delay for all instances of a particular subclass derived from `ImgWindow`, then you simply call `setTextureBakeDelay(true)` from within the class' constructor, since it is always run after the main `ImgWindow::ImgWindow()` constructor is run, thus setting the "bake delay" so it takes effect before you return from creating the window.)

**Example** (for a single, specific window, just after you've created it, but before you return control to XPLM):

```cpp
// Immediately after creating the window, e.g.:
MyImgWindowSubclass *myHeavyWindow = new MyImgWindowSubclass(...);

// Hold the window transparent for 2 frames (default) upon creation:
myHeavyWindow->setTextureBakeDelay(true); 

// Or specify a custom frame delay for exceptionally heavy textures:
myHeavyWindow->setTextureBakeDelay(true, 5);  // 5-frame initial delay
```
*(Note: This setting is ignored completely if the window falls back to legacy OpenGL, so you do not need to wrap it in an `IsUsingPanelGraphics()` check.)*
