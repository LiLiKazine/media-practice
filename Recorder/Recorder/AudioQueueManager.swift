//
//  AudioQueueManager.swift
//  Recorder
//
//  Created by LiLi Kazine on 2020/5/13.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

import AudioToolbox

class AudioQueueManager {
    
    private init() {}
    
    static let shared: AudioQueueManager = .init()
    
    func createInputQueue(inFormatPointer: UnsafePointer<AudioStreamBasicDescription>) {
        
        var audioQueue: AudioQueueRef?
        AudioQueueNewInput(inFormatPointer, { (inUserData, aqRef, aqBufferRef, timestampPointer, inNumPackets, aspd) in
            
        }, nil, nil, CFRunLoopMode.commonModes.rawValue, 0, &audioQueue)
        
    }
    
}
