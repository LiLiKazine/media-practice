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

AVFormatContext *context = NULL;
AVDictionary *options = NULL;

void registerDevices() {
    avdevice_register_all();
        
    AVInputFormat *format =  av_find_input_format("avfoundation");
    //[[video device]:[audio device]]
    char *devicename = ":0";
    int ret = avformat_open_input(&context,
                                  devicename,
                                  format,
                                  &options);
    if (ret < 0) {
        char errbuf[1024];
        av_strerror(ret, errbuf, 1024);
        printf(stderr, "Fialed to open audio device, [%d]%s\n", ret, errbuf);
        return;
    }
}
