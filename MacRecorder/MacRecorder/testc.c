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
    char *outPath = "/Users/lisheng/Desktop/audio.pcm";
    FILE *outfile = fopen(outPath, "wb+");
    
    //resample
    SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 44100, 0, NULL);
    
    if (!swr_ctx) {
        //TODO: NULL!
    }
    
    if (swr_init(swr_ctx) < 0) {
        //TODO: ERROR!
    }
    
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
        
        //write file
        fwrite(dst_data[0], dst_linesize, 1, outfile);
//        fflush(outfile);
        count++;
        av_packet_unref(&pkt); //release pkt
    }
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
    av_freep(&swr_ctx);
    
    //close device and release ctx
    avformat_close_input(&fmt_ctx);
    
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}
