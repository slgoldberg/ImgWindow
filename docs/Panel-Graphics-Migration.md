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
> This section is *only* for custom 2D images you want to draw inside your ImGui windows (like plugin icons, custom gauges, or photos), using functions like `ImGui::Image()`, `ImGui::ImageButton()`, or any other ImGui API that requires an `ImTextureID`.
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
| 🟢 Always Call This: | | 🔴 NEVER Call This (Raw XPLM): |
| :--- | :---: | :--- |
| `ImgPanelGraphics::CreateTexture` | &rarr; | `XPLMCreateTexture` |
| `ImgPanelGraphics::DestroyTexture` | &rarr; | `XPLMDestroyTexture` |
| `ImgPanelGraphics::TransformPush` | &rarr; | `XPLMTransformPush` |
| `ImgPanelGraphics::TransformPop` | &rarr; | `XPLMTransformPop` |
| `ImgPanelGraphics::TransformTranslate` | &rarr; | `XPLMTransformTranslate` |
| `ImgPanelGraphics::TransformScale` | &rarr; | `XPLMTransformScale` |
|  _n/a_* | <span style="color: gray;">&rarr;</span> | `XPLMDrawCalls` |

*\* We purposefully do not expose a proxy for `XPLMDrawCalls`, as the framework strictly manages the ImGui vertex buffer submissions internally.*

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

While `ImgWindow` automatically abstracts away most of the rendering pipeline multiplexing (OpenGL vs. Panel Graphics), there are a few important caveats where the abstraction is weaker. In the following cases, you **must** modify your code to prevent crashes:

#### Caveat A: Strict Main-Thread Execution (No Background Allocation)
The X-Plane SDK enforces a strict **Serialization Rule**: all XPLM API calls must occur sequentially on X-Plane's main thread. 
*   **The Trap:** Because legacy OpenGL is a separate library, some developers got away with allocating textures on background threads. However, Panel Graphics texture allocation is an *XPLM SDK feature*. If you try to call `ImgWindow::CreateCustomTexture()` from a background thread (`std::thread`, `std::async`), the XPLM SDK will immediately assert and crash the simulator.
*   **The Fix:** Keep your file I/O and pixel decoding (`stbi_load`) on your background worker thread. Once the bytes are decoded, hand the raw buffer back to the **main X-Plane thread** (e.g., during your next window draw or flight-loop callback) where you will safely call `ImgWindow::CreateCustomTexture()`.

#### Caveat B: The Uninitialized Handle Trap
In legacy OpenGL, attempting to bind an uninitialized or garbage texture handle might simply fail silently or draw a blank white square. Panel Graphics is not forgiving.
*   **The Trap:** If you pass a random, uninitialized memory address (e.g., garbage data from an uninitialized variable) into Panel Graphics, the driver will instantly crash when trying to dereference it.
*   **The Fix:** Ensure every single `ImTextureID` variable in your plugin is explicitly initialized to `nullptr` (or `0`). If a texture is explicitly null, the `ImgWindow` framework will safely ignore it and protect the GPU. However, the framework cannot magically detect the difference between a valid texture handle and random garbage memory. **You must null-initialize your pointers!**

#### Caveat C: Custom Textures & `ImGui::Image()` Legacy Conversion
The Panel Graphics Vulkan/Metal backend has no knowledge of legacy OpenGL texture IDs. If your UI code generates custom textures via `glGenTextures()` and passes those raw GL integer IDs into `ImGui::Image()`, **X-Plane will instantly crash** if that specific window is being rendered via Panel Graphics.

**The Fix:** Upgrade your texture generation to use the unified `ImgWindow::CreateCustomTexture()` API so it seamlessly multiplexes between both backends.

**Incremental Migration:** Alternatively, if you aren't ready to refactor all your OpenGL textures right now, but still want to test Panel Graphics, you can temporarily hide those specific legacy `ImGui::Image` calls when Panel Graphics is active. By using the `IsUsingPanelGraphics()` method on your `ImgWindow` subclass, you can safely branch your draw logic:

```cpp
#define HIDE_FROM_PG(x) if (!this->IsUsingPanelGraphics()) { x }

HIDE_FROM_PG(
    ImGui::Image((void*)(intptr_t)myLegacyGLTextureId, ImVec2(100, 100));
)
```

---

### 5. Visual Polish: Texture Bake Delay (Ghosting)

Because X-Plane 12's VRAM texture uploads take time under the modern graphics pipeline, heavy windows with complex font atlases may exhibit visual jitter or texture pop-in for the first few frames as the GPU bakes the new glyphs under the hood.

To mitigate this, `ImgWindow` includes an optional **Texture Bake Delay**. This feature holds the window entirely transparent for a specified number of frames immediately after creation, masking the texture upload process.

**YMMV (Your Mileage May Vary):** Depending on your hardware and the complexity of your font atlas, this delay may or may not make a visually significant difference. It is provided strictly as a tuning knob for developers trying to smooth out off-putting text flashing during initial window loads.

To enable the delay, call the setter **immediately after** constructing the window (within the same flight loop cycle), or place it directly inside your derived window class's constructor. If you defer the call, it will have no effect.

```cpp
// Option 1: Inside your derived window class constructor
MyHeavyWindow::MyHeavyWindow(...) : ImgWindow(...) {
    this->SetTextureBakeDelay(true); // Hold transparent for 2 frames (default)
}

// Option 2: Immediately after instantiation
MyHeavyWindow *win = new MyHeavyWindow(...);
win->SetTextureBakeDelay(true); 
```
*(Note: This setting is ignored completely if the window falls back to legacy OpenGL).*

### Call for Errata or Omissions

As with the main [README](../README.md), please feel free to submit a PR or feedback directly to the author if you are interested in improving this document, correcting any inaccuracies or outright errors, and/or adding more relevant examples, tools, documentation, or references.

This file was last updated in *September, 2026* by Steven L. Goldberg.
