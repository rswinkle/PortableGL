// dear imgui: Renderer Backend for PortableGL based on the SDL_Renderer backend
//
// Technically not true 
// This needs to be used along with a Platform Backend (currently only tested with SDL2)

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

IMGUI_IMPL_API bool     ImGui_ImplPGL_Geometry_Init();
IMGUI_IMPL_API void     ImGui_ImplPGL_Geometry_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplPGL_Geometry_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplPGL_Geometry_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplPGL_Geometry_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplPGL_Geometry_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplPGL_Geometry_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplPGL_Geometry_DestroyDeviceObjects();

