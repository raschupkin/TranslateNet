//
//  ViewController.m
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import "LoginViewController.h"
#import "TNApp.h"
#import "Lang.h"
#import "User.h"

@interface LoginViewController ()

@end
@implementation LoginViewController

-(int)onPacketError:(Packet_Error *)p
{
    if (p == nil || p->command == nil)
        return -1;
    if (p->code == ERROR_VERSION) {
        [NetworkClient.client Disconnect];
        [self showProgress:false];
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:p->code]];
        return -1;
    }
    if (p->code == ERROR_PHONE_CHANGED)
        return [TNApp onPacketError:p];
    if ([p->command compare:@"login"] == 0) {
        switch (p->code) {
            case ERROR_NO_PHONE:
            case ERROR_PHONE_AWAITING:
            case ERROR_NOERROR: {
                NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
                [userDefaults setObject:mEmail forKey:DEFAULTS_EMAIL];
                [userDefaults setObject:mPassword forKey:DEFAULTS_PASSWORD];
                [userDefaults synchronize];
                // sendPacket_SetBusy:true    for translator
                [TNApp sendPacket_GetLanguages];
                return 0;
            }
            case ERROR_NO_USER:
            case ERROR_ANOTHER_LOGIN: {
                [NetworkClient.client Disconnect];
                [TNApp UserMessage: [TNApp getString: @"error"] message:[TNApp getErrorMessage:p->code]];
                return 0;
            }
            case ERROR_WRONG_PASSWORD: {
                [NetworkClient.client Disconnect];
                [TNApp UserMessage: [TNApp getString: @"error"] message:[TNApp getErrorMessage:p->code]];
                return 0;
            }
            default: {
                [NetworkClient.client Disconnect];
                [TNApp UserMessage: [TNApp getString: @"error"] message:[TNApp getErrorMessage:p->code]];
                return 0;
            }
        }
    }
    [TNApp onPacketError: p];
    return 0;
}

-(int)onPacketLanguages: (Packet_Languages *) p
{
    [self showProgress:false];
    [self performSegueWithIdentifier:@"LoginToTList" sender:self];
    return 0;
}

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if (packet == nil)
        return 0;
    if ([packet isKindOfClass:[Packet_Error class]])
        return [self onPacketError: (Packet_Error *)packet];
    else if ([packet isKindOfClass:[Packet_Languages class]])
        return [self onPacketLanguages:(Packet_Languages *)packet];
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
            if (isLoginPressed)
                [TNApp.class UserMessage: [TNApp getString: @"error_network"]
                                 message: [TNApp getString: @"network_error_connect"]];
            isLoginPressed = false;
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

-(IBAction)loginClick:(id)sender
{
    [NetworkClient.client Disconnect];
    
    mEmail = TF_Email.text;
    mEmail = [TNApp removeTrailingSpaces:mEmail];
    if (![TNApp checkEmail:mEmail]) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"error_invalid_email"]];
        return;
    }
    mPassword = TF_Password.text;
    if ([NetworkClient.client Connect]) {
        [self showProgress:false];
        [TNApp.class UserMessage: [TNApp getString:@"error_network"]
                                           message: [TNApp getString:@"network_error_connection"]];
        return;
    }
    
    isLoginPressed = true;
    [TNApp sendPacket_Login:mEmail password:mPassword];
    [self showProgress:true];
    //    NSLog([NSString stringWithFormat:@"%d" , pr.intValue]);
}


-(int)loadCountries
{
    NSString *countriesFile = [[NSBundle mainBundle] pathForResource: @"countries" ofType: @"xml"];
    NSData *countriesData = [NSData dataWithContentsOfFile:countriesFile];
    if (countriesData == nil)
        return 0;
    [parser parsePacket:countriesData];
    return -1;
}

-(void)viewDidAppear:(BOOL)animated
{
    [NetworkClient.client setReceiver:self];    // in viewDidLoad too
    
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    mEmail = [userDefaults stringForKey:DEFAULTS_EMAIL];
    mPassword = [userDefaults stringForKey:DEFAULTS_PASSWORD];
    TF_Email.text = mEmail;
    TF_Password.text = mPassword;
}

-(void)initControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"email"];
    [TNApp setUILabelText:L_Email text:text font:font];
    
    text = [TNApp getString:@"password"];
    [TNApp setUILabelText:L_Password text:text font:font];
    
    [TF_Password setSecureTextEntry:true];
    
    text = [TNApp getString:@"action_sign_in"];
    [TNApp setUIButtonText:B_Login text:text font:font];
    
    text = [TNApp getString:@"action_register"];
    [TNApp setUIButtonText:B_Register text:text font:font];
    
    text = [TNApp getString:@"action_reset"];
    [TNApp setUIButtonText:B_Reset text:text font:font];
}

-(void)viewDidLoad
{
    [super viewDidLoad];

    // Do any additional setup after loading the view, typically from a nib.
    [TNApp init: @"en"];
    
    [self initControls];
    [self initProgress];
    if (NetworkClient.client == nil)
        [NetworkClient init];
    [NetworkClient.client setReceiver:self];

    parser = [[Parser alloc] init];
    [Lang init];
    [Country init];
    [self loadCountries];
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [[self view] endEditing:YES];
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

- (IBAction)unwindToLogin:(UIStoryboardSegue *)unwindSegue
{
}

-(void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
