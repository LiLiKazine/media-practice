//
//  record_video.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/2.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "record_video.h"

static int rec_status = 0;

void stop_rec_video() {
    rec_status = 0;
}

/**
 * @brief open audio device
 * @return succ: AVFormat*, fail: NULL
 */
static AVFormatContext* open_dev() {
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    //[[video device]:[audio device]]
    //mac 0 camero
    // 1 desktop
    char *devicename = "0";
    int ret = 0;
    char errors[1024] = {0,};
    //get format
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    av_dict_set(&options, "video_size", "640x480", 0);
    av_dict_set(&options, "framerate", "29.97", 0);
    av_dict_set(&options, "pixel_format", "nv12", 0);
    
    //open device
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open video device, [%d]%s\n", ret, errors);
        return NULL;
    }
    return fmt_ctx;
}

void rec_video() {
    
    //ctx
    AVFormatContext *fmt_ctx = NULL;
    
    
    //set_log_level
    av_log_set_level(AV_LOG_DEBUG);
    
    avdevice_register_all();
    
    rec_status = 1;
    
    //create file
    char *outPath = "/Users/lisheng/Desktop/video.yuv";
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
    
    int ret = 0;
    
    AVPacket pkt;
    
     //read data from device
        while (rec_status) {
            ret = av_read_frame(fmt_ctx, &pkt);
            if(ret < 0){
                printf("%s\n", av_err2str(ret));
                if (ret == AVERROR(EAGAIN)) {
                    av_usleep(10000);
                    continue;
                }
                break;
            }
            av_log(NULL, AV_LOG_INFO, "packet size is %d(%p)\n", pkt.size, pkt.data);
        
            fwrite(pkt.data, 1, 460800, outfile);
//            fflush(outfile);
            av_packet_unref(&pkt); //release pkt
        }

__ERROR:
    
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

