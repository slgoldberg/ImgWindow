// ============================================================================
// ImGui Contrib Markdown: Default Image Service & Cache (Tiers 1, 2 & 3)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Implements 3-tier image resolution for Markdown rendering:
//   Tier 1: In-memory named textures (via ImgWindow::GetNamedTexture or SetNamedTextureResolver)
//   Tier 2: Local disk image files (searched across CWD, X-Plane root & calling plugin folders)
//   Tier 3: Asynchronous remote HTTP/HTTPS downloads (libcurl/CLI on POSIX, WinINet on Windows)
// ============================================================================

#pragma once

#ifndef IMGUI_MARKDOWN_IMAGE_SERVICE_H
#define IMGUI_MARKDOWN_IMAGE_SERVICE_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#include <string>
#include <vector>
#include <map>
#if !defined(_WIN32)
#include <mutex>
#include <thread>
#endif
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <sys/stat.h>
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#include <algorithm>
#include <utility>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#include "imgui_markdown.h"

// If ImgWindow is available, use its texture upload driver & XPLM path helpers
#if defined(IMGWINDOW_H) || (defined(__has_include) && __has_include("ImgWindow.h"))
    #if !defined(IMGWINDOW_H)
        #include "ImgWindow.h"
    #endif
    #define IMGEE_HAS_IMGWINDOW 1
    #if defined(__has_include)
        #if __has_include(<XPLMUtilities.h>) && __has_include(<XPLMPlugin.h>)
            #include <XPLMUtilities.h>
            #include <XPLMPlugin.h>
            #define IMGEE_HAS_XPLM_PATHS 1
        #endif
    #else
        #include <XPLMUtilities.h>
        #include <XPLMPlugin.h>
        #define IMGEE_HAS_XPLM_PATHS 1
    #endif
#endif

// Include ImGee's isolated internal stb_image decoder (unless disabled by host)
#if !defined(IMGEE_DISABLE_STB_IMAGE)
    #include "../../imgee_stbi_internal.h"
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

inline unsigned char* DecodeImageMemoryInternal(const void* data, size_t len, int* outW, int* outH) {
#if defined(IMGEE_HAS_STBI_INTERNAL)
    return ImGeeStbi::LoadFromMemory(data, len, outW, outH);
#else
    (void)data; (void)len;
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    return nullptr;
#endif
}

inline unsigned char* DecodeImageFileInternal(const char* filename, int* outW, int* outH) {
#if defined(IMGEE_HAS_STBI_INTERNAL)
    return ImGeeStbi::LoadFromFile(filename, outW, outH);
#else
    (void)filename;
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    return nullptr;
#endif
}

inline void FreeDecodedImageInternal(void* pixels) {
#if defined(IMGEE_HAS_STBI_INTERNAL)
    if (pixels) ImGeeStbi::ImageFree(pixels);
#else
    (void)pixels;
#endif
}

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
    uint64_t generation = 0;
};

#if defined(_WIN32)
// Use native Win32 CRITICAL_SECTION on Windows so ImGee compiles cleanly on
// MinGW-w64's 'win32' thread model (where std::mutex / std::thread are omitted)
// and avoids introducing a runtime dependency on libwinpthread-1.dll.
struct CompletedMutex {
    CRITICAL_SECTION cs;
    CompletedMutex()  { InitializeCriticalSection(&cs); }
    ~CompletedMutex() { DeleteCriticalSection(&cs); }
    CompletedMutex(const CompletedMutex&) = delete;
    CompletedMutex& operator=(const CompletedMutex&) = delete;
    void lock()   { EnterCriticalSection(&cs); }
    void unlock() { LeaveCriticalSection(&cs); }
};

struct CompletedLockGuard {
    CompletedMutex& mtx;
    explicit CompletedLockGuard(CompletedMutex& m) : mtx(m) { mtx.lock(); }
    ~CompletedLockGuard() { mtx.unlock(); }
    CompletedLockGuard(const CompletedLockGuard&) = delete;
    CompletedLockGuard& operator=(const CompletedLockGuard&) = delete;
};
#else
using CompletedMutex = std::mutex;
using CompletedLockGuard = std::lock_guard<std::mutex>;
#endif

