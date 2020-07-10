//
//  Thumbnails.m
//  Imitator
//
//  Created by Li Sheng on 2020/7/9.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import "Thumbnails.h"

@interface OCFrame : NSObject {
    
@public
    AVFrame* frame;
    
}

@end
@implementation OCFrame

@end


@implementation Thumbnails

NSURL* saveUrl;

- (instancetype)init
{
    self = [super init];
    if (self) {
        
    }
    return self;
}

- (AVFrame*)encodeRGB:(AVFrame* )frame sws: (struct SwsContext*) sws_ctx
{
    AVFrame* oframe = NULL;
    
    oframe = av_frame_alloc();
    
    if (!oframe) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc packet or frame.\n");
        return NULL;
    }
    
    oframe->width = frame->width;
    oframe->height = frame->height;
    oframe->format = AV_PIX_FMT_RGBA;
    
    int buf_size = av_image_get_buffer_size(oframe->format,
                                            oframe->width,
                                            oframe->height,
                                            1);
    uint8_t* img_buf = av_malloc(buf_size);
    av_image_fill_arrays(&oframe->data[0],
                         oframe->linesize,
                         img_buf,
                         frame->format,
                         frame->width,
                         frame->height,
                         1);
    sws_scale(sws_ctx,
              (const uint8_t *const *)frame->data,
              frame->linesize,
              0,
              frame->height,
              oframe->data,
              oframe->linesize);
    if (img_buf) {
        av_free(img_buf);
    }
  
    return oframe;
    
}

- (void)savevJPG:(AVFrame*) frame context:(AVCodecContext*) decode_ctx
{
    NSString* filename = [NSString stringWithFormat:@"/Users/lisheng/Movies/%lld_file.jpg", frame->pkt_pos];
    int ret = 0;
    AVCodec* encoder = NULL;
    AVCodecContext* encode_ctx = NULL;
    struct SwsContext *sws_ctx = NULL;
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc packet.\n");
        return;
    }

    AVFrame* newFrame = av_frame_alloc();
    if (!newFrame) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc frame.\n");
        return;
    }

    newFrame->width = 64;
    newFrame->height = 48;
    newFrame->format = frame->format;
   
    sws_ctx = sws_getContext(frame->width,
                             frame->height,
                             frame->format,
                             newFrame->width,
                             newFrame->width,
                             newFrame->format,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);
    if (!sws_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc SwsContext.\n");
        goto fail;
    }
   
    ret = av_image_alloc(newFrame->data, newFrame->linesize,
                         newFrame->width, newFrame->height, newFrame->format, 1);
    if (ret < 0) {
        goto fail;
    }
    

    ret = sws_scale(sws_ctx,
              (const uint8_t *const *)frame->data,
              frame->linesize,
              0,
              frame->height,
              newFrame->data,
              newFrame->linesize);
    
    encoder = avcodec_find_encoder(AV_CODEC_ID_JPEG2000);
    if (!encoder) {
        av_log(NULL, AV_LOG_ERROR, "Can't find encoder for jpeg.\n");
        goto fail;
    }
    
    encode_ctx = avcodec_alloc_context3(encoder);
    if (!encode_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't find encoder context.\n");
        goto fail;
    }
    
    encode_ctx->pix_fmt = decode_ctx->pix_fmt;
    encode_ctx->height = newFrame->height;
    encode_ctx->width = newFrame->width;
    encode_ctx->time_base = decode_ctx->time_base;
    ret = avcodec_open2(encode_ctx, encoder, NULL);
    if (ret < 0) {
        goto fail;
    }

    ret = avcodec_send_frame(encode_ctx, newFrame);
    if (ret < 0) {
        goto fail;
    }
    FILE* file = fopen([filename UTF8String], "wb");
    if (!file) {
        av_log(NULL, AV_LOG_ERROR, "Can't open file.\n");
        goto fail;
    }
    while ((ret = avcodec_receive_packet(encode_ctx, packet)) == 0) {
        fwrite(packet->data, 1, packet->size, file);
    }
    fclose(file);
fail:

    av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    if (packet) {
        av_packet_free(&packet);
    }
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }
    if (encode_ctx) {
        avcodec_free_context(&encode_ctx);
    }
    if (newFrame) {
        av_frame_free(&newFrame);
    }
}

- (void)openH264:(NSString *)src
{
    int ret = 0;
    AVFormatContext* fmt_ctx = NULL;
    
    AVCodec* decoder = NULL;
    AVCodecContext* decode_ctx = NULL;
    AVFrame* frame = NULL;
    
  
    
    ret = avformat_open_input(&fmt_ctx, [src UTF8String], NULL, NULL);
    if (ret < 0) {
        goto fail;
    }
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        goto fail;
    }
    decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!decoder) {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder for h264.\n");
        goto fail;
    }
    
    decode_ctx = avcodec_alloc_context3(decoder);
    if (!decode_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc decoder context.\n");
        goto fail;
    }
    
    ret = avcodec_open2(decode_ctx, decoder, NULL);
    if (ret < 0) {
        goto fail;
    }
    
   

    AVPacket packet;
    av_init_packet(&packet);
    frame = av_frame_alloc();
    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc frame.\n");
        goto fail;
    }
    while ((ret = av_read_frame(fmt_ctx, &packet)) == 0) {
        if (ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            goto fail;
        }
        ret = avcodec_send_packet(decode_ctx, &packet);
        while (ret == 0) {
            ret = avcodec_receive_frame(decode_ctx, frame);
            if (ret != 0 || frame->key_frame == 0) {
                continue;
            }
            
            [self savevJPG:frame context:decode_ctx];
            
            av_packet_unref(&packet);
        }
    }
    
    
fail:
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (decode_ctx) {
        avcodec_free_context(&decode_ctx);
    }
    
    if (decode_ctx) {
        avcodec_free_context(&decode_ctx);
    }
    
    if (frame) {
        av_frame_free(&frame);
    }
}

- (void)saveThumbnails:(NSString*)src
{
    [self openH264:src];
}

//- (NSArray<NSImage*> *)getThumbnails:(NSString *)src
//{
//    NSMutableArray<NSImage*>* resArr = [[NSMutableArray alloc] init];
//    NSArray<OCFrame*>* frames = [self openH264:src];
//    for (OCFrame* frame in frames) {
//        AVFrame* av = frame->frame;
//        CGBitmapInfo bitmapInfo = kCGImageAlphaPremultipliedLast;
//        CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
//                                                     av->data[0],
//                                                     av->linesize[0]*av->height,
//                                                     kCFAllocatorNull);
//        CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
//        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
//        CGImageRef cgImage = CGImageCreate(av->width,
//                                           av->height,
//                                           8,
//                                           32,
//                                           av->linesize[0],
//                                           colorSpace,
//                                           bitmapInfo,
//                                           provider,
//                                           NULL,
//                                           NO,
//                                           kCGRenderingIntentDefault);
//        CGColorSpaceRelease(colorSpace);
//        NSSize size = NSMakeSize(av->width, av->height);
//        NSImage* image = [[NSImage alloc] initWithCGImage:cgImage size:size];
//        CGImageRelease(cgImage);
//        CGDataProviderRelease(provider);
//        CFRelease(data);
//        [resArr addObject:image];
//    }
//
//    return resArr;
//}


@end
