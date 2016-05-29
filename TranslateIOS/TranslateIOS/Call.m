//
//  Call->m
//  Translate-Net
//
//  Created by Admin on 31->12->14->
//  Copyright (c) 2014 Translate-Net-> All rights reserved->
//

#import <Foundation/Foundation.h>
#import "Call.h"

@implementation Call : ParsedPacket

-(void)initFromPacket:(Packet_PhonecallStatus *)p user:(User *)user
{
    active = p->active;
    if (client != nil)
        client->balance = p->balance;
    length = p->time;
    cost = p->cost;
    TranslateLang = [p->translate_lang copy];
    ClientLang = [p->client_lang copy];
    if (user->isTranslator) {
        client->ID = p->peer;
        translator->ID = user->ID;
    } else {
        client->ID = user->ID;
        translator->ID = p->peer;
    }
    if (translator != nil)
        translator->name = [p->translator_name copy];
    if (client != nil)
        client->name = [p->client_name copy];
    price = p->price;
}

-(id)copyWithZone:(NSZone *)zone
{
    Call *c = [[Call alloc] init];
    c->active = active;
    c->client = [client copyWithZone:zone];
    c->translator = [translator copyWithZone:zone];
    c->cost = cost;
    c->TranslateLang = [TranslateLang copyWithZone:zone];
    c->ClientLang = [ClientLang copyWithZone:zone];
    c->price = price;
    c->start = [start copyWithZone:zone];
    c->end = [end copyWithZone:zone];
    c->length = length;
    c->displayed = displayed;
    return c;
}

@end
