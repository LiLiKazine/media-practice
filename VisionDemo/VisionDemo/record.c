//
//  record.c
//  VisionDemo
//
//  Created by Li Sheng on 2020/7/23.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#include "record.h"

void open_input(int width, int height)
{
    
    avdevice_register_all();
    
    int ret = 0;
    AVFormatContext* fmt_ctx = NULL;
    AVDictionary* options = NULL;
    
    char* devicename = "0";
    
    AVInputFormat* ifmt = av_find_input_format("avfoundation");
    
    char video_size[10];
    sprintf(video_size, "%dx%d", width, height);
    
    av_dict_set(&options, "video_size", video_size, 0);
    av_dict_set(&options, "framerate", "60.00", 0);
    av_dict_set(&options, "pixel_format", "yuv444p", 0);
    
    ret = avformat_open_input(&fmt_ctx, devicename, ifmt, &options);
    if (ret < 0) {
        goto end;
    }
    
    
end:
    if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
}
