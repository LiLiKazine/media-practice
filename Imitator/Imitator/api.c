//
//  api.c
//  Imitator
//
//  Created by Li Sheng on 2020/6/29.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#include "api.h"

void dump(AVFormatContext* fmt_ctx,
          int index,
          const char* url,
          int is_output) {
    int ret = 0;
    if (!fmt_ctx) {
        if ((ret = avformat_open_input(&fmt_ctx, url, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(ret));
            return;
        }
        ret = 1;
    }
    av_dump_format(fmt_ctx, index, url, is_output);
    if (ret > 0) {
        avformat_close_input(&fmt_ctx);
    }
}

void media_legth(AVFormatContext* fmt_ctx,
                 const char* url,
                 struct AudioInfo** audio_info,
                 struct VideoInfo** video_info) {
    int ret = 0;
    if (!fmt_ctx) {
        if ((ret = avformat_open_input(&fmt_ctx, url, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(ret));
            return;
        }
        ret = 1;
    }
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            *audio_info = (struct AudioInfo*)malloc(sizeof(struct AudioInfo));
            (*audio_info)->metadata = stream->metadata;
        }
        
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            *video_info = (struct VideoInfo*)malloc(sizeof(struct VideoInfo));
            (*video_info)->duration = stream->duration * av_q2d(stream->time_base);
            (*video_info)->metadata = stream->metadata;
        }
    }
    if (ret > 0) {
        avformat_close_input(&fmt_ctx);
    }
}

void cut_video(const char* src,
               const char* dst,
               int64_t begin,
               int64_t end) {
    int ret = 0;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;

    AVCodec* decoder = NULL;
    
    ret = avformat_open_input(&ifmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        
    }
    
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, dst);
    if (ret < 0) {
        
    }
    
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* istream = ifmt_ctx->streams[i];
        AVCodec* decoder = avcodec_find_decoder(istream->codecpar->codec_id);
        AVStream* ostream = avformat_new_stream(ofmt_ctx, decoder);
        avcodec_parameters_copy(ostream->codecpar, istream->codecpar);
//        ostream->codecpar->codec_tag = 0;
        
    }
//
    
    int i_vstream = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
//
//    AVStream* istream = ifmt_ctx->streams[i_vstream];
//    AVStream* ostream = avformat_new_stream(ofmt_ctx, decoder);
//
//    avcodec_parameters_copy(ostream->codecpar, istream->codecpar);
//
    av_dump_format(ofmt_ctx, 0, dst, 1);

    
    ret = avio_open(&ofmt_ctx->pb, dst, AVIO_FLAG_WRITE);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(ret));
        return;
    }
    
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        
    }
    int64_t begin_time = begin / av_q2d(ifmt_ctx->streams[i_vstream]->time_base);
    
    av_log(NULL, AV_LOG_INFO, "begin time: %lld", begin_time);
    
    ret = avformat_seek_file(ifmt_ctx, i_vstream, INT64_MIN, begin_time, INT64_MAX, AVSEEK_FLAG_FRAME);
    if (ret < 0) {

    }
    
    while (1) {

    }
    
//
//    AVPacket ipkt, opkt;
//    av_init_packet(&ipkt);
//    while (av_read_frame(ifmt_ctx, &ipkt)) {
//        if (ipkt.pts * av_q2d()) {
//            <#statements#>
//        }
//        if (ipkt.stream_index != i_vstream) {
//            continue;
//        }
//        av_write_frame(ofmt_ctx, &ipkt);
//    }
//    av_write_trailer(ofmt_ctx);
    
}
