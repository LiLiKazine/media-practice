//
//  testc.c
//  MacRecorder
//
//  Created by LiLi Kazine on 2020/5/16.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "testc.h"

void foo() {
    printf("a foo test.\n");
    av_log_set_level(AV_LOG_DEBUG);
    av_log(NULL, AV_LOG_DEBUG, "ffmpeg lib test.\n");
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
    
    //read data from device
    while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0 &&
           count++ < 500) {
        av_log(NULL, AV_LOG_DEBUG,
               "packet size is %d(%p), count=%d \n",
               pkt.size, pkt.data, count);
        av_packet_unref(&pkt); //release pkt
    }
    
    //close device and release ctx
    avformat_close_input(&fmt_ctx);
    
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}
