#pragma once
#ifndef IMGUI_CONTRIB_FORWARD_IMGUI_MARKDOWN_EXT_H
#define IMGUI_CONTRIB_FORWARD_IMGUI_MARKDOWN_EXT_H
#if defined(__has_include)
  #if !__has_include("../../ImGee/extensions/Markdown/imgui_markdown_ext.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "../../ImGee/extensions/Markdown/imgui_markdown_ext.h"
  #endif
#else
  #include "../../ImGee/extensions/Markdown/imgui_markdown_ext.h"
#endif
#endif
