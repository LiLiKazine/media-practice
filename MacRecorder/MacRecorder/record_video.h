//
//  record_video.h
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/2.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#ifndef record_video_h
#define record_video_h

#include <unistd.h>
#include <stdio.h>
#include "avutil.h"
#include "avdevice.h"
#include "avformat.h"
#include "avcodec.h"
#include "swresample.h"
#include "time.h"

void rec_video(void);
void stop_rec_video(void);


#endif /* record_video_h */
