## `ImgWindow` & Panel Graphics: Migration Guide

With the release of X-Plane 12.4.4b1, Laminar Research introduced the **Panel Graphics API** (XPLM v4.4), routing UI rendering through a modern Vulkan/Metal backend.

`ImgWindow` now provides a **dynamic, backward-compatible bridge** to this new pipeline. You can inject ImGui directly into the modern Panel Graphics rendering pipeline on X-Plane v12.4.4+ while simultaneously maintaining **100% backward compatibility** with any OpenGL-based versions of X-Plane starting with v11.10 onward (including all versions of v12 as well).

There is no need to maintain two separate codebases or force your users to upgrade X-Plane. The framework detects the host simulator's capabilities at runtime and routes the ImGui draw data accordingly.

> [!NOTE]
> **Beta Warning:** The XPLM Panel Graphics API is currently a moving target. Laminar Research recently introduced breaking phase relaxations in **X-Plane 12.4.4b3** that are fully supported by this version of `ImgWindow`. 
> 
> Because of these breaking changes, native Panel Graphics support is physically locked out on the earlier `b1` and `b2` betas to prevent crashes. If you run a dynamic bridge plugin on `b1` or `b2`, it will safely fall back to legacy OpenGL. Throughout this document, any unqualified references to "X-Plane 12.4.4" assume you are targeting the final release API introduced in `b3`.

### 1. Build Configurations

You control how `ImgWindow` interacts with Panel Graphics entirely through compiler definition flags. For the purposes of documenting the specific compiler flags and their behavior (i.e., in the table below and referenced throughout this document), we will assume they are defined using `CMake` configuration directives.

| Build Flags | Rendering Pipeline | Simulator Compatibility |
| :--- | :--- | :--- |
| <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code> | **Dynamic Bridge (Recommended).** Uses Panel Graphics if XPLM 4.4 is detected at runtime. Falls back to legacy OpenGL otherwise. | **Maximum.** X-Plane 11.10+ through X-Plane 12.4.4+ |
| <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code><br> <code>&#8209;DXPLM440=1</code> | **Strict Panel Graphics.** Forces modern rendering and strips the OpenGL fallback logic. | **Modern Only.** X-Plane 12.4.4 and newer. Will not load on older versions. |
| *(None)* | **Strict OpenGL.** Ignores Panel Graphics entirely and forces legacy OpenGL rendering. | **Standard.** X-Plane 11.10+ through current. |

**To enable the recommended Dynamic Bridge in CMake:**
```cmake
add_definitions(-DIMGWINDOW_USE_PANEL_GRAPHICS)
```
*(Note: Be sure to run a `make clean` or clear your CMake cache after altering these definitions to ensure precompiled headers are rebuilt correctly).*

---

### 2. Verifying the Pipeline

When your plugin initializes its first ImGui window, `ImgWindow` will log its routing decision to X-Plane's `Log.txt`. To verify that Panel Graphics is active, look for the following line:

> `ImgWindow: Rendering via XPLM v4.4 Panel Graphics`

---

### 3. Custom Textures (e.g., `ImGui::Image`)

> [!NOTE]
> **What "custom textures" are we talking about here?**
> This section is *only* for custom 2D images you want to draw inside your ImGui windows (like plugin icons, custom gauges, or photos), rendered in your UI using `ImGui::Image()`.
> * **Not Font Atlases:** The `ImgWindow` framework automatically manages ImGui's font textures for you with its `ImgFontAtlas` service.
> * **Not World Textures:** X-Plane's scenery, aircraft liveries, and `.obj` textures are managed natively.

Does your plugin load custom textures? If so, you will need to migrate your texture code to support Panel Graphics. 

First, the good news: any rumors you heard about Panel Graphics requiring complex flight-loops to safely create or destroy textures are officially outdated. As of `12.4.4b3`, you can keep your existing synchronous code structure! You can safely create and destroy textures right inside your draw callbacks.

However, you **must** use the new Panel Graphics API to allocate those textures. If you want your plugin to maintain backward compatibility with legacy OpenGL, you will need to explicitly check which pipeline is active and multiplex your calls. You have two choices for how to do this:

#### A. The Proxy Namespace (Do-It-Yourself)

To support this backward compatibility, the framework introduces a dedicated proxy namespace called `ImgPanelGraphics`.

If you compile a plugin using the native `XPLM` functions (like `XPLMCreateTexture`) from the v4.4 SDK, the operating system linker creates a hard dependency on those symbols. If a user attempts to run your plugin in older simulators like X-Plane 11, the OS will fail to load the plugin entirely because those symbols do not exist in the older binary.

This namespace solves the problem by dynamically looking up the Vulkan/Metal functions at runtime. It seamlessly adapts to your build configuration: if you build a backward-compatible plugin (the default), it safely accesses Panel Graphics features only when available; if you explicitly build against the strict v4.4 SDK (which drops legacy OpenGL support), it routes directly. 

