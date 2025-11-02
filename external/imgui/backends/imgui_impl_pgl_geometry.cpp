// dear imgui: Renderer Backend for PortableGL based on the SDL_Renderer backend

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
//  2021-12-21: Update SDL_RenderGeometryRaw() format to work with SDL 2.0.19.
//  2021-12-03: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2021-10-06: Backup and restore modified ClipRect/Viewport.
//  2021-09-21: Initial version.

#include "imgui.h"
#include "imgui_impl_pgl_geometry.h"
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

#include "portablegl.h"

// SDL_Renderer data
struct ImGui_ImplPGL_Geometry_Data
{
    GLuint    FontTexture;
    ImGui_ImplPGL_Geometry_Data() { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplPGL_Geometry_Data* ImGui_ImplPGL_Geometry_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplPGL_Geometry_Data*)ImGui::GetIO().BackendRendererUserData : NULL;
}

// Functions
bool ImGui_ImplPGL_Geometry_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplPGL_Geometry_Data* bd = IM_NEW(ImGui_ImplPGL_Geometry_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_pgl_geometry";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    return true;
}

void ImGui_ImplPGL_Geometry_Shutdown()
{
    ImGui_ImplPGL_Geometry_Data* bd = ImGui_ImplPGL_Geometry_GetBackendData();
    IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplPGL_Geometry_DestroyDeviceObjects();

    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplPGL_Geometry_SetupRenderState()
{
    ImGui_ImplPGL_Geometry_Data* bd = ImGui_ImplPGL_Geometry_GetBackendData();
    // TODO Set viewport and cliprect to whole screen ... once draw_geometry_raw
    // actually pays attention to viewport or cliprect rather than just clipping
    // to the framebuffer size
}

void ImGui_ImplPGL_Geometry_NewFrame()
{
    ImGui_ImplPGL_Geometry_Data* bd = ImGui_ImplPGL_Geometry_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplPGL_Geometry_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplPGL_Geometry_CreateDeviceObjects();
}

void ImGui_ImplPGL_Geometry_RenderDrawData(ImDrawData* draw_data)
{
    ImGui_ImplPGL_Geometry_Data* bd = ImGui_ImplPGL_Geometry_GetBackendData();

    // If there's a scale factor set by the user, use that instead
    float rsx = 1.0f;
    float rsy = 1.0f;
    //SDL_RenderGetScale(bd->PGL_Geometry, &rsx, &rsy);
    ImVec2 render_scale;
    render_scale.x = (rsx == 1.0f) ? draw_data->FramebufferScale.x : 1.0f;
    render_scale.y = (rsy == 1.0f) ? draw_data->FramebufferScale.y : 1.0f;

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * render_scale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * render_scale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    // Backup SDL_Renderer state that will be modified to restore it afterwards
    struct BackupPGL_GeometryState
    {
        ivec4    Viewport;
        bool     ClipEnabled;
        ivec4    ClipRect;
    };
    BackupPGL_GeometryState old = {};
    //old.ClipEnabled = SDL_RenderIsClipEnabled(bd->PGL_Geometry) == SDL_TRUE;
    //SDL_RenderGetViewport(bd->PGL_Geometry, &old.Viewport);
    //SDL_RenderGetClipRect(bd->PGL_Geometry, &old.ClipRect);
    glGetIntegerv(GL_SCISSOR_BOX, (GLint*)&old.ClipRect);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = render_scale;

    // Render command lists
    ImGui_ImplPGL_Geometry_SetupRenderState();
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplPGL_Geometry_SetupRenderState();
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
                if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
                if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                //SDL_Rect r = { (int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y) };
                //SDL_RenderSetClipRect(bd->PGL_Geometry, &r);
                glScissor((int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));

                const float* xy = (const float*)((const char*)(vtx_buffer + pcmd->VtxOffset) + IM_OFFSETOF(ImDrawVert, pos));
                const float* uv = (const float*)((const char*)(vtx_buffer + pcmd->VtxOffset) + IM_OFFSETOF(ImDrawVert, uv));
                const Color* color = (const Color*)((const char*)(vtx_buffer + pcmd->VtxOffset) + IM_OFFSETOF(ImDrawVert, col));

                // Bind texture, Draw
                GLuint tex = (GLuint)(intptr_t)pcmd->GetTexID();
                pgl_draw_geometry_raw(tex,
                    xy, (int)sizeof(ImDrawVert),
                    color, (int)sizeof(ImDrawVert),
                    uv, (int)sizeof(ImDrawVert),
                    cmd_list->VtxBuffer.Size - pcmd->VtxOffset,
                    idx_buffer + pcmd->IdxOffset, pcmd->ElemCount, sizeof(ImDrawIdx));
            }
        }
    }

    // Restore modified SDL_Renderer state
    //SDL_RenderSetViewport(bd->PGL_Geometry, &old.Viewport);
    //SDL_RenderSetClipRect(bd->PGL_Geometry, old.ClipEnabled ? &old.ClipRect : NULL);
    glScissor(old.ClipRect.x, old.ClipRect.y, old.ClipRect.z, old.ClipRect.w);
}

// Called by Init/NewFrame/Shutdown
bool ImGui_ImplPGL_Geometry_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPGL_Geometry_Data* bd = ImGui_ImplPGL_Geometry_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    glGenTextures(1, &bd->FontTexture);
    glBindTexture(GL_TEXTURE_2D, bd->FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH // Not on WebGL/ES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

    return true;
}

void ImGui_ImplPGL_Geometry_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPGL_Geometry_Data* bd = ImGui_ImplPGL_Geometry_GetBackendData();
    if (bd->FontTexture)
    {
        glDeleteTextures(1, &bd->FontTexture);
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

bool ImGui_ImplPGL_Geometry_CreateDeviceObjects()
{
    return ImGui_ImplPGL_Geometry_CreateFontsTexture();
}

void ImGui_ImplPGL_Geometry_DestroyDeviceObjects()
{
    ImGui_ImplPGL_Geometry_DestroyFontsTexture();
}

