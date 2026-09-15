## `ImgWindow` & Panel Graphics: Migration Guide

With the release of X-Plane 12.4.4b1, Laminar Research introduced the **Panel Graphics API** (XPLM v4.4), routing UI rendering through a modern Vulkan/Metal backend. 

`ImgWindow` now provides a **dynamic, backward-compatible bridge** to this new pipeline. You can inject ImGui directly into the modern Panel Graphics pipeline on XP12.4.4+ while simultaneously maintaining 100% backward compatibility with older OpenGL-based versions of X-Plane (XP11.10 through early XP12). 

There is no need to maintain two separate codebases or force your users to upgrade X-Plane. The framework detects the host simulator's capabilities at runtime and routes the ImGui draw data accordingly.

### 1. Build Configurations

You control how `ImgWindow` interacts with Panel Graphics entirely through CMake compiler definitions. 

| Build Flags | Rendering Pipeline | Simulator Compatibility |
| :--- | :--- | :--- |
| <nobr>`-DIMGWINDOW_USE_PANEL_GRAPHICS`</nobr> | **Dynamic Bridge (Recommended).** Uses Panel Graphics if XPLM 4.4 is detected at runtime. Falls back to legacy OpenGL otherwise. | **Maximum.** X-Plane 11.10+ through X-Plane 12.4.4+ |
| <nobr>`-DIMGWINDOW_USE_PANEL_GRAPHICS`</nobr><br> `-DXPLM440=1` | **Strict Panel Graphics.** Forces modern rendering and strips the OpenGL fallback logic. | **Modern Only.** X-Plane 12.4.4 and newer. Will not load on older versions. |
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

**Crucial:** Do not allocate Vulkan/Metal textures inside an active drawing callback (like `ImgWindow::buildInterface()`). Allocate them during your plugin's initialization (`XPluginEnable`) or in a dedicated pre-drawing flight loop callback (i.e., in code that is only executed once, or when a texture needs to be lazily created). (This is discussed in more detail below.)

#### B. Loading Custom Textures

**IMPORTANT:** Some plugins use `ImGui::Image()`, for example, to explicitly inject previously-loaded GPU textures into their `Dear ImGui` interfaces. Other plugins stay clear and only use fonts, which are handled entirely by the framework. **If you are using Panel Graphics, and your plugin loads custom textures via `ImgUI::Image()`, then you need to read this _before_ you enable `-DIMGWINDOW_USE_PANEL_GRAPHICS`** -- especially if you intend to support a mixed mode release using Panel Graphics textures on v12.4.4 or later, and OpenGL textures on older versions as far back as XP11.10!  In fact, whether you force the build to only run on `XPLM440` or not, you should _always_ use our proxy `ImgPanelGraphics::CreateTexture()` API wrapper to call the XPLM v4.4 SDK's `XPLMCreateTexture()` function!  (I.e., you should _not_ call it directly when building textures for ImGui via `ImgWindow`.) Read on for a full explanation and example...

##### Always FORCE 4 channels (RGBA) when loading images for compatibility with Panel Graphics

When loading external images (e.g., PNGs via `stb_image`), X-Plane's Panel Graphics API strictly requires a 4-channel RGBA8 buffer. If you feed it a 3-channel RGB buffer, the simulator will instantly crash due to a buffer overrun!  _(To be clear: don't pass 3 or 0 as the final parameter to `stbi_load()`, for example -- instead, **explicitly pass 4** as the final parameter!  See example below.)_

Here is the standard pattern for safely loading custom textures based on whether `ImgWindow` is rendering your ImGui code through OpenGL vs. Panel Graphics by way of `ImgPanelGraphics::IsAvailable()`:

