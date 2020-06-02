//
//  ViewController.swift
//  MacRecorder
//
//  Created by LiLi Kazine on 2020/5/16.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {
    
    @IBOutlet weak var recButton: NSButton!
    var recStatus: Bool = false
    var thread: Thread?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        
    }
    
    @IBAction func action(_ sender: NSButton) {
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

