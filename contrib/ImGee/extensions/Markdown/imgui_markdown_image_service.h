// ============================================================================
// ImGui Contrib Markdown: Default Image Service & Cache (Tiers 1, 2 & 3)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Implements 3-tier image resolution for Markdown rendering:
//   Tier 1: In-memory named textures (via ImgWindow::GetNamedTexture or SetNamedTextureResolver)
//   Tier 2: Local disk image files loaded via stb_image and uploaded via host texture driver
//   Tier 3: Asynchronous remote HTTP/HTTPS downloads with background worker decoding
// ============================================================================

#pragma once

#ifndef IMGUI_MARKDOWN_IMAGE_SERVICE_H
#define IMGUI_MARKDOWN_IMAGE_SERVICE_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <cmath>
#include <sys/stat.h>
#include <algorithm>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

#include "imgui_markdown.h"

// If ImgWindow is available, use its texture upload driver; otherwise fallback to external
#if defined(__has_include)
    #if __has_include("ImgWindow.h")
        #include "ImgWindow.h"
        #define IMGEE_HAS_IMGWINDOW 1
    #endif
#endif

#if !defined(STB_IMAGE_STATIC)
#define STB_IMAGE_STATIC
#endif
#if !defined(STB_IMAGE_IMPLEMENTATION)
#define STB_IMAGE_IMPLEMENTATION
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "../../../stb_image.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace ImGui {

#ifndef IMGUI_NAMED_TEXTURE_RESOLVER_DEFINED
#define IMGUI_NAMED_TEXTURE_RESOLVER_DEFINED
    namespace Detail {
        typedef ImTextureID (*NamedTextureResolverFn)(const char* name, ImVec2* outSize);
        inline NamedTextureResolverFn& GetNamedTextureResolver() {
            static NamedTextureResolverFn s_Resolver = nullptr;
            return s_Resolver;
        }
    }
#endif
#ifndef IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED
#define IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED
    inline void SetNamedTextureResolver(Detail::NamedTextureResolverFn fn) {
        Detail::GetNamedTextureResolver() = fn;
    }
#endif

namespace MarkdownImageService {

struct MarkdownDiskTextureEntry {
    ImTextureID textureID = (ImTextureID)0;
    ImVec2 size = ImVec2(0.0f, 0.0f);
    bool inFlight = false;
    bool failed = false;
    double lastAttemptTime = 0.0;
    int retryCount = 0;
};

inline std::map<std::string, MarkdownDiskTextureEntry>& GetDiskTextureCache() {
    static std::map<std::string, MarkdownDiskTextureEntry> s_Cache;
    return s_Cache;
}

struct MarkdownRemoteDecodedPayload {
    std::string url;
    uint8_t* pixels = nullptr;
    int width = 0;
    int height = 0;
    bool success = false;
};

inline std::mutex& GetCompletedMutex() {
    static std::mutex s_Mutex;
    return s_Mutex;
}

inline std::vector<MarkdownRemoteDecodedPayload>& GetCompletedQueue() {
    static std::vector<MarkdownRemoteDecodedPayload> s_Queue;
    return s_Queue;
}

typedef bool (*RemoteFetchCallback)(const std::string& url, std::vector<uint8_t>& outData);

inline RemoteFetchCallback& GetRemoteFetchCallbackRef() {
    static RemoteFetchCallback s_Callback = nullptr;
    return s_Callback;
}

inline void SetRemoteFetchCallback(RemoteFetchCallback callback) {
    GetRemoteFetchCallbackRef() = callback;
}

inline RemoteFetchCallback GetRemoteFetchCallback() {
    return GetRemoteFetchCallbackRef();
}

inline void ClearTextureCache() {
    {
        std::lock_guard<std::mutex> lock(GetCompletedMutex());
        for (auto& item : GetCompletedQueue()) {
            if (item.pixels) {
                stbi_image_free(item.pixels);
                item.pixels = nullptr;
            }
        }
        GetCompletedQueue().clear();
    }

    for (auto& pair : GetDiskTextureCache()) {
        if (pair.second.textureID) {
#if defined(IMGEE_HAS_IMGWINDOW)
            ImgWindow::DeleteTexture(pair.second.textureID);
#endif
        }
    }
    GetDiskTextureCache().clear();
}

inline void ProcessPendingTextures() {
    std::vector<MarkdownRemoteDecodedPayload> completed;
    {
        std::lock_guard<std::mutex> lock(GetCompletedMutex());
        if (!GetCompletedQueue().empty()) {
            completed.swap(GetCompletedQueue());
        }
    }

    if (completed.empty()) {
        return;
    }

    for (auto& item : completed) {
        auto& entry = GetDiskTextureCache()[item.url];
        entry.inFlight = false;

        if (item.success && item.pixels && item.width > 0 && item.height > 0) {
            ImTextureID texID = (ImTextureID)0;
#if defined(IMGEE_HAS_IMGWINDOW)
            texID = ImgWindow::CreateTexture(item.pixels, item.width, item.height);
#endif
            stbi_image_free(item.pixels);
            item.pixels = nullptr;

            if (texID) {
                entry.textureID = texID;
                entry.size = ImVec2(static_cast<float>(item.width), static_cast<float>(item.height));
                entry.failed = false;
                entry.retryCount = 0;
            } else {
                entry.failed = true;
                entry.lastAttemptTime = ImGui::GetTime();
            }
        } else {
            if (item.pixels) {
                stbi_image_free(item.pixels);
                item.pixels = nullptr;
            }
            entry.failed = true;
            entry.lastAttemptTime = ImGui::GetTime();
        }
    }
}

#if !defined(_WIN32)
typedef void CURL;
typedef int CURLcode;
typedef int CURLoption;
typedef int CURLINFO;

#define IMGEE_CURLE_OK 0
#define IMGEE_CURLOPT_URL 10002
#define IMGEE_CURLOPT_FOLLOWLOCATION 52
#define IMGEE_CURLOPT_USERAGENT 10018
#define IMGEE_CURLOPT_TIMEOUT 13
#define IMGEE_CURLOPT_WRITEFUNCTION 20011
#define IMGEE_CURLOPT_WRITEDATA 10001
#define IMGEE_CURLINFO_RESPONSE_CODE 0x200002

typedef CURL* (*curl_easy_init_t)();
typedef CURLcode (*curl_easy_setopt_t)(CURL*, CURLoption, ...);
typedef CURLcode (*curl_easy_perform_t)(CURL*);
typedef void (*curl_easy_cleanup_t)(CURL*);
typedef CURLcode (*curl_easy_getinfo_t)(CURL*, CURLINFO, ...);

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* mem = reinterpret_cast<std::vector<uint8_t>*>(userp);
    auto* bytes = reinterpret_cast<uint8_t*>(contents);
    mem->insert(mem->end(), bytes, bytes + total);
    return total;
}

