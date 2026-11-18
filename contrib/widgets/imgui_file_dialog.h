#pragma once
#ifndef IMGUI_CONTRIB_FORWARD_IMGUI_FILE_DIALOG_H
#define IMGUI_CONTRIB_FORWARD_IMGUI_FILE_DIALOG_H
#if defined(__has_include)
  #if !__has_include("../ImGee/widgets/imgui_file_dialog.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "../ImGee/widgets/imgui_file_dialog.h"
  #endif
#else
  #include "../ImGee/widgets/imgui_file_dialog.h"
#endif
#endif
