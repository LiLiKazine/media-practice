//
//  cp02.h
//  ffmpeg-libav-tutorial
//
//  Created by Li Sheng on 2020/7/16.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "libavformat/avformat.h"

NS_ASSUME_NONNULL_BEGIN

@interface cp02 : NSObject

-(void) reformat: (const char*) src destination: (const char*) dst;

@end

NS_ASSUME_NONNULL_END
