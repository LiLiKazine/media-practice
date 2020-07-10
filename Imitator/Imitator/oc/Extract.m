//
//  Extract.m
//  Imitator
//
//  Created by Li Sheng on 2020/7/10.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import "Extract.h"

//@interface Extract()
//
//@property(nullable) AVFormatContext* infmt_ctx;
//@property(nullable) NSString* src;
//
//@end

@implementation Extract

AVFormatContext* infmt_ctx;
NSString* src;
int ret = 0;

-(int) openInput:(NSString*) path
{
    
    src = path;
    ret = avformat_open_input(&infmt_ctx, [src UTF8String], NULL, NULL);
    if (ret < 0) {
        goto end;
    }
    
end:
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    return ret;
}

-(void) extractH264: (NSString*) dst
{
    if (!infmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Did not open input file.\n");
        return;
    }
    
    int stream_index;
    AVStream* istream = NULL;
    AVCodec* decoder = NULL;
    
    AVFormatContext* outfmt_ctx = NULL;
    AVStream* ostream = NULL;
    
    AVBSFContext* bsf_ctx = NULL;
    
    
    ret = avformat_find_stream_info(infmt_ctx, NULL);
    if (ret < 0) {
        goto end;
    }
    
    ret = av_find_best_stream(infmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        goto end;
    }
    stream_index = ret;
    istream = infmt_ctx->streams[stream_index];
    
    ret = avformat_alloc_output_context2(&outfmt_ctx, NULL, NULL, [dst UTF8String]);
    if (ret < 0) {
        goto end;
    }
    
    ostream = avformat_new_stream(outfmt_ctx, decoder);
    if (!ostream) {
        av_log(NULL, AV_LOG_ERROR, "Can't create new outout stream.\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ret = avcodec_parameters_copy(ostream->codecpar, istream->codecpar);
    if (ret < 0) {
        goto end;
    }
    ostream->codecpar->codec_tag = 0;
    
    ret = avio_open(&outfmt_ctx->pb, [dst UTF8String], AVIO_FLAG_WRITE);
    if (ret < 0) {
        goto end;
    }
    
    ret = avformat_write_header(outfmt_ctx, NULL);
    if (ret < 0) {
        goto end;
    }
    const AVBitStreamFilter* bsf_filter = av_bsf_get_by_name("h264_mp4toannexb");
    if (!bsf_filter) {
        av_log(NULL, AV_LOG_ERROR, "Can't get filter h264_mp4toannexb.\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    
    ret = av_bsf_alloc(bsf_filter, &bsf_ctx);
    if (ret < 0) {
        goto end;
    }
    
    ret = avcodec_parameters_copy(bsf_ctx->par_in, istream->codecpar);
    if (ret < 0) {
        goto end;
    }
    
    bsf_ctx->time_base_in = istream->time_base;
    
    ret = av_bsf_init(bsf_ctx);
    if (ret < 0) {
        goto end;
    }
    
    AVPacket ipkt;
    
    while ((ret = av_read_frame(infmt_ctx, &ipkt)) == 0) {
        if (ipkt.stream_index != stream_index) {
            continue;
        }
        
        ret = av_bsf_send_packet(bsf_ctx, &ipkt);
        if (ret < 0) {
            goto end;
        }
        av_packet_unref(&ipkt);
        while ((ret = av_bsf_receive_packet(bsf_ctx, &ipkt)) == 0) {
            ret = av_interleaved_write_frame(outfmt_ctx, &ipkt);
            av_packet_unref(&ipkt);
        }
        
        if (ret == AVERROR(EAGAIN)) {
            av_packet_unref(&ipkt);
            continue;
        }
        
        if (ret == AVERROR_EOF) {
            break;
        }
        
        if (ret < 0) {
            goto end;
        }
    }
    ret = av_bsf_send_packet(bsf_ctx, NULL);
    while ((ret = av_bsf_receive_packet(bsf_ctx, &ipkt)) == 0) {
        ret = av_interleaved_write_frame(outfmt_ctx, &ipkt);
        av_packet_unref(&ipkt);
    }
    av_write_trailer(outfmt_ctx);
    
end:
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    if (outfmt_ctx) {
        avio_closep(&outfmt_ctx->pb);
        avformat_close_input(&outfmt_ctx);
    }
    if (bsf_ctx) {
        av_bsf_free(&bsf_ctx);
    }
    
}

@end
