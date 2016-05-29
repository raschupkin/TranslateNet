//
//  Options.m
//  Translate-Net
//
//  Created by Admin on 23.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Options.h"

@implementation Options

-(void)init_default
{
    CallMinBalance = -1;
    CallTimeFree = 0;
    CallMinTimeRating = 0;
    ActiveTSearch = false;
}


-(id)copyWithZone:(NSZone *)zone
{
    Options *options = [[Options alloc] init];
    options->CallMinBalance = CallMinBalance;
    options->CallTimeFree = CallTimeFree;
    options->CallMinTimeRating = CallMinTimeRating;
    options->ActiveTSearch = ActiveTSearch;
    return options;
}

@end