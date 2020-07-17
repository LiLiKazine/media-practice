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

-(int) fill_stream_info:(AVStream*) stream codec:(AVCodec**) codec codec_ctx:(AVCodecContext**) codec_ctx
{
    int ret = 0;
    
    *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!*codec) {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder.\n");
        ret = AVERROR_UNKNOWN;
        return ret;
    }
    
    *codec_ctx = avcodec_alloc_context3(*codec);
    if (!*codec_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc decode context.\n");
        ret = AVERROR_UNKNOWN;
        return ret;
    }
    
    ret = avcodec_parameters_to_context(*codec_ctx, stream->codecpar);
    if (ret < 0) {
        return ret;
    }
    
    ret = avcodec_open2(*codec_ctx, *codec, NULL);
    
    return ret;
}

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
    
    for (int i = 0; i < sc->fmt_ctx->nb_streams; i++) {
        if (sc->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            sc->vstream = sc->fmt_ctx->streams[i];
            sc->video_index = i;
            
            ret = [self fill_stream_info:sc->vstream codec:&sc->video_codec codec_ctx:&sc->vc_ctx];
            if (ret < 0) {
                return ret;
            }
        } else if (sc->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            sc->astream = sc->fmt_ctx->streams[i];
            sc->audio_index = i;
            
            ret = [self fill_stream_info:sc->astream codec:&sc->audio_codec codec_ctx:&sc->ac_ctx];
            if (ret < 0) {
                return ret;
            }
        }
    }
    
    return ret;
}

-(int) prepare_video_encoder:(StreamingContext*) sc decoder_ctx:(AVCodecContext*) decoder_ctx input_framerate:(AVRational) input_framerate streamingParam:(StreamingParams) sp
{
    int ret = 0;
    
    sc->vstream = avformat_new_stream(sc->fmt_ctx, NULL);
    
    sc->video_codec = avcodec_find_encoder(sp.video_codec_id);
    if (!sc->video_codec) {
        ret = AVERROR_UNKNOWN;
        av_log(NULL, AV_LOG_ERROR, "Can't find video encoder.\n");
        return ret;
    }
    
    sc->vc_ctx = avcodec_alloc_context3(sc->video_codec);
    if (!sc->video_codec) {
        ret = AVERROR_UNKNOWN;
        av_log(NULL, AV_LOG_ERROR, "Can't find video encoder context.\n");
        return ret;
    }
    
    av_opt_set(sc->vc_ctx->priv_data, "preset", "fast", 0);
    if (sp.codec_priv_key && sp.codec_priv_value) {
        av_opt_set(sc->vc_ctx, sp.codec_priv_key, sp.codec_priv_value, 0);
    }
    
    sc->vc_ctx->height = decoder_ctx->height;
    sc->vc_ctx->width = decoder_ctx->width;
    sc->vc_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    if (sc->vc_ctx->pix_fmt) {
        sc->vc_ctx->pix_fmt = sc->video_codec->pix_fmts[0];
    } else {
        sc->vc_ctx->pix_fmt = decoder_ctx->pix_fmt;
    }
    
    sc->vc_ctx->bit_rate = 2 * 1000 * 1000;
    sc->vc_ctx->rc_buffer_size = 4 * 1000 * 1000;
    sc->vc_ctx->rc_max_rate = 2 * 1000 * 1000;
    sc->vc_ctx->rc_min_rate = 2.5 * 1000 * 1000;
    
    sc->vc_ctx->time_base = av_inv_q(input_framerate);
    sc->vstream->time_base = sc->vc_ctx->time_base;
    
    ret = avcodec_open2(sc->vc_ctx, sc->video_codec, NULL);
    if (ret < 0) {
        return ret;
    }
    
    ret = avcodec_parameters_from_context(sc->vstream->codecpar, sc->vc_ctx);
    
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
