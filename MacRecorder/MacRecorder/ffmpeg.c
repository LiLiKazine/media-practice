//
//  ffmpeg.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/9.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "ffmpeg.h"

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


