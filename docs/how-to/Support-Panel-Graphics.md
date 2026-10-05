# How-To: Support Panel Graphics (Migrating from Legacy OpenGL)

If your plugin currently relies on legacy OpenGL rendering through older versions of `ImgWindow`, you can easily migrate to modern Panel Graphics (introduced in X-Plane 12.4.4) while maintaining 100% backward compatibility for your X-Plane 11 and older X-Plane 12 users.

`ImgWindow v2` provides a **dynamic bridge** that automatically routes ImGui draw data to the correct pipeline at runtime.

## Step 1: Update Build Configurations

You control how `ImgWindow` interacts with Panel Graphics entirely through compiler definition flags. The recommended approach is to use the **Dynamic Bridge**, which falls back to legacy OpenGL on older simulators.

In your `CMakeLists.txt`:

```cmake
add_definitions(-DIMGWINDOW_USE_PANEL_GRAPHICS)
```

> [!WARNING]
> **The Boilerplate CMake Trap**
> Do **NOT** define `XPLM440` (e.g., `-DXPLM440=1` or `-DXPLM440=0`) in your CMake configuration if you want backward compatibility. Doing so forces Strict Panel Graphics mode and breaks compatibility with X-Plane 11 and earlier X-Plane 12 versions. If you need the bridge, stop at `-DXPLM430=1`.

## Step 2: Verify the Pipeline

When your plugin initializes its first ImGui window, `ImgWindow` logs its routing decision to X-Plane's `Log.txt`. To verify Panel Graphics is working on X-Plane 12.4.4+:

Look for: `ImgWindow: Rendering via XPLM v4.4 Panel Graphics`

## Step 3: Add Safe Teardown to Shutdown

Because X-Plane 12's modern Panel Graphics executes draw calls synchronously, destroying a texture handle (like a font atlas) while ImGui is building its draw list can cause a crash. `ImgWindow v2` uses a 1-frame deferred deletion queue to prevent this.

However, during plugin shutdown, X-Plane's flight loop completely stops running. To prevent orphaned textures and VRAM leaks, you **must** manually flush the queue whenever you destroy your textures.

**The Lifecycle Symmetry Rule:**
* If you built your font atlas and custom textures in `XPluginStart()`, you must destroy them in `XPluginStop()`.
* If you built them in `XPluginEnable()`, you must destroy them in `XPluginDisable()`.

Wherever you choose to destroy your atlas, you must immediately call `ImgWindow::Shutdown()` afterward to flush the 1-frame queue before the plugin halts.

For example, if you tear down in `XPluginStop()`:

```cpp
// 1. Destroy your font atlas
ImgWindow::sFontAtlas.reset();

// 2. Instantly flush the deferred deletion queue
ImgWindow::Shutdown();
```
