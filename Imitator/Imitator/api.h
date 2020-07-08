//
//  api.h
//  Imitator
//
//  Created by Li Sheng on 2020/6/29.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#ifndef api_h
#define api_h

#include <stdio.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/timestamp.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

struct AudioInfo {
    AVDictionary* metadata;
};

struct VideoInfo {
    AVDictionary* metadata;
    int64_t duration;
};

void dump(AVFormatContext* fmt_ctx,
          int index,
          const char* url,
          int is_output);

void media_legth(AVFormatContext* fmt_ctx,
                 const char* url,
                 struct AudioInfo** audio_info,
                 struct VideoInfo** video_info);

void cut_video(const char* src,
               const char* dst,
               int64_t begin,
               int64_t end,
               AVCodec* encoder);

void dump_filters(void);

void extract_video(const char* src,
                   const char* dst,
                   int64_t begin,
                   int64_t end);

void frame_2_pic(const char* src, const char* dir);

AVFrame* h264_2_data(const char* src);

AVFrame* frame_2_data(AVFrame* inframe, AVFrame** outframe);
#endif /* api_h */
