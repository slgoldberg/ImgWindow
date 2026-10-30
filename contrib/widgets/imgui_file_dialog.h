// ============================================================================
// ImGui Contrib Widgets: File Picker Combo & In-Cockpit File Dialog (imgui_file_dialog.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone C++11 and Lua file selection widgets for Dear ImGui.
// Features:
// - Universal C++11 Standard: Uses native POSIX (opendir/readdir) on macOS/Linux
//   and native Win32 (FindFirstFileW/FindNextFileW) on Windows.
// - Zero pipe/cmd.exe execution: Completely eliminates Windows io.popen() console
//   flashing, deadlocks, and process-spawning overhead.
// - In-Cockpit Modal Dialog: Renders inside the simulation viewport; safe for VR,
//   does not block the X-Plane main rendering thread or stall graphics drivers.
// - FilePickerCombo: Lightweight inline toolbar dropdown for quick file switching.
// - OpenFileDialog & SaveFileDialog: Full modal browser with breadcrumb navigation,
//   live search filter, file size formatting, and extension enforcement.
// ============================================================================

#pragma once

#if !defined(CONTRIB_DISABLE_FILE_DIALOG) && \
    !defined(CONTRIB_DISABLE_FILE_PICKER) && \
    !defined(IMGUI_CONTRIB_DISABLE_FILE_DIALOG) && \
    !defined(IMGUI_CONTRIB_DISABLE_FILE_PICKER) && \
    !defined(IMGUI_DISABLE_CONTRIB_FILE_DIALOG) && \
    !defined(IMGUI_DISABLE_FILE_DIALOG)

#ifndef IMGUI_FILE_DIALOG_H
#define IMGUI_FILE_DIALOG_H

