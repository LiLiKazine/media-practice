//
//  ffmpeg.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/9.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "ffmpeg.h"

static void adts_header(char *szAdtsHeader, int dataLen){
    
    int audio_object_type = 2;
    int sampling_frequency_index = 7;
    int channel_config = 2;

    int adtsLen = dataLen + 7;

    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
    szAdtsHeader[1] |= 1;           //protection absent:1                     1bit

    szAdtsHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
    szAdtsHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
    szAdtsHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               高1bit

    szAdtsHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      低2bits
    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

    szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    szAdtsHeader[6] = 0xfc;
}
void set_log_level(int level) {
    av_log_set_level(level);
}

void output(int level, const char *fmt) {
    av_log(NULL, level, "%s", fmt);
}

int delete_file(const char *url) {
    int ret = avpriv_io_delete(url);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

int move_file(const char *src, const char *dst) {
    int ret = avpriv_io_move(src, dst);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

void read_dir(const char *url) {
    AVIODirContext *ctx = NULL;
    AVIODirEntry *entry = NULL;
    int ret = 0;
    
    ret = avio_open_dir(&ctx, url, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open dir: %s\n", av_err2str(ret));
        return;
    }
   
    while (1) {
        ret = avio_read_dir(ctx, &entry);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot read dir: %s\n", av_err2str(ret));
            break;
        }
        
        if (!entry) {
            break;
        }
        av_log(NULL, AV_LOG_INFO, "%12"PRId64" %s \n", entry->size, entry->name);
        avio_free_directory_entry(&entry);
    }
    
    avio_close_dir(&ctx);
}

void dump_meta(const char *url) {
//    av_register_all();  /* deprecated */
    int ret = 0;
    AVFormatContext *ctx = NULL;
    ret = avformat_open_input(&ctx, url, NULL, NULL);
    if (!ctx || ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open file: %s\n", av_err2str(ret));
        return;
    }
    av_dump_format(ctx, 0, url, 0);
    avformat_close_input(&ctx);
}

void extract_audio(const char *src, const char *dst) {
    int ret = 0;
    int audio_index;
    unsigned long len;
    AVFormatContext *ctx = NULL;
    AVPacket pkt;
    FILE *audio_file = NULL;
    ret = avformat_open_input(&ctx, src, NULL, NULL);
    if (!ctx || ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open file: %s\n", av_err2str(ret));
        return;
    }
    audio_file = fopen(dst, "wb");
    if (!audio_file) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open output file.");
        return;
    }
    ret = av_find_best_stream(ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find best stream: %s\n", av_err2str(ret));
        avformat_close_input(&ctx);
        fclose(audio_file);
        return;
    } else {
        audio_index = ret;
    }
    av_init_packet(&pkt);

    while (ret >= 0) {
        ret = av_read_frame(ctx, &pkt);
        if (pkt.stream_index == audio_index) {
            char adts_header_buf[7];
            adts_header(adts_header_buf, pkt.size);
            fwrite(adts_header_buf, 1, 7, audio_file);
            len = fwrite(pkt.data, 1, pkt.size, audio_file);
            if (len != pkt.size) {
                av_log(NULL, AV_LOG_WARNING, "Warning, length of data is not equal to the size of packet: %lu, %d", len, pkt.size);
            }
        }
        av_packet_unref(&pkt);
    }
    
    avformat_close_input(&ctx);
    if (audio_file) {
        fclose(audio_file);
    }
}


