//
//  RegisterViewController.m
//  Translate-Net
//
//  Created by Admin on 14.02.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RegisterViewController.h"
#import "TNApp.h"
#import "LoginViewController.h"

@implementation RegisterViewController

-(void)onPacket_Error:(int)code
{
    [self showProgress:false];
    if (code != ERROR_NOERROR)
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:code]];
}

-(void)onPacket_AwaitLoginConfirm
{
    [self showProgress:false];
    if (mEmail != nil) {
        NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
        [userDefaults setObject:mEmail forKey:DEFAULTS_EMAIL];
        [userDefaults setObject:@"" forKey:DEFAULTS_PASSWORD];
        [userDefaults synchronize];
    }
    [self backClick:nil];
}

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if (packet == nil)
        return 0;
    if ([packet isKindOfClass:[Packet_Error class]]) {
        Packet_Error *pError = (Packet_Error *)packet;
        if ([pError->command compare:@"register_user"] == 0)
            [self onPacket_Error:pError->code];
    } else if ([packet isKindOfClass:[Packet_AwaitLoginConfirm class]]) {
        [self onPacket_AwaitLoginConfirm];
    }
    return 0;
}

-(int)onNetwork_Error:(int)code
{
    switch (code) {
        case NETWORK_ERROR_OTHER: {
            [self showProgress:false];
            [TNApp.class UserMessage: [TNApp getString: @"error_network"]
                             message: [TNApp getErrorMessage:ERROR_OTHER]];
            return -1;
        }
        case NETWORK_ERROR_CONNECTION: {
            [self showProgress:false];
            [TNApp.class UserMessage: [TNApp getString: @"error_network"]
                             message: [TNApp getString: @"network_error_connect"]];
            return -1;
        }
        case NETWORK_ERROR_FORMAT: {
            [self showProgress:false];
            [TNApp.class UserMessage: [TNApp getString: @"error_network"]
                             message: [TNApp getString: @"network_error_format"]];
            return 0;
        }
    }
    return -1;
}

-(IBAction)backClick:(id)sender
{
    [[NetworkClient client] Disconnect];
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
}

-(IBAction)registerClick:(id)sender
{
    mEmail = [TF_Email text];
    mEmail = [TNApp removeTrailingSpaces:mEmail];
    if (![TNApp checkEmail:mEmail]) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"error_invalid_email"]];
        mEmail = nil;
        return;
    }
    if (![NetworkClient.client Connect]) {
        [TNApp sendPacket_RegisterUser: mEmail isT:false];
        [self showProgress:true];
    } else
        [TNApp UserMessage:[TNApp getString:@"error_network"] message:[TNApp getString:@"network_error_connect"]];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

-(void)viewDidAppear:(BOOL)animated
{
    [NetworkClient.client setReceiver:self];    // in viewDidLoad too
}

-(void)initControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"ios_client_descr"];
    [TNApp setUILabelText:L_IOS_Descr text:text font:font];
    [L_IOS_Descr setNumberOfLines:0];
    [L_IOS_Descr setLineBreakMode:NSLineBreakByWordWrapping];
    
    text = [TNApp getString:@"prompt_email"];
    [TNApp setUILabelText:L_EmailDescr text:text font:font];

    [TF_Email setText:[TNApp getString:@""]];
    [TF_Email setKeyboardType:UIKeyboardTypeEmailAddress];
    [TF_Email setDelegate:self];
    	
    text = [TNApp getString:@"cancel"];
    [TNApp setUIButtonText:B_Cancel text:text font:font];
    
    text = [TNApp getString:@"action_register"];
    [TNApp setUIButtonText:B_Register text:text font:font];
    
    text = [TNApp getString:@"register_descr"];
    [TNApp setUILabelText:L_Descr text:text font:font];
    [L_Descr setNumberOfLines:0];
    [L_Descr setLineBreakMode:NSLineBreakByWordWrapping];
    [[self view] layoutSubviews];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    
    // Do any additional setup after loading the view, typically from a nib.
    [self initControls];
    [self initProgress];
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
                                   initWithTarget:self
                                   action:@selector(dismissKeyboard)];
    [self.view addGestureRecognizer:tap];
    
    if (NetworkClient.client == nil)
        [NetworkClient init];
    [NetworkClient.client setReceiver:self];
    [NetworkClient.client Disconnect];

    parser = [[Parser alloc] init];
}

-(void)dismissKeyboard {
    [TF_Email resignFirstResponder];
}

-(void)initProgress
{
    activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    CGRect MainFrame = [[self view] frame];
    [activityIndicator setFrame:CGRectMake((MainFrame.size.width - PROGRESS_INDICATOR_SIZE)/2,
                                           (MainFrame.size.height - PROGRESS_INDICATOR_SIZE)/2,
                                           PROGRESS_INDICATOR_SIZE, PROGRESS_INDICATOR_SIZE)];
    [self.view addSubview:activityIndicator];
}

-(void)showProgress: (bool) show
{
    if (show)
        [activityIndicator startAnimating];
    else
        [activityIndicator stopAnimating];
}

@end
