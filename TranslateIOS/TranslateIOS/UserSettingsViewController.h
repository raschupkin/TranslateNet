//
//  UserSettingsViewController.h
//  Translate-Net
//
//  Created by Admin on 09.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_UserSettingsViewController_h
#define Translate_Net_UserSettingsViewController_h

#import <UIKit/UIKit.h>
#import "NetworkClient.h"
#import "User.h"
#import "IAPHelper.h"

@class SettingsViewController;
@interface UserSettingsViewController : UIViewController <NetworkReceiver,
                                            UITextFieldDelegate> {
    int NameMinLength;
    NSString *NameSymbols;
    
    User *user;
    bool DonePressed;
    
    IAPHelper *iaph;
    NSArray *_products;

    UIActivityIndicatorView *activityIndicator;
@public
    SettingsViewController *SettingsVC;
    bool fromTList, fromPhonecall;
}
@property (weak, nonatomic) IBOutlet UIButton *B_Back;
@property (weak, nonatomic) IBOutlet UILabel *L_NameDescr;
@property (weak, nonatomic) IBOutlet UITextField *TF_Name;
@property (weak, nonatomic) IBOutlet UILabel *L_EmailLabel;
@property (weak, nonatomic) IBOutlet UILabel *L_Email;
@property (weak, nonatomic) IBOutlet UILabel *L_BalanceLabel;
@property (weak, nonatomic) IBOutlet UILabel *L_Balance;
@property (weak, nonatomic) IBOutlet UILabel *L_CreditDescr;
@property (weak, nonatomic) IBOutlet UIButton *B_1credit;
@property (weak, nonatomic) IBOutlet UIButton *B_2credits;
@property (weak, nonatomic) IBOutlet UIButton *B_5credits;
@property (weak, nonatomic) IBOutlet UIButton *B_10credits;
@property (weak, nonatomic) IBOutlet UIButton *B_25credits;
@property (weak, nonatomic) IBOutlet UIButton *B_Done;
@end
#endif
