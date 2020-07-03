//
//  ViewController.swift
//  Imitator
//
//  Created by Li Sheng on 2020/6/29.
//  Copyright © 2020 lilikazine. All rights reserved.
//

import Cocoa
import Combine

class ViewController: NSViewController {
    
    @IBOutlet weak var extractButton: NSButton!
    @IBOutlet weak var fmtTextField: NSTextField!
    @IBOutlet weak var importButton: NSButton!
    @IBOutlet weak var fileTextField: NSTextField!
    @IBOutlet weak var startTextField: NSTextField!
    @IBOutlet weak var endTextField: NSTextField!
    @IBOutlet weak var startSlider: NSSlider!
    @IBOutlet weak var endSlider: NSSlider!
    
    private var subscriptions: Set<AnyCancellable> = []
    
    var filePath: URL? {
        didSet {
            DispatchQueue.main.async {
                if let path = self.filePath?.path {
                    self.display(path: path)
                }
                self.fileTextField.stringValue = self.filePath?.path ?? ""
            }
        }
    }
    
    var videoLength: Int? {
        didSet {
            endTextField.stringValue = timeString(sec: videoLength ?? 0)
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    @IBAction func sliderAction(_ sender: NSSlider) {
        switch sender {
        case startSlider, endSlider:
            updateSection(val: sender.integerValue, isBegin: startSlider == sender)
        default:
            break
        }
    }
    
    func updateSection(val: Int, isBegin: Bool) {
        guard let length = videoLength else {
            return
        }
        let time = timeString(sec: length / 100 * val)
        let targetView = isBegin ? startTextField : endTextField
        targetView?.stringValue = time
    }
    
    @IBAction func buttonAction(_ sender: NSButton) {
        switch sender {
        case importButton:
            openFM()
        case extractButton:
            extract()
        default:
            break
        }
    }
    
    func extract(){
        guard let src = filePath, let length = videoLength else {
            return
        }
        let ext = fmtTextField.stringValue.isEmpty ?
            "mp4" : fmtTextField.stringValue
        let name = src.deletingPathExtension().appendingPathExtension(ext).lastPathComponent
        let dst = src.deletingLastPathComponent().appendingPathComponent("new_" + name)
        let begin = Int64(length / 100 * startSlider.integerValue)
        let end = Int64(length / 100 * endSlider.integerValue)
        DispatchQueue.global().async {
            cut_video(src.path, dst.path, begin, end, nil)
        }
    }
    
    func openFM() {
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
        }
        .store(in: &subscriptions)
    }
    
    func display(path: String) {
        dump(nil, 0, path, 0)
        
        var audioInfo: UnsafeMutablePointer<AudioInfo>?
        var videoInfo: UnsafeMutablePointer<VideoInfo>?
        media_legth(nil, path, &audioInfo, &videoInfo)
        
        if let _ = audioInfo?.pointee {
            
        }
        
        if let videoInfo = videoInfo?.pointee {
            videoLength = Int(videoInfo.duration)
        }
    }
    
}


func timeString(sec: Int) -> String {
    let h = sec / 3600
    let m = sec % 3600 / 60
    let s = sec % 3600 % 60
    return String(format: "%02d:%02d:%02d", h, m, s)
}
