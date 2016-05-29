//
//  User.h
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef TranslateIOS_User_h
#define TranslateIOS_User_h

#import "Options.h"
#import "Parser.h"

#define PHONE_STATUS_NONE           0
#define PHONE_STATUS_CONFIRMED      1
#define PHONE_STATUS_AWAIT          2


@interface User : ParsedPacket<NSCopying> {
@public
    int ID;
    bool isTranslator;
    bool _exist_isTranslator;

    NSString *email;
    NSString *name;
    int phone_status;
    NSString *phone;
    NSString *await_phone;
    int balance;
    
    NSMutableDictionary *translate;
    NSString *_cur_lang;
    int _cur_price;

    NSString *country;
    float rating;           // 0-100
    int rating_num;
    int fee_market;
    int fee_app;
    
    bool Delete;
    bool _exist_Delete;
    bool Busy;
    bool _exist_Busy;
    
    bool await, confirmed, rejected, error;

    NSString *lang;

    Options *options;
};
-(bool) CheckLangs;
@end

#endif