If you want to manually manage your own Panel Graphics rendering, you should **never** call the raw XPLM versions directly. Instead, you should always route your calls through our proxies:
* `ImgPanelGraphics::CreateTexture`
* `ImgPanelGraphics::DestroyTexture`
* `ImgPanelGraphics::TransformPush`
* `ImgPanelGraphics::TransformPop`
* `ImgPanelGraphics::TransformTranslate`
* `ImgPanelGraphics::TransformScale`

*(Note: We purposefully do not expose a proxy for `XPLMDrawCalls` here, as the framework strictly manages the ImGui vertex buffer submissions internally).*

If you choose to use these proxies directly, and you intend to support either Panel Graphics _or_ OpenGL rendering pipelines (of course, only one or the other, based on the build parameters and/or the runtime environment), you must manually _multiplex_ your plugin's calls based on `ImgPanelGraphics::IsAvailable()` (or the convenience method, `ImgWindow::IsUsingPanelGraphics()` that returns the same boolean result). For example, instead of replacing your legacy OpenGL texture creation logic (if your plugin needed such) with a direct call to `XPLMCreateTexture()`, foregoing the legacy support, you might use the `ImgPanelGraphics::` proxy instead -- for example:

```cpp
if (ImgPanelGraphics::IsAvailable()) {
    myPanelGraphicsHandle = ImgPanelGraphics::CreateTexture(pixels, w, h);
} else {
    // legacy OpenGL fallback
    glGenTextures(1, &myLegacyGLHandle);
}
```

But for an even simpler way to do this, `ImgWindow` provides a unified API to do this "the easy way", detailed below.

#### B. The Unified API (The Easy Way)

If your plugin loads custom textures to inject into `Dear ImGui` (e.g., using `ImGui::Image()`), writing boilerplate `if/else` multiplexing blocks everywhere as described above can be quite tedious. Among other things, just managing the return values that are of different types can cause serious issues. (For example, OpenGL texture handles of type `GLuint` will instantly crash the simulator if you attempt to load such handles within a Panel Graphics window!)

To make it so you can have your plugin support _either_ backend (OpenGL _or_ Panel Graphics) completely painlessly, we have pre-baked the two most common use cases directly into the framework. This way, you can skip the manual proxy methods entirely for these two cases, and just use our unified helpers: `ImgWindow::CreateCustomTexture()` and `ImgWindow::DestroyCustomTexture()`. (Of course, you are in no way _required_ to use these; they're just available if you would like to use them, i.e. to reduce unnecessary boilerplate for such common cases.)

These multiplexers automatically determine the active rendering pipeline (Panel Graphics vs. OpenGL), and create the correct type of texture for you **under the hood** -- safely returning an agnostic `ImTextureID` that you can pass directly to ImGui!  These common IDs can be used regardless of whether ImGui is rendering via Panel Graphics, or using OpenGL on older versions of X-Plane that don't support Panel Graphics. Basically, it lets developers focus on the *what* in ImGui terms, not the *how* in low-level rendering pipeline terms.

##### Always FORCE 4 channels (RGBA) when loading images
When loading external images (e.g., PNGs via `stb_image`), X-Plane's Panel Graphics API strictly requires a 4-channel RGBA8 buffer. If you feed it a 3-channel RGB buffer, the simulator will instantly crash due to a buffer overrun!  _(To be clear: don't pass 3 or 0 as the final parameter to `stbi_load()`—**explicitly pass 4**)._

```cpp
// 1. Define your texture handle globally or in your plugin class
ImTextureID myCustomTexture = nullptr;

// 2. Load the texture (Run this on the MAIN THREAD only)
void LoadMyCustomTexture(const char* filepath) {
    int width, height, channels;
    
    // FORCE 4 channels (RGBA) to prevent Vulkan/Metal buffer overruns
    unsigned char* rgba_pixels = stbi_load(filepath, &width, &height, &channels, 4);
    
    if (!rgba_pixels) return;

    // Unified Multiplexing API: Let ImgWindow abstract the backends!
    myCustomTexture = ImgWindow::CreateCustomTexture(rgba_pixels, width, height);
    
    stbi_image_free(rgba_pixels);
}
```

*Note on Alpha Blending:* Panel Graphics relies on straight alpha blending. If your image has fully transparent areas with black RGB values (0, 0, 0, 0), it may cause dark halos around semi-transparent edges. Ensure your assets are exported with a white matte, or manually sanitize the RGB channels of fully transparent pixels before calling `ImgWindow::CreateCustomTexture()`.

##### Don't forget to clean up!
Thanks to X-Plane 12.4.4b3 handling memory deferral natively, you no longer have to manually branch texture destruction or build flight loops to protect Vulkan queues. Just hand the texture back to ImgWindow to destroy it safely:

