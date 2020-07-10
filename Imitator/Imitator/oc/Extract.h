//
//  Extract.h
//  Imitator
//
//  Created by Li Sheng on 2020/7/10.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "libavformat/avformat.h"
#import "libavcodec/avcodec.h"
#import "libavformat/avio.h"

NS_ASSUME_NONNULL_BEGIN

@interface Extract : NSObject

-(int) openInput:(NSString*) path;
-(void) extractH264: (NSString*) dst;

@end

NS_ASSUME_NONNULL_END