static bool FetchRemoteBytesCurl(const std::string& url, std::vector<uint8_t>& outBuffer) {
#if defined(__APPLE__) || defined(__MACH__)
    const char* dylibNames[] = { "libcurl.4.dylib", "libcurl.dylib", "/usr/lib/libcurl.4.dylib", nullptr };
#else
    const char* dylibNames[] = { "libcurl.so.4", "libcurl.so", nullptr };
#endif

    void* handle = nullptr;
    for (int i = 0; dylibNames[i] != nullptr; ++i) {
        handle = dlopen(dylibNames[i], RTLD_LAZY);
        if (handle) break;
    }
    if (!handle) return false;

    auto p_init = (curl_easy_init_t)dlsym(handle, "curl_easy_init");
    auto p_setopt = (curl_easy_setopt_t)dlsym(handle, "curl_easy_setopt");
    auto p_perform = (curl_easy_perform_t)dlsym(handle, "curl_easy_perform");
    auto p_cleanup = (curl_easy_cleanup_t)dlsym(handle, "curl_easy_cleanup");
    auto p_getinfo = (curl_easy_getinfo_t)dlsym(handle, "curl_easy_getinfo");

    if (!p_init || !p_setopt || !p_perform || !p_cleanup) {
        dlclose(handle);
        return false;
    }

    CURL* curl = p_init();
    if (!curl) {
        dlclose(handle);
        return false;
    }

    outBuffer.clear();
    p_setopt(curl, IMGEE_CURLOPT_URL, url.c_str());
    p_setopt(curl, IMGEE_CURLOPT_FOLLOWLOCATION, 1L);
    p_setopt(curl, IMGEE_CURLOPT_USERAGENT, "ImGee/1.0.0");
    p_setopt(curl, IMGEE_CURLOPT_TIMEOUT, 10L);
    p_setopt(curl, IMGEE_CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    p_setopt(curl, IMGEE_CURLOPT_WRITEDATA, &outBuffer);

    CURLcode res = p_perform(curl);
    long http_code = 0;
    p_getinfo(curl, IMGEE_CURLINFO_RESPONSE_CODE, &http_code);

    p_cleanup(curl);
    dlclose(handle);

    return (res == IMGEE_CURLE_OK && http_code == 200 && !outBuffer.empty());
}
#endif

inline void FetchRemoteImageWorker(std::string url) {
    std::vector<uint8_t> rawBytes;
    bool fetchOk = false;

    if (GetRemoteFetchCallback()) {
        fetchOk = GetRemoteFetchCallback()(url, rawBytes);
    }
#if !defined(_WIN32)
    if (!fetchOk) {
        fetchOk = FetchRemoteBytesCurl(url, rawBytes);
    }
#endif

    MarkdownRemoteDecodedPayload payload;
    payload.url = url;

    if (fetchOk && !rawBytes.empty()) {
        int w = 0, h = 0, comp = 0;
        uint8_t* pixels = stbi_load_from_memory(rawBytes.data(), (int)rawBytes.size(), &w, &h, &comp, 4);
        if (pixels && w > 0 && h > 0) {
            payload.pixels = pixels;
            payload.width = w;
            payload.height = h;
            payload.success = true;
        } else {
            if (pixels) stbi_image_free(pixels);
            payload.success = false;
        }
    } else {
        payload.success = false;
    }

    {
        std::lock_guard<std::mutex> lock(GetCompletedMutex());
        GetCompletedQueue().push_back(payload);
    }
}

struct MarkdownImageModifiers {
    std::string clean_url;
    float req_width = 0.0f;
    float req_height = 0.0f;
    bool has_width = false;
    bool has_height = false;
    bool has_treatment = false;
    std::string treatment;
};

inline MarkdownImageModifiers ParseMarkdownImageModifiers(const std::string& input) {
    MarkdownImageModifiers mods;
    std::string s = input;

    size_t start = s.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) s = s.substr(start);
    size_t end = s.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) s = s.substr(0, end + 1);

    if (s.size() >= 2 && s.front() == '<' && s.back() == '>') {
        s = s.substr(1, s.size() - 2);
        start = s.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) s = s.substr(start);
        end = s.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) s = s.substr(0, end + 1);
    }

    size_t space_pos = s.find_last_of(" \t");
    if (space_pos != std::string::npos) {
        std::string token = s.substr(space_pos + 1);
        if (!token.empty() && token[0] == '=') {
            std::string dim = token.substr(1);
            size_t x_pos = dim.find('x');
            if (x_pos != std::string::npos) {
                std::string w_str = dim.substr(0, x_pos);
                std::string h_str = dim.substr(x_pos + 1);
                if (!w_str.empty()) {
                    try { mods.req_width = std::stof(w_str); mods.has_width = true; } catch (...) {}
                }
                if (!h_str.empty()) {
                    try { mods.req_height = std::stof(h_str); mods.has_height = true; } catch (...) {}
                }
                s = s.substr(0, space_pos);
            }
        }
    }

    mods.clean_url = s;
    return mods;
}

