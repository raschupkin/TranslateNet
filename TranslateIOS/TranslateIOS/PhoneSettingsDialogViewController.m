//
//  PhoneSettingsDialogViewController.m
//  Translate-Net
//
//  Created by Admin on 29.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PhoneSettingsDialogViewController.h"
#import "TNApp.h"

@implementation PhoneSettingsDialogViewController

-(void)setPhoneSettingsVC:(PhoneSettingsViewController *)vc
{
    PhoneSettingsVC = vc;
}

- (IBAction)doneClick:(id)sender
{
    NSString *phone = [_TF_Phone text];
    if (![TNApp checkPhoneNumber:phone]) {
        if (phone != nil && [phone length] > 1 && [phone characterAtIndex:0] != '+')
            [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"invalid_phone_plus"]];
            else
                [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"invalid_phone"]];
        return;
    }
    if (PhoneSettingsVC != nil)
        [PhoneSettingsVC onEnteredPhoneNumber:phone];
    [self performSegueWithIdentifier:@"unwindToPhoneSettings" sender:self];
    return;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

-(void)InitControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];
    
    [_L_Descr setNumberOfLines:0];
    [_L_Descr setLineBreakMode:NSLineBreakByWordWrapping];
    _L_Descr.preferredMaxLayoutWidth = [_L_Descr alignmentRectForFrame:_L_Descr.frame].size.width;
    text = [TNApp getString:@"settings_phone_enter_descr"];
    [TNApp setUILabelText:_L_Descr text:text font:font];

    [_TF_Phone setText:@""];
    [_TF_Phone setKeyboardType:UIKeyboardTypePhonePad];
    [_TF_Phone setDelegate:self];
    [_TF_Phone becomeFirstResponder];
    
    text = [TNApp getString:@"done"];
    [TNApp setUIButtonText:_B_Done text:text font:font];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    [self InitControls];
}

@end