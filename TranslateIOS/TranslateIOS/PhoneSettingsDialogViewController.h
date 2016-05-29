//
//  PhoneSettingsDialogViewController.h
//  Translate-Net
//
//  Created by Admin on 29.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_PhoneSettingsDialogViewController_h
#define Translate_Net_PhoneSettingsDialogViewController_h

#import <UIKit/UIKit.h>
#import "PhoneSettingsViewController.h"

@interface PhoneSettingsDialogViewController : UIViewController <UITextFieldDelegate> {
    PhoneSettingsViewController *PhoneSettingsVC;
}
@property (weak, nonatomic) IBOutlet UIButton *B_Back;
@property (weak, nonatomic) IBOutlet UILabel *L_Descr;
@property (weak, nonatomic) IBOutlet UITextField *TF_Phone;
@property (weak, nonatomic) IBOutlet UIButton *B_Done;
-(void)setPhoneSettingsVC:(PhoneSettingsViewController *)vc;
@end

#endif
