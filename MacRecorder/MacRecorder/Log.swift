//
//  Log.swift
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/9.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

class Log {
    
    class func setLevel(level: Int32 = AV_LOG_DEBUG) {
        set_log_level(level)
    }
    
    class func print(level: Int32 = AV_LOG_INFO, _ content: String) {
        output(level, "\(content)\n")
    }

}
