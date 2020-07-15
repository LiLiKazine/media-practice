//
//  Extract.m
//  Imitator
//
//  Created by Li Sheng on 2020/7/10.
//  Copyright © 2020 lilikazine. All rights reserved.
//
#import "Extract.h"

@interface OCFrame : NSObject {
    
@public
    AVFrame* frame;
    
}
@end

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

-(void) closeInput
{
    if (infmt_ctx) {
        avformat_close_input(&infmt_ctx);
    }
}

-(NSArray<OCFrame*>*) extractFrame: (AVPacket*) inPacket codecID: (enum AVCodecID*) codecID codecContext: (AVCodecContext*) codecContext
{
    NSMutableArray<OCFrame*>* frames = [[NSMutableArray alloc] init];
    
    int ret = 0;
    
    AVCodec* decoder = NULL;
    AVCodecContext* decode_ctx = NULL;
    
    if (codecContext) {
        
        decode_ctx = codecContext;
        decoder = (AVCodec*)codecContext->codec;
        
    } else if (codecID) {
        
        decoder = avcodec_find_decoder(*codecID);
        if (!decoder) {
            av_log(NULL, AV_LOG_ERROR, "Can't find decoder for h264.\n");
            goto end;
        }
        
        decode_ctx = avcodec_alloc_context3(decoder);
        if (!decode_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Can't alloc decoder context.\n");
            goto end;
        }
        
        ret= avcodec_open2(decode_ctx, decoder, NULL);
        if (ret < 0) {
            goto end;
        }
        
    } else {
        av_log(NULL, AV_LOG_ERROR, "No codec information.\n");
        goto end;
    }
    
    ret = avcodec_send_packet(decode_ctx, inPacket);
    if (ret < 0) {
        goto end;
    }

    while (ret == 0) {
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            av_log(NULL, AV_LOG_ERROR, "Can't alloc frame.\n");
            goto end;
        }
        ret = avcodec_receive_frame(decode_ctx, frame);
        if (ret != 0 || frame->key_frame == 0) {
            continue;
        }
        OCFrame* ocFrame = [[OCFrame alloc] init];
        ocFrame->frame = frame;
        [frames addObject: ocFrame];
    }
    
end:
    
    if (ret < 0 && ret != -35) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    
    return frames;
}

-(void) rescaleTime: (AVPacket*) packet dts_start: (int64_t*) dts_start pts_start: (int64_t*) pts_start
       time_base_in: (AVRational*) time_base_in time_base_out: (AVRational*) time_base_out
{
    if (*dts_start < 0) {
        *dts_start = packet->dts;
    }
    if (*pts_start < 0) {
        *pts_start = packet->pts;
    }
    
    packet->pts = av_rescale_q_rnd(packet->pts-*pts_start, *time_base_in, *time_base_out, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    packet->dts = av_rescale_q_rnd(packet->dts-*dts_start, *time_base_in, *time_base_out, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    if (packet->pts < 0) {
        packet->pts = 0;
    }
    
    if (packet->dts < 0) {
        packet->dts = 0;
    }
    
    if (packet->pts < packet->dts) {
        packet->pts = packet->dts;
    }
    
    packet->duration = av_rescale_q(packet->duration, *time_base_in, *time_base_out);
    packet->pos = -1;
}

-(void) extractH264: (NSString*) dst begin: (int64_t) begin end: (int64_t) end
{
    if (!infmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Did not open input file.\n");
        return;
    }
    
    int stream_index;
    AVStream* istream = NULL;
    AVCodec* decoder = NULL;
    AVCodecContext* decode_ctx = NULL;
    
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
    
    decode_ctx = avcodec_alloc_context3(decoder);
    if (!decode_ctx) {
         av_log(NULL, AV_LOG_ERROR, "Can't alloc decoder context.\n");
         goto end;
     }
    
    ret = avcodec_open2(decode_ctx, decoder, NULL);
    if (ret < 0) {
        goto end;
    }
    
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
    
    BOOL seek = begin > 0 && end > 0;
    
    if (seek) {
        int64_t begin_time = begin / av_q2d(istream->time_base);
        
        ret = avformat_seek_file(infmt_ctx, stream_index, INT64_MIN, begin_time, INT64_MAX, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            goto end;
        }
        
    }
    int64_t dts_start = -1;
    int64_t pts_start = -1;
    
    AVPacket ipkt;
    
    while ((ret = av_read_frame(infmt_ctx, &ipkt)) == 0) {
        if (ipkt.stream_index != stream_index) {
            continue;
        }
        
        if (seek && ipkt.pts * av_q2d(istream->time_base) > end) {
            av_packet_unref(&ipkt);
            break;
        }
        
        ret = av_bsf_send_packet(bsf_ctx, &ipkt);
        if (ret < 0) {
            goto end;
        }
        av_packet_unref(&ipkt);
        while ((ret = av_bsf_receive_packet(bsf_ctx, &ipkt)) == 0) {
            
            if (seek) {
                [self rescaleTime:&ipkt dts_start:&dts_start pts_start:&pts_start time_base_in:&istream->time_base time_base_out:&ostream->time_base];
            }
            
//            NSArray<OCFrame*>* frames = [self extractFrame:&ipkt codecID:NULL codecContext:decode_ctx];
//
//            for (OCFrame* ocf in frames) {
//                av_frame_free(&ocf->frame);
//            }
            
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
        if (seek) {
            [self rescaleTime:&ipkt dts_start:&dts_start pts_start:&pts_start time_base_in:&istream->time_base time_base_out:&ostream->time_base];
        }
        ret = av_interleaved_write_frame(outfmt_ctx, &ipkt);
        av_packet_unref(&ipkt);
    }
    av_write_trailer(outfmt_ctx);
    
end:
    if (ret < 0 && ret != -35) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    if (outfmt_ctx) {
        avio_closep(&outfmt_ctx->pb);
        avformat_close_input(&outfmt_ctx);
    }
    if (bsf_ctx) {
        av_bsf_free(&bsf_ctx);
    }
    if (decode_ctx) {
        avcodec_free_context(&decode_ctx);
    }
    
}

@end
