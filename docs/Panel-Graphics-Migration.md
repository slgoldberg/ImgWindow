## `ImgWindow` & Panel Graphics: Migration Guide

With the release of X-Plane 12.4.4b1, Laminar Research introduced the **Panel Graphics API** (XPLM v4.4), routing UI rendering through a modern Vulkan/Metal backend. 

`ImgWindow` now provides a **dynamic, backward-compatible bridge** to this new pipeline. You can inject ImGui directly into the modern Panel Graphics rendering pipeline on X-Plane v12.4.4+ while simultaneously maintaining **100% backward compatibility** with any OpenGL-based versions of X-Plane starting with v11.10 onward (including all versions of v12 as well).

There is no need to maintain two separate codebases or force your users to upgrade X-Plane. The framework detects the host simulator's capabilities at runtime and routes the ImGui draw data accordingly.

### 1. Build Configurations

You control how `ImgWindow` interacts with Panel Graphics entirely through CMake compiler definitions. 

| Build Flags | Rendering Pipeline | Simulator Compatibility |
| :--- | :--- | :--- |
| <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code> | **Dynamic Bridge (Recommended).** Uses Panel Graphics if XPLM 4.4 is detected at runtime. Falls back to legacy OpenGL otherwise. | **Maximum.** X-Plane 11.10+ through X-Plane 12.4.4+ |
| <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code><br> `-DXPLM440=1` | **Strict Panel Graphics.** Forces modern rendering and strips the OpenGL fallback logic. | **Modern Only.** X-Plane 12.4.4 and newer. Will not load on older versions. |
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

### 3. Panel Graphics & Custom Textures (`ImgPanelGraphics`)

To support the modern Vulkan/Metal rendering pipeline introduced in X-Plane 12, this framework includes a dedicated proxy namespace: `ImgPanelGraphics`.

#### Why a Proxy Namespace?

If you compile a plugin using the native `XPLMCreateTexture` functions from the v4.4 SDK, the operating system linker will create a hard dependency on those symbols. If a user attempts to run your plugin in X-Plane 11, the OS will fail to load the plugin entirely because those symbols do not exist.

The `ImgPanelGraphics` namespace solves this by dynamically looking up the Vulkan/Metal functions at runtime. By routing your custom texture and draw calls through this namespace, your plugin will seamlessly utilize modern Panel Graphics on X-Plane 12, while gracefully falling back to OpenGL on older simulators—**without requiring you to compile against the v4.4 SDK.**

#### A. Checking Availability

Because graphics pipelines are strict, you must determine which pipeline is active before allocating graphics memory. Use the global `ImgPanelGraphics::IsAvailable()` method to branch your initialization logic.

**Crucial:** Do not allocate Vulkan/Metal textures inside an active drawing callback (like `ImgWindow::buildInterface()`). Allocate them during your plugin's initialization (`XPluginEnable`) or in a dedicated pre-drawing flight loop callback (i.e., in code that is only executed once, or when a texture needs to be lazily created).

#### B. Loading Custom Textures

