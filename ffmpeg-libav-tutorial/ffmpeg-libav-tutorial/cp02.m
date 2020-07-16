//
//  cp02.m
//  ffmpeg-libav-tutorial
//
//  Created by Li Sheng on 2020/7/16.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import "cp02.h"

@implementation cp02

-(void) reformat: (const char*) src destination: (const char*) dst
{
    int ret = 0;
    
    AVPacket packet;
    
    AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;
    
    int* streams_list = NULL;
    
    ret = avformat_open_input(&in_fmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        goto end;
    }
    
    ret = avformat_find_stream_info(in_fmt_ctx, NULL);
    if (ret < 0) {
        goto end;
    }
    
    ret = avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, dst);
    if (ret < 0) {
        goto end;
    }
    
    streams_list = av_mallocz_array(in_fmt_ctx->nb_streams, sizeof(streams_list));
    int stream_index = 0;
    for (int i = 0; i < in_fmt_ctx->nb_streams; i++) {
        AVStream* ostream = NULL,
        *istream = in_fmt_ctx->streams[i];
        AVCodecParameters* icp = istream->codecpar;
        if (icp->codec_type != AVMEDIA_TYPE_AUDIO &&
            icp->codec_type != AVMEDIA_TYPE_VIDEO &&
            icp->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            streams_list[i] = -1;
            continue;
        }
        streams_list[i] = stream_index++;
        ostream = avformat_new_stream(out_fmt_ctx, NULL);
        if (!ostream) {
            av_log(NULL, AV_LOG_ERROR, "Can't alloc output stream.\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        ret = avcodec_parameters_copy(ostream->codecpar, icp);
        if (ret < 0) {
            goto end;
        }
    }
    
    av_dump_format(out_fmt_ctx, 0, dst, 1);
    
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_fmt_ctx->pb, dst, AVIO_FLAG_WRITE);
        if (ret < 0) {
            goto end;
        }
    }
    
    AVDictionary* opts = NULL;
    
    if (/* DISABLES CODE */ (false)) { // fragmented mp4
        av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    }
    
    ret = avformat_write_header(out_fmt_ctx, &opts);
    if (ret < 0) {
        goto end;
    }
    
    while (ret == 0) {
        AVStream *istrem = NULL, *ostream = NULL;
        ret = av_read_frame(in_fmt_ctx, &packet);
        if (ret < 0) {
            goto end;
        }
        if (streams_list[packet.stream_index] == -1) {
            av_packet_unref(&packet);
            continue;
        }
        
        istrem = in_fmt_ctx->streams[packet.stream_index];
        ostream = out_fmt_ctx->streams[streams_list[packet.stream_index]];
        
        packet.stream_index = streams_list[packet.stream_index];
        packet.pts = av_rescale_q_rnd(packet.pts, istrem->time_base, ostream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet.dts = av_rescale_q_rnd(packet.dts, istrem->time_base, ostream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet.duration = av_rescale_q(packet.duration, istrem->time_base, ostream->time_base);
        packet.pos = -1;
        
        ret = av_interleaved_write_frame(out_fmt_ctx, &packet);
        av_packet_unref(&packet);
        if (ret < 0) {
            goto end;
        }
    }
    
    av_write_trailer(out_fmt_ctx);
    
end:
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    
    if (in_fmt_ctx) {
        avformat_close_input(&in_fmt_ctx);
    }
    
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb);
        avformat_free_context(out_fmt_ctx);
    }
    
    if (streams_list) {
        av_freep(&streams_list);
    }
    
}

@end
