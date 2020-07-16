//
//  cp03.m
//  ffmpeg-libav-tutorial
//
//  Created by Li Sheng on 2020/7/16.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import "cp03.h"

@interface cp03()

typedef struct StreamingParams {
    char copy_video;
    char copy_audio;
    char* output_extension;
    char* muxer_opt_key;
    char* muxer_opt_value;
    int video_codec_id;
    int audio_codec_id;
    char* codec_priv_key;
    char* codec_priv_value;
} StreamingParams;

typedef struct StreamingContext {
    AVFormatContext* fmt_ctx;
    AVCodec* video_codec;
    AVCodec* audio_codec;
    AVStream* vstream;
    AVStream* astream;
    AVCodecContext* vc_ctx;
    AVCodecContext* ac_ctx;
    int video_index;
    int audio_index;
    char* filename;
} StreamingContext;

@end

@implementation cp03

-(int) open_media:(const char*) src fmt_ctx:(AVFormatContext**) fmt_ctx
{
    int ret = 0;
    
    ret = avformat_open_input(fmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        return ret;
    }
    
    ret = avformat_find_stream_info(*fmt_ctx, NULL);
    return ret;
}

-(int) prepare_decoder:(StreamingContext*) sc
{
    int ret = 0;
    
    
    
    return ret;
}



-(void) recode:(const char*) src destination:(const char*) dst
{
    /*
     * H264 -> H265
     * Audio -> remuxed (untouched)
     * MP4 - MP4
     */
    
    int ret = 0;
    
    StreamingParams sp = {0};
    sp.copy_audio = 1;
    sp.copy_video = 0;
    sp.video_codec_id = AV_CODEC_ID_H265;
    sp.codec_priv_key = "x265-params";
    sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";
    
    StreamingContext* decoder = (StreamingContext*) calloc(1, sizeof(StreamingContext));
    decoder->filename = (char*)src;
    
    StreamingContext* encoder = (StreamingContext*) calloc(1, sizeof(StreamingContext));
    encoder->filename = (char*)dst;
    
    if (sp.output_extension) {
        strcat(encoder->filename, sp.output_extension);
    }
    
    ret = [self open_media:decoder->filename fmt_ctx:&decoder->fmt_ctx];
    if (ret < 0) {
        goto end;
    }
    
    avformat_alloc_output_context2(&encoder->fmt_ctx, NULL, NULL, encoder->filename);
    if (!encoder->fmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc output context.\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    
    if (!sp.copy_video) {
        
    } else {
        
    }
    
    
end:
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    
    if (decoder) {
        free(decoder); decoder = NULL;
    }
}

@end
