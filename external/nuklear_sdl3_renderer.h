/*
 * Nuklear - 4.9.4 - public domain
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */

#ifndef NK_SDL3_RENDERER_H_
#define NK_SDL3_RENDERER_H_

#if SDL_MAJOR_VERSION < 3
#error "nuklear_sdl3_renderer requires at least SDL 3.0.0"
#endif

NK_API struct nk_context*   nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer);
NK_API struct nk_font_atlas* nk_sdl_font_stash_begin(struct nk_context* ctx);
NK_API void                 nk_sdl_font_stash_end(struct nk_context* ctx);
NK_API int                  nk_sdl_handle_event(struct nk_context* ctx, SDL_Event *evt);
NK_API void                 nk_sdl_render(struct nk_context* ctx, enum nk_anti_aliasing);
NK_API void                 nk_sdl_shutdown(struct nk_context* ctx);
NK_API nk_handle            nk_sdl_userdata(struct nk_context* ctx);
NK_API void                 nk_sdl_set_userdata(struct nk_context* ctx, nk_handle userdata);
NK_API void                 nk_sdl_scale(struct nk_context* ctx, float x_scale, float y_scale);

#endif /* NK_SDL3_RENDERER_H_ */

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_SDL3_RENDERER_IMPLEMENTATION
#ifndef NK_SDL3_RENDERER_IMPLEMENTATION_ONCE
#define NK_SDL3_RENDERER_IMPLEMENTATION_ONCE

#ifndef NK_INCLUDE_COMMAND_USERDATA
#error "nuklear_sdl3 requires the NK_INCLUDE_COMMAND_USERDATA define"
#endif

enum { NK_SDL3_NO_CHANGE, NK_SDL3_START_TEXT, NK_SDL3_STOP_TEXT };

struct nk_sdl_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture tex_null;
    SDL_Texture *font_tex;
};

struct nk_sdl_vertex {
    float position[2];
    float uv[2];
    float col[4];
};

struct nk_sdl {
    SDL_Window *win;
    SDL_Renderer *renderer;
    float scale_x, scale_y;
    struct nk_sdl_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    nk_handle userdata;
    Uint64 time_of_last_frame;
};

NK_API nk_handle nk_sdl_userdata(struct nk_context* ctx) {
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    return sdl->userdata;
}

NK_API void nk_sdl_set_userdata(struct nk_context* ctx, nk_handle userdata) {
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    sdl->userdata = userdata;
}

NK_INTERN void
nk_sdl_device_upload_atlas(struct nk_context* ctx, const void *image, int width, int height)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    /* Clean up if the texture already exists. */
    if (sdl->ogl.font_tex != NULL) {
        SDL_DestroyTexture(sdl->ogl.font_tex);
        sdl->ogl.font_tex = NULL;
    }

    SDL_Texture *g_SDLFontTexture = SDL_CreateTexture(sdl->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if (g_SDLFontTexture == NULL) {
        SDL_Log("error creating texture");
        return;
    }
    SDL_UpdateTexture(g_SDLFontTexture, NULL, image, 4 * width);
    SDL_SetTextureBlendMode(g_SDLFontTexture, SDL_BLENDMODE_BLEND);
    sdl->ogl.font_tex = g_SDLFontTexture;
}