inline CompletedMutex& GetCompletedMutex() {
    static CompletedMutex s_Mutex;
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

inline std::atomic<uint64_t>& GetCacheGeneration() {
    static std::atomic<uint64_t> s_Generation{0};
    return s_Generation;
}

inline void ClearTextureCache() {
    GetCacheGeneration().fetch_add(1);
    {
        CompletedLockGuard lock(GetCompletedMutex());
        for (auto& item : GetCompletedQueue()) {
            if (item.pixels) {
                FreeDecodedImageInternal(item.pixels);
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
        CompletedLockGuard lock(GetCompletedMutex());
        if (!GetCompletedQueue().empty()) {
            completed.swap(GetCompletedQueue());
        }
    }

    if (completed.empty()) {
        return;
    }

    for (auto& item : completed) {
        if (item.generation != GetCacheGeneration().load()) {
            if (item.pixels) {
                FreeDecodedImageInternal(item.pixels);
                item.pixels = nullptr;
            }
            continue;
        }

        auto& entry = GetDiskTextureCache()[item.url];
        entry.inFlight = false;

        if (item.success && item.pixels && item.width > 0 && item.height > 0) {
            ImTextureID texID = (ImTextureID)0;
#if defined(IMGEE_HAS_IMGWINDOW)
            texID = ImgWindow::CreateTexture(item.pixels, item.width, item.height);
#endif
            FreeDecodedImageInternal(item.pixels);
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
                FreeDecodedImageInternal(item.pixels);
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

inline size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* mem = reinterpret_cast<std::vector<uint8_t>*>(userp);
    auto* bytes = reinterpret_cast<uint8_t*>(contents);
    mem->insert(mem->end(), bytes, bytes + total);
    return total;
}

inline bool FetchRemoteBytesCurl(const std::string& url, std::vector<uint8_t>& outBuffer) {
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
    long http_code = 200;
    if (res == IMGEE_CURLE_OK && p_getinfo) {
        p_getinfo(curl, IMGEE_CURLINFO_RESPONSE_CODE, &http_code);
    }

    p_cleanup(curl);
    dlclose(handle);

    return (res == IMGEE_CURLE_OK && http_code >= 200 && http_code < 300 && !outBuffer.empty());
}

inline bool FetchRemoteBytesCLI(const std::string& url, std::vector<uint8_t>& outBuffer) {
    if (url.find_first_of("'\"`;\r\n") != std::string::npos) {
        return false;
    }
    std::string cmd = "curl -s -L --max-time 10 '" + url + "'";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return false;

    uint8_t chunk[4096];
    size_t n = 0;
    outBuffer.clear();
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        outBuffer.insert(outBuffer.end(), chunk, chunk + n);
    }
    int status = pclose(fp);
    return (status == 0 && !outBuffer.empty());
}
#endif

#if defined(_WIN32)
inline bool FetchRemoteBytesWin(const std::string& url, std::vector<uint8_t>& outBuffer) {
    HMODULE hWinINet = LoadLibraryA("wininet.dll");
    if (!hWinINet) return false;

    typedef void* (__stdcall *InternetOpenA_t)(const char*, unsigned long, const char*, const char*, unsigned long);
    typedef void* (__stdcall *InternetOpenUrlA_t)(void*, const char*, const char*, unsigned long, unsigned long, size_t);
    typedef int   (__stdcall *InternetReadFile_t)(void*, void*, unsigned long, unsigned long*);
    typedef int   (__stdcall *InternetCloseHandle_t)(void*);

    auto p_Open    = reinterpret_cast<InternetOpenA_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetOpenA")));
    auto p_OpenUrl = reinterpret_cast<InternetOpenUrlA_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetOpenUrlA")));
    auto p_Read    = reinterpret_cast<InternetReadFile_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetReadFile")));
    auto p_Close   = reinterpret_cast<InternetCloseHandle_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetCloseHandle")));

    if (!p_Open || !p_OpenUrl || !p_Read || !p_Close) {
        FreeLibrary(hWinINet);
        return false;
    }

    void* hInternet = p_Open("ImGee/1.0.0", 0, nullptr, nullptr, 0);
    if (!hInternet) {
        FreeLibrary(hWinINet);
        return false;
    }

    // INTERNET_FLAG_RELOAD (0x80000000) | INTERNET_FLAG_SECURE (0x00800000) | INTERNET_FLAG_NO_CACHE_WRITE (0x04000000)
    unsigned long flags = 0x80000000UL | 0x04000000UL;
    if (url.rfind("https://", 0) == 0) {
        flags |= 0x00800000UL;
    }
    void* hUrl = p_OpenUrl(hInternet, url.c_str(), nullptr, 0, flags, 0);
    if (!hUrl) {
        p_Close(hInternet);
        FreeLibrary(hWinINet);
        return false;
    }

    outBuffer.clear();
    char chunk[4096];
    unsigned long bytesRead = 0;
    while (p_Read(hUrl, chunk, sizeof(chunk), &bytesRead) && bytesRead > 0) {
        outBuffer.insert(outBuffer.end(), chunk, chunk + bytesRead);
    }

    p_Close(hUrl);
    p_Close(hInternet);
    FreeLibrary(hWinINet);
    return !outBuffer.empty();
}
#endif

inline bool FetchRemoteBytesDefault(const std::string& url, std::vector<uint8_t>& outBuffer) {
#if defined(_WIN32)
    if (FetchRemoteBytesWin(url, outBuffer)) return true;
#else
    if (FetchRemoteBytesCurl(url, outBuffer)) return true;
    if (FetchRemoteBytesCLI(url, outBuffer)) return true;
#endif
    return false;
}

inline void FetchRemoteImageWorker(std::string url, uint64_t gen) {
    if (GetCacheGeneration().load() != gen) {
        return;
    }

    // Detect shields.io vector badge URLs lacking a raster extension and query the PNG raster variant:
    std::string fetchUrl = url;
    if ((fetchUrl.find("img.shields.io/") != std::string::npos ||
         fetchUrl.find("shields.io/") != std::string::npos) &&
        fetchUrl.find(".png") == std::string::npos &&
        fetchUrl.find(".jpg") == std::string::npos &&
        fetchUrl.find(".jpeg") == std::string::npos) {
        fetchUrl += ".png";
    }

    std::vector<uint8_t> rawBytes;
    bool fetchOk = false;

    if (GetRemoteFetchCallback()) {
        fetchOk = GetRemoteFetchCallback()(fetchUrl, rawBytes);
    }
    if (!fetchOk && GetCacheGeneration().load() == gen) {
        fetchOk = FetchRemoteBytesDefault(fetchUrl, rawBytes);
    }

    // Fallback: if modified fetchUrl failed, retry with original url
    if (!fetchOk && fetchUrl != url && GetCacheGeneration().load() == gen) {
        if (GetRemoteFetchCallback()) {
            fetchOk = GetRemoteFetchCallback()(url, rawBytes);
        }
        if (!fetchOk && GetCacheGeneration().load() == gen) {
            fetchOk = FetchRemoteBytesDefault(url, rawBytes);
        }
    }

    if (GetCacheGeneration().load() != gen) {
        return;
    }

    MarkdownRemoteDecodedPayload payload;
    payload.url = url;
    payload.generation = gen;

    if (fetchOk && !rawBytes.empty()) {
        int w = 0, h = 0;
        uint8_t* pixels = DecodeImageMemoryInternal(rawBytes.data(), rawBytes.size(), &w, &h);
        if (GetCacheGeneration().load() != gen) {
            if (pixels) FreeDecodedImageInternal(pixels);
            return;
        }
        if (pixels && w > 0 && h > 0) {
            payload.pixels = pixels;
            payload.width = w;
            payload.height = h;
            payload.success = true;
        } else {
            if (pixels) FreeDecodedImageInternal(pixels);
            payload.success = false;
        }
    } else {
        payload.success = false;
    }

    if (GetCacheGeneration().load() != gen) {
        if (payload.pixels) FreeDecodedImageInternal(payload.pixels);
        return;
    }

    {
        CompletedLockGuard lock(GetCompletedMutex());
        if (GetCacheGeneration().load() == gen) {
            GetCompletedQueue().push_back(payload);
        } else if (payload.pixels) {
            FreeDecodedImageInternal(payload.pixels);
        }
    }
}

#if defined(_WIN32)
struct Win32FetchWorkerArgs {
    std::string url;
    uint64_t generation;
};

inline DWORD WINAPI Win32FetchWorkerProc(LPVOID lpParam) {
    Win32FetchWorkerArgs* args = static_cast<Win32FetchWorkerArgs*>(lpParam);
    if (args) {
        std::string workerUrl = std::move(args->url);
        uint64_t workerGen = args->generation;
        delete args;
        try {
            FetchRemoteImageWorker(workerUrl, workerGen);
        } catch (...) {
        }
    }
    return 0;
}

inline bool SpawnDetachedFetchWorker(const std::string& url, uint64_t currentGen) {
    // Ensure main-thread static initialization before the worker thread starts
    (void)GetCompletedMutex();
    (void)GetCompletedQueue();
    (void)GetCacheGeneration();
    Win32FetchWorkerArgs* args = nullptr;
    try {
        args = new Win32FetchWorkerArgs{url, currentGen};
    } catch (...) {
        return false;
    }
    HANDLE hThread = CreateThread(NULL, 0, &Win32FetchWorkerProc, args, 0, NULL);
    if (!hThread) {
        delete args;
        return false;
    }
    CloseHandle(hThread);
    return true;
}
#else
inline bool SpawnDetachedFetchWorker(const std::string& url, uint64_t currentGen) {
    try {
        std::thread([url, currentGen]() {
            FetchRemoteImageWorker(url, currentGen);
        }).detach();
        return true;
    } catch (...) {
        return false;
    }
}
#endif

struct MarkdownImageModifiers {
    std::string clean_url;
    float width = -1.0f;
    float height = -1.0f;
    bool width_is_percent = false;
    float width_percent = 0.0f;
    bool height_is_percent = false;
    float height_percent = 0.0f;
    bool has_border = false;
    ImVec4 border_col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
};

inline std::string TrimMarkdownModifierWhitespace(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n')) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\r' || str[end - 1] == '\n')) {
        --end;
    }
    return str.substr(start, end - start);
}

inline MarkdownImageModifiers ParseMarkdownImageModifiers(const std::string& raw_input) {
    MarkdownImageModifiers mods;
    std::string s = TrimMarkdownModifierWhitespace(raw_input);
    if (s.empty()) return mods;

    // 1. HTML <img ... /> attribute string containing "src=" (synthesized by imgui_markdown_ext.h)
    size_t src_pos = s.find("src=");
    if (src_pos != std::string::npos && (src_pos == 0 || s[src_pos - 1] == ' ' || s[src_pos - 1] == '\t')) {
        size_t p = 0;
        while (p < s.size()) {
            while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n')) ++p;
            if (p >= s.size()) break;

            size_t key_start = p;
            while (p < s.size() && s[p] != '=' && s[p] != ' ' && s[p] != '\t' && s[p] != '/' && s[p] != '>') ++p;
            std::string key = s.substr(key_start, p - key_start);
            for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            std::string val;
            if (p < s.size() && s[p] == '=') {
                ++p;
                while (p < s.size() && (s[p] == ' ' || s[p] == '\t')) ++p;
                if (p < s.size()) {
                    if (s[p] == '"' || s[p] == '\'') {
                        char quote = s[p++];
                        size_t vstart = p;
                        while (p < s.size() && s[p] != quote) ++p;
                        val = s.substr(vstart, p - vstart);
                        if (p < s.size()) ++p;
                    } else {
                        size_t vstart = p;
                        while (p < s.size() && s[p] != ' ' && s[p] != '\t' && s[p] != '/' && s[p] != '>') ++p;
                        val = s.substr(vstart, p - vstart);
                    }
                }
            } else if (p < s.size() && (s[p] == '/' || s[p] == '>')) {
                if (key.empty()) { ++p; continue; }
            }

            if (key == "src") {
                mods.clean_url = val;
            } else if (key == "width" || key == "w") {
                if (!val.empty() && val.back() == '%') {
                    mods.width_is_percent = true;
                    mods.width_percent = static_cast<float>(std::atof(val.c_str()));
                } else {
                    mods.width = static_cast<float>(std::atof(val.c_str()));
                }
            } else if (key == "height" || key == "h") {
                if (!val.empty() && val.back() == '%') {
                    mods.height_is_percent = true;
                    mods.height_percent = static_cast<float>(std::atof(val.c_str()));
                } else {
                    mods.height = static_cast<float>(std::atof(val.c_str()));
                }
            } else if (key == "border") {
                if (val.empty() || val == "1" || val == "true") {
                    mods.has_border = true;
                    mods.border_col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
                } else if (val == "0" || val == "false" || val == "none") {
                    mods.has_border = false;
                } else {
                    mods.has_border = true;
                    ImGui::ParseMarkdownColor(val, mods.border_col);
                }
            }
        }
        return mods;
    }

    // 2. CommonMark / Extended URL & Pandoc attribute block syntax
    if (s.front() == '<' && s.back() == '>') {
        s = TrimMarkdownModifierWhitespace(s.substr(1, s.size() - 2));
        if (s.empty()) return mods;
    }

    std::string url;
    std::string remainder;

    if (s.front() == '<') {
        size_t end_angle = s.find('>');
        if (end_angle != std::string::npos) {
            url = s.substr(1, end_angle - 1);
            remainder = s.substr(end_angle + 1);
        } else {
            url = s.substr(1);
        }
    } else if (s.front() == '"' || s.front() == '\'') {
        char quote = s.front();
        size_t end_quote = s.find(quote, 1);
        if (end_quote != std::string::npos) {
            url = s.substr(1, end_quote - 1);
            remainder = s.substr(end_quote + 1);
        } else {
            url = s.substr(1);
        }
    } else {
        size_t sep = s.find_first_of(" \t{");
        if (sep != std::string::npos) {
            url = s.substr(0, sep);
            remainder = s.substr(sep);
        } else {
            url = s;
        }
    }

    mods.clean_url = url;
    remainder = TrimMarkdownModifierWhitespace(remainder);
    if (remainder.empty()) return mods;

    if (remainder.front() == '{' && remainder.back() == '}') {
        remainder = TrimMarkdownModifierWhitespace(remainder.substr(1, remainder.size() - 2));
    }

    size_t p = 0;
    while (p < remainder.size()) {
        while (p < remainder.size() && (remainder[p] == ' ' || remainder[p] == '\t' || remainder[p] == '{' || remainder[p] == '}')) ++p;
        if (p >= remainder.size()) break;

        if (remainder[p] == '"' || remainder[p] == '\'') {
            char quote = remainder[p++];
            while (p < remainder.size() && remainder[p] != quote) ++p;
            if (p < remainder.size()) ++p;
            continue;
        }

        size_t tok_start = p;
        while (p < remainder.size() && remainder[p] != ' ' && remainder[p] != '\t' && remainder[p] != '}') ++p;
        std::string tok = remainder.substr(tok_start, p - tok_start);
        if (tok.empty()) continue;

        if (tok.front() == '=') {
            std::string dim = tok.substr(1);
            if (!dim.empty() && dim.back() == '%') {
                mods.width_is_percent = true;
                mods.width_percent = static_cast<float>(std::atof(dim.c_str()));
            } else {
                size_t x_pos = dim.find_first_of("xX");
                if (x_pos != std::string::npos) {
                    std::string sw = dim.substr(0, x_pos);
                    std::string sh = dim.substr(x_pos + 1);
                    if (!sw.empty()) mods.width = static_cast<float>(std::atof(sw.c_str()));
                    if (!sh.empty()) mods.height = static_cast<float>(std::atof(sh.c_str()));
                } else {
                    mods.width = static_cast<float>(std::atof(dim.c_str()));
                }
            }
        } else if (tok.rfind("width=", 0) == 0 || tok.rfind("w=", 0) == 0) {
            size_t eq = tok.find('=');
            std::string val = tok.substr(eq + 1);
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }
            if (!val.empty() && val.back() == '%') {
                mods.width_is_percent = true;
                mods.width_percent = static_cast<float>(std::atof(val.c_str()));
            } else {
                mods.width = static_cast<float>(std::atof(val.c_str()));
            }
        } else if (tok.rfind("height=", 0) == 0 || tok.rfind("h=", 0) == 0) {
            size_t eq = tok.find('=');
            std::string val = tok.substr(eq + 1);
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }
            if (!val.empty() && val.back() == '%') {
                mods.height_is_percent = true;
                mods.height_percent = static_cast<float>(std::atof(val.c_str()));
            } else {
                mods.height = static_cast<float>(std::atof(val.c_str()));
            }
        } else if (tok == "border") {
            mods.has_border = true;
            mods.border_col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        } else if (tok.rfind("border=", 0) == 0) {
            std::string val = tok.substr(7);
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }
            if (val == "0" || val == "none" || val == "false") {
                mods.has_border = false;
            } else if (val == "1" || val == "true") {
                mods.has_border = true;
                mods.border_col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
            } else {
                mods.has_border = true;
                ImGui::ParseMarkdownColor(val, mods.border_col);
            }
        }
    }
    return mods;
}