**IMPORTANT:** Some plugins use `ImGui::Image()`, for example, to explicitly inject previously-loaded GPU textures into their `Dear ImGui` interfaces. Other plugins stay clear and only use fonts, which are handled entirely by the framework. **If you are using Panel Graphics, and your plugin loads custom textures via `ImgUI::Image()`, then you need to read this _before_ you enable `-DIMGWINDOW_USE_PANEL_GRAPHICS`** -- especially if you intend to support a mixed mode release using Panel Graphics textures on v12.4.4 or later, and OpenGL textures on older versions as far back as XP11.10!  In fact, whether you force the build to only run on `XPLM440` or not, you should _always_ use our proxy `ImgPanelGraphics::CreateTexture()` API wrapper to call the XPLM v4.4 SDK's `XPLMCreateTexture()` function! 

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

    if (ImgPanelGraphics::IsAvailable()) {
        // MODERN PIPELINE: Vulkan / Metal
        void* panelTexHandle = ImgPanelGraphics::CreateTexture(rgba_pixels, width, height);
        myCustomTexture = (ImTextureID)(intptr_t)panelTexHandle;
    } else {
        // LEGACY PIPELINE: OpenGL Fallback
        GLuint glTextureId = 0;
        glGenTextures(1, &glTextureId);
        glBindTexture(GL_TEXTURE_2D, glTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_pixels);
        
        myCustomTexture = (ImTextureID)(intptr_t)glTextureId;
    }
    
    stbi_image_free(rgba_pixels);
}
```

*Note on Alpha Blending:* Panel Graphics relies on straight alpha blending. If your image has fully transparent areas with black RGB values (0, 0, 0, 0), it may cause dark halos around semi-transparent edges. Ensure your assets are exported with a white matte, or manually sanitize the RGB channels of fully transparent pixels before calling `ImgPanelGraphics::CreateTexture()`.

##### Don't forget to clean up!
When a texture is no longer needed, you must use the same matching `ImgPanelGraphics::DestroyTexture()` **proxy** method. 

```cpp
void UnloadMyCustomTexture() {
    if (myCustomTexture) {
        if (ImgPanelGraphics::IsAvailable()) {
            ImgPanelGraphics::DestroyTexture((void*)(intptr_t)myCustomTexture);
        } else {
            GLuint glTextureId = (GLuint)(intptr_t)myCustomTexture;
            glDeleteTextures(1, &glTextureId);
        }
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
*   **The Trap:** If you use background threads (e.g., `std::thread`, `std::async`) for asynchronous texture loading, you **cannot** call `ImgPanelGraphics::CreateTexture()` from that background thread. Doing so will generate an invalid cross-thread handle or fatally crash the Vulkan driver. Legacy OpenGL drivers occasionally permitted off-thread allocation, but Panel Graphics strictly forbids it.
*   **The Fix:** Keep your file I/O and pixel decoding (`stbi_load`) on your background worker thread. Once the bytes are decoded, save them to a struct and use a flag or a flight loop callback to hand those bytes back to the **main X-Plane thread**, where you will actually call `CreateTexture`.

#### Caveat B: Deferred Texture Destruction (Asynchronous Garbage Collection)
Under Panel Graphics, all draw operations are deferred. `ImgWindow` submits your UI draw lists to a Vulkan command queue to be rendered later by the GPU.
*   **The Trap:** If you navigate away from a UI screen and synchronously call `ImgPanelGraphics::DestroyTexture()` on its custom images, you will free that VRAM while the GPU is still trying to read it to process the previous frame's queue. This will trigger an instant `SIGSEGV` crash.
*   **The Fix:** Application-side texture destruction must be deferred. Create a simple "garbage collection" array in your plugin. When an image is no longer needed, push its handle into the array alongside the current `XPLMGetCycleNumber()`. In a background flight loop, safely call `DestroyTexture` only on handles that are at least 2 or 3 cycles old.

#### Caveat C: The Uninitialized Handle / Null Pointer Trap
In legacy OpenGL, attempting to bind texture ID `0` would safely unbind the texture, often just drawing a blank white square. Vulkan and Metal are not forgiving.
*   **The Trap:** If you pass a garbage memory address (an uninitialized handle) or a `nullptr` directly into Vulkan, the driver will instantly crash.
*   **The Fix:** Ensure every single `ImTextureID` variable in your plugin is explicitly initialized to `nullptr` or `0` upon creation. (The `ImgWindow` framework now internally guards against passing `nullptr` references to the GPU, but it cannot protect you against random, uninitialized memory addresses.)

#### Caveat D: Custom Textures & `ImGui::Image()` Legacy Conversion
The Panel Graphics Vulkan/Metal backend has no knowledge of legacy OpenGL texture IDs. If your UI code generates custom textures via `glGenTextures()` and passes those raw GL integer IDs into `ImGui::Image()`, **X-Plane will instantly crash** if that specific window is being rendered via Panel Graphics.

**The Fix:** Upgrade your texture generation to use the `ImgPanelGraphics::IsAvailable()` proxy functions. If you need to temporarily prevent legacy OpenGL textures from crashing modern windows while you migrate, branch your draw logic using `ImgWindow::IsUsingPanelGraphics()`:

```cpp
#define HIDE_FROM_PG(x) if (!this->IsUsingPanelGraphics()) { x }

HIDE_FROM_PG(
    ImGui::Image((void*)(intptr_t)myLegacyGLTextureId, ImVec2(100, 100));
)
```

#### Caveat E: No "Lazy" Window Creation in Draw Callbacks
Under OpenGL, it was technically possible to lazily instantiate a new XPLM Window (`XPLMCreateWindowEx`) from *within* an active drawing callback. 
**This is strictly forbidden under Panel Graphics.** Attempting to create a new window while the pipeline is mid-execution will trigger a hard assert in Laminar's engine and instantly crash the simulator.
**The Fix:** Window creation must happen outside of the draw cycle. Set a boolean flag during your draw cycle, and construct the window inside a standard `xplm_FlightLoop_Phase_BeforeFlightModel` callback instead.

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

---

### Call for Errata or Omissions

As with the main [README](../README.md), please feel free to submit a PR or feedback directly to the author if you are interested in improving this document, correcting any inaccuracies or outright errors, and/or adding more relevant examples, tools, documentation, or references.

This file was last updated in *September, 2026* by Steven L. Goldberg.
