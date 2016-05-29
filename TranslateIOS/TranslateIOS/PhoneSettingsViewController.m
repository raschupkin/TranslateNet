//
//  PhoneSettingsViewController.m
//  Translate-Net
//
//  Created by Admin on 09.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PhoneSettingsViewController.h"
#import "PhoneSettingsDialogViewController.h"
#import "SettingsViewController.h"
#import "TNApp.h"

@implementation PhoneSettingsViewController

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if ([packet isKindOfClass:[Packet_Error class]]) {
        Packet_Error *p = (Packet_Error *)packet;
        if ([p->command isEqualToString:@"register_phone"])
            [self onPacketError_RegisterPhone:p];
        else if ([p->command isEqualToString:@"resend_sms"])
            [self onPacketError_ResendSMS:p];
        else if ([p->command isEqualToString:@"confirm_register_phone"])
            [self onPacketError_ConfirmRegisterPhone:p->code];
        else
            [TNApp onPacketError: p];
    } else if ([packet isKindOfClass:[Packet_AwaitPhoneConfirm class]])
        [self onPacket_AwaitPhoneConfirm];
    return 0;
}

-(int)onNetwork_Error:(int)code
{
    switch (code) {
        case NETWORK_ERROR_OTHER: {
            [TNApp UserMessage: [TNApp getString: @"error_network"]
                       message: [TNApp getErrorMessage:ERROR_OTHER]];
            //            [self dismissViewControllerAnimated:true completion:nil];
            [self Relogin];
            return -1;
        }
        case NETWORK_ERROR_CONNECTION: {
            if (TNApp.app->inForeground)
                [TNApp UserMessage: [TNApp getString: @"error_network"]
                       message: [TNApp getString: @"network_error_connection"]];
            //            [self dismissViewControllerAnimated:true completion:nil];
            [self Relogin];
            return -1;
        }
        case NETWORK_ERROR_FORMAT: {
            //[self Relogin];
            [TNApp UserMessage: [TNApp getString: @"error_network"]
                       message: [TNApp getString: @"network_error_format"]];
            return 0;
        }
    }
    return -1;
}

-(void)UpdateUserView
{
    if (user == nil)
        return;
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    switch (user->phone_status) {
        case PHONE_STATUS_NONE: {
            text = [TNApp getString:@"back"];
            [TNApp setUIButtonText:_B_Back text:text font:font];
            text = [TNApp getString:@"default_value"];
            [TNApp setUILabelText:_L_Phone text:text font:font];
            [_L_Phone setEnabled:true];
            text = [TNApp getString:@"back"];
            [TNApp setUIButtonText:_B_Back text:text font:font];
            [_L_CodeDescr setEnabled: false];
            [_TF_Code setEnabled:false];
            [_TF_Code setText:@""];
            [_B_Done setEnabled:true];
            [_B_ResendSMS setEnabled:false];
            break;
        }
        case PHONE_STATUS_CONFIRMED: {
            text = [TNApp getString:@"status_phone_confirmed"];
            [TNApp setUILabelText:_L_Status text:text font:font];
             text = user->phone;
            [TNApp setUILabelText:_L_Phone text:text font:font];
            [_L_Phone setEnabled:true];
            [_L_CodeDescr setEnabled:false];
            [_TF_Code setEnabled:false];
            [_TF_Code setText:@""];
            [_B_Done setEnabled:false];
            [_B_ResendSMS setEnabled:false];
            break;
        }
        case PHONE_STATUS_AWAIT:  {
            text = [TNApp getString:@"status_phone_await"];
            [TNApp setUILabelText:_L_Status text:text font:font];
            text = user->await_phone;
            [TNApp setUILabelText:_L_Phone text:text font:font];
            [_L_Phone setEnabled:false];
            [_L_CodeDescr setEnabled: true];
            [_TF_Code setEnabled:true];
            [_B_Done setEnabled:true];
            if (EnteredPhone != nil) {
                [_L_Phone setText:EnteredPhone];
                [_L_Phone sizeToFit];
            }
            break;
        }
    }
}