```cpp
// 1. Define your texture handle globally or in your plugin class
ImTextureID myCustomTexture = nullptr;

// 2. Load the texture (Run this in XPluginEnable or a setup Flight Loop)
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

When your plugin is disabled, calling your `XPluginDisable()`, or when a texture is no longer needed, you must use the same matching `ImgPanelGraphics::DestroyTexture()` **proxy** method for the corresponding `XPLMDestroyTexture()` function from the v4.4 SDK (again, whether or not you explicitly require `XPLM440` in your build configuration)!  As with the warning above, **do not** call the native `XPLMDestroyTexture()` function directly since it won't exist if you didn't build with `XPLM440`, and even if you did, you'd have to change the code if you ever changed your mind and removed the requirement for `XPLM440` so that you could achieve backwards-compatibility.

Below is the analagous "delete" sample code for the above "create" example:

```cpp
void UnloadMyCustomTexture() {
    if (myCustomTexture) {
        if (ImgPanelGraphics::IsAvailable()) {
            ImgPanelGraphics::DestroyTexture((void*)(intptr_t)myCustomTexture);
        } else {
            GLuint glTextureId = (GLuint)(intptr_t)myCustomTexture;
            glDeleteTextures(1, &glTextureId);
        }
        myCustomTexture = nullptr;
    }
}
```

#### C. Drawing Custom Textures

Once your texture is loaded and cast to an `ImTextureID`, rendering it inside your window's `ImgWindow::buildInterface()` method is completely agnostic. Both pipelines automatically join back together here.

```cpp
void MyWindow::buildInterface() {
    if (myCustomTexture != nullptr) {
        ImGui::Image(myCustomTexture, ImVec2(256.0f, 256.0f));
    }
}
```

---

### 4. Developer Caveats & Required Code Changes

While `ImgWindow` automatically handles the rendering pipeline abstraction, it **cannot** automatically translate custom OpenGL state managed by your plugin. If you use custom textures or spawn windows dynamically, you must account for the following constraints.

#### Caveat A: Custom Textures & `ImGui::Image()`
The Panel Graphics Vulkan/Metal backend has no knowledge of legacy OpenGL texture IDs. If your UI code generates custom textures via `glGenTextures()` and passes those raw GL integer IDs into `ImGui::Image()`, **X-Plane will instantly crash (CTD)** if that specific window is being rendered via Panel Graphics.

**The Fix:** As outlined in Section 3 above, you should ideally upgrade your texture generation to use the `ImgPanelGraphics::IsAvailable()`, `ImgPanelGraphics::CreateTexture()`, and `ImgPanelGraphics::DestroyTexture()` proxy functions (the latter two only being valid if the former is `true`!).

However, if you are migrating an older plugin and need to temporarily prevent legacy OpenGL textures from crashing your modern windows, you must branch your *draw logic* inside your override of `buildInterface()` using the bool getter function, `ImgWindow::IsUsingPanelGraphics()`.

In that case, you can temporarily stub out any crashing OpenGL textures using a macro until you are ready to rewrite your texture generation for Panel Graphics. This way, you can at least confirm that the rest of your ImGui rendering is properly using the Panel Graphics API (i.e., for fonts and standard UI elements):
```cpp
// Example macro to hide legacy OpenGL textures while testing ImGui via Panel Graphics:
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

---

### 5. Visual Polish: Texture Bake Delay (Ghosting)

Because X-Plane 12's VRAM texture uploads are asynchronous under Vulkan/Metal, heavy windows with complex font atlases may exhibit visual jitter or texture pop-in for the first few frames as the GPU bakes the new glyphs in the background.

To mitigate this, `ImgWindow` includes an optional **Texture Bake Delay**. This feature holds the window entirely transparent for a specified number of frames immediately after creation, masking the asynchronous upload.

**YMMV (Your Mileage May Vary):** Depending on your hardware and the complexity of your font atlas, this delay may or may not make a visually significant difference. It is provided strictly as a tuning knob for developers trying to smooth out off-putting text flashing during initial window loads.

To enable the delay, call the setter **immediately after** constructing the window (within the same flight loop cycle). If you defer the call, it will have no effect.
(If you want such a delay for all instances of a particular subclass derived from `ImgWindow`, then you simply call `SetTextureBakeDelay(true)` from within the class' constructor, since it is always run after the main `ImgWindow::ImgWindow()` constructor is run, thus setting the "bake delay" so it takes effect before you return from creating the window.)

**_Caveat:_** This method should **not** be _required_ at all, so you can safely ignore this section entirely. It's only here for visual polish once things are working, if you find you have any windows that are so font-laden that they end up looking strange to users when they are first opened!  **There is no _requirement_ in Panel Graphics to "bake" your font textures (or any textures, for that matter).**

**Example** (for a single, specific window, just after you've created it, but before you return control to XPLM):

```cpp
// Immediately after creating the window, e.g.:
MyImgWindowSubclass *myHeavyWindow = new MyImgWindowSubclass(...);

// Hold the window transparent for 2 frames (default) upon creation:
myHeavyWindow->SetTextureBakeDelay(true); 

// Or specify a custom frame delay for exceptionally heavy textures:
myHeavyWindow->SetTextureBakeDelay(true, 5);  // 5-frame initial delay
```
*(Note: This setting is ignored completely if the window falls back to legacy OpenGL, so you do not need to wrap it in an `IsUsingPanelGraphics()` check.)*

---

### Call for Errata or Omissions

As with the main [README](../README.md), please feel free to submit a PR or feedback directly to the author if you are interested in improving this document, correcting any inaccuracies or outright errors, and/or adding more relevant examples, tools, documentation, or references.

This file was last updated in *September, 2026* by Steven L. Goldberg.
