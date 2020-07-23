//
//  ViewController.swift
//  VisionDemo
//
//  Created by Li Sheng on 2020/7/23.
//  Copyright © 2020 lilikazine. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
    }

    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        DispatchQueue.main.async {
            open_input(3840, 2160)
        }
        
    }

}

