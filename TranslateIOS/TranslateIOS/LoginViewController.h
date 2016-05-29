//
//  ViewController.h
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "Parser.h"
#import "NetworkClient.h"

#define DEFAULTS_EMAIL  @"email"
#define DEFAULTS_PASSWORD  @"password"

@interface LoginViewController : UIViewController <NetworkReceiver> {
    Parser *parser;

    UIActivityIndicatorView *activityIndicator;
    NSString *mEmail;
    NSString *mPassword;
    bool isLoginPressed;
    __weak IBOutlet UITextField *TF_Email;
    __weak IBOutlet UITextField *TF_Password;
    __weak IBOutlet UILabel *L_Email;
    __weak IBOutlet UILabel *L_Password;
    __weak IBOutlet UIButton *B_Login;
    __weak IBOutlet UIButton *B_Register;
    __weak IBOutlet UIButton *B_Reset;
}
-(int)onNetwork_PacketReceived:(ParsedPacket *)packet;
-(int)onNetwork_Error: (int) code;
-(IBAction)loginClick:(id)sender;

@end
