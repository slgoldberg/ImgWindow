# Migration Guide: `ImgWindow v2`

Welcome to the `ImgWindow v2` Migration Guide. With the release of X-Plane 12.4.4b3, Laminar fundamentally upgraded their Vulkan rendering pipeline, allowing us to drop several restrictive hacks and unify our APIs.

Depending on what version of `ImgWindow` your plugin is currently using, choose your migration path below:

---

## Part 1: Upgrading from ImgWindow v1.3.0 (Panel Graphics Early Adopters)

If you were an early adopter of `ImgWindow` v1.3.0 (which brought initial Panel Graphics support during the X-Plane 12.4.4b1 and b2 betas), you successfully navigated a highly restrictive, asynchronous rendering era under the authoritarian rule of King Laminar "bee-wan" and his son, Laminar "bee-too".

Fortunately, Prince I-Am-GeeWin-Dough II (b3) has democratized the kingdom! A new polymorphic era of true happiness has arrived for the citizens of *Panelgraphica* and *Opengeel*, who can now live in peace and harmony thanks to the new Bridge of Synchrony!

Because X-Plane 12.4.4b3 now natively handles deferred Vulkan command encoding, we have stripped out the complex Flight Loop Callback (FLCB) garbage collection queues. 

### Required Code Changes for v2:
1. **Texture Destruction:** The `SafeDeleteTexture()` method is officially deprecated.
   * **Migration:** Replace all calls to `SafeDeleteTexture(tex)` with the new, unified `ImgWindow::DestroyCustomTexture(tex)` method. You can safely call this synchronously from the main thread!
2. **Texture Bake Delays:** The `SetTextureBakeDelay()` method is officially deprecated. Panel Graphics now builds and binds textures instantly and synchronously.
   * **Migration:** You can completely delete any calls to `SetTextureBakeDelay()`. (The method has been stubbed out as an inline no-op, so your code will still compile if you forget, but it is no longer doing anything).

*That's it! Everything else from v1.3.0 works perfectly.*

---

## Part 2: Upgrading from earlier versions of ImgWindow (Legacy OpenGL)

If you are migrating your plugin from an older version of the framework (Legacy OpenGL) to `v2` (which adds modern Panel Graphics support), this section covers everything you need to know to safely transition to the modern, hyper-performant backend—while retaining full backwards compatibility for your X-Plane 11 users!

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
| <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code><br>~~XPLM440~~&nbsp;_must&nbsp;**NOT**&nbsp;be&nbsp;defined_ | **Dynamic Bridge (Recommended).** Uses Panel Graphics if XPLM 4.4 is detected at runtime. Falls back to legacy OpenGL otherwise. | **Maximum.** X-Plane 11.10+ through X-Plane 12.4.4+ |
| <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code><br> <code>&#8209;DXPLM440=1</code>&nbsp;defined | **Strict Panel Graphics.** Forces modern rendering and strips the OpenGL fallback logic. *(See Caveat below regarding X-Plane 11)* | **Modern Only.** X-Plane 12.4.4 and newer. |
| *(None)* | **Strict OpenGL.** Ignores Panel Graphics entirely and forces legacy OpenGL rendering. | **Standard.** X-Plane 11.10+ through current. |

