//
//  ResetViewController.m
//  Translate-Net
//
//  Created by Admin on 19.02.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ResetViewController.h"
#import "TNApp.h"
#import "LoginViewController.h"

@implementation ResetViewController

-(void)onPacket_Error:(int)code
{
    if (code != ERROR_NOERROR)
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:code]];
    else {
        [TNApp UserMessage:[TNApp getString:@"note"] message:[TNApp getString:@"reset_done"]];
        [self Close];
    }
}

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if (packet == nil)
        return 0;
    if ([packet isKindOfClass:[Packet_Error class]]) {
        Packet_Error *pError = (Packet_Error *)packet;
        if ([pError->command compare:@"reset_password"] == 0)
            [self onPacket_Error:pError->code];
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

-(IBAction)resetClick:(id)sender
{
    mEmail = [TF_Email text];
    mEmail = [TNApp removeTrailingSpaces:mEmail];
    if (![TNApp checkEmail:mEmail]) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"error_invalid_email"]];
        mEmail = nil;
        return;
    }
    if (![NetworkClient.client Connect])
        [TNApp sendPacket_ResetPassword: mEmail];
}

-(void)Close
{
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
}

-(BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

-(void)viewDidAppear:(BOOL)animated
{
    [NetworkClient.client setReceiver:self];    // in viewDidLoad too
}

-(IBAction)backPressed:(id)sender
{
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
}

-(void)initControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";

    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:B_Back text:text font:font];
    
    text = [TNApp getString:@"action_reset"];
    [TNApp setUILabelText:L_Descr text:text font:font];
    
    text = [TNApp getString:@"email"];
    [TNApp setUILabelText:L_EmailLabel text:text font:font];
    
    text = [TNApp getString:@"reset"];
    [TNApp setUIButtonText:B_Reset text:text font:font];
    
    text = [TNApp getString:@"reset_password_descr"];
    [TNApp setUILabelText:L_LetterDescr text:text font:font];
    [L_LetterDescr setNumberOfLines:0];
    [L_LetterDescr setLineBreakMode:NSLineBreakByWordWrapping];
    [[self view] layoutSubviews];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    
    // Do any additional setup after loading the view, typically from a nib.
    [self initControls];
    [self initProgress];
    if (NetworkClient.client == nil)
        [NetworkClient init];
    [NetworkClient.client setReceiver:self];
    [NetworkClient.client Disconnect];
    
    parser = [[Parser alloc] init];
    
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    mEmail = [userDefaults stringForKey:DEFAULTS_EMAIL];
    if (mEmail != nil)
        [TF_Email setText:mEmail];
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
