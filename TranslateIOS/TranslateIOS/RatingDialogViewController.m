//
//  RatingViewController.m
//  Translate-Net
//
//  Created by Admin on 25.05.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RatingDialogViewController.h"
#import "TNApp.h"

@implementation RatingDialogViewController


-(IBAction)onButtonPressed_Rate: (id)sender
{
    float rating = [_RatingBar rating]*2;
    if (translator != nil)
        [TNApp sendPacket_MarkRating:translator->ID rating:rating];
    [self onButtonPressed_Back:sender];
}

-(IBAction)onButtonPressed_Back: (id)sender
{
    if (HistoryVC != nil)
        [self performSegueWithIdentifier:@"unwindToHistory" sender:self];
    else if (PCallVC != nil)
        [self performSegueWithIdentifier:@"unwindToPCall" sender:self];
}

-(void)setPCallVC:(PhonecallViewController *)vc
{
    PCallVC = vc;
    HistoryVC = nil;
}

-(void)setHistoryVC:(HistoryViewController *)vc
{
    HistoryVC = vc;
    PCallVC = nil;
}

-(void)setTranslator:(User *)_translator _time:(NSDate *)_time
{
    translator = _translator;
    time = _time;
}

-(void)Relogin
{
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
}

-(void)InitControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    UIFont *largeFont = [UIFont systemFontOfSize:LARGE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];
    
    NSString *name = [TNApp getString:@"default_value"];
    if (translator != nil)
        name = [translator->name copy];
    text = [[TNApp getString:@"rating_descr"] stringByAppendingString:name];
    [TNApp setUILabelText:_L_Descr text:text font:largeFont];

    NSString *t = [TNApp getString:@"default_value"];
    if (time != nil) {
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setDateFormat:@"yy-MM-dd HH:mm:ss"];
        t = [formatter stringFromDate:time];
    }
    text = [[TNApp getString:@"time"] stringByAppendingString:t];
    [TNApp setUILabelText:_L_Time text:text font:largeFont];
    int height = [_L_Time frame].size.height;
    
    _RatingBar = [[TNRateView alloc] init];
    [_RatingBar setNotSelectedImage:[UIImage imageNamed:IMAGE_RATING_EMPTY]];
    [_RatingBar setHalfSelectedImage:[UIImage imageNamed:IMAGE_RATING_HALF]];
    [_RatingBar setFullSelectedImage:[UIImage imageNamed:IMAGE_RATING_FULL]];
    [_RatingBar setMaxRating:5];
    _RatingBar.editable = true;
    if (translator != nil)
        _RatingBar.rating = translator->rating/20;
    _RatingBar.translatesAutoresizingMaskIntoConstraints = false;
    [self.view addSubview:_RatingBar];
    NSLayoutConstraint *constraint_Vert_RatingBar = [NSLayoutConstraint constraintWithItem:self.L_Time attribute:NSLayoutAttributeBottom
                relatedBy:NSLayoutRelationEqual toItem:self.RatingBar
                attribute:NSLayoutAttributeTopMargin multiplier:1.0 constant:PADDING_VERT];
    [self.view addConstraint:constraint_Vert_RatingBar];
    NSLayoutConstraint *constraint_Hor_RatingBar = [NSLayoutConstraint constraintWithItem:self.RatingBar attribute:NSLayoutAttributeLeadingMargin
                relatedBy:NSLayoutRelationEqual toItem:self.view
                attribute:NSLayoutAttributeLeft multiplier:1.0 constant:PADDING_HOR];
    [self.view addConstraint:constraint_Hor_RatingBar];
    NSLayoutConstraint *constraint_Height_RatingBar = [NSLayoutConstraint constraintWithItem:self.RatingBar attribute:NSLayoutAttributeHeight
                relatedBy:NSLayoutRelationEqual toItem:nil
                attribute:NSLayoutAttributeNotAnAttribute multiplier:1.0 constant:height*2];
    [self.view addConstraint:constraint_Height_RatingBar];
    NSLayoutConstraint *constraint_Width_RatingBar = [NSLayoutConstraint constraintWithItem:self.RatingBar attribute:NSLayoutAttributeWidth
                relatedBy:NSLayoutRelationEqual toItem:nil
                attribute:NSLayoutAttributeNotAnAttribute multiplier:1.0 constant:RATING_WIDTH*4];
    [self.view addConstraint:constraint_Width_RatingBar];
    
    text = [TNApp getString:@"rate"];
    [TNApp setUIButtonText:_B_Rate text:text font:font];
    NSLayoutConstraint *constraint_Vert_Rate = [NSLayoutConstraint constraintWithItem:self.B_Rate attribute:NSLayoutAttributeTop
                relatedBy:NSLayoutRelationEqual toItem:self.RatingBar
                attribute:NSLayoutAttributeBottomMargin multiplier:1.0 constant:PADDING_VERT];
    [self.view addConstraint:constraint_Vert_Rate];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    [self InitControls];
}

@end