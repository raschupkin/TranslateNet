//
//  RegisterViewController.h
//  Translate-Net
//
//  Created by Admin on 14.02.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_RegisterViewController_h
#define Translate_Net_RegisterViewController_h


#import <UIKit/UIKit.h>
#import "Parser.h"
#import "NetworkClient.h"

@interface RegisterViewController : UIViewController <NetworkReceiver, UITextFieldDelegate>  {
    NSString *mEmail;
    
    Parser *parser;
    UIActivityIndicatorView *activityIndicator;
    __weak IBOutlet UIButton *L_Back;
    __weak IBOutlet UILabel *L_IOS_Descr;
    __weak IBOutlet UILabel *L_EmailDescr;
    __weak IBOutlet UITextField *TF_Email;
    __weak IBOutlet UIButton *B_Cancel;
    __weak IBOutlet UIButton *B_Register;
    __weak IBOutlet UILabel *L_Descr;
}
@end

#endif