-(void)onPacketError_RegisterPhone:(Packet_Error *)p
{
    [self showProgress:false];
    switch (p->code) {
        case ERROR_NOERROR:
            [self UpdateUserView];
            [self NotifyUser_SMSSent];
            [self startTimer_ButtonResendSMS];
            break;
        case ERROR_TEMP_BLOCKED:
            TNApp.app->SMS_BlockDays = p->sms_block_days;
            TNApp.app->SMS_SentNum = p->sms_sent_num;
            [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:p->code]];
            break;
        default:
            [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"error_register_phone"]];
            break;
    }
}

-(void)NotifyUser_SMSSent
{
    [TNApp UserMessage:@"" message:[TNApp getString:@"sms_sent"]];
}

-(void)onPacket_AwaitPhoneConfirm
{
    [self showProgress:false];
    user->await_phone = [EnteredPhone copy];
    user->phone_status = PHONE_STATUS_AWAIT;
    [TNApp.app setUser:[user copy]];
    [self UpdateUserView];
    [self startTimer_ButtonResendSMS];
    [self NotifyUser_SMSSent];
}

-(void)onPacketError_ResendSMS:(Packet_Error *)p
{
    [self startTimer_ButtonResendSMS];
    switch (p->code) {
        case ERROR_NOERROR:
            [self NotifyUser_SMSSent];
            break;
        case ERROR_TEMP_BLOCKED:
        default:
            [self onPacketError_RegisterPhone:p];
            break;
    }
}

-(void)onEnteredPhoneNumber:(NSString *)phone
{
    [_B_ResendSMS setEnabled:false];
    if (user->phone_status == PHONE_STATUS_CONFIRMED) {
        if (user->phone != nil && [phone isEqualToString:user->phone])
            return;
    } else if (user->phone_status == PHONE_STATUS_AWAIT)
        if (user->await_phone != nil && [phone isEqualToString:user->await_phone])
            return;
    EnteredPhone = [phone copy];
    [self showProgress:true];
    [TNApp sendPacket_RegisterPhone:phone user_input:1];
}

-(void)onPacketError_ConfirmRegisterPhone:(int)code
{
    [self showProgress:false];
    switch (code) {
        case ERROR_NOERROR: {
            if (EnteredPhone != nil) {
                user->phone = [EnteredPhone copy];
                EnteredPhone = nil;
            } else if (user->await_phone != nil)
                user->phone = [user->await_phone copy];
            user->phone_status=  PHONE_STATUS_CONFIRMED;
            [self UpdateUserView];
            [TNApp.app setUser:[user copy]];
            [self Close];
            break;
        }
        default: {
            user = [[TNApp.app getUser] copy];
            [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:code]];
            break;
        }
    }
}

-(void)onEnteredCode:(NSString *)code
{
    if (code == nil || [code length] < CODE_MIN)
        return;
    [TNApp sendPacket_ConfirmRegisterPhone:code];
}

-(bool)checkCode:(NSString *)code
{
    if ([code length] < CODE_MIN)
        return false;
    for (int i=0; i<[code length]; i++)
        if ([code characterAtIndex:i] < '0' || [code characterAtIndex:i] > '9')
            return false;
    return true;
}

- (IBAction)doneClick:(id)sender
{
    NSString *code = [_TF_Code text];
    if (![self checkCode: code]) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"invalid_phone_code"]];
        return;
    }
    [self onEnteredCode:code];
}

- (IBAction)updatePhoneClick:(id)sender
{
    [self performSegueWithIdentifier:@"PhoneSettingsToDialog" sender:self];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"PhoneSettingsToDialog"]) {
        PhoneSettingsDialogVC = [segue destinationViewController];
        [PhoneSettingsDialogVC setPhoneSettingsVC:self];
        isShown = false;
    }
}

-(void)Close
{
    if (SettingsVC == nil || SettingsVC->SegueFromTList)
        [self performSegueWithIdentifier:@"unwindToTList" sender:self];
    else if (SettingsVC->SegueFromPhonecall)
        [self performSegueWithIdentifier:@"unwindToPCall" sender:self];
}

