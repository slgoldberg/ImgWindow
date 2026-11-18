#pragma once
#ifndef IMGUI_CONTRIB_FORWARD_INCLUDES_H
#define IMGUI_CONTRIB_FORWARD_INCLUDES_H
#if defined(__has_include)
  #if !__has_include("../ImGee/widgets/includes.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "../ImGee/widgets/includes.h"
  #endif
#else
  #include "../ImGee/widgets/includes.h"
#endif
#endif
