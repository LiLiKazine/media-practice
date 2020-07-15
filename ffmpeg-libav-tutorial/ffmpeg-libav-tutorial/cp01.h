//
//  cp01.h
//  ffmpeg-libav-tutorial
//
//  Created by Li Sheng on 2020/7/15.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "libavformat/avformat.h"

NS_ASSUME_NONNULL_BEGIN

@interface cp01 : NSObject {
    int packet_decoded;
}

@property NSString* dst;
@property int packet_limit;

-(void) open_input: (const char*) src;

-(void) logging: (const char*) fmt, ...;

-(void) save_grey_frame: (unsigned char*) buf wrap: (int) wrap xsize: (int) xsize ysize: (int) ysize filename: (char*) filename;

-(int) decode_packet: (AVPacket*) packet decode_ctx: (AVCodecContext*) decode_ctx frame: (AVFrame*) frame;

@end

NS_ASSUME_NONNULL_END
