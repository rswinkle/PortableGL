#include "video_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void init_ffmpeg(void)
{
    static bool initialized = false;
    if (!initialized) {
        av_log_set_level(AV_LOG_WARNING);
        initialized = true;
    }
}

double video_texture_get_time(const VideoTexture* vt)
{
    return vt ? vt->current_time : 0.0;
}

void video_texture_seek(VideoTexture* vt, double seconds)
{
    if (!vt) return;
    vt->current_time = fmax(0.0, fmin(seconds, vt->duration));
    int64_t target = (int64_t)(vt->current_time * vt->fmt_ctx->streams[vt->video_stream_idx]->time_base.den /
                               vt->fmt_ctx->streams[vt->video_stream_idx]->time_base.num);
    av_seek_frame(vt->fmt_ctx, vt->video_stream_idx, target, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(vt->dec_ctx);
}

bool video_texture_open(VideoTexture* vt, const char* filename, bool loop)
{
    init_ffmpeg();
    if (!vt) return false;

    // Should I zero out vt here?

    AVCodecParameters* codecpar;
    AVCodec* codec;

    vt->looping = loop;
    vt->playing = true;

    if (avformat_open_input(&vt->fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open video: %s\n", filename);
        goto error;
    }
    if (avformat_find_stream_info(vt->fmt_ctx, NULL) < 0) goto error;

    // Find video stream
    vt->video_stream_idx = -1;
    for (int i = 0; i < vt->fmt_ctx->nb_streams; i++) {
        if (vt->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vt->video_stream_idx = i;
            break;
        }
    }
    if (vt->video_stream_idx == -1) goto error;

    codecpar = vt->fmt_ctx->streams[vt->video_stream_idx]->codecpar;
    codec = (AVCodec*)avcodec_find_decoder(codecpar->codec_id);
    if (!codec) goto error;

    vt->dec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(vt->dec_ctx, codecpar);
    if (avcodec_open2(vt->dec_ctx, codec, NULL) < 0) goto error;

    vt->width = codecpar->width;
    vt->height = codecpar->height;
    vt->fps = av_q2d(vt->fmt_ctx->streams[vt->video_stream_idx]->avg_frame_rate);
    if (vt->fps <= 0) vt->fps = 30.0;

    // Duration in seconds
    if (vt->fmt_ctx->duration != AV_NOPTS_VALUE)
        vt->duration = vt->fmt_ctx->duration / (double)AV_TIME_BASE;

    vt->packet = av_packet_alloc();
    vt->frame = av_frame_alloc();
    vt->rgb_frame = av_frame_alloc();

    // Allocate RGBA buffer for PGL
    vt->rgba_buffer = (uint8_t*)malloc(vt->width * vt->height * 4);
    if (!vt->rgba_buffer) goto error;

    av_image_fill_arrays(vt->rgb_frame->data, vt->rgb_frame->linesize,
                        vt->rgba_buffer, AV_PIX_FMT_RGBA,
                        vt->width, vt->height, 1);

    // Setup scaler
    vt->sws_ctx = sws_getContext(
        vt->width, vt->height, vt->dec_ctx->pix_fmt,
        vt->width, vt->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!vt->sws_ctx) goto error;

    // Decode first frame
    vt->last_frame_time = -1.0/vt->fps; // force fetch of first frame
    video_texture_update(vt, 0.0);
    return vt;

error:
    video_texture_free(vt);
    return false;
}

void video_texture_update(VideoTexture* vt, double delta_time)
{
    if (!vt || !vt->playing) return;

    vt->current_time += delta_time;

    // Handle end of video / looping
    if (vt->duration > 0 && vt->current_time >= vt->duration) {
        if (vt->looping) {
            vt->current_time = fmod(vt->current_time, vt->duration);
            av_seek_frame(vt->fmt_ctx, vt->video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(vt->dec_ctx);
        } else {
            vt->playing = false;
            return;
        }
    }

    // === Early out: only decode if enough time has passed for a new frame ===
    const double frame_duration = 1.0 / vt->fps;
    double time_since_last_frame = vt->current_time - vt->last_frame_time;

    if (time_since_last_frame < frame_duration * 0.8) {  // small safety margin
        return;  // No new frame needed yet
    } else {
        ;
    }

    // Update last frame time
    vt->last_frame_time = vt->current_time;

    // === Target timestamp calculation ===
    AVRational time_base = vt->fmt_ctx->streams[vt->video_stream_idx]->time_base;
    int64_t target_pts = (int64_t)(vt->current_time * time_base.den / time_base.num);

    // Seek if we're significantly off (large time jump)
    int64_t current_pts = (vt->frame && vt->frame->pts != AV_NOPTS_VALUE)
                          ? vt->frame->pts : 0;

    if (llabs(target_pts - current_pts) > (int64_t)(frame_duration * 3 * time_base.den / time_base.num)) {
        av_seek_frame(vt->fmt_ctx, vt->video_stream_idx, target_pts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(vt->dec_ctx);
    }

    // Decode until we get a frame that is at or past the target time
    while (1) {
        if (av_read_frame(vt->fmt_ctx, vt->packet) < 0) {
            // EOF
            if (vt->looping) {
                av_seek_frame(vt->fmt_ctx, vt->video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
                continue;
            }
            break;
        }

        if (vt->packet->stream_index != vt->video_stream_idx) {
            av_packet_unref(vt->packet);
            continue;
        }

        if (avcodec_send_packet(vt->dec_ctx, vt->packet) < 0) {
            av_packet_unref(vt->packet);
            continue;
        }

        while (avcodec_receive_frame(vt->dec_ctx, vt->frame) >= 0) {
            if (vt->frame->pts != AV_NOPTS_VALUE) {
                double frame_time = vt->frame->pts * av_q2d(time_base);

                if (frame_time >= vt->current_time - (frame_duration * 0.5)) {
                    // Good frame — convert to RGBA
                    sws_scale(vt->sws_ctx,
                              (const uint8_t* const*)vt->frame->data, vt->frame->linesize, 0, vt->height,
                              vt->rgb_frame->data, vt->rgb_frame->linesize);

                    av_packet_unref(vt->packet);
                    return;
                }
            }
        }

        av_packet_unref(vt->packet);
    }
}

void video_texture_free(VideoTexture* vt)
{
    if (!vt) return;
    if (vt->sws_ctx) sws_freeContext(vt->sws_ctx);
    if (vt->rgb_frame) av_frame_free(&vt->rgb_frame);
    if (vt->frame) av_frame_free(&vt->frame);
    if (vt->packet) av_packet_free(&vt->packet);
    if (vt->dec_ctx) avcodec_free_context(&vt->dec_ctx);
    if (vt->fmt_ctx) avformat_close_input(&vt->fmt_ctx);
    free(vt->rgba_buffer);
}
