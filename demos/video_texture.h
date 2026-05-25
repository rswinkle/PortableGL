#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

typedef struct VideoTexture {
    AVFormatContext* fmt_ctx;
    AVCodecContext*  dec_ctx;
    AVPacket*        packet;
    AVFrame*         frame;
    AVFrame*         rgb_frame;
    struct SwsContext* sws_ctx;

    int video_stream_idx;
    uint8_t*         rgba_buffer;  // PGL texture data (width * height * 4)
    int              width, height;
    double           fps;
    double           duration;
    bool             looping;
    bool             playing;
    double           last_frame_time;
    double           current_time; // in seconds
} VideoTexture;

// API
bool          video_texture_open(VideoTexture* vt, const char* filename, bool loop);
void          video_texture_update(VideoTexture* vt, double delta_time); // advance + decode if needed
void          video_texture_bind(VideoTexture* vt, int unit); // or whatever your texture API is
void          video_texture_free(VideoTexture* vt);
