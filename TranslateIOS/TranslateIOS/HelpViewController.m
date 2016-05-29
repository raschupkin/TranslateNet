//
//  HelpViewController.m
//  Translate-Net
//
//  Created by Admin on 24.05.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HelpViewController.h"
#import "TNApp.h"

@implementation HelpViewController


-(void)InitControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    [self InitControls];
}

@end