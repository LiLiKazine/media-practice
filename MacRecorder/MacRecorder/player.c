//
//  player.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/23.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "player.h"


void play_video(const char* src) {
    int ret = -1;
    
    AVFormatContext *fmt_ctx = NULL;
    
    int i, video_stream;
    
    AVCodecContext *src_codec_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    
    struct SwsContext *sws_ctx = NULL;
    
    AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    
    AVPacket packet;
    
//    int frame_finished;
//    float aspect_ratio;
    
    AVFrame *dst_frame = NULL;
    
    SDL_Rect rect;
    Uint32 pixformat;
    
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    
    int w_width = 640;
    int w_height = 480;
    
    ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (ret) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initailize SDL - %s\n", SDL_GetError());
        return;
    }
    
    ret = avformat_open_input(&fmt_ctx, src, NULL, NULL);
    if (ret != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open video file! - %s\n", av_err2str(ret));
        goto __FAIL;
    }
    
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find stream information! - %s\n", av_err2str(ret));
        goto __FAIL;
    }
    
    av_dump_format(fmt_ctx, 0, src, 0);
    
    video_stream = -1;
    
    for (i=0; i<fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    
    if (video_stream == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find a video stream!\n");
        goto __FAIL;
    }
    
    enum AVCodecID id = fmt_ctx->streams[video_stream]->codecpar->codec_id;
    codec = avcodec_find_decoder(id);
    if (!codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported codec!\n");
        goto __FAIL;
    }
    
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to alloc codec context!\n");
        goto __FAIL;
    }
    ret = avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream]->codecpar);
    if (ret < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to copy parameters to codec context! - %s\n", av_err2str(ret));
        goto __FAIL;
    }
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open decoder! - %s\n", av_err2str(ret));
        goto __FAIL;
    }
    
    frame = av_frame_alloc();
    
    w_width = codec_ctx->width;
    w_height = codec_ctx->height;
    
    window = SDL_CreateWindow("Player",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w_width,
                              w_height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window.\n");
        goto __FAIL;
    }
    
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create renderer.\n");
        goto __FAIL;
    }
    
    pixformat = SDL_PIXELFORMAT_IYUV;
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                w_width,
                                w_height);
    
    sws_ctx = sws_getContext(codec_ctx->width,
                             codec_ctx->height,
                             codec_ctx->pix_fmt,
                             codec_ctx->width,
                             codec_ctx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);
    
    
    
    
    dst_frame = av_frame_alloc();
    
    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream) {
            ret = avcodec_send_packet(codec_ctx, &packet);
            if (ret < 0) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to send packet. - %s\n", av_err2str(ret));
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to receive frame. - %s\n", av_err2str(ret));
                    break;
                }
                
                sws_scale(sws_ctx,
                          (const uint8_t *const *)frame->data,
                          frame->linesize,
                          0,
                          codec_ctx->height,
                          dst_frame->data, dst_frame->linesize);
                
                SDL_UpdateYUVTexture(texture,
                                     NULL,
                                     dst_frame->data[0],
                                     dst_frame->linesize[0],
                                     dst_frame->data[1],
                                     dst_frame->linesize[1],
                                     dst_frame->data[2],
                                     dst_frame->linesize[2]);
                rect.x = 0;
                rect.y = 0;
                rect.w = codec_ctx->width;
                rect.h = codec_ctx->height;
                
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_RenderPresent(renderer);
            }
        }
        
        av_packet_unref(&packet);
        
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                goto __QUIT;
                break;
            default:
                break;
        }
    }
    
__QUIT:
    ret = 0;
    
__FAIL:
    
    if (dst_frame) {
        av_frame_free(&dst_frame);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (codec_ctx) {
        avcodec_close(codec_ctx);
    }
    if (src_codec_ctx) {
        avcodec_close(src_codec_ctx);
    }
    
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (window) {
        SDL_DestroyWindow(window);
    }
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    
    SDL_Quit();
}
