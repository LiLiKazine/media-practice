//
//  testc.h
//  myapp
//
//  Created by lichao on 2020/1/30.
//  Copyright © 2020年 lichao. All rights reserved.
//

#ifndef testc_h
#define testc_h

#include <stdio.h>
#include "avutil.h"
#include "avdevice.h"
#include "avformat.h"
#include "avcodec.h"
#include "swresample.h"
//#include "avutil.h"
//#include "time.h"
//#include "avdevice.h"
//#include "avformat.h"
//#include "avcodec.h"
//#include "swresample.h"

void set_status(int status);
void rec_audio(void);

#endif /* testc_h */
