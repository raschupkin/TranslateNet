//
//  Call.h
//  Translate-Net
//
//  Created by Admin on 31.12.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_Call_h
#define Translate_Net_Call_h

#import "User.h"
#import "Parser.h"

@interface Call : ParsedPacket<NSCopying> {
@public
    BOOL active;
    User *client, *translator;
    int cost;
    NSString *TranslateLang, *ClientLang;
    int price;
    NSDate *start, *end;
    int length;
    
    bool displayed;
};
-(void)initFromPacket:(Packet_PhonecallStatus *)p user:(User *)user;
@end
#endif
