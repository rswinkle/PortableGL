// dear imgui: Renderer Backend for PortableGL based on OpenGL3 backend
// This needs to be used along with a Platform Backend (currently only tested with SDL2)

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplPortableGL_Init();
IMGUI_IMPL_API void     ImGui_ImplPortableGL_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplPortableGL_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplPortableGL_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplPortableGL_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplPortableGL_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplPortableGL_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplPortableGL_DestroyDeviceObjects();


#endif // #ifndef IMGUI_DISABLE

