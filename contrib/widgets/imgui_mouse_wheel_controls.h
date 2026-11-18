#pragma once
#ifndef IMGUI_CONTRIB_FORWARD_IMGUI_MOUSE_WHEEL_CONTROLS_H
#define IMGUI_CONTRIB_FORWARD_IMGUI_MOUSE_WHEEL_CONTROLS_H
#if defined(__has_include)
  #if !__has_include("../ImGee/widgets/imgui_mouse_wheel_controls.h")
    #error "ImgWindow/contrib/ImGee submodule is not initialized! Please run: git submodule update --init --recursive"
  #else
    #include "../ImGee/widgets/imgui_mouse_wheel_controls.h"
  #endif
#else
  #include "../ImGee/widgets/imgui_mouse_wheel_controls.h"
#endif
#endif
