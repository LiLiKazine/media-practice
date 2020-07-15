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
    

    class func isExisted(_ url: URL, isDir: Bool) -> Bool {
        var flag = ObjCBool(isDir)
        return FileManager.default.fileExists(atPath: url.path, isDirectory: &flag)
    }
    
    class func createDir(_ url: URL) throws {
        try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true, attributes: nil)
    }
    
    class func selectPath(window: NSWindow, allowDirectory: Bool = false) -> AnyPublisher<URL, FileError>  {
        return Future<URL, FileError> { promise in
            let home = FileManager.default.homeDirectoryForCurrentUser
            let panel = NSOpenPanel()
            panel.directoryURL = home
            panel.canChooseFiles = true
            panel.canChooseDirectories = allowDirectory
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


