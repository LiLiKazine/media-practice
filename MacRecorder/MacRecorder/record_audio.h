//
//  record_audio.h
//  MacRecorder
//
//  Created by LiLi Kazine on 2020/5/16.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#ifndef record_audio_h
#define record_audio_h

#include <unistd.h>
#include <stdio.h>
#include "avutil.h"
#include "avdevice.h"
#include "avformat.h"
#include "avcodec.h"
#include "swresample.h"
#include "time.h"

void foo(void);
void rec_audio(void);
void stop_rec(void);

#endif /* record_audio_h */
