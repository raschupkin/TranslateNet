//
//  SettingsViewController.h
//  Translate-Net
//
//  Created by Admin on 06.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_SettingsViewController_h
#define Translate_Net_SettingsViewController_h

#import <UIKit/UIKit.h>

@interface SettingsViewController : UIViewController {
@public
    bool SegueFromTList;
    bool SegueFromPhonecall;
}
@property (weak, nonatomic) IBOutlet UIButton *B_UserSettings;
@property (weak, nonatomic) IBOutlet UIButton *B_PhoneSettings;
@property (weak, nonatomic) IBOutlet UIButton *B_History;
@property (weak, nonatomic) IBOutlet UIButton *B_Disconnect;
@property (weak, nonatomic) IBOutlet UIButton *B_Back;
-(void)setFromTList;
-(void)setFromPhonecall;
@end

#endif
