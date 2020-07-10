//
//  Extract.m
//  Imitator
//
//  Created by Li Sheng on 2020/7/10.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#import "Extract.h"

//@interface Extract()
//
//@property(nullable) AVFormatContext* infmt_ctx;
//@property(nullable) NSString* src;
//
//@end

@implementation Extract

AVFormatContext* infmt_ctx;
NSString* src;
int ret = 0;

-(void) openInput:(NSString*) path
{
    
    src = path;
    ret = avformat_open_input(&infmt_ctx, [src UTF8String], NULL, NULL);
    if (ret < 0) {
        
    }
end:
    av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
}

-(void) extractH264: (NSString*) src: destination: (NSString*) dst
{
    
}

@end
