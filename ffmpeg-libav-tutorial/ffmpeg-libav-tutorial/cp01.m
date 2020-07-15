//
//  cp01.m
//  ffmpeg-libav-tutorial
//
//  Created by Li Sheng on 2020/7/15.
//  Copyright © 2020 lilikazine. All rights reserved.
//

///0_hello_world.c

#import "cp01.h"

@implementation cp01

-(void) open_input: (const char*) src
{
    int ret = 0;
    
    AVFormatContext* fmt_ctx = NULL;
    AVCodec* decoder = NULL;
    
    AVCodecContext* decode_ctx = NULL;
    
    AVFrame* frame = NULL;
    AVPacket* packet = NULL;
    
    ret = avformat_open_input(&fmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        goto end;
    }
    
    [self logging:"format %s, duration %lld us, bit_rate %lld", fmt_ctx->iformat->name, fmt_ctx->duration, fmt_ctx->bit_rate];
    
    
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        goto end;
    }
    
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        goto end;
    }
    int stream_index = ret;
    
    decode_ctx = avcodec_alloc_context3(decoder);
    if (!decode_ctx) {
        [self logging:"Can't alloc decode context.\n"];
        goto end;
    }
    ret = avcodec_parameters_to_context(decode_ctx, fmt_ctx->streams[stream_index]->codecpar);
    if (ret < 0) {
        goto end;
    }
    
    ret = avcodec_open2(decode_ctx, decoder, NULL);
    if (ret < 0) {
        goto end;
    }
    
    frame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) {
        [self logging:"Can't alloc frame or packet.\n"];
        goto end;
    }
    
    while ((ret = av_read_frame(fmt_ctx, packet)) >= 0) {
        if (packet->stream_index == stream_index) {
            ret = [self decode_packet:packet decode_ctx:decode_ctx frame:frame];
            
            if (ret < 0)
                goto end;
            
            if (_packet_limit != 0 && ++packet_decoded >= _packet_limit)
                break;
        }
        
        av_packet_unref(packet);
    }
    
    
end:
    if (ret < 0) {
        [self logging:"%s", av_err2str(ret)];
    }
    
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (decode_ctx) {
        avcodec_free_context(&decode_ctx);
    }
    
    if (frame) {
        av_frame_free(&frame);
    }
    
    if (packet) {
        av_packet_free(&packet);
    }
}

-(void) save_grey_frame: (unsigned char*) buf wrap: (int) wrap xsize: (int) xsize ysize: (int) ysize filename: (char*) filename
{
    FILE *f;
    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (int i = 0; i<ysize; i++) {
        fwrite(buf + i * wrap, 1, xsize, f);
    }
    fclose(f);
}

-(void) logging:(const char *)fmt, ...
{
    va_list args;
    fprintf(stderr, "Log: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

-(int) decode_packet: (AVPacket*) packet decode_ctx: (AVCodecContext*) decode_ctx frame: (AVFrame*) frame
{
    int ret = 0;
    
    ret = avcodec_send_packet(decode_ctx, packet);
    if (ret < 0) {
        goto end;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(decode_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;
        
        if (ret < 0)
            goto end;
        
        [self logging:"Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
         decode_ctx->frame_number,
         av_get_picture_type_char(frame->pict_type),
         frame->pkt_size,
         frame->pts,
         frame->key_frame,
         frame->coded_picture_number];
        
        char frame_filename[1024];
        snprintf(frame_filename, sizeof(frame_filename), "%s/%s-%d.pgm", [_dst UTF8String], "frame", decode_ctx->frame_number);
        [self save_grey_frame:frame->data[0] wrap:frame->linesize[0] xsize:frame->width ysize:frame->height filename:frame_filename];
    }
    
end:
    return ret;
}

@end
