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

static SwrContext* init_swr() {
    SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 44100, 0, NULL);
    if (!swr_ctx) {
        printf("swr_alloc_set_opts return null.");
    }
    
    if (swr_init(swr_ctx) < 0) {
        printf("swr_init error.");
    }
    return swr_ctx;
}

static AVCodecContext* open_encoder() {
    //open codec
//    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!codec) {
        return NULL;
    }
    AVCodecContext *c_ctx = avcodec_alloc_context3(codec);
    if (!c_ctx) {
        printf("avcodec_alloc_context3 return null");
        return NULL;
    }
    c_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
//    codec_ctx->sample_fmt = codec->sample_fmts[0];
    c_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    //    codec_ctx->channels = 2;
    c_ctx->sample_rate = 44100;
    c_ctx->bit_rate = 0; // profile 指定
    c_ctx->profile = FF_PROFILE_AAC_HE_V2;
    
    if (avcodec_open2(c_ctx, codec, NULL) < 0) {
        //TODO: FAILED!
        printf("avcodec_open2 failed.");
        return NULL;
    }
    return c_ctx;
}

static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE * output) {
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
        //write file
        fwrite(pkt->data, 1, pkt->size, output);
        fflush(output);
    }
    return;
}

void stop_rec() {
    rec_status = 0;
}

/**
 * @brief open audio device
 * @return succ: AVFormat*, fail: NULL
 */
static AVFormatContext* open_dev() {
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *option = NULL;
    //[[video device]:[audio device]]
    char *devicename = ":0";
    int ret = 0;
    char errors[1024] = {0,};
    //get format
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    //open device
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &option)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return NULL;
    }
    return fmt_ctx;
}

static void alloc_data_4_resample(uint8_t ***src_data,
                           int *src_linesize,
                           uint8_t ***dst_data,
                           int *dst_linesize) {

    av_samples_alloc_array_and_samples(src_data,
                                       src_linesize,
                                       2,
                                       4096/4/2,
                                       AV_SAMPLE_FMT_FLT,
                                       0);
    
    av_samples_alloc_array_and_samples(dst_data,
                                       dst_linesize,
                                       2,
                                       4096/4/2,
                                       AV_SAMPLE_FMT_S16,
                                       0);
}

static AVFrame* create_frame() {
    //frame
    AVFrame *frame = NULL;
    frame = av_frame_alloc();
    if (!frame) {
        //TODO:Failure
        printf("Error, No Memory!\n");
        goto __ERROR;
    }
    // set parameters
    frame->nb_samples = 4096/4/2;
    frame->format = AV_SAMPLE_FMT_S16;
    frame->channel_layout = AV_CH_LAYOUT_STEREO;
    // alloc inner memory
    av_frame_get_buffer(frame, 0); //512 * 2 * 2 = 2048
    if (!frame->data[0]) {
        //TODO:Failure
        printf("Error, Failed to alloc buf in frame!\n");
        goto __ERROR;
    }
    return frame;
__ERROR:
    if (frame) {
        av_frame_free(&frame);
    }
    return NULL;
}

static void free_data_4_resample(uint8_t **src_data, uint8_t **dst_data) {
    if (src_data) {
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);
    if (dst_data) {
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
    
}

static void read_data_and_encode(AVFormatContext *fmt_ctx,
                          SwrContext *swr_ctx,
                          AVCodecContext *c_ctx,
                          FILE *outfile) {
    int errcount = 0;

    int ret = 0;
    //pkt
    AVPacket pkt;
    
    AVPacket *newpkt = NULL;
    
    AVFrame *frame = NULL;
    
    uint8_t **src_data = NULL;
    int src_linesize = 0;
    
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    
    frame = create_frame();
    
    if (!frame) {
        printf("Error, Failed to create frame!\n");
        goto __ERROR;
    }
    
    newpkt = av_packet_alloc();
    if (!newpkt) {
        printf("Error, Failed to alloc buf in frame!\n");
        goto __ERROR;
    }
    
    alloc_data_4_resample(&src_data, &src_linesize, &dst_data, &dst_linesize);
    
    //read data from device
    while (rec_status) {
        
        ret = av_read_frame(fmt_ctx, &pkt);
        if(ret < 0){
            printf("%s\n", av_err2str(ret));
            if (ret == AVERROR(EAGAIN)) {

                //连续5次则退出
                if(5 == errcount++){
                    break;
                }

                //如果设备没有准备好，那就等一小会
                av_usleep(10000);
                continue;
            }

            break;
        }
        
        errcount = 0;
        
        memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);
        
        //resample
        swr_convert(swr_ctx,
                    dst_data,
                    512,
                    (const uint8_t **)src_data,
                    512);
        
        memcpy((void *)frame->data[0], dst_data[0], dst_linesize);
        
        encode(c_ctx, frame, newpkt, outfile);
        
        av_packet_unref(&pkt); //release pkt
    }
    encode(c_ctx, NULL, newpkt, outfile);

__ERROR:
    if (frame) {
        av_frame_free(&frame);
    }
    if (newpkt) {
        av_packet_free(&newpkt);
    }
       
    free_data_4_resample(src_data, dst_data);
}

void rec_audio() {
    
    //ctx
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext  *c_ctx = NULL;
    SwrContext *swr_ctx = NULL;
    
    
    //set_log_level
    av_log_set_level(AV_LOG_DEBUG);
    
    avdevice_register_all();
    
    rec_status = 1;
    
    //create file
    char *outPath = "/Users/lisheng/Desktop/audio.aac";
    FILE *outfile = fopen(outPath, "wb+");
    if (!outfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }
  
    fmt_ctx = open_dev();
    if (!fmt_ctx) {
        printf("Error, Failed to open device!\n");
        goto __ERROR;
    }
    
    c_ctx = open_encoder();
    if (!c_ctx) {
        printf("Error, Failed to open encoder!\n");
        goto __ERROR;
    }
        
    //resample
    swr_ctx = init_swr();
    if (!swr_ctx) {
        printf("Error, Failed to alloc buf in frame!\n");
        goto __ERROR;
    }
    
    read_data_and_encode(fmt_ctx, swr_ctx, c_ctx, outfile);
      
__ERROR:
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }
    if (c_ctx) {
        avcodec_free_context(&c_ctx);
    }
    
    //close device and release ctx
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (outfile) {
        //close file
        fclose(outfile);
    }
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}
