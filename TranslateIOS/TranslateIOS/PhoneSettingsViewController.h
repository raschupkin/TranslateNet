//
//  PhoneSettingsViewController.h
//  Translate-Net
//
//  Created by Admin on 09.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_PhoneSettingsViewController_h
#define Translate_Net_PhoneSettingsViewController_h

#import <UIKit/UIKit.h>
#import "NetworkClient.h"
#import "User.h"

#define TIMER_BUTTON_RESEND_SMS 5
@class PhoneSettingsDialogViewController;
@class SettingsViewController;
@interface PhoneSettingsViewController : UIViewController <NetworkReceiver, UITextFieldDelegate>{
    User *user;
    NSString *EnteredPhone;

    NSTimer *resendSMSButtonTimer;
    UIActivityIndicatorView *activityIndicator;
    
    bool isShown;
    PhoneSettingsDialogViewController *PhoneSettingsDialogVC;
@public
    SettingsViewController *SettingsVC;
    bool isStarting;
}
@property (weak, nonatomic) IBOutlet UIButton *B_Back;
@property (weak, nonatomic) IBOutlet UILabel *L_Descr;
@property (weak, nonatomic) IBOutlet UILabel *L_PhoneLabel;
@property (weak, nonatomic) IBOutlet UILabel *L_Phone;
@property (weak, nonatomic) IBOutlet UILabel *L_StatusLabel;
@property (weak, nonatomic) IBOutlet UILabel *L_Status;
@property (weak, nonatomic) IBOutlet UIButton *B_Update;
@property (weak, nonatomic) IBOutlet UILabel *L_CodeDescr;
@property (weak, nonatomic) IBOutlet UITextField *TF_Code;
@property (weak, nonatomic) IBOutlet UIButton *B_Done;
@property (weak, nonatomic) IBOutlet UIButton *B_ResendSMS;
-(void)onEnteredPhoneNumber:(NSString *)phone;
@end
#endif
