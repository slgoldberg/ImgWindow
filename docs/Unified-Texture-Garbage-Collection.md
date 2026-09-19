### ImgWindow Phase 6: Unified Texture Garbage Collection

**The Problem:**
Under X-Plane 12's modern Vulkan and Metal pipelines, all rendering is deferred via command queues[cite: 7]. If you close a UI window and synchronously destroy a custom texture (via `XPLMDestroyTexture` or `glDeleteTextures`), you free the VRAM while the GPU is still processing the previous frame's queue. This results in an instant `SIGSEGV` crash[cite: 7].

Previously, developers had to build their own custom flight-loop arrays to manually defer texture deletion by 2–3 cycles[cite: 5].

**The Solution:**
ImgWindow now provides a native, framework-level garbage collection state machine to safely manage the lifecycle of custom plugin-owned textures[cite: 7]. Instead of managing flight loops or branching your code for OpenGL vs. Panel Graphics, you simply hand the texture to the framework.

#### How to Use It
When you are done with a custom texture (e.g., a user closes a window, or you are swapping an image), pass the `ImTextureID` to the new framework method:

```cpp
// Old Way (Crash prone, or requires manual flight-loop timers):
// ImgPanelGraphics::DestroyTexture(myCustomTex);

// New Way (100% Safe):
myWindow->TrashCustomTexture(myCustomTex);
myCustomTex = nullptr; // Always null out your own pointers!
```

#### How It Works Behind the Scenes
1. **Unified API:** It works seamlessly regardless of whether you are running the modern Panel Graphics pipeline or the legacy OpenGL fallback[cite: 7].
2. **Smart Synchronization:** The framework queries ImGui's internal CPU draw lists. It waits until the texture is no longer being actively drawn in *any* viewport.
3. **Vulkan Cooldown:** Once the texture clears the CPU, the framework applies a strict 3-frame cooldown to guarantee the GPU command queues have completely flushed before silently destroying the texture in the background.

*(Note: This Phase 6 API relies on modern ImGui internal structures and is strictly gated behind the `IMGUI_V192_REFACTOR` compile flag. Ensure your CMake configuration includes this definition!)*
