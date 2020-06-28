//
//  player.h
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/23.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#ifndef player_h
#define player_h

#include <stdio.h>
#include "SDL2/SDL.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

void play_video(const char* src);

#endif /* player_h */
