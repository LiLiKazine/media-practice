//
//  testc.c
//  MacRecorder
//
//  Created by LiLi Kazine on 2020/5/16.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "testc.h"

static int rec_status = 0;

void foo() {
    printf("a foo test.\n");
    av_log_set_level(AV_LOG_DEBUG);
    av_log(NULL, AV_LOG_DEBUG, "ffmpeg lib test.\n");
}

SwrContext* init_swr() {
    SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 44100, 0, NULL);
    if (!swr_ctx) {
        //TODO: NULL!
    }
    
    if (swr_init(swr_ctx) < 0) {
        //TODO: ERROR!
    }
    return swr_ctx;
}

AVCodecContext* open_encoder() {
    //open codec
//    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!codec) {
        return NULL;
    }
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        return NULL;
    }
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
//    codec_ctx->sample_fmt = codec->sample_fmts[0];
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    //    codec_ctx->channels = 2;
    codec_ctx->sample_rate = 44100;
    codec_ctx->bit_rate = 0; // profile 指定
    codec_ctx->profile = FF_PROFILE_AAC_HE_V2;
    
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        //TODO: FAILED!
        return NULL;
    }
    return codec_ctx;
}

void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE * output) {
    int ret = 0;
    
    ret = avcodec_send_frame(ctx, frame);
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            //no packet avaliable
            return;
        } else if (ret < 0) {
            printf("%s\n", av_err2str(ret));
            exit(-1);
        }
    }
    //write file
    fwrite(pkt->data, 1, pkt->size, output);
//  fflush(outfile);
    
    return;
}

void stop_rec() {
    rec_status = 0;
}

void rec_audio() {
    int ret = 0;
    char errors[1024] = {0,};
    
    //ctx
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *option = NULL;
    
    //pkt
    AVPacket pkt;
    int count = 0;
    
    //[[video device]:[audio device]]
    char *devicename = ":0";
    
    //set_log_level
    av_log_set_level(AV_LOG_DEBUG);
    
    avdevice_register_all();
    
    //get format
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    //open device
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &option)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return;
    }
    
    //create file
    char *outPath = "/Users/lisheng/Desktop/audio.aac";
    FILE *outfile = fopen(outPath, "wb+");
    
    AVCodecContext *codec_ctx = open_encoder();
    
    //frame
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        //TODO:Failure
    }
    frame->nb_samples = 4096/4/2;
    frame->format = AV_SAMPLE_FMT_S16;
    frame->channel_layout = AV_CH_LAYOUT_STEREO;
    av_frame_get_buffer(frame, 0); //512 * 2 * 2 = 2048
    
    if (!frame->buf[0]) {
        //TODO:Failure
    }
    
    AVPacket *newpkt = av_packet_alloc();
    if (!newpkt) {
        //TODO:Failure
    }
    
    //resample
    SwrContext *swr_ctx = init_swr();
    
    uint8_t **src_data = NULL;
    int src_linesize = 0;
    av_samples_alloc_array_and_samples(&src_data, &src_linesize, 2, 4096/4/2, AV_SAMPLE_FMT_FLT, 0);
    
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, 2, 4096/4/2, AV_SAMPLE_FMT_S16, 0);
    
    //read data from device
    rec_status = 1;
    while (rec_status) {
        if ((ret = av_read_frame(fmt_ctx, &pkt)) != 0) {
            printf("%s\n", av_err2str(ret));
            if (ret == -35) {
                sleep(1);
            }
            continue;
        }
        av_log(NULL, AV_LOG_DEBUG,
               "packet size is %d(%p), count=%d \n",
               pkt.size, pkt.data, count);
        
        
        memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);
        
        //resample
        swr_convert(swr_ctx, dst_data, 512, (const uint8_t **)src_data, 512);
        
        memcpy((void *)frame->data[0], dst_data[0], dst_linesize);
        
        encode(codec_ctx, frame, newpkt, outfile);
        
        count++;
        av_packet_unref(&pkt); //release pkt
    }
    encode(codec_ctx, NULL, newpkt, outfile);
    //close file
    fclose(outfile);
    
    if (src_data) {
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);
    if (dst_data) {
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
    
    swr_free(&swr_ctx);
    
    av_frame_free(&frame);
    
    av_packet_free(&newpkt);
    
    //close device and release ctx
    avformat_close_input(&fmt_ctx);
    
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}
