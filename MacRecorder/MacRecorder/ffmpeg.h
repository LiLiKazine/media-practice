//
//  ffmpeg.h
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/9.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#ifndef ffmpeg_h
#define ffmpeg_h

#include <stdio.h>
#include "libavutil/log.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"

void set_log_level(int);
void output(int level, const char *fmt);
int delete_file(const char *url);
int move_file(const char *src, const char *dst);
void read_dir(const char *url);
void dump_meta(const char *url);
void extract_audio(const char *src, const char *dst);
void extract_video(const char *src, const char *dst);
void mp4_2_flv(const char *in_filename, const char *out_filename);
void cut_video(double from_seconds,
              double end_seconds,
              const char *in_filename,
              const char *out_filename);

#endif /* ffmpeg_h */
