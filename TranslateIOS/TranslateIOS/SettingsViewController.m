//
//  SettingsViewController.m
//  Translate-Net
//
//  Created by Admin on 06.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TNApp.h"
#import "SettingsViewController.h"
#import "UserSettingsViewController.h"
#import "PhoneSettingsDialogViewController.h"
#import "HistoryViewController.h"
#import "NetworkClient.h"

@implementation SettingsViewController
-(IBAction)UserSettingsClick:(id)sender
{
    [self performSegueWithIdentifier:@"SettingsToUserSettings" sender:self];
}

-(IBAction)PhoneSettingsClick:(id)sender
{
    [self performSegueWithIdentifier:@"SettingsToPhoneSettings" sender:self];
}

-(IBAction)HistoryClick:(id)sender
{
    [self performSegueWithIdentifier:@"SettingsToHistory" sender:self];
}


-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"SettingsToUserSettings"]) {
        UserSettingsViewController *UserSettingsVC = [segue destinationViewController];
        UserSettingsVC->SettingsVC = self;
    } else if ([[segue identifier] isEqualToString:@"SettingsToPhoneSettings"]) {
        PhoneSettingsViewController *PhoneSettingsVC = [segue destinationViewController];
        PhoneSettingsVC->SettingsVC = self;
    } else if ([[segue identifier] isEqualToString:@"SettingsToHistory"]) {
        HistoryViewController *HistoryVC = [segue destinationViewController];
        HistoryVC->SettingsVC = self;
    }
}

- (IBAction)unwindToSettings:(UIStoryboardSegue *)unwindSegue
{
}

-(IBAction)DisconnectClick:(id)sender
{
    [self Relogin];
}

-(void)Relogin
{
    [NetworkClient.client Disconnect];
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
}

-(void)InitControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"act_settings_user"];
    [TNApp setUIButtonText:_B_UserSettings text:text font:font];
   
    text = [TNApp getString:@"act_settings_phone"];
    [TNApp setUIButtonText:_B_PhoneSettings text:text font:font];
    
    text = [TNApp getString:@"act_history"];
    [TNApp setUIButtonText:_B_History text:text font:font];
    
    text = [TNApp getString:@"disconnect"];
    [TNApp setUIButtonText:_B_Disconnect text:text font:font];
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];
}

-(IBAction)onBackPressed:(id)sender
{
    if (SegueFromTList)
        [self performSegueWithIdentifier:@"unwindToTList" sender:self];
    else if (SegueFromPhonecall)
        [self performSegueWithIdentifier:@"unwindToPCall" sender:self];
    SegueFromPhonecall = false;
    SegueFromTList = false;
}

-(void)setFromTList
{
    SegueFromTList = true;
    SegueFromPhonecall = false;
}

-(void)setFromPhonecall
{
    SegueFromPhonecall = true;
    SegueFromTList = false;
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    [self InitControls];
}
@end