```cpp
void UnloadMyCustomTexture() {
    if (myCustomTexture) {
        // Safe to call synchronously anywhere!
        ImgWindow::DestroyCustomTexture(myCustomTexture);
        myCustomTexture = nullptr; // Always reset handles!
    }
}
```

#### C. Drawing Custom Textures
Once your texture is loaded and cast to an `ImTextureID`, rendering it inside your window's `ImgWindow::buildInterface()` method is completely agnostic:

```cpp
void MyWindow::buildInterface() {
    if (myCustomTexture != nullptr) {
        ImGui::Image(myCustomTexture, ImVec2(256.0f, 256.0f));
    }
}
```

---

### 4. Developer Caveats & Required Code Changes

While `ImgWindow` automatically handles the rendering pipeline abstraction, it **cannot** automatically translate custom OpenGL state managed by your plugin. Transitioning from synchronous OpenGL to asynchronous Vulkan/Metal introduces strict new rules for your plugin architecture.

#### Caveat A: Strict Main-Thread Execution (No Background Allocation)
The X-Plane SDK enforces a strict **Serialization Rule**. All core XPLM API calls must occur sequentially on X-Plane's main thread. 
*   **The Trap:** If you use background threads (e.g., `std::thread`, `std::async`) for asynchronous texture loading, you **cannot** call `ImgWindow::CreateCustomTexture()` from that background thread. Doing so will generate an invalid cross-thread handle or fatally crash the Vulkan driver. Legacy OpenGL drivers occasionally permitted off-thread allocation, but Panel Graphics strictly forbids it.
*   **The Fix:** Keep your file I/O and pixel decoding (`stbi_load`) on your background worker thread. Once the bytes are decoded, hand them back to the **main X-Plane thread** (e.g., during your next window draw or flight-loop callback) where you will actually call `ImgWindow::CreateCustomTexture()`.

#### Caveat B: The Uninitialized Handle / Null Pointer Trap
In legacy OpenGL, attempting to bind texture ID `0` would safely unbind the texture, often just drawing a blank white square. Vulkan and Metal are not forgiving.
*   **The Trap:** If you pass a garbage memory address (an uninitialized handle) or a `nullptr` directly into Vulkan, the driver will instantly crash.
*   **The Fix:** Ensure every single `ImTextureID` variable in your plugin is explicitly initialized to `nullptr` or `0` upon creation. (The `ImgWindow` framework now internally guards against passing `nullptr` references to the GPU, but it cannot protect you against random, uninitialized memory addresses.)

#### Caveat C: Custom Textures & `ImGui::Image()` Legacy Conversion
The Panel Graphics Vulkan/Metal backend has no knowledge of legacy OpenGL texture IDs. If your UI code generates custom textures via `glGenTextures()` and passes those raw GL integer IDs into `ImGui::Image()`, **X-Plane will instantly crash** if that specific window is being rendered via Panel Graphics.

**The Fix:** Upgrade your texture generation to use the unified `ImgWindow::CreateCustomTexture()` API. If you have legacy UI components that you cannot migrate yet, you can temporarily prevent their OpenGL textures from crashing modern windows by branching your draw logic using `ImgWindow::IsUsingPanelGraphics()`:

```cpp
#define HIDE_FROM_PG(x) if (!this->IsUsingPanelGraphics()) { x }

HIDE_FROM_PG(
    ImGui::Image((void*)(intptr_t)myLegacyGLTextureId, ImVec2(100, 100));
)
```

---

### 5. Visual Polish: Texture Bake Delay (Ghosting)

Because X-Plane 12's VRAM texture uploads are asynchronous under Vulkan/Metal, heavy windows with complex font atlases may exhibit visual jitter or texture pop-in for the first few frames as the GPU bakes the new glyphs in the background.

To mitigate this, `ImgWindow` includes an optional **Texture Bake Delay**. This feature holds the window entirely transparent for a specified number of frames immediately after creation, masking the asynchronous upload.

**YMMV (Your Mileage May Vary):** Depending on your hardware and the complexity of your font atlas, this delay may or may not make a visually significant difference. It is provided strictly as a tuning knob for developers trying to smooth out off-putting text flashing during initial window loads.

To enable the delay, call the setter **immediately after** constructing the window (within the same flight loop cycle). If you defer the call, it will have no effect.

```cpp
// Immediately after creating the window, e.g.:
MyImgWindowSubclass *myHeavyWindow = new MyImgWindowSubclass(...);

// Hold the window transparent for 2 frames (default) upon creation:
myHeavyWindow->SetTextureBakeDelay(true); 
```
*(Note: This setting is ignored completely if the window falls back to legacy OpenGL).*

### Call for Errata or Omissions

As with the main [README](../README.md), please feel free to submit a PR or feedback directly to the author if you are interested in improving this document, correcting any inaccuracies or outright errors, and/or adding more relevant examples, tools, documentation, or references.

This file was last updated in *September, 2026* by Steven L. Goldberg.
