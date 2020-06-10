//
//  FileOperator.swift
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/9.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

import Cocoa
import Combine

class FileOperator {
    
    class func openFile(window: NSWindow) -> AnyPublisher<URL, FileError>  {
        return Future<URL, FileError> { promise in
            let home = FileManager.default.homeDirectoryForCurrentUser
            let panel = NSOpenPanel()
            panel.directoryURL = home
            panel.canChooseFiles = true
            panel.canChooseDirectories = false
            panel.allowsMultipleSelection = false
            panel.beginSheetModal(for: window) { result in
                if result == .OK, let url = panel.url {
                    promise(.success(url))
                } else {
                    promise(.failure(.unknown))
                }
            }
        }
        .eraseToAnyPublisher()
        
        
    }
    
    enum FileError: Error {
        case unknown
    }
    
}


