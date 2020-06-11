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
#include "log.h"
#include "avformat.h"

void set_log_level(int);
void output(int level, const char *fmt);
int delete_file(const char *url);
int move_file(const char *src, const char *dst);
void read_dir(const char *url);
void dump_meta(const char *url);
void extract_audio(const char *src, const char *dst);

#endif /* ffmpeg_h */
