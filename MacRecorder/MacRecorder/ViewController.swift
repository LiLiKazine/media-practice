//
//  ViewController.swift
//  MacRecorder
//
//  Created by LiLi Kazine on 2020/5/16.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

import Cocoa
import Combine

class ViewController: NSViewController {
    
    @IBOutlet weak var filePathTf: NSTextField!
    @IBOutlet weak var dirPathTf: NSTextField!
    @IBOutlet weak var recButton: NSButton!
    @IBOutlet weak var fileBtn: NSButton!
    @IBOutlet weak var deleteBtn: NSButton!
    @IBOutlet weak var dirBtn: NSButton!
    @IBOutlet weak var movBtn: NSButton!
    @IBOutlet weak var metaBtn: NSButton!
    @IBOutlet weak var extractAudioBtn: NSButton!
    @IBOutlet weak var extractVideoBtn: NSButton!
    @IBOutlet weak var reformatBtn: NSButton!
    @IBOutlet weak var cutVideoBtn: NSButton!
    @IBOutlet weak var playBtn: NSButton!
    
    var subscriptions: Set<AnyCancellable> = []
    
    var filePath: URL? {
        didSet {
            DispatchQueue.main.async {
                self.filePathTf.stringValue = self.filePath?.path ?? ""
            }
        }
    }
    
    var dirPath: URL? {
        didSet {
            DispatchQueue.main.async {
                self.dirPathTf.stringValue = self.dirPath?.path ?? ""
            }
        }
    }
    
    var recStatus: Bool = false
    var thread: Thread?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        Log.setLevel()
        
    }
    
    @IBAction func playAction(_ sender: NSButton) {
        DispatchQueue.main.async {
            create_renderer()            
        }
    }
    
    @IBAction func cutAction(_ sender: NSButton) {
        guard
            let src = filePath?.path,
            let filename = filePath?.deletingPathExtension().lastPathComponent,
            let dst = dirPath?.appendingPathComponent(filename).appendingPathExtension("mp4")
                .path else {
                    return
        }
        DispatchQueue.global(qos: .userInitiated)
            .async {
                cut_video(200, 800, src, dst)
        }
    }
    @IBAction func reformatAction(_ sender: NSButton) {
        guard
            let src = filePath?.path,
            let filename = filePath?.deletingPathExtension().lastPathComponent,
            let dst = dirPath?.appendingPathComponent(filename).appendingPathExtension("flv")
                .path else {
                    return
        }
        DispatchQueue.global(qos: .userInitiated)
            .async {
                mp4_2_flv(src, dst)
        }
    }
    
    @IBAction func extractVideoAction(_ sender: NSButton) {
        guard
            let src = filePath?.path,
            let filename = filePath?.deletingPathExtension().lastPathComponent,
            let dst = dirPath?.appendingPathComponent(filename).appendingPathExtension("h264")
                .path else {
                    return
        }
        DispatchQueue.global(qos: .userInitiated)
            .async {
                extract_video(src, dst)
        }
    }
    
    @IBAction func extractAudioAction(_ sender: NSButton) {
        guard
            let src = filePath?.path,
            let filename = filePath?.deletingPathExtension().lastPathComponent,
            let dst = dirPath?.appendingPathComponent(filename).appendingPathExtension("aac")
                .path else {
                    return
        }
        DispatchQueue.global(qos: .userInitiated)
            .async {
                extract_audio(src, dst)
        }
    }
    
    @IBAction func metaAction(_ sender: NSButton) {
        guard let path = filePath?.path else {
            return
        }
        dump_meta(path)
    }
    
    @IBAction func moveAction(_ sender: NSButton) {
        guard
            let src = filePath?.path,
            let filename = filePath?.lastPathComponent,
            let dst = dirPath?.appendingPathComponent(filename).path else {
                return
        }
        
        let ret = move_file(src, dst)
        if ret >= 0 {
            Log.print("move \(src) to \(dst) succeeded.")
            filePath = nil
            dirPath = nil
        }
    }
    
    
    @IBAction func openDirectoryAction(_ sender: NSButton) {
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
                self.dirPath = url
                read_dir(url.path)
                Log.print(url.path)
        }
        .store(in: &subscriptions)
    }
    
    @IBAction func deleteFileAction(_ sender: NSButton) {
        guard let path = filePath?.path else {
            return
        }
        let res = delete_file(path)
        if res >= 0 {
            Log.print("delete \(path) succeeded.")
            filePath = nil
        }
    }
    
    @IBAction func importFileAction(_ sender: NSButton) {
        guard let window = view.window else {
            return
        }
        FileOperator.selectPath(window: window)
            .receive(on: DispatchQueue.main)
            .sink(receiveCompletion: { completion in
                switch completion{
                case .finished: break
                case .failure(_): break
                }
            }) { url in
                self.filePath = url
                Log.print(url.path)
        }
        .store(in: &subscriptions)
    }
    
    @IBAction func recAction(_ sender: NSButton) {
        recStatus.toggle()
        sender.title = recStatus ? "结束录制" : "开始录制"
        if recStatus {
            thread = .init(target: self, selector: #selector(recAudio), object: nil)
            thread?.start()
        } else {
            stop_rec_video()
        }
    }
    
    @objc func recAudio() {
        print("start thread.")
        rec_video()
    }
    
    override var representedObject: Any? {
        didSet {
            // Update the view, if already loaded.
        }
    }
    
    
}