inline void ApplyMarkdownImageModifiers(ImGui::MarkdownImageData& img, const MarkdownImageModifiers& mods, const ImVec2& native_size, bool preserve_existing_border = false) {
    float avail_w = ImGui::GetContentRegionAvail().x;

    if (mods.has_border) {
        img.border_col = mods.border_col;
    } else if (!preserve_existing_border) {
        img.border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float base_w = (native_size.x > 0.0f) ? native_size.x : 64.0f;
    float base_h = (native_size.y > 0.0f) ? native_size.y : 64.0f;
    float target_w = base_w;
    float target_h = base_h;
    float aspect = (base_w > 0.0f) ? (base_h / base_w) : 1.0f;

    if (mods.width_is_percent) {
        float ref_w = (avail_w > 10.0f) ? avail_w : (base_w > 0.0f ? base_w : 100.0f);
        target_w = ref_w * (mods.width_percent / 100.0f);
        if (mods.height > 0.0f) {
            target_h = mods.height;
        } else if (mods.height_is_percent) {
            target_h = ref_w * (mods.height_percent / 100.0f);
        } else {
            target_h = std::round(target_w * aspect);
        }
    } else if (mods.width > 0.0f && mods.height > 0.0f) {
        target_w = mods.width;
        target_h = mods.height;
    } else if (mods.width > 0.0f) {
        target_w = mods.width;
        target_h = std::round(target_w * aspect);
    } else if (mods.height > 0.0f) {
        target_h = mods.height;
        target_w = (aspect > 0.0f) ? std::round(target_h / aspect) : base_w;
    }

    // Auto-constraint to available viewport width (max-width: 100%; height: auto)
    if (avail_w > 10.0f && target_w > avail_w && target_w > 0.0f) {
        float scale = avail_w / target_w;
        target_w = avail_w;
        target_h = std::round(target_h * scale);
    }

    if (target_w < 1.0f) target_w = 1.0f;
    if (target_h < 1.0f) target_h = 1.0f;

    img.size = ImVec2(target_w, target_h);
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
    ImVec2 uv0(0.0f, 0.0f);
    ImVec2 uv1(1.0f, 1.0f);
    ImVec4 tintCol(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 borderCol(0.0f, 0.0f, 0.0f, 0.0f);
    ImTextureID namedTex = (ImTextureID)0;

    std::string key = url;
    if (key.rfind("texture://", 0) == 0) {
        key = key.substr(10);
    }

#if defined(IMGEE_HAS_IMGWINDOW)
    namedTex = ImgWindow::GetNamedTexture(key, &nativeSize, &uv0, &uv1, &tintCol, &borderCol);
    if (!namedTex && key != url) {
        namedTex = ImgWindow::GetNamedTexture(url, &nativeSize, &uv0, &uv1, &tintCol, &borderCol);
    }
#endif
    if (!namedTex && Detail::GetNamedTextureResolver()) {
        namedTex = Detail::GetNamedTextureResolver()(key.c_str(), &nativeSize);
        if (!namedTex && key != url) {
            namedTex = Detail::GetNamedTextureResolver()(url.c_str(), &nativeSize);
        }
    }

    if (namedTex) {
        ImGui::MarkdownImageData namedData;
        namedData.isValid = true;
        namedData.useLinkCallback = false;
        namedData.user_texture_id = namedTex;
        namedData.size = nativeSize;
        namedData.uv0 = uv0;
        namedData.uv1 = uv1;
        namedData.tint_col = tintCol;
        namedData.border_col = borderCol;
        bool hasStoredBorder = (borderCol.w > 0.0f);
        ApplyMarkdownImageModifiers(namedData, mods, nativeSize, hasStoredBorder);
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

        uint64_t currentGen = GetCacheGeneration().load();
        if (!SpawnDetachedFetchWorker(url, currentGen)) {
            entry.inFlight = false;
            entry.failed = true;
            entry.lastAttemptTime = ImGui::GetTime();
            ImGui::MarkdownImageData invalid;
            invalid.isValid = false;
            return invalid;
        }

        ImGui::MarkdownImageData loadingData;
        loadingData.isValid = false;
        loadingData.isLoading = true;
        return loadingData;
    }

    // 4. Tier 2: Resolve File Path on Disk (CWD -> X-Plane Root -> Calling Plugin Folders)
    std::string resolvedPath = url;
    bool fileFound = false;

    auto checkCandidate = [](const std::string& candidate, struct stat& st_out) -> bool {
        return (stat(candidate.c_str(), &st_out) == 0 && S_ISREG(st_out.st_mode));
    };

    struct stat st;
    if (checkCandidate(resolvedPath, st)) {
        fileFound = true;
    }

#if defined(IMGEE_HAS_XPLM_PATHS)
    std::string cleanUrl = url;
    if (cleanUrl.rfind("./", 0) == 0) {
        cleanUrl = cleanUrl.substr(2);
    } else if (cleanUrl.rfind("/", 0) == 0 || cleanUrl.rfind("\\", 0) == 0) {
        cleanUrl = cleanUrl.substr(1);
    }

    // Try relative to X-Plane system root
    if (!fileFound && !cleanUrl.empty()) {
        char sysPath[512] = {0};
        XPLMGetSystemPath(sysPath);
        std::string sPath(sysPath);
        if (!sPath.empty() && sPath.back() != '/' && sPath.back() != '\\') {
            sPath += '/';
        }
        std::string candidate = sPath + cleanUrl;
        if (checkCandidate(candidate, st)) {
            resolvedPath = candidate;
            fileFound = true;
        }
    }

    // Try relative to calling plugin directory (e.g. Resources/plugins/<Plugin>/)
    if (!fileFound && !cleanUrl.empty()) {
        XPLMPluginID myId = XPLMGetMyID();
        if (myId != XPLM_NO_PLUGIN_ID) {
            char pluginFilePath[512] = {0};
            XPLMGetPluginInfo(myId, nullptr, pluginFilePath, nullptr, nullptr);
            std::string pPath(pluginFilePath);
            size_t slash = pPath.find_last_of("/\\");
            if (slash != std::string::npos) {
                std::string pluginDir = pPath.substr(0, slash);
                size_t prevSlash = pluginDir.find_last_of("/\\");
                std::string parentDir = (prevSlash != std::string::npos) ? pluginDir.substr(0, prevSlash) : pluginDir;

                std::vector<std::string> searchDirs = {
                    pluginDir,
                    parentDir,
                    parentDir + "/Resources",
                    parentDir + "/images",
                    parentDir + "/notes"
                };

                for (const auto& d : searchDirs) {
                    std::string candidate = d + "/" + cleanUrl;
                    if (checkCandidate(candidate, st)) {
                        resolvedPath = candidate;
                        fileFound = true;
                        break;
                    }
                }
            }
        }
    }
#endif

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

    int width = 0, height = 0;
    unsigned char* pixels = DecodeImageFileInternal(resolvedPath.c_str(), &width, &height);
    if (!pixels || width <= 0 || height <= 0) {
        if (pixels) FreeDecodedImageInternal(pixels);
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
    FreeDecodedImageInternal(pixels);

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

namespace MarkdownImageDetail {
    struct AutoMarkdownImageServiceInit {
        AutoMarkdownImageServiceInit() {
            EnableDefaultImageService(true);
        }
    };
    static AutoMarkdownImageServiceInit s_AutoMarkdownImageServiceInit;
}

} // namespace MarkdownImageService
} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_MARKDOWN_IMAGE_SERVICE_H
