# How-To: Render Custom Textures (`ImGui::Image`)

If your plugin loads custom 2D images (like plugin icons or custom gauges) to draw inside ImGui using functions like `ImGui::Image()`, you need to migrate your texture code to support Panel Graphics. 

Legacy OpenGL integer handles (`GLuint`) cannot be sent through the Vulkan/Metal Panel Graphics pipeline. `ImgWindow` provides a unified texture API that works safely across both pipelines.

## Step 1: Create the Texture

Instead of manually generating OpenGL textures or calling raw XPLM v4.4 functions, use `ImgWindow::CreateTexture()`. This handles the backend multiplexing for you automatically.

```cpp
ImTextureID myTexture = nullptr; // Always initialize to nullptr!

void LoadMyTexture(const char* filepath) {
    int width, height, channels;
    
    // FORCE 4 channels (RGBA) to prevent Vulkan/Metal buffer overruns
    unsigned char* rgba_pixels = stbi_load(filepath, &width, &height, &channels, 4);
    if (!rgba_pixels) return;

    // Must be called on the MAIN THREAD!
    myTexture = ImgWindow::CreateTexture(rgba_pixels, width, height);
    
    stbi_image_free(rgba_pixels);
}
```

> [!CAUTION]
> **Strict Main-Thread Execution**
> `ImgWindow::CreateTexture()` wraps XPLM SDK calls. Calling this from a background worker thread will instantly assert and crash X-Plane. Decode your pixels on a background thread, but call `CreateTexture()` on the main thread.

> [!WARNING]
> **The 64-bit Vulkan Trap**
> `CreateTexture` returns a 64-bit pointer (`void*`). Do **not** store this in 32-bit types like `GLuint` or `XPLMTextureID`. Truncating this pointer will crash the simulator with a `Resource does not belong to your plugin` error. Always use `ImTextureID`.

## Step 2: Draw the Texture

Once loaded safely into an `ImTextureID`, rendering is identical to standard ImGui:

```cpp
void MyWindow::buildInterface() {
    // Prevent the "Uninitialized Handle Trap" by checking for null
    if (myTexture != nullptr) {
        ImGui::Image(myTexture, ImVec2(256.0f, 256.0f));
    }
}
```

> [!NOTE]
> If you have existing legacy OpenGL `ImGui::Image` calls that you cannot refactor yet, you can hide them from Panel Graphics by using the `IsUsingPanelGraphics()` method on your window.

## Step 3: Delete the Texture

To clean up custom textures, use the unified `DeleteTexture()` method. It is safe to call synchronously on the main thread.

*(Note: The old `SafeDeleteTexture()` method from v1.3.0 is deprecated. `SetTextureBakeDelay()` is also deprecated and can be removed).*

```cpp
void UnloadMyTexture() {
    if (myTexture) {
        // Safe to call synchronously on the MAIN THREAD
        ImgWindow::DeleteTexture(myTexture);
        myTexture = nullptr;
    }
}
```
