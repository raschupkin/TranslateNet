//
//  User.m
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "User.h"
#import "Lang.h"

@implementation User

-(bool) CheckLangs
{
    if (isTranslator)
        return false;
    if (lang == nil)
        return false;
    if (![Lang isLang:lang])
        return false;
    return true;
}

-(id)copyWithZone:(NSZone *)zone
{
    User *u = [[User alloc] init];
    u->ID = ID;
    u->isTranslator = isTranslator;
    u->_exist_isTranslator = _exist_isTranslator;
    u->email = [email copyWithZone:zone];
    u->name = [name copyWithZone:zone];
    u->phone_status = phone_status;
    u->phone = [phone copyWithZone:zone];
    u->await_phone = [await_phone copyWithZone:zone];
    u->balance = balance;
    u->translate = [translate copyWithZone:zone];
    u->_cur_lang = [_cur_lang copyWithZone:zone];
    u->_cur_price = _cur_price;
    u->country = [country copyWithZone:zone];
    u->rating = rating;
    u->rating_num = rating_num;
    u->fee_market = fee_market;
    u->fee_app = fee_app;
    u->Delete = Delete;
    u->_exist_Delete = _exist_Delete;
    u->Busy= Busy;
    u->_exist_Busy = _exist_Busy;
    u->await = await;
    u->confirmed = confirmed;
    u->rejected = rejected;
    u->error = error;
    u->lang = [lang copyWithZone:zone];
    u->options = [options copyWithZone:zone];
    return u;
}
@end