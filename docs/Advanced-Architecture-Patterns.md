# ImgWindow v2.0: Advanced Architecture Patterns

While the `ImgWindow` base class provides a 1-to-1 abstraction over X-Plane's modern window API, large-scale plugins often require more sophisticated architectural patterns to manage complex UIs, optimize VRAM, and keep their codebase DRY (Don't Repeat Yourself). 

This document outlines several advanced design patterns gleaned from real-world, production plugins using this framework.

---

## 1. The ImGui Lifecycle (Initialization & Cleanup)

Correctly kick-starting and tearing down ImGui is critical to preventing GPU memory leaks and crashes in X-Plane. We strongly recommend wrapping this lifecycle in global initialization and teardown functions. Read on for detailed examples, including using fonts (which is further described in the section titled, "**_Configuring, Loading, and Using Fonts_**").

### Initialization (Usually in `XPluginStart` or `XPluginEnable`)

Before diving into the code, it is important to understand *why* we initialize the font atlas upfront:

By default, ImGui expects each UI context to manage its own fonts. However, X-Plane plugins often spawn dozens of `ImgWindow` instances. To prevent your plugin from needlessly duplicating fonts and wasting massive amounts of VRAM, the framework provides the `ImgFontAtlas` wrapper class. This acts as a singleton service that ensures all your windows share a single, unified texture in GPU memory. 

To initialize the framework, you must instantiate this shared font atlas, load your desired fonts into it, and assign it to the framework **before** you instantiate your first custom `ImgWindow` implementation.

```cpp
void InitializeImGui() {
    // 1. Instantiate the Atlas
    auto myAtlas = std::make_shared<ImgFontAtlas>();

    // 2. Load Fonts (See Section 2 for advanced font configuration)
    myAtlas->AddFontDefault(); 

    // 3. Assign the atlas to the framework's static service
    ImgWindow::sFontAtlas = myAtlas;
}
```
*(Note: `ImgWindow` handles GPU texture baking automatically upon window creation or whenever fonts are dynamically modified.)*

### Cleanup (Usually in `XPluginStop` or `XPluginDisable`)

Good hygiene dictates that your cleanup hook should match your initialization hook. When shutting down, ImGui is very sensitive to how its font atlas is destroyed. 

> [!WARNING]
> **Close Your Windows First!** Because your plugin might have multiple floating windows actively open in the simulator when the user disables your plugin, you **must** safely delete/close all your active `ImgWindow` instances *before* calling this global teardown. Otherwise, those active windows might attempt to draw on the very next frame using a null atlas, causing an immediate crash!

To prevent ImGui from attempting a double-delete of the font texture on shutdown (especially in modern ImGui v1.92+), you must explicitly nullify ImGui's internal font pointer when you destroy your atlas:

```cpp
void TeardownImGui() {
    // 1. Reset the shared pointer, triggering destruction of the texture in VRAM
    if (ImgWindow::sFontAtlas) {
        ImgWindow::sFontAtlas.reset(); 
    }

    // 2. Immediately sever ImGui's internal reference to prevent double-delete crashes
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::GetIO().Fonts = NULL; 
    }
}
```

---

## 2. Configuring, Loading, and Using Fonts

ImGui manages fonts completely differently than traditional UI frameworks. Understanding this paradigm is key to building rich, customized interfaces.

### 2.1 The Font Bank Paradigm (`PushFont` / `PopFont`)

ImGui loads all fonts into a single, massive "Font Atlas" texture. The *first* font you load becomes the default font for all UI elements. 

To use different fonts, sizes, or styles for specific text, you must load them into the atlas upfront, save their pointers, and wrap your UI drawing code in `ImGui::PushFont()` and `ImGui::PopFont()`. 

**Best Practice:** Instead of managing scattered `ImFont*` pointers, manage an array or `std::vector<ImFont*>` in your plugin context:

```cpp
std::vector<ImFont*> gFonts;

// Later in your UI rendering loop:
ImGui::PushFont(gFonts[1]); // Use your secondary font (e.g., Large Header)
ImGui::Text("Settings");
ImGui::PopFont(); // Return to the default font
```

### 2.2 Loading the Default Font

If you just want the standard ImGui pixel font (ProggyClean), simply call `AddFontDefault()`:

```cpp
auto myAtlas = std::make_shared<ImgFontAtlas>();
myAtlas->AddFontDefault();
```

### 2.3 Loading Custom Fonts

You have two primary ways to load custom TrueType (`.ttf`) fonts into your atlas:

1. **From File (`AddFontFromFileTTF`)**: The simplest method. However, if the user deletes or moves the file, your plugin will crash upon initialization.
2. **From Memory (`AddFontFromMemoryCompressedTTF`)**: The safest method. You embed the font directly into your C++ binary using ImGui's `binary_to_compressed_c.cpp` tool. This guarantees the font is always available.

```cpp
// Example: Loading a custom font from a file
ImFont* roboto = myAtlas->AddFontFromFileTTF("Resources/plugins/MyPlugin/fonts/Roboto-Regular.ttf", 16.0f);
gFonts.push_back(roboto);

// Example: Loading a font compiled directly into your C++ binary
ImFont* titleFont = myAtlas->AddFontFromMemoryCompressedTTF(MyTitleFont_compressed_data, MyTitleFont_compressed_size, 24.0f);
gFonts.push_back(titleFont);
```

### 2.4 Merging Symbol Fonts (FontAwesome) and Optimizing VRAM

Many developers merge symbol fonts (like FontAwesome) directly into their base fonts so they can seamlessly use icons inside standard `ImGui::Text()` calls. 

