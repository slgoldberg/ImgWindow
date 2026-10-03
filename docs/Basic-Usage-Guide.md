# `ImgWindow v2` Framework: Basic Usage Guide

Welcome to the `ImgWindow` framework! This guide covers the basic usage model for integrating [Dear ImGui](https://github.com/ocornut/imgui) into your C++ X-Plane plugins using XPLM's modern window API.

To future-proof UI development, Laminar Research introduced the **Panel Graphics** API in X-Plane 12.4.4. This is a modern, general-purpose graphics framework allowing developers to draw 2D interfaces directly through X-Plane's native Vulkan or Metal rendering pipelines, eventually replacing the legacy OpenGL architecture.

While this framework supports a powerful "dynamic bridge" to seamlessly multiplex your UI rendering between legacy OpenGL (on older sims) and the new Panel Graphics pipeline (on XP12.4.4+), this guide focuses strictly on the fundamental architecture: **how to actually build and manage ImGui windows.**

> ---
> 🚀 **Already familiar with the basics?**
> ---
> If your plugin already uses an older version of `ImgWindow`, and you really just want to upgrade to using `Panel Graphics` (Vulkan/Metal instead of OpenGL), you can **skip this guide** and jump straight to the **[Panel Graphics Migration Guide](Panel-Graphics-Migration.md)**!

---

## 1. The Core Architecture

At its core, `Dear ImGui` is renderer-agnostic. It doesn't know how to draw to a screen; it simply records your UI commands into **"draw lists"** (geometry such as vertices, draw commands, and custom texture IDs) that form the content flowing through the X-Plane 2D rendering pipeline.

The `ImgWindow` framework's job is to:
1. Provide one or more native X-Plane windows (`XPLMCreateWindowEx`).
2. Intercept X-Plane mouse, cursor position, and keyboard events using the standard callbacks set in the `XPLMCreateWindow_t` descriptor (which `ImgWindow` manages and shunts straight into ImGui's IO system).
3. Take the resulting ImGui **draw lists** and safely render them into the active X-Plane graphics pipeline.
    * *Note: The framework is configured at **compile time** using build flags. You can force classic OpenGL, force Panel Graphics, or enable the "dynamic bridge" to automatically use the best backend available at runtime. See the [Migration Guide](Panel-Graphics-Migration.md) for build configurations.*

To use the framework, you only need to interact with two main components:
1. **`ImgFontAtlas`**: A shared service to load fonts and bake them into the GPU.
2. **`ImgWindow`**: The base class you must subclass to define your specific UI windows. *(Note: You can instantiate as many unique subclasses and windows as you need!)*

---

## 2. Managing Fonts (`ImgFontAtlas`)

Before you can render any text in ImGui, ImGui needs a "font atlas" -- a single large texture containing all the glyphs for the fonts you want to use.

### Why do we have an `ImgFontAtlas` wrapper class?
In a standard desktop app, ImGui manages its own font atlas. However, inside X-Plane, we must strictly control how and when that texture is baked and uploaded to the GPU (especially to support both OpenGL and Vulkan rendering pipelines). 

Our `ImgFontAtlas` class wraps ImGui's native atlas, intercepting the texture generation process to ensure the glyphs are safely uploaded to X-Plane's VRAM.

### The Contract & Lifetime
* **Do NOT subclass `ImgFontAtlas`:** The base class provides everything you need.
* **Shared Instance:** `ImgWindow` uses a static scoped shared pointer (`std::shared_ptr<ImgFontAtlas>`) under the hood. This means you only need to create **one** font atlas for your entire plugin. All instances of your windows will automatically share this single font texture, saving massive amounts of VRAM.
* **Initialization:** You *must* instantiate the font atlas and call `bindTexture()` before you ever attempt to draw your first window. Typically, this is done in your `XPluginStart` or `XPluginEnable` callback.

### Font Setup Example
```cpp
// 1. Create a shared pointer to the atlas
std::shared_ptr<ImgFontAtlas> myFontAtlas = std::make_shared<ImgFontAtlas>();

// 2. Add your fonts (or just use the default)
myFontAtlas->AddFontDefault();
// myFontAtlas->AddFontFromFileTTF("Resources/plugins/MyPlugin/fonts/Roboto.ttf", 16.0f);

// (Advanced: You can also use ImGui's MergeMode to merge symbol fonts like FontAwesome 
// right over your base fonts here! See the `sample-code/InitializeImGui.cpp` file for 
// advanced font merging techniques).

// 3. Bake the texture to the GPU!
myFontAtlas->bindTexture();

// 4. Assign the shared atlas to the ImgWindow system
ImgWindow::sFontAtlas = myFontAtlas;
```

---

## 3. Creating Your First Window (Customizing and Using `ImgWindow`)

To create an actual window, you **must** subclass the `ImgWindow` base class. The framework relies on polymorphism to let you define your unique UI layout while it handles the heavy lifting of the XPLM window lifecycle.

### 3.1 Extending `ImgWindow` for Your Plugin's Window(s)

#### The `ImgWindow` Base Constructor
When creating your custom window class, you must invoke the base `ImgWindow` constructor. The base constructor signature looks like this:

```cpp
ImgWindow(
    int left, int top, int right, int bottom,
    XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
    XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
    bool cursors = false);
```

**Base Constructor Options:**
* **`left, top, right, bottom`**: The initial bounds of the window in global X-Plane screen coordinates. 
* **`decoration`**: Dictates how X-Plane borders the window. The default (`xplm_WindowDecorationRoundRectangle`) provides the standard translucent X-Plane 11/12 bezel. If you want ImGui to render its own square borders and title bars without the X-Plane bezel, you can pass `xplm_WindowDecorationSelfDecorated`.
* **`layer`**: Dictates the Z-order of the window. The default (`xplm_WindowLayerFloatingWindows`) keeps the window above the cockpit but below system alerts.
* **`cursors`**: If set to true, allows ImGui to render its own *software* cursors (e.g., a hand icon over links, or an I-beam over text inputs) inside the window's content area. This provides cursor variety that X-Plane historically lacks. The framework automatically negotiates the hand-off so X-Plane retains hardware cursor control over the outer window edges for standard resizing interactions. Because ImGui's cursors are drawn during the UI frame cycle, they may feel slightly less snappy than the native X-Plane hardware cursor. By default (`false`), X-Plane strictly manages the standard hardware arrow cursor everywhere.

#### Building Your Plugin's Window Class Hierarchy
#### The Basic Subclass Pattern
For most plugins, you will simply create one custom C++ class for each unique window in your UI, and have it inherit directly from `ImgWindow` (e.g., `class MySettingsWindow : public ImgWindow`). 

*(If your plugin has many different windows that all need to share the same custom fonts, styling, or behaviors, you can create your own intermediate base class to centralize that logic. To learn how to implement that pattern, see the &rarr;[Advanced Architecture Patterns Guide](Advanced-Architecture-Patterns.md)).*

For this basic guide, we will stick to the standard approach: inheriting directly from `ImgWindow`.
#### Designing Your Subclass Constructor
When designing your subclass, you should strongly consider hardcoding the static `ImgWindow` configuration parameters inside your subclass's initialization list, rather than forcing the caller to provide them. This keeps your instantiation code incredibly clean. 

Furthermore, you should use your constructor body to set static window properties (like the title or resize limits) so they don't have to be repeatedly evaluated inside the per-frame `buildInterface()` loop.

**Example of an optimized constructor:**
```cpp
class MySettingsWindow : public ImgWindow {
public:
    // Notice we don't ask the caller for decoration or layer params!
    // We only ask for the bounds, and we pass the optimized defaults up to the base.
    MySettingsWindow(int left, int top, int right, int bottom) 
        : ImgWindow(left, top, right, bottom, xplm_WindowDecorationSelfDecorated) {
        
        // 1. Set the X-Plane window title (used by the OS and X-Plane's window manager)
        SetWindowTitle("My Settings");

        // 2. Set default sizing restrictions
        SetWindowResizingLimits(300, 200, 800, 600);
    }
    
    // ... buildInterface() ...
};
```

### 3.2 The Required Override: `buildInterface()`
Your subclass must implement the pure virtual method `buildInterface()`. This method is called every single frame that your window is drawn. This is where you write your standard `ImGui::` layout code.

```cpp
class MyPluginWindow : public ImgWindow {
public:
    MyPluginWindow(int left, int top, int right, int bottom) 
        : ImgWindow(left, top, right, bottom) {
        
        SetWindowTitle("My Awesome Plugin");
    }

    // REQUIRED: Define your UI here
    virtual void buildInterface() override {
        ImGui::Text("Hello World from X-Plane!");
        if (ImGui::Button("Close")) {
            SafeDelete();
        }
    }
};
```

### 3.3 Optional Overrides (Lifecycle Hooks)
The `ImgWindow` API provides several optional hooks you can override to tightly control your window's behavior:

* **`beforeBegin()`**: Called right before `ImGui::Begin()` is executed. You can use this to return specific window flags (e.g., `return ImGuiWindowFlags_MenuBar;`) or set up ImGui styling variables that apply to the whole window.
* **`afterRendering()`**: Called after all ImGui rendering is complete, right before the XPLM draw callback returns. Useful for custom state cleanup.
* **`onShow()`**: Called before the window is made visible. Returning `false` will suppress the window from showing.

### 3.4 VR Handling
The `ImgWindow` framework includes built-in support for seamlessly displaying your windows in X-Plane's VR environment.

By default, windows are created in the 2D layered window system (e.g., `xplm_WindowLayerFloatingWindows`). However, if a user opens your window while they are actively in VR, the framework's `moveForVR()` logic automatically intercepts this and forces the window to spawn in the VR world (`xplm_WindowVR`) instead. 

*(Note: The framework checks the VR state automatically whenever you call `SetVisible(true)`. It does not continuously poll the VR state in the background. If you want an already-open 2D window to instantly teleport into VR the moment the user puts their headset on, your plugin will need to track the VR DataRef and explicitly toggle the window's visibility to trigger the transition!)*

### 3.5 Safe Destruction
Never use the standard C++ `delete` operator to destroy your window from inside an ImGui callback (like a button press). Doing so will destroy the object while ImGui is still actively processing its draw tree, causing an instant crash.

Instead, always call **`SafeDelete()`**. This queues the window for destruction, deferring the actual deletion to a static XPLM Flight Loop Callback that safely destroys the window pointer in the `BeforeFlightModel` phase, entirely outside of the ImGui and X-Plane drawing loops.

## What's Next?
Once you understand the basic usage model, you can safely write your UI code without worrying about how X-Plane actually gets it onto the screen.

If you are ready to scale up your plugin with custom fonts, FontAwesome icons, global styling, or auto-positioning widgets, be sure to
>   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&rarr;&nbsp; read the [Advanced Architecture Patterns Guide](Advanced-Architecture-Patterns.md).

If your plugin uses **Custom Textures** (e.g., drawing icons or photos using `ImGui::Image()`), or if you are interested in how this framework seamlessly bridges legacy OpenGL with X-Plane 12's modern Vulkan/Metal graphics pipeline, please
>   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&rarr;&nbsp; read the [Panel Graphics Migration Guide](Panel-Graphics-Migration.md).

To see real-world example initialization and tear-down functions, and/or some included font files that you're free to use if you like,
>   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&rarr;&nbsp;start with [Sample Code README](../sample-code/README.md).

For more information on this repository's overall features and history, options for configuring your builds for ImGui, how to replace an older version of `ImgWindow` with a `git submodule`, and more, you can always
>   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&rarr;&nbsp; return to the main [Top-Level README](../README.md).