#include "imgui.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdint>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace ImGui {

    struct FileDialogItem {
        std::string name;
        std::string full_path;
        bool        is_directory = false;
        uint64_t    size_bytes   = 0;
    };

    namespace Detail {

        inline std::string NormalizePath(const std::string& path) {
            std::string result = path;
            for (char& c : result) {
                if (c == '\\') c = '/';
            }
            while (result.size() > 1 && result.back() == '/') {
                result.pop_back();
            }
            return result.empty() ? "." : result;
        }

        inline std::string GetParentDirectory(const std::string& path) {
            std::string norm = NormalizePath(path);
            size_t slash = norm.find_last_of('/');
            if (slash == std::string::npos) return ".";
            if (slash == 0) return "/";
            return norm.substr(0, slash);
        }

        inline bool MatchesExtensionFilter(const std::string& filename, const std::string& filter) {
            if (filter.empty() || filter == "*.*" || filter == "*") return true;

            // Extract filename extension (lowercase)
            size_t dot = filename.find_last_of('.');
            if (dot == std::string::npos) return false;
            std::string ext = filename.substr(dot);
            for (char& c : ext) c = (char)tolower((unsigned char)c);

            // Filter can be semicolon- or comma-separated, e.g. ".md;.txt;.lua"
            std::string lower_filter = filter;
            for (char& c : lower_filter) c = (char)tolower((unsigned char)c);

            size_t start = 0;
            while (start < lower_filter.size()) {
                size_t sep = lower_filter.find_first_of(";, ", start);
                size_t len = (sep == std::string::npos) ? (lower_filter.size() - start) : (sep - start);
                if (len > 0) {
                    std::string token = lower_filter.substr(start, len);
                    if (token.front() != '.') token = "." + token;
                    if (ext == token) return true;
                }
                if (sep == std::string::npos) break;
                start = sep + 1;
            }
            return false;
        }

        inline std::vector<FileDialogItem> ScanDirectory(const std::string& directory_path, const std::string& filter) {
            std::vector<FileDialogItem> entries;
            std::string dir = NormalizePath(directory_path);

#if defined(_WIN32)
            std::string search_mask = dir + "/*";
            // Convert UTF-8 path to wide string for Windows Unicode API
            int wlen = MultiByteToWideChar(CP_UTF8, 0, search_mask.c_str(), -1, NULL, 0);
            if (wlen <= 0) return entries;
            std::vector<wchar_t> wmask(wlen);
            MultiByteToWideChar(CP_UTF8, 0, search_mask.c_str(), -1, wmask.data(), wlen);

            WIN32_FIND_DATAW fd;
            HANDLE hFind = FindFirstFileW(wmask.data(), &fd);
            if (hFind == INVALID_HANDLE_VALUE) return entries;

            do {
                // Convert back from wide to UTF-8
                int nlen = WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, NULL, 0, NULL, NULL);
                if (nlen <= 0) continue;
                std::vector<char> nbuf(nlen);
                WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, nbuf.data(), nlen, NULL, NULL);
                std::string fname = nbuf.data();

                if (fname == "." || fname == "..") continue;

                bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                if (!is_dir && !MatchesExtensionFilter(fname, filter)) {
                    continue;
                }

                FileDialogItem item;
                item.name = fname;
                item.full_path = dir + "/" + fname;
                item.is_directory = is_dir;
                item.size_bytes = is_dir ? 0 : (((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow);
                entries.push_back(item);
            } while (FindNextFileW(hFind, &fd));

            FindClose(hFind);
#else
            DIR* d = opendir(dir.c_str());
            if (!d) return entries;

            struct dirent* de;
            while ((de = readdir(d)) != NULL) {
                std::string fname = de->d_name;
                if (fname == "." || fname == "..") continue;

                std::string full = dir + "/" + fname;
                struct stat st;
                if (stat(full.c_str(), &st) != 0) continue;

                bool is_dir = S_ISDIR(st.st_mode);
                if (!is_dir && !MatchesExtensionFilter(fname, filter)) {
                    continue;
                }

                FileDialogItem item;
                item.name = fname;
                item.full_path = full;
                item.is_directory = is_dir;
                item.size_bytes = is_dir ? 0 : (uint64_t)st.st_size;
                entries.push_back(item);
            }
            closedir(d);
#endif

            // Sort: directories first (alphabetical), then files (alphabetical)
            std::sort(entries.begin(), entries.end(), [](const FileDialogItem& a, const FileDialogItem& b) {
                if (a.is_directory != b.is_directory) {
                    return a.is_directory > b.is_directory;
                }
                return a.name < b.name;
            });

            return entries;
        }

        inline std::string FormatFileSize(uint64_t bytes) {
            char buf[32];
            if (bytes < 1024) {
                snprintf(buf, sizeof(buf), "%u B", (unsigned int)bytes);
            } else if (bytes < 1024 * 1024) {
                snprintf(buf, sizeof(buf), "%.1f KB", (double)bytes / 1024.0);
            } else {
                snprintf(buf, sizeof(buf), "%.1f MB", (double)bytes / (1024.0 * 1024.0));
            }
            return std::string(buf);
        }

    } // namespace Detail

    // ========================================================================
    // 1. FilePickerCombo: Inline Toolbar File Dropdown
    // ========================================================================
    // Scans a directory and renders a compact dropdown selector.
    // Returns true when the user selects a new file.
    // ========================================================================
    inline bool FilePickerCombo(const char* label, std::string* current_file, const char* directory, const char* extensions = nullptr, ImGuiComboFlags flags = 0) {
        if (!current_file || !directory) return false;

        bool value_changed = false;
        std::string preview_text = current_file->empty() ? "(None)" : *current_file;
        // Show only filename in preview if a full path is stored
        size_t slash = preview_text.find_last_of("/\\");
        if (slash != std::string::npos) {
            preview_text = preview_text.substr(slash + 1);
        }

        if (ImGui::BeginCombo(label, preview_text.c_str(), flags)) {
            std::vector<FileDialogItem> items = Detail::ScanDirectory(directory, extensions ? extensions : "");
            
            if (items.empty()) {
                ImGui::TextDisabled("  (No matching files)");
            } else {
                for (const auto& item : items) {
                    if (item.is_directory) continue; // Only files in the combo selector

                    bool is_selected = (*current_file == item.name || *current_file == item.full_path);
                    if (ImGui::Selectable(item.name.c_str(), is_selected)) {
                        *current_file = item.name;
                        value_changed = true;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }
        return value_changed;
    }

    // C-string buffer overload for C-linkage and Lua bindings:
    inline bool FilePickerCombo(const char* label, char* file_buf, size_t file_buf_size, const char* directory, const char* extensions = nullptr, ImGuiComboFlags flags = 0) {
        if (!file_buf || file_buf_size == 0) return false;
        std::string current(file_buf);
        if (FilePickerCombo(label, &current, directory, extensions, flags)) {
            snprintf(file_buf, file_buf_size, "%s", current.c_str());
            return true;
        }
        return false;
    }

    // ========================================================================
    // 2. OpenFileDialog & SaveFileDialog: In-Cockpit Modal File Browser
    // ========================================================================
    // Renders an immediate-mode modal dialog inside the X-Plane viewport.
    // Returns true when the user confirms their selection ([Open] or [Save]).
    // ========================================================================
    namespace Detail {
        struct FileDialogState {
            std::string current_dir;
            std::string selected_file;
            char        filter_buf[64] = {0};
            char        filename_input[256] = {0};
            bool        needs_scan = true;
            std::vector<FileDialogItem> cached_items;
        };

        inline FileDialogState& GetDialogState(const char* title) {
            // Static registry keyed to dialog title
            static std::unordered_map<std::string, FileDialogState> s_DialogStates;
            return s_DialogStates[title ? title : "FileDialog"];
        }
    }

    inline bool OpenFileDialog(const char* title, bool* is_open, std::string* out_filepath, const char* starting_dir = ".", const char* extensions = nullptr) {
        if (!is_open || !*is_open) return false;

        bool confirmed = false;
        Detail::FileDialogState& state = Detail::GetDialogState(title);

        if (state.current_dir.empty()) {
            state.current_dir = Detail::NormalizePath(starting_dir ? starting_dir : ".");
            state.needs_scan = true;
        }

        ImGui::SetNextWindowSize(ImVec2(560.0f, 380.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::IsPopupOpen(title)) {
            ImGui::OpenPopup(title);
        }

        if (ImGui::BeginPopupModal(title, is_open, ImGuiWindowFlags_NoScrollbar)) {
            // --- 1. Navigation & Path Bar ---
            if (ImGui::Button("< Back") || ImGui::Button("Up")) {
                state.current_dir = Detail::GetParentDirectory(state.current_dir);
                state.needs_scan = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Path:");
            ImGui::SameLine();
            ImGui::TextUnformatted(state.current_dir.c_str());

            // --- 2. Live Search Filter ---
            ImGui::SameLine(ImGui::GetWindowWidth() - 200.0f);
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::InputTextWithHint("##search", "Search...", state.filter_buf, sizeof(state.filter_buf))) {
                // Filter string updated
            }

            ImGui::Separator();

            // Refresh directory cache if needed
            if (state.needs_scan) {
                state.cached_items = Detail::ScanDirectory(state.current_dir, extensions ? extensions : "");
                state.needs_scan = false;
            }

            // --- 3. Directory & File List Table ---
            float table_height = ImGui::GetContentRegionAvail().y - 42.0f;
            if (ImGui::BeginChild("##file_list", ImVec2(0.0f, table_height), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                for (const auto& item : state.cached_items) {
                    // Filter match
                    if (state.filter_buf[0] != '\0') {
                        std::string lower_name = item.name;
                        for (char& c : lower_name) c = (char)tolower((unsigned char)c);
                        std::string lower_filter = state.filter_buf;
                        for (char& c : lower_filter) c = (char)tolower((unsigned char)c);
                        if (lower_name.find(lower_filter) == std::string::npos) continue;
                    }

                    if (item.is_directory) {
                        // Directory entry
                        std::string dir_label = "[DIR]  " + item.name;
                        if (ImGui::Selectable(dir_label.c_str(), false)) {
                            state.current_dir = item.full_path;
                            state.needs_scan = true;
                        }
                    } else {
                        // File entry
                        bool is_sel = (state.selected_file == item.name);
                        std::string file_label = "       " + item.name;
                        if (ImGui::Selectable(file_label.c_str(), is_sel)) {
                            state.selected_file = item.name;
                            snprintf(state.filename_input, sizeof(state.filename_input), "%s", item.name.c_str());
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            state.selected_file = item.name;
                            if (out_filepath) *out_filepath = item.full_path;
                            confirmed = true;
                            *is_open = false;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine(ImGui::GetWindowWidth() - 90.0f);
                        ImGui::TextDisabled("%s", Detail::FormatFileSize(item.size_bytes).c_str());
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            // --- 4. Bottom Action Bar ---
            ImGui::TextUnformatted("File:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 170.0f);
            ImGui::InputText("##sel_file", state.filename_input, sizeof(state.filename_input));

            ImGui::SameLine();
            if (ImGui::Button("Open", ImVec2(75.0f, 0.0f))) {
                if (state.filename_input[0] != '\0') {
                    if (out_filepath) *out_filepath = state.current_dir + "/" + state.filename_input;
                    confirmed = true;
                    *is_open = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(75.0f, 0.0f))) {
                *is_open = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        return confirmed;
    }

    inline bool SaveFileDialog(const char* title, bool* is_open, std::string* out_filepath, const char* starting_dir = ".", const char* default_filename = "", const char* extensions = nullptr) {
        if (!is_open || !*is_open) return false;

        bool confirmed = false;
        Detail::FileDialogState& state = Detail::GetDialogState(title);

        if (state.current_dir.empty()) {
            state.current_dir = Detail::NormalizePath(starting_dir ? starting_dir : ".");
            if (default_filename && default_filename[0] != '\0') {
                snprintf(state.filename_input, sizeof(state.filename_input), "%s", default_filename);
            }
            state.needs_scan = true;
        }

        ImGui::SetNextWindowSize(ImVec2(560.0f, 380.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::IsPopupOpen(title)) {
            ImGui::OpenPopup(title);
        }

        if (ImGui::BeginPopupModal(title, is_open, ImGuiWindowFlags_NoScrollbar)) {
            // Navigation
            if (ImGui::Button("< Back") || ImGui::Button("Up")) {
                state.current_dir = Detail::GetParentDirectory(state.current_dir);
                state.needs_scan = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Path:");
            ImGui::SameLine();
            ImGui::TextUnformatted(state.current_dir.c_str());

            ImGui::Separator();

            if (state.needs_scan) {
                state.cached_items = Detail::ScanDirectory(state.current_dir, extensions ? extensions : "");
                state.needs_scan = false;
            }

            float table_height = ImGui::GetContentRegionAvail().y - 42.0f;
            if (ImGui::BeginChild("##save_file_list", ImVec2(0.0f, table_height), true)) {
                for (const auto& item : state.cached_items) {
                    if (item.is_directory) {
                        std::string dir_label = "[DIR]  " + item.name;
                        if (ImGui::Selectable(dir_label.c_str(), false)) {
                            state.current_dir = item.full_path;
                            state.needs_scan = true;
                        }
                    } else {
                        bool is_sel = (state.selected_file == item.name);
                        std::string file_label = "       " + item.name;
                        if (ImGui::Selectable(file_label.c_str(), is_sel)) {
                            state.selected_file = item.name;
                            snprintf(state.filename_input, sizeof(state.filename_input), "%s", item.name.c_str());
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::TextUnformatted("Save As:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 170.0f);
            ImGui::InputText("##save_filename", state.filename_input, sizeof(state.filename_input));

            ImGui::SameLine();
            if (ImGui::Button("Save", ImVec2(75.0f, 0.0f))) {
                if (state.filename_input[0] != '\0') {
                    std::string target = state.filename_input;
                    // Auto-append primary extension if missing and an extension filter is provided
                    if (extensions && extensions[0] != '\0' && target.find('.') == std::string::npos) {
                        std::string first_ext = extensions;
                        size_t sep = first_ext.find_first_of(";, ");
                        if (sep != std::string::npos) first_ext = first_ext.substr(0, sep);
                        if (!first_ext.empty() && first_ext.front() != '.') first_ext = "." + first_ext;
                        if (first_ext != ".*") target += first_ext;
                    }
                    if (out_filepath) *out_filepath = state.current_dir + "/" + target;
                    confirmed = true;
                    *is_open = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(75.0f, 0.0f))) {
                *is_open = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        return confirmed;
    }

    // C-string buffer overloads for Lua / C-linkage
    inline bool OpenFileDialog(const char* title, bool* is_open, char* out_buf, size_t out_buf_size, const char* starting_dir = ".", const char* extensions = nullptr) {
        if (!out_buf || out_buf_size == 0) return false;
        std::string chosen;
        if (OpenFileDialog(title, is_open, &chosen, starting_dir, extensions)) {
            snprintf(out_buf, out_buf_size, "%s", chosen.c_str());
            return true;
        }
        return false;
    }

    inline bool SaveFileDialog(const char* title, bool* is_open, char* out_buf, size_t out_buf_size, const char* starting_dir = ".", const char* default_filename = "", const char* extensions = nullptr) {
        if (!out_buf || out_buf_size == 0) return false;
        std::string chosen;
        if (SaveFileDialog(title, is_open, &chosen, starting_dir, default_filename, extensions)) {
            snprintf(out_buf, out_buf_size, "%s", chosen.c_str());
            return true;
        }
        return false;
    }

} // namespace ImGui

#endif // IMGUI_FILE_DIALOG_H
#endif // !CONTRIB_DISABLE_FILE_DIALOG
