//
//  Thumbnails.h
//  Imitator
//
//  Created by Li Sheng on 2020/7/9.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <CoreVideo/CoreVideo.h>
#import "libavformat/avformat.h"
#import "libavcodec/avcodec.h"
#import "libswscale/swscale.h"
#import "libavutil/imgutils.h"
#import "libavutil/parseutils.h"

NS_ASSUME_NONNULL_BEGIN

@interface Thumbnails : NSObject
- (void)saveThumbnails:(NSString*)src;
@end

NS_ASSUME_NONNULL_END