NK_API void
nk_sdl_render(struct nk_context* ctx, enum nk_anti_aliasing AA)
{
    /* setup global state */
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    {
        SDL_Rect saved_clip;
        bool clipping_enabled;
        int vs = sizeof(struct nk_sdl_vertex);
        size_t vp = NK_OFFSETOF(struct nk_sdl_vertex, position);
        size_t vt = NK_OFFSETOF(struct nk_sdl_vertex, uv);
        size_t vc = NK_OFFSETOF(struct nk_sdl_vertex, col);

        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        struct nk_buffer vbuf, ebuf;

        /* fill converting configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION,    NK_FORMAT_FLOAT,                NK_OFFSETOF(struct nk_sdl_vertex, position)},
            {NK_VERTEX_TEXCOORD,    NK_FORMAT_FLOAT,                NK_OFFSETOF(struct nk_sdl_vertex, uv)},
            {NK_VERTEX_COLOR,       NK_FORMAT_R32G32B32A32_FLOAT,   NK_OFFSETOF(struct nk_sdl_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };

        Uint64 now = SDL_GetTicks();
        sdl->ctx.delta_time_seconds = (float)(now - sdl->time_of_last_frame) / 1000;
        sdl->time_of_last_frame = now;

        NK_MEMSET(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_sdl_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
        config.tex_null = sdl->ogl.tex_null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;

        /* convert shapes into vertexes */
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&sdl->ctx, &sdl->ogl.cmds, &vbuf, &ebuf, &config);

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);

        clipping_enabled = SDL_RenderClipEnabled(sdl->renderer);
        SDL_GetRenderClipRect(sdl->renderer, &saved_clip);

        nk_draw_foreach(cmd, &sdl->ctx, &sdl->ogl.cmds)
        {
            if (!cmd->elem_count) continue;

            {
                SDL_Rect r;
                r.x = cmd->clip_rect.x;
                r.y = cmd->clip_rect.y;
                r.w = cmd->clip_rect.w;
                r.h = cmd->clip_rect.h;
                SDL_SetRenderClipRect(sdl->renderer, &r);
            }

            {
                const void *vertices = nk_buffer_memory_const(&vbuf);

                SDL_RenderGeometryRaw(sdl->renderer,
                        (SDL_Texture *)cmd->texture.ptr,
                        (const float*)((const nk_byte*)vertices + vp), vs,
                        (const SDL_FColor*)((const nk_byte*)vertices + vc), vs,
                        (const float*)((const nk_byte*)vertices + vt), vs,
                        (vbuf.needed / vs),
                        (void *) offset, cmd->elem_count, 2);

                offset += cmd->elem_count;
            }
        }

        SDL_SetRenderClipRect(sdl->renderer, &saved_clip);
        if (!clipping_enabled) {
            SDL_SetRenderClipRect(sdl->renderer, NULL);
        }

        nk_clear(&sdl->ctx);
        nk_buffer_clear(&sdl->ogl.cmds);
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }
}

static void
nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    NK_UNUSED(usr);
    if (text) {
        nk_textedit_paste(edit, text, nk_strlen(text));
        SDL_free((void  *)text);
    }
}

static void
nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    NK_UNUSED(usr);
    if (len <= 0 || text == NULL) return;
    str = SDL_strndup(text, (size_t)len);
    if (str == NULL) return;
    SDL_SetClipboardText(str);
    SDL_free((void*)str);
}

NK_API struct nk_context*
nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer)
{
    struct nk_sdl* sdl;
    NK_ASSERT(win);
    NK_ASSERT(renderer);
    sdl = SDL_malloc(sizeof(struct nk_sdl));
    NK_ASSERT(sdl);
    nk_zero(sdl, sizeof(struct nk_sdl));
    sdl->win = win;
    sdl->renderer = renderer;
    sdl->scale_x = sdl->scale_y = 1.0f;
    sdl->time_of_last_frame = SDL_GetTicks();
    nk_init_default(&sdl->ctx, 0);
    sdl->ctx.userdata = nk_handle_ptr((void*)sdl);
    sdl->ctx.clip.copy = nk_sdl_clipboard_copy;
    sdl->ctx.clip.paste = nk_sdl_clipboard_paste;
    sdl->ctx.clip.userdata = nk_handle_ptr(0);
    nk_buffer_init_default(&sdl->ogl.cmds);
    return &sdl->ctx;
}

NK_API struct nk_font_atlas*
nk_sdl_font_stash_begin(struct nk_context* ctx)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    nk_font_atlas_init_default(&sdl->atlas);
    nk_font_atlas_begin(&sdl->atlas);
    return &sdl->atlas;
}