**To enable the recommended Dynamic Bridge in CMake:**
```cmake
add_definitions(-DIMGWINDOW_USE_PANEL_GRAPHICS)
# IMPORTANT: Ensure you do NOT define XPLM440 anywhere in your CMakeLists!
# Laminar's headers check `#if defined(XPLM440)`. Defining it at all (even as XPLM440=0)
# will break the dynamic bridge and force Strict Panel Graphics mode.
```
*(Note: Be sure to run a `make clean` or clear your CMake cache after altering these definitions to ensure precompiled headers are rebuilt correctly).*

---

### 2. Verifying the Pipeline

When your plugin initializes its first ImGui window, `ImgWindow` will log its routing decision to X-Plane's `Log.txt`. To verify that Panel Graphics is active, look for the following line:

> `ImgWindow: Rendering via XPLM v4.4 Panel Graphics`

---

### 3. Custom Textures (e.g., `ImGui::Image`)

> [!NOTE]
> **What _"custom textures"_ are we talking about here?**<br>
> This section is *only* for custom 2D images you want to draw inside your ImGui windows (like plugin icons, custom gauges, or photos), using functions like `ImGui::Image()`, `ImGui::ImageButton()`, and any other ImGui calls taking an `ImTextureID` parameter.
> * **Not _"Font Atlas"_ textures:**
The `ImgWindow` framework automatically manages ImGui's font textures for you with its `ImgFontAtlas` service.
> * **Not _"World" (scenery)_ textures:**
X-Plane's scenery, aircraft liveries, and `.obj` textures are managed natively.

**Does your plugin load custom textures?**

&nbsp;&rarr;&nbsp;If you answered _"yes"_, then you will need to **migrate your texture code** to support Panel Graphics.

First, the **good news**:

Any discussion you heard previously about Panel Graphics requiring complex flight-loop refactoring to safely create or destroy textures outside the `draw` callback is now officially _outdated_ (and invalid). As of `XPLM v4.4b3` (and presumably the final release), which starts with `X-Plane 12.4.4b3`, you can **keep your existing synchronous code structure!** I.e., you can safely create and destroy textures right inside your draw callbacks if you like. Panel Graphics will provide an almost identical developer experience to the OpenGL pipeline.

However, you **must** use the new Panel Graphics API to allocate those textures, since you cannot send OpenGL texture handles through the Panel Graphics pipeline without a hard crash!

And when you do so -- if you want your plugin to maintain backward compatibility with legacy OpenGL -- you will need to explicitly check which pipeline is active and multiplex your calls. You have two choices for how to do this:

#### A. Directly, Using the Proxy Namespace _(The "Do-It-Yourself" Way)_

To support this backward compatibility, the framework introduces a dedicated proxy namespace called `ImgPanelGraphics`.

If you compile a plugin using the native `XPLM` functions (like `XPLMCreateTexture`) from the v4.4 SDK, the operating system linker creates a hard dependency on those symbols. If a user attempts to run your plugin in older simulators like X-Plane 11, the OS will fail to load the plugin entirely because those symbols do not exist in the older binary.

This namespace solves the problem by dynamically looking up the Vulkan/Metal functions at runtime. It seamlessly adapts to your build configuration: if you build a backward-compatible plugin (the default), it safely accesses Panel Graphics features only when available; if you explicitly build against the strict v4.4 SDK (which drops legacy OpenGL support), it routes directly. 

If you want to manually manage your own Panel Graphics rendering, you should **never** call the raw XPLM versions directly. Instead, you should always route your calls through our proxies:
| 🟢 Always Call This: | | 🔴 NEVER Call This (Raw XPLM): |
| :--- | :---: | :--- |
| `ImgPanelGraphics::CreateTexture`<br>*or* `ImgWindow::CreateCustomTexture` | &rarr; | `XPLMCreateTexture` |
| ~~`ImgPanelGraphics::DestroyTexture`~~<br>`ImgWindow::DestroyCustomTexture` | &rarr; | `XPLMDestroyTexture` |
| `ImgPanelGraphics::TransformPush` | &rarr; | `XPLMTransformPush` |
| `ImgPanelGraphics::TransformPop` | &rarr; | `XPLMTransformPop` |
| `ImgPanelGraphics::TransformTranslate` | &rarr; | `XPLMTransformTranslate` |
| `ImgPanelGraphics::TransformScale` | &rarr; | `XPLMTransformScale` |
| _&nbsp;n/a<sup>*</sup>_ | <span style="color: gray;">&rarr;</span> | `XPLMDrawCalls` |

_<sup>*</sup>We purposefully do not expose a proxy for `XPLMDrawCalls`, as the framework strictly manages the ImGui vertex buffer submissions internally._

If you choose to use these proxies directly, and you intend to support either Panel Graphics _or_ OpenGL rendering pipelines (of course, only one or the other, based on the build parameters and/or the runtime environment), you must manually _multiplex_ your plugin's calls based on `ImgPanelGraphics::IsAvailable()` (or the convenience method, `ImgWindow::IsUsingPanelGraphics()` that returns the same boolean result). For example, instead of replacing your legacy OpenGL texture creation logic (if your plugin needed such) with a direct call to `XPLMCreateTexture()`, foregoing the legacy support, you might use the `ImgPanelGraphics::` proxy instead -- for example:

```cpp
#ifdef IMGWINDOW_USE_PANEL_GRAPHICS
if (ImgPanelGraphics::IsAvailable()) {
    myPanelGraphicsHandle = ImgPanelGraphics::CreateTexture(pixels, w, h);
} else 
#endif
{
    // legacy OpenGL fallback
    glGenTextures(1, &myLegacyGLHandle);
    // ... setup texture params ...
}
```

Just by contrast, here is how you do the exact same thing "the easy way":

```cpp
// Look ma, no #ifdefs!
myImTextureID = ImgWindow::CreateCustomTexture(pixels, w, h);
```

Read on into the next section for a full explanation...

#### B. Through the `ImgWindow` Unified Texture API _("The Easy Way")_

If your plugin loads custom textures to inject into `Dear ImGui` (e.g., using `ImGui::Image()`), writing boilerplate `if/else` multiplexing blocks everywhere as described above can be quite tedious. Among other things, just managing the return values that are of different types can cause serious issues. (For example, OpenGL texture handles of type `GLuint` will instantly crash the simulator if you attempt to load such handles within a Panel Graphics window!)

To make it so you can have your plugin support _either_ backend (OpenGL _or_ Panel Graphics) completely painlessly, we have pre-baked the two most common use cases directly into the framework. This way, you can skip the manual proxy methods entirely for these two cases, and just use our unified helpers: `ImgWindow::CreateCustomTexture()` and `ImgWindow::DestroyCustomTexture()`. (Of course, you are in no way _required_ to use these; they're just available if you would like to use them, i.e. to reduce unnecessary boilerplate for such common cases.)

These multiplexers automatically determine the active rendering pipeline (Panel Graphics vs. OpenGL), and create the correct type of texture for you **under the hood** -- safely returning an agnostic `ImTextureID` that you can pass directly to ImGui!  These common IDs can be used regardless of whether ImGui is rendering via Panel Graphics, or using OpenGL on older versions of X-Plane that don't support Panel Graphics. Basically, it lets developers focus on the *what* in ImGui terms, not the *how* in low-level rendering pipeline terms.

##### 1. Creating the Texture (Force 4 Channels!)
When loading external images (e.g., PNGs via `stb_image`), X-Plane's Panel Graphics API strictly requires a 4-channel RGBA8 buffer. If you feed it a 3-channel RGB buffer, the simulator will instantly crash due to a buffer overrun!  _(To be clear: don't pass 3 or 0 as the final parameter to `stbi_load()` -- **explicitly pass 4**)._

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
> [!WARNING]
> 🔥 **CRITICAL TYPE WARNING: The 64-bit Vulkan Trap!** 🔥
> If you are migrating an existing OpenGL codebase, you **must** declare `myCustomTexture` (and any variable that stores it) as an `ImTextureID`. 
> 
> You absolutely cannot store it in legacy 32-bit integer types like `GLuint`, `unsigned int`, or `XPLMTextureID` anymore. `CreateCustomTexture` returns a **64-bit pointer** (`void*`). If you assign it to a 32-bit `GLuint`, your C++ compiler will violently truncate the top half of the memory address. When you later pass that mangled variable into `ImGui::Image()`, X-Plane will instantly crash with `Resource does not belong to your plugin`. **You are no longer writing OpenGL code; your contract with the framework is strictly through `ImTextureID`!**

*Note on Alpha Blending:* Panel Graphics relies on straight alpha blending. If your image has fully transparent areas with black RGB values (0, 0, 0, 0), it may cause dark halos around semi-transparent edges. Ensure your assets are exported with a white matte, or manually sanitize the RGB channels of fully transparent pixels before calling `ImgWindow::CreateCustomTexture()`.

##### 2. Drawing the Texture
Once your texture is loaded and cast to an `ImTextureID`, rendering it inside your window's `ImgWindow::buildInterface()` method is completely agnostic:

```cpp
void MyWindow::buildInterface() {
    if (myCustomTexture != nullptr) {
        ImGui::Image(myCustomTexture, ImVec2(256.0f, 256.0f));
    }
}
```

##### 3. Cleaning Up
Thanks to X-Plane 12.4.4 handling memory deferral _natively_ (as of v12.4.4b3), you no longer have to manually branch texture destruction or build flight loops to protect Vulkan queues. Just hand the texture back to ImgWindow to destroy it safely. 

*(Note: Just like creating textures, destroying them is an XPLM SDK call under the hood. You must **never** call `ImgWindow::DestroyCustomTexture()` from a background worker thread!)*

```cpp
void UnloadMyCustomTexture() {
    if (myCustomTexture) {
        // Safe to call synchronously on the MAIN THREAD!
        ImgWindow::DestroyCustomTexture(myCustomTexture);  // the easy way :-)
        myCustomTexture = nullptr; // Always null out your own pointers!
    }
}
```

---

### 4. Developer Caveats & Required Code Changes

While `ImgWindow` automatically abstracts away most of the rendering pipeline multiplexing (OpenGL vs. Panel Graphics), there are a few important caveats where the abstraction is weaker. In the following cases, you **must** modify your code to prevent crashes:

---

#### Caveat A: Strict Main-Thread Execution (No Background Allocation)
The X-Plane SDK enforces a strict **Serialization Rule**: all XPLM API calls must occur sequentially on X-Plane's main thread.

*   **The Trap:** Because legacy OpenGL is a separate library, some developers got away with allocating textures on background threads. However, Panel Graphics texture allocation is an *XPLM SDK feature*. If you try to call `ImgWindow::CreateCustomTexture()` from a background thread (`std::thread`, `std::async`), the XPLM SDK will immediately assert and crash the simulator.

*   **The Fix:** Keep your file I/O and pixel decoding (`stbi_load`) on your background worker thread. Once the bytes are decoded, hand the raw buffer back to the **main X-Plane thread** (e.g., during your next window draw or flight-loop callback) where you will safely call `ImgWindow::CreateCustomTexture()`.

---

#### Caveat B: The Uninitialized Handle Trap
In legacy OpenGL, attempting to bind an uninitialized or garbage texture handle might simply fail silently or draw a blank white square. Panel Graphics is not forgiving.

*   **The Trap:** If you pass a random, uninitialized memory address (e.g., garbage data from an uninitialized variable) into Panel Graphics, the driver will instantly crash when trying to dereference it.

*   **The Fix:** Ensure every single `ImTextureID` variable in your plugin is explicitly initialized to `nullptr` (or `0`). If a texture is explicitly null, the `ImgWindow` framework will safely ignore it and protect the GPU. However, the framework cannot magically detect the difference between a valid texture handle and random garbage memory. **You must null-initialize your pointers!**

---

#### Caveat C: Legacy OpenGL Types and Pointer Truncation (The 32-bit Trap)
If you are migrating an existing OpenGL codebase, it is highly likely you stored your texture handles using legacy 32-bit integer types like `GLuint` or `XPLMTextureID`. You **must** refactor these to `ImTextureID`.

*   **The Trap:** While `ImgWindow::CreateCustomTexture()` perfectly abstracts the backend, it returns an `ImTextureID`, which is a **64-bit pointer** (`void*`) on modern OSes. If you assign this return value to an old `GLuint` or `XPLMTextureID` variable, your C++ compiler will violently truncate the top 32 bits of the Vulkan pointer. When you later pass that truncated variable into `ImGui::Image()`, `ImgWindow` will hand the garbage pointer to X-Plane, which will instantly abort the simulator with: `Resource does not belong to your plugin`.

*   **The Fix:** You are no longer writing OpenGL code; you are writing *ImGui* code! Search your entire codebase and replace any `GLuint`, `unsigned int`, or `XPLMTextureID` variables that store texture handles with `ImTextureID` (or `void*`). Your contract with the framework is strictly through `ImTextureID`.

---

#### Caveat D: Custom Textures & `ImGui::Image()` Legacy Conversion
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

#### Caveat E: The Boilerplate CMake Trap (`XPLM440`)
It is common practice for developers to blindly append the latest SDK version to their compiler flags, e.g.:<br>
<code>&#8209;DXPLM200=1 &#8209;DXPLM210=1 ... &#8209;DXPLM430=1 &#8209;DXPLM440=1</code><br>

**&rarr; Don't do this!**  I.e., do not set `XPLM440` if you want the Dynamic Bridge to use Panel Graphics when it's available, and falling back to OpenGL when it's not (i.e., on older versions of X-Plane back to v11.10).

However, if you are defining `XPLM440` as a build requirement because you are using _other_ XPLM v4.4 SDK features -- _aside from_ or in addition to Panel Graphics -- then so be it! (But please understand that this disables the "bridge" back to OpenGL and won't run on any older version of X-Plane that doesn't support the XPLM v4.4 SDK!)

*   **The Trap:** If you define `XPLM440` (or even `XPLM440=0`!), then `ImgWindow` is forced to compile its `XPLMCreateWindow_t` struct to the size defined in the XPLM v4.4 SDK! If this plugin is loaded into X-Plane 11 or even earlier versions of X-Plane 12 that don't support Panel Graphics, the `ImgWindow` dynamic bridge will correctly fall back to legacy OpenGL, but it will attempt to pass the massive v4.4 SDK's window-creation struct to an older SDK that has no idea how to read it. (While X-Plane's forward-compatibility may allow this "Frankenstein" window to spawn, it is unsupported behavior.)

*   **The Fix:** If the only reason you want the v4.4 SDK is to use Panel Graphics, and you want full backwards compatibility with older versions of X-Plane, then define <code>&#8209;DIMGWINDOW_USE_PANEL_GRAPHICS</code> but stop at `-DXPLM430=1`. (In other words, do *not* define `XPLM440`. You don't need it with `ImgWindow`'s panel graphics bridge support!)

> [!NOTE]
> **Only define `XPLM440` in your build configuration** if you explicitly intend to abandon X-Plane 11 through 12.4.4b2 compatibility, dropping support for users who don't or can't update to X-Plane v12.4.4b3 or later. But understand that you don't _need_ to abandon support if the _only_ v4.4 feature you need is Panel Graphics, since `ImgWindow` provides that for "free" when you define `IMGWINDOW_USE_PANEL_GRAPHICS`! (This provides the entire Panel Graphics API through the `ImgPanelGraphics::` namespaced proxy that lets you use the API if it's available, else it gracefully degrades to OpenGL rendering instead!)

---

### Call for Errata or Omissions

As with the main [README](../README.md), please feel free to submit a PR or feedback directly to the author if you are interested in improving this document, correcting any inaccuracies or outright errors, and/or adding more relevant examples, tools, documentation, or references.

This file was last updated in *October, 2026* by Steven L. Goldberg, for `ImgWindow v2`.

---

#### Caveat F: The 1-Frame Deferred Deletion & Teardown Leaks (`Shutdown`)
Because X-Plane 12's modern Panel Graphics backend executes draw calls synchronously during your flight loop, it introduced a new timing hazard: if you destroy a texture handle (like your font atlas) while ImGui is still building its draw list, X-Plane's Vulkan backend will instantly crash when it attempts to draw the destroyed handle milliseconds later.

*   **The Trap:** To prevent this crash, `ImgWindow v2` implements a **1-frame deferred deletion queue**. When you call `ImgWindow::DestroyCustomTexture()`, the texture isn't actually deleted immediately; it is placed in a queue and deleted during the *next* flight loop cycle. 
However, if your plugin is being disabled or stopped (e.g., inside `XPluginDisable` or `XPluginStop`), the flight loop *stops running*. If you destroy the font atlas at shutdown, it gets put into the 1-frame queue, but the next frame never comes! This leaves orphaned textures in VRAM and can cause a driver crash when X-Plane tears down your plugin's graphics context.

*   **The Fix:** You **must** manually flush the deferred deletion queue at the end of your plugin's lifecycle. Inside your `XPluginDisable` (or wherever you do your final ImGui teardown), immediately after you destroy the font atlas and ensure no other ImgWindow code will run, you must call:
    ```cpp
    ImgWindow::Shutdown();
    ```
    
    This instantly bypasses the 1-frame delay and wipes the queue clean, ensuring a safe exit. *(For a complete, copy-pasteable example of a safe font atlas teardown block, see the "Safe Teardown Example" section in the [Basic Usage Guide](Basic-Usage-Guide.md)).*
