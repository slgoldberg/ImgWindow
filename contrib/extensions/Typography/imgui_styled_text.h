#pragma once
#ifndef IMGUI_CONTRIB_FORWARD_IMGUI_STYLED_TEXT_H
#define IMGUI_CONTRIB_FORWARD_IMGUI_STYLED_TEXT_H
#if defined(__has_include)
  #if !__has_include("../../ImGee/extensions/Typography/imgui_styled_text.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "../../ImGee/extensions/Typography/imgui_styled_text.h"
  #endif
#else
  #include "../../ImGee/extensions/Typography/imgui_styled_text.h"
#endif
#endif
