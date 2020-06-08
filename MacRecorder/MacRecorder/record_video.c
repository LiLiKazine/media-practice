//
//  record_video.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/2.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "record_video.h"

#define V_WIDTH 640
#define V_HEIGHT 480

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

static void open_encoder(int width, int height, AVCodecContext **enc_ctx) {
    
    int ret = 0;
    
    AVCodec *codec = NULL;
    
    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        printf("Codec libx264 not found.\n");
        exit(1);
    }
    
    *enc_ctx = avcodec_alloc_context3(codec);
    
    if (!enc_ctx) {
        printf("Could not allocate  video codec context.\n");
        exit(1);
    }
    
    (*enc_ctx)->profile = FF_PROFILE_H264_HIGH_444;
    (*enc_ctx)->level = 50;
    
    (*enc_ctx)->width = width;
    (*enc_ctx)->height = height;
    
    (*enc_ctx)->gop_size = 250;
    (*enc_ctx)->keyint_min = 25;
    
    (*enc_ctx)->max_b_frames = 3;
    (*enc_ctx)->has_b_frames = 1;
    
    (*enc_ctx)->refs = 3;
    
    (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;
    
    (*enc_ctx)->bit_rate = 1000000; //1000kbps
    
    (*enc_ctx)->time_base = (AVRational){1, 25}; // 帧间隔
    (*enc_ctx)->framerate = (AVRational){25, 1};;
    
    ret = avcodec_open2((*enc_ctx), codec, NULL);
    
    if (ret < 0) {
        printf("Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }
}

static AVFrame* create_frame(int widht, int height) {
    
    int ret = 0;
    
    AVFrame *frame = av_frame_alloc();
    
    if (!frame) {
        printf("Error, NO Memory!\n");
        goto __ERROR;
    }
    
    frame->width = widht;
    frame->height = height;
    
    frame->format = AV_PIX_FMT_YUV420P;
    
    ret = av_frame_get_buffer(frame, 32);//按32位对齐
    if (ret<0) {
        printf("Error, Failed to alloc buffer for frame!\n");
        goto __ERROR;
    }
    return frame;
    
__ERROR:
    if (frame) {
        av_frame_free(&frame);
    }
    return NULL;
}

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *newpkt, FILE *outfile) {
    if (!enc_ctx) {
        
    }
    
    if (!frame) {
        printf("frame is null.\n");
    } else {
        printf("send frame to encoder, pts=%lld", frame->pts);
    }
    
    int ret = 0;
    
    
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret<0) {
        printf("Error, Failed to send a frame for encoding! %s\n", av_err2str(ret));
        exit(1);
    }
    while (ret>=0) {
        ret = avcodec_receive_packet(enc_ctx, newpkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0){
            printf("Error, Failed to encode! %s\n", av_err2str(ret));
            exit(1);
        }
        fwrite(newpkt->data, 1, newpkt->size, outfile);
        av_packet_unref(newpkt);
    }
}

void rec_video() {
    
    //ctx
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *enc_ctx = NULL;
    
    AVFrame *frame = NULL;
    AVPacket *newpkt = NULL;
    
    //set_log_level
    av_log_set_level(AV_LOG_DEBUG);
    
    avdevice_register_all();
    
    rec_status = 1;
    
    //create file
    char *yuvout = "/Users/lisheng/Desktop/video.yuv";
    FILE *yuvoutfile = fopen(yuvout, "wb+");
    
    char *out = "/Users/lisheng/Desktop/video.h264";
    FILE *outfile = fopen(out, "wb+");
    
    if (!yuvoutfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }
    
    if (!outfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }
    
    fmt_ctx = open_dev();
    if (!fmt_ctx) {
        printf("Error, Failed to open device!\n");
        goto __ERROR;
    }
    
    open_encoder(V_WIDTH, V_HEIGHT, &enc_ctx);
    
    //input
    frame = create_frame(V_WIDTH, V_HEIGHT);
    if (!frame) {
        goto __ERROR;
    }
    
    //output
    newpkt = av_packet_alloc();
    if (!newpkt) {
        printf("Error, Failed to allocate avpacket!\n");
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
        
//            fwrite(pkt.data, 1, 460800, yuvoutfile);
//            fflush(outfile);
            
            //YYYYYYYYYUVUV  NV12
            //YYYYYYYYYUUVV  YUV420
            memcpy(frame->data[0], pkt.data, V_WIDTH*V_HEIGHT); //copy y data
            for (int i=0; i<V_WIDTH*V_HEIGHT/4; i++) {
                frame->data[1][i] = pkt.data[V_WIDTH*V_HEIGHT+i*2];
                frame->data[2][i] = pkt.data[V_WIDTH*V_HEIGHT+i*2+1];
            }
            
            fwrite(frame->data[0], 1, V_WIDTH*V_HEIGHT, yuvoutfile);
            fwrite(frame->data[1], 1, V_WIDTH*V_HEIGHT/4, yuvoutfile);
            fwrite(frame->data[2], 1, V_WIDTH*V_HEIGHT/4, yuvoutfile);
            
            encode(enc_ctx, frame, newpkt, outfile);
            
            av_packet_unref(&pkt); //release pkt
        }
    encode(enc_ctx, NULL, newpkt, outfile);

__ERROR:
    
    //close device and release ctx
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (yuvoutfile) {
        //close file
        fclose(yuvoutfile);
    }
    
    if (outfile) {
        fclose(outfile);
    }
    
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}

