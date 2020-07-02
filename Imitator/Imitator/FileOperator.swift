//
//  FileOperator.swift
//  Imitator
//
//  Created by Li Sheng on 2020/7/2.
//  Copyright © 2020 lilikazine. All rights reserved.
//
import Cocoa
import Combine

class FileOperator {
    
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
