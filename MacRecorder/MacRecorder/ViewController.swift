//
//  ViewController.swift
//  MacRecorder
//
//  Created by LiLi Kazine on 2020/5/16.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        
    }
    
    @IBAction func action(_ sender: NSButton) {
        rec_audio()
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

