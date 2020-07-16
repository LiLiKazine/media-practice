//
//  ViewController.swift
//  ffmpeg-libav-tutorial
//
//  Created by Li Sheng on 2020/7/15.
//  Copyright © 2020 lilikazine. All rights reserved.
//

import Cocoa
import Combine

class ViewController: NSViewController {
    
    @IBOutlet weak var greyscaleButton: NSButton!
    @IBOutlet weak var reformatButton: NSButton!
    @IBOutlet weak var filenameTextField: NSTextField!
    @IBOutlet weak var importButton: NSButton!
    
    lazy var p1 = cp01()
    lazy var p2 = cp02()
    
    var subscriptions: Set<AnyCancellable> = []
    
    private var src: URL? {
        didSet {
            update()
        }
    }
    
    private var greyscaleURL: URL?
    
    func update() {
        let filename = src?.lastPathComponent ?? ""
        filenameTextField.stringValue = filename
        guard let url = src else { return }
        greyscaleURL = url.deletingLastPathComponent().appendingPathComponent("greyscales", isDirectory: true)
        
        
    }
    
    func createDir(_ url: URL) -> Bool {
        guard !FileOperator.isExisted(url, isDir: true) else {
            return true
        }
        
        do {
            try FileOperator.createDir(url)
            return true
        } catch {
            print(error.localizedDescription)
        }
        return false
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        

    
    }
    
    @IBAction func actions(_ sender: NSButton) {
        switch sender {
        case greyscaleButton:
            saveGreyscale()
        case reformatButton:
            reformatVideo()
        default:
            //import
            importFile()
        }
    }
    
    func saveGreyscale() {
        guard let src = src?.path, let dst = greyscaleURL, createDir(dst) else {
                return
        }
        p1.packet_limit = 200
        p1.dst = dst.path
        DispatchQueue.global().async {
            self.p1.open_input(src)
        }
        
    }
    
    func reformatVideo() {
        guard let filename = src?.deletingPathExtension().lastPathComponent,
            let dst = src?.deletingLastPathComponent().appendingPathComponent(filename).appendingPathExtension("ts").path,
            let src = src?.path else { return }
        DispatchQueue.global().async {
            self.p2.reformat(src, destination: dst)
        }
    }
    
    func importFile() {
        guard let window = view.window else {
            return
        }
        FileOperator.selectPath(window: window, allowDirectory: true)
            .receive(on: DispatchQueue.main)
            .sink(receiveCompletion: { completion in
                switch completion {
                case .failure(_): break
                case .finished: break
                }
            }) { url in
                self.src = url
        }
        .store(in: &subscriptions)
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

