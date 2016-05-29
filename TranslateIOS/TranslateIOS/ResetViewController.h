//
//  ResetViewController.h
//  Translate-Net
//
//  Created by Admin on 19.02.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_ResetViewController_h
#define Translate_Net_ResetViewController_h


#import <UIKit/UIKit.h>
#import "Parser.h"
#import "NetworkClient.h"

@interface ResetViewController : UIViewController <NetworkReceiver, UITextFieldDelegate>  {
    NSString *mEmail;
    
    Parser *parser;
    UIActivityIndicatorView *activityIndicator;
    __weak IBOutlet UIButton *B_Back;
    __weak IBOutlet UILabel *L_Descr;
    __weak IBOutlet UILabel *L_EmailLabel;
    __weak IBOutlet UITextField *TF_Email;
    __weak IBOutlet UIButton *B_Reset;
    __weak IBOutlet UILabel *L_LetterDescr;
}
@end


#endif