However, loading the *entire* FontAwesome TTF file into the `ImgFontAtlas` consumes a massive amount of VRAM, baking thousands of unused glyphs into X-Plane's texture memory.

**The Fix:** Use the community standard `IconsFontAwesome5.h` header, but do **not** load the default glyph ranges. Instead, build a highly targeted `ImWchar` array containing *only* the specific `ICON_FA_*` macros your plugin actually uses. 

The framework provides sample fonts (including FontAwesome) and the `IconsFontAwesome5.h` file in the `sample-code/fonts` directory.

*(Note: Don't forget to actually `#include "IconsFontAwesome5.h"` in your C++ project so the `ICON_FA_*` macros compile!)*

```cpp
// 1. Use the ImFontGlyphRangesBuilder to parse the specific icons you need
static ImVector<ImWchar> icon_ranges;
ImFontGlyphRangesBuilder builder;

// Add all icons that are actually used (they seamlessly concatenate into one string)
builder.AddText(ICON_FA_UNDO ICON_FA_ARROW_LEFT ICON_FA_SEARCH ICON_FA_TRASH_ALT);
builder.BuildRanges(&icon_ranges);

// 2. Configure ImGui to merge the next font into the PREVIOUS font
ImFontConfig config;
config.MergeMode = true;
config.PixelSnapH = true;

// 3. Load your base font first (this becomes the default)
myAtlas->AddFontFromFileTTF("fonts/Roboto-Regular.ttf", 16.0f);

// 4. Load the icon font second, passing your built ranges pointer
myAtlas->AddFontFromFileTTF("fonts/fa-solid-900.ttf", 16.0f, &config, icon_ranges.Data);


// Now you can freely mix text and icons seamlessly:
// ImGui::Button(ICON_FA_UNDO " Reset View");
```

---

## 3. The "Intermediate Base Class" Pattern

In a large plugin, deriving every single UI window directly from `ImgWindow` leads to massive code duplication. Instead, insert an **Intermediate Base Class** between the framework and your actual windows.

**Example Hierarchy:**
`ImgWindow` &rarr; `PluginBaseWindow` &rarr; `MySpecificWindow`

Your `PluginBaseWindow` can override the `beforeBegin()`, `buildInterface()`, and `afterRendering()` hooks to transparently inject global features into every window without the child classes knowing:
*   **Global Styling:** Injecting a universal color-scheme picker that applies to the whole plugin.
*   **Custom Behaviors:** Implementing delayed tooltip rendering logic that standardizes tooltips across your entire UI.
*   **Contextual Chrome:** Automatically hiding the window's drag bar or close button if the mouse isn't hovering over it, keeping the UI clean.
*   **Defensive UI in VR/Cockpits:** Suppressing window interactivity or resizing handles unless a specific modifier key is held, preventing the UI from stealing mouse clicks away from critical cockpit instruments.

---

## 4. Smart Widgets (Auto-Positioning & Shrink-Wrapping)

For "Widgets" (like a 1-line Status Bar, a tooltip, or a floating Clock), you can create another tier of subclassing (e.g., `class SmartWidget : public PluginBaseWindow`).

This class can handle advanced layout logic natively, interacting dynamically with X-Plane's window manager based on the size of the ImGui content.

### 4.1 Shrink-Wrapping
Instead of guessing the size of your window, you can use ImGui's auto-sizing capabilities and dynamically shrink your X-Plane window bounding box to tightly fit the content.

```cpp
void SmartWidget::buildInterface() {
    // 1. Group your content so ImGui can calculate its total bounding box
    ImGui::BeginGroup();
    
    // ... [Draw your dynamic UI, e.g., ImGui::Text("Status: OK")] ...
    
    ImGui::EndGroup();

    // 2. Fetch the dynamically calculated bounds of the group we just drew
    ImVec2 contentSize = ImGui::GetItemRectSize();
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    
    int targetWidth = (int)(contentSize.x + padding.x * 2);
    int targetHeight = (int)(contentSize.y + padding.y * 2);

    // 3. Shrink-wrap the actual X-Plane Window to match the ImGui content
    int left, top, right, bottom;
    XPLMGetWindowGeometry(mWindowID, &left, &top, &right, &bottom);
    
    if ((right - left) != targetWidth || (top - bottom) != targetHeight) {
        XPLMSetWindowGeometry(mWindowID, left, top, left + targetWidth, top - targetHeight);
    }
}
```

### 4.2 Edge Snapping & Auto-Positioning
Widgets often need to live in specific corners of the screen. You can combine shrink-wrapping with screen-boundary logic to anchor a widget to the top-right corner of the monitor. You can safely trigger this logic at the end of your `buildInterface()` loop.

```cpp
void SmartWidget::snapToTopRight() {
    // Get the size of the user's primary monitor
    int screenLeft, screenTop, screenRight, screenBottom;
    XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);

    // Get our current window dimensions
    int winLeft, winTop, winRight, winBottom;
    XPLMGetWindowGeometry(mWindowID, &winLeft, &winTop, &winRight, &winBottom);
    int width = winRight - winLeft;
    int height = winTop - winBottom;

    // Calculate a 20px padding margin
    int margin = 20;
    
    // Reposition the window to the top right corner
    XPLMSetWindowGeometry(mWindowID, 
        screenRight - width - margin, 
        screenTop - margin, 
        screenRight - margin, 
        screenTop - height - margin
    );
}
```
By combining these concepts inside a dedicated `SmartWidget` base class, your status bars and popups will automatically resize and smoothly slide into position across the screen whenever their internal content changes!