NK_API void
nk_sdl_font_stash_end(struct nk_context* ctx)
{
    struct nk_sdl* sdl;
    const void *image; int w, h;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    image = nk_font_atlas_bake(&sdl->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    NK_ASSERT(image);
    nk_sdl_device_upload_atlas(&sdl->ctx, image, w, h);
    nk_font_atlas_end(&sdl->atlas, nk_handle_ptr(sdl->ogl.font_tex), &sdl->ogl.tex_null);
    if (sdl->atlas.default_font) {
        nk_style_set_font(&sdl->ctx, &sdl->atlas.default_font->handle);
    }
}

NK_API int
nk_sdl_handle_event(struct nk_context* ctx, SDL_Event *evt)
{
    NK_ASSERT(ctx);
    NK_ASSERT(evt);

    struct nk_sdl* sdl;
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    static int insert_toggle = 0;


    switch(evt->type)
    {
        case SDL_EVENT_KEY_UP: /* KEYUP & KEYDOWN share same routine */
        case SDL_EVENT_KEY_DOWN:
            {
                int down = evt->type == SDL_EVENT_KEY_DOWN;
                int ctrl_down = evt->key.mod & SDL_KMOD_CTRL;
                //SDL_Log("SPELLING ctrl_down = %d\n", ctrl_down);

                switch(evt->key.key)
                {
                    case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
                    case SDLK_LSHIFT:    nk_input_key(ctx, NK_KEY_SHIFT, down); break;
                    case SDLK_DELETE:    nk_input_key(ctx, NK_KEY_DEL, down); break;

                    case SDLK_KP_ENTER:
                    case SDLK_RETURN:    nk_input_key(ctx, NK_KEY_ENTER, down); break;

                    case SDLK_TAB:       nk_input_key(ctx, NK_KEY_TAB, down); break;
                    case SDLK_BACKSPACE: nk_input_key(ctx, NK_KEY_BACKSPACE, down); break;
                    case SDLK_HOME:      nk_input_key(ctx, NK_KEY_TEXT_START, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_START, down); break;
                    case SDLK_END:       nk_input_key(ctx, NK_KEY_TEXT_END, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_END, down); break;
                    case SDLK_PAGEDOWN:  nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down); break;
                    case SDLK_PAGEUP:    nk_input_key(ctx, NK_KEY_SCROLL_UP, down); break;

                    case SDLK_Z:         if (ctrl_down) nk_input_key(ctx, NK_KEY_TEXT_UNDO, down); break;
                    case SDLK_R:         if (ctrl_down) nk_input_key(ctx, NK_KEY_TEXT_REDO, down); break;
                    case SDLK_C:         if (ctrl_down) nk_input_key(ctx, NK_KEY_COPY, down); break;
                    case SDLK_V:         if (ctrl_down) nk_input_key(ctx, NK_KEY_PASTE, down); break;
                    case SDLK_X:         if (ctrl_down) nk_input_key(ctx, NK_KEY_CUT, down); break;
                    case SDLK_B:         if (ctrl_down) nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down); break;
                    case SDLK_E:         if (ctrl_down) nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down); break;

                    case SDLK_UP:        nk_input_key(ctx, NK_KEY_UP, down); break;
                    case SDLK_DOWN:      nk_input_key(ctx, NK_KEY_DOWN, down); break;
                    case SDLK_A:
                        if (ctrl_down)
                            nk_input_key(ctx,NK_KEY_TEXT_SELECT_ALL, down);
                        break;
                    case SDLK_LEFT:
                        if (ctrl_down)
                            nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
                        else nk_input_key(ctx, NK_KEY_LEFT, down);
                        break;
                    case SDLK_RIGHT:
                        if (ctrl_down)
                            nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                        else nk_input_key(ctx, NK_KEY_RIGHT, down);
                        break;
                    case SDLK_INSERT:
                        if (down) insert_toggle = !insert_toggle;
                        if (insert_toggle) {
                            nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, down);
                        } else {
                            nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, down);
                        }
                        break;

                        /*
                    case SDLK_RSHIFT: // RSHIFT & LSHIFT share same routine
                    case SDLK_LSHIFT:    nk_input_key(ctx, NK_KEY_SHIFT, down); break;
                    case SDLK_DELETE:    nk_input_key(ctx, NK_KEY_DEL, down); break;
                    case SDLK_RETURN:    nk_input_key(ctx, NK_KEY_ENTER, down); break;
                    case SDLK_TAB:       nk_input_key(ctx, NK_KEY_TAB, down); break;
                    case SDLK_BACKSPACE: nk_input_key(ctx, NK_KEY_BACKSPACE, down); break;
                    case SDLK_HOME:      nk_input_key(ctx, NK_KEY_TEXT_START, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_START, down); break;
                    case SDLK_END:       nk_input_key(ctx, NK_KEY_TEXT_END, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_END, down); break;
                    case SDLK_PAGEDOWN:  nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down); break;
                    case SDLK_PAGEUP:    nk_input_key(ctx, NK_KEY_SCROLL_UP, down); break;
                    case SDLK_A:         nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, down && ctrl_down); break;
                    case SDLK_Z:         nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && ctrl_down); break;
                    case SDLK_R:         nk_input_key(ctx, NK_KEY_TEXT_REDO, down && ctrl_down); break;
                    case SDLK_C:         nk_input_key(ctx, NK_KEY_COPY, down && ctrl_down); break;
                    case SDLK_V:         nk_input_key(ctx, NK_KEY_PASTE, down && ctrl_down); break;
                    case SDLK_X:         nk_input_key(ctx, NK_KEY_CUT, down && ctrl_down); break;
                    case SDLK_B:         nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && ctrl_down); break;
                    case SDLK_E:         nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && ctrl_down); break;
                    case SDLK_UP:        nk_input_key(ctx, NK_KEY_UP, down); break;
                    case SDLK_DOWN:      nk_input_key(ctx, NK_KEY_DOWN, down); break;
                    case SDLK_LEFT:
                        if (ctrl_down)
                            nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
                        else
                            nk_input_key(ctx, NK_KEY_LEFT, down);
                        break;
                    case SDLK_RIGHT:
                        if (ctrl_down)
                            nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                        else
                            nk_input_key(ctx, NK_KEY_RIGHT, down);
                        break;
                        */
                }
                return 1;
            }

        case SDL_EVENT_MOUSE_BUTTON_UP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                int down = evt->button.down;
                //const int x = evt->button.x, y = evt->button.y;
                const int x = evt->button.x/sdl->scale_x, y = evt->button.y/sdl->scale_y;
                switch(evt->button.button)
                {
                    case SDL_BUTTON_LEFT:
                        if (evt->button.clicks > 1)
                            nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
                        else
                            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
                        break;
                    case SDL_BUTTON_MIDDLE: nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down); break;
                    case SDL_BUTTON_RIGHT:  nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down); break;
                }
            }
            return 1;

        case SDL_EVENT_MOUSE_MOTION:
            //if (ctx->input.mouse.grabbed) {
            //    // previous positions were already scaled
            //    int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
            //    nk_input_motion(ctx, x + evt->motion.xrel/scale_x, y + evt->motion.yrel/scale_y);
            //}
            //else nk_input_motion(ctx, evt->motion.x/scale_x, evt->motion.y/scale_y);

            nk_input_motion(ctx, evt->motion.x/sdl->scale_x, evt->motion.y/sdl->scale_y);

            // original unscaled inline version
            //ctx->input.mouse.pos.x = evt->motion.x;
            //ctx->input.mouse.pos.y = evt->motion.y;
            //ctx->input.mouse.delta.x = ctx->input.mouse.pos.x - ctx->input.mouse.prev.x;
            //ctx->input.mouse.delta.y = ctx->input.mouse.pos.y - ctx->input.mouse.prev.y;
            return 1;

        case SDL_EVENT_TEXT_INPUT:
            {
                nk_glyph glyph;
                SDL_memcpy(glyph, evt->text.text, NK_UTF_SIZE);
                nk_input_glyph(ctx, glyph);
            }
            return 1;

        case SDL_EVENT_MOUSE_WHEEL:
            nk_input_scroll(ctx, nk_vec2(evt->wheel.x, evt->wheel.y));
            return 1;
    }
    return 0;
}

NK_API
void nk_sdl_shutdown(struct nk_context* ctx)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    nk_font_atlas_clear(&sdl->atlas);
    nk_buffer_free(&sdl->ogl.cmds);

    if (sdl->ogl.font_tex != NULL) {
        SDL_DestroyTexture(sdl->ogl.font_tex);
        sdl->ogl.font_tex = NULL;
    }

    ctx->userdata = nk_handle_ptr(0);
    SDL_free(sdl);
    nk_free(ctx);
}

NK_API void
nk_sdl_scale(struct nk_context* ctx, float x_scale, float y_scale)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    sdl->scale_x = x_scale;
    sdl->scale_y = y_scale;
}

#endif /* NK_SDL3_RENDERER_IMPLEMENTATION_ONCE */
#endif /* NK_SDL3_RENDERER_IMPLEMENTATION */