inline void ApplyMarkdownImageModifiers(ImGui::MarkdownImageData& data, const MarkdownImageModifiers& mods, const ImVec2& native_size) {
    float avail_width = ImGui::GetContentRegionAvail().x;
    float base_w = (native_size.x > 0.0f) ? native_size.x : 64.0f;
    float base_h = (native_size.y > 0.0f) ? native_size.y : 64.0f;

    if (mods.has_width && mods.has_height) {
        data.size = ImVec2(mods.req_width, mods.req_height);
    } else if (mods.has_width && !mods.has_height) {
        float aspect = (base_w > 0.0f) ? (base_h / base_w) : 1.0f;
        data.size = ImVec2(mods.req_width, mods.req_width * aspect);
    } else if (!mods.has_width && mods.has_height) {
        float aspect = (base_h > 0.0f) ? (base_w / base_h) : 1.0f;
        data.size = ImVec2(mods.req_height * aspect, mods.req_height);
    } else {
        data.size = ImVec2(base_w, base_h);
    }

    if (avail_width > 10.0f && data.size.x > avail_width) {
        float aspect = (data.size.x > 0.0f) ? (data.size.y / data.size.x) : 1.0f;
        data.size.x = avail_width;
        data.size.y = avail_width * aspect;
    }
}

inline ImGui::MarkdownImageData DefaultMarkdownImageCallback(ImGui::MarkdownLinkCallbackData data) {
    if (!data.link || data.linkLength <= 0) {
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    std::string raw_input(data.link, (size_t)data.linkLength);
    MarkdownImageModifiers mods = ParseMarkdownImageModifiers(raw_input);
    const std::string& url = mods.clean_url;

    if (url.empty()) {
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    ProcessPendingTextures();

    // 1. Tier 1: Check Named Texture Resolver or ImgWindow named textures:
    ImVec2 nativeSize(0.0f, 0.0f);
    ImTextureID namedTex = (ImTextureID)0;

    std::string key = url;
    if (key.rfind("texture://", 0) == 0) {
        key = key.substr(10);
    }

    if (Detail::GetNamedTextureResolver()) {
        namedTex = Detail::GetNamedTextureResolver()(key.c_str(), &nativeSize);
        if (!namedTex && key != url) {
            namedTex = Detail::GetNamedTextureResolver()(url.c_str(), &nativeSize);
        }
    }
#if defined(IMGEE_HAS_IMGWINDOW)
    if (!namedTex) {
        namedTex = ImgWindow::GetNamedTexture(key, &nativeSize);
        if (!namedTex && key != url) {
            namedTex = ImgWindow::GetNamedTexture(url, &nativeSize);
        }
    }
#endif

    if (namedTex) {
        ImGui::MarkdownImageData namedData;
        namedData.isValid = true;
        namedData.useLinkCallback = false;
        namedData.user_texture_id = namedTex;
        namedData.size = nativeSize;
        ApplyMarkdownImageModifiers(namedData, mods, nativeSize);
        return namedData;
    }

    // 2. Check Disk & Remote Texture Cache
    auto it = GetDiskTextureCache().find(url);
    if (it != GetDiskTextureCache().end()) {
        if (it->second.inFlight) {
            ImGui::MarkdownImageData loadingData;
            loadingData.isValid = false;
            loadingData.isLoading = true;
            return loadingData;
        }
        if (it->second.failed) {
            double now = ImGui::GetTime();
            if (it->second.retryCount >= 3 || (now - it->second.lastAttemptTime < 60.0)) {
                ImGui::MarkdownImageData invalid;
                invalid.isValid = false;
                invalid.isLoading = false;
                return invalid;
            }
        } else if (it->second.textureID) {
            ImGui::MarkdownImageData img;
            img.isValid = true;
            img.isLoading = false;
            img.user_texture_id = it->second.textureID;
            img.size = it->second.size;
            ApplyMarkdownImageModifiers(img, mods, it->second.size);
            return img;
        }
    }

    // 3. Tier 3: Remote Web URLs
    bool isRemoteUrl = (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0);
    if (isRemoteUrl) {
        auto& entry = GetDiskTextureCache()[url];
        entry.inFlight = true;
        entry.failed = false;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;

        std::thread([url]() {
            FetchRemoteImageWorker(url);
        }).detach();

        ImGui::MarkdownImageData loadingData;
        loadingData.isValid = false;
        loadingData.isLoading = true;
        return loadingData;
    }

    // 4. Tier 2: Resolve File Path on Disk
    std::string resolvedPath = url;
    struct stat st;
    bool fileFound = (stat(resolvedPath.c_str(), &st) == 0 && S_ISREG(st.st_mode));

    if (!fileFound) {
        auto& entry = GetDiskTextureCache()[url];
        entry.inFlight = false;
        entry.failed = true;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        auto& entry = GetDiskTextureCache()[url];
        entry.inFlight = false;
        entry.failed = true;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    ImTextureID texID = (ImTextureID)0;
#if defined(IMGEE_HAS_IMGWINDOW)
    texID = ImgWindow::CreateTexture(pixels, width, height);
#endif
    stbi_image_free(pixels);

    if (!texID) {
        auto& entry = GetDiskTextureCache()[url];
        entry.inFlight = false;
        entry.failed = true;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    MarkdownDiskTextureEntry successEntry;
    successEntry.textureID = texID;
    successEntry.size = ImVec2(static_cast<float>(width), static_cast<float>(height));
    successEntry.inFlight = false;
    successEntry.failed = false;
    successEntry.retryCount = 0;
    GetDiskTextureCache()[url] = successEntry;

    ImGui::MarkdownImageData img;
    img.isValid = true;
    img.isLoading = false;
    img.user_texture_id = texID;
    img.size = successEntry.size;
    ApplyMarkdownImageModifiers(img, mods, successEntry.size);
    return img;
}

inline void EnableDefaultImageService(bool enable = true) {
    if (enable) {
        ImGui::SetDefaultMarkdownImageCallback(&DefaultMarkdownImageCallback);
    } else {
        if (ImGui::GetDefaultMarkdownImageCallback() == &DefaultMarkdownImageCallback) {
            ImGui::SetDefaultMarkdownImageCallback(nullptr);
        }
    }
}

namespace {
    struct AutoMarkdownImageServiceInit {
        AutoMarkdownImageServiceInit() {
            EnableDefaultImageService(true);
        }
    };
    static AutoMarkdownImageServiceInit s_AutoMarkdownImageServiceInit;
}

} // namespace MarkdownImageService
} // namespace ImGui

#endif // IMGUI_MARKDOWN_IMAGE_SERVICE_H