-(void)Relogin
{
    [TNApp.app setUser:nil];                // to skip TListVC:assertUserData
    [NetworkClient.client Disconnect];
    if (isShown) {
/*        [self dismissViewControllerAnimated:YES completion: ^{
            [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
        }];
 */       [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
    } else
        if (PhoneSettingsDialogVC != nil)
            [PhoneSettingsDialogVC performSegueWithIdentifier:@"unwindToLogin" sender:PhoneSettingsDialogVC];
}

-(IBAction)onBackPressed:(id)sender
{
    [self Close];
}

-(IBAction)onResendSMSPRessed:(id)sender
{
    if (user == nil)
        return;
    if (user->phone_status == PHONE_STATUS_AWAIT ) {
        [TNApp sendPacket_ResendSMS];
        [_B_ResendSMS setEnabled:false];
        [self startTimer_ButtonResendSMS];
    }
}

-(void)resendSMSButtonTimerHandle
{
    [_B_ResendSMS setEnabled:true];
}

-(void)startTimer_ButtonResendSMS
{
    [self cancelTimer_ButtonResendSMS];
    resendSMSButtonTimer = [NSTimer scheduledTimerWithTimeInterval:
                                            TIMER_BUTTON_RESEND_SMS target:self
                                               selector:@selector(resendSMSButtonTimerHandle) userInfo:nil repeats:false];
  
}

-(void)cancelTimer_ButtonResendSMS
{
    if (resendSMSButtonTimer != nil)
        [resendSMSButtonTimer invalidate];
    resendSMSButtonTimer = nil;
 
}

-(void)viewDidAppear:(BOOL)animated
{
    isShown = true;
    PhoneSettingsDialogVC = nil;
    [NetworkClient.client setReceiver:self];
    [self UpdateUserView];
    if (isStarting) {
        if (user->phone_status != PHONE_STATUS_CONFIRMED ||
            user->phone == nil || [user->phone length] == 0) {
            [TNApp UserMessage:nil message:[TNApp getString:@"ERROR_NO_PHONE"]];
        }
        isStarting = false;
    }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

-(void)InitControls
{
    int FontSize = [UIFont systemFontSize];
    UIFont *font = [UIFont systemFontOfSize:FontSize];
    NSString *text = @"";
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];
   
    text = [TNApp getString:@"act_settings_phone"];
    [TNApp setUILabelText:_L_Descr text:text font:font];
    
    text = [TNApp getString:@"phone"];
    [TNApp setUILabelText:_L_PhoneLabel text:text font:font];
    
    text = [TNApp getString:@"default_value"];
    [TNApp setUILabelText:_L_Phone text:text font:font];
   
    text = [TNApp getString:@"status"];
    [TNApp setUILabelText:_L_StatusLabel text:text font:font];
    
    text = [TNApp getString:@"default_value"];
    [TNApp setUILabelText:_L_Status text:text font:font];
    
    text = [TNApp getString:@"update"];
    [TNApp setUIButtonText:_B_Update text:text font:font];
    
    text = [TNApp getString:@"sms_code"];
    [TNApp setUILabelText:_L_CodeDescr text:text font:font];
    
    [_TF_Code setText:@""];
    [_TF_Code setDelegate:self];
    [_TF_Code setKeyboardType:UIKeyboardTypeDecimalPad];
    
    text = [TNApp getString:@"done"];
    [TNApp setUIButtonText:_B_Done text:text font:font];
    
    text = [TNApp getString:@"resend_sms"];
    [TNApp setUIButtonText:_B_ResendSMS text:text font:font];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    [self InitControls];
    [self initProgress];
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
                                   initWithTarget:self
                                   action:@selector(dismissKeyboard)];
    [self.view addGestureRecognizer:tap];

    [_B_ResendSMS setEnabled:false];
    
    user = [[TNApp.app getUser] copy];
}

-(void)dismissKeyboard {
    [_TF_Code resignFirstResponder];
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

- (IBAction)unwindToPhoneSettings:(UIStoryboardSegue *)unwindSegue
{
}

@end