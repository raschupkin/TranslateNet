//
//  Lang.m
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Lang.h"

@implementation Lang : NSObject
static NSMutableArray *Langs;
+ (NSMutableArray *) Langs {    return Langs;   }

+ (void) init
{
    Langs = [[NSMutableArray alloc] init];
}

+ (bool) isLang: (NSString *) code
{
    if (code == nil)
        return false;
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->code isEqualToString: code] && !l->isGroup)
            return true;
    }
    return false;
}

+ (NSString *) CodeToLang: (NSString *) code
{
    if (code == nil)
        return @"";
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->code isEqualToString: code])
            return l->name;
    }
    return @"";
}

+ (NSString *) CodeToNative: (NSString *) code
{
    if (code == nil)
        return @"";
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->code isEqualToString: code])
            return l->nativeName;
    }
    return @"";
}

+ (NSString *) LangToCode: (NSString *) name
{
    if (name == nil)
        return @"";
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->name isEqualToString: name])
            return l->code;
    }
    return @"";
}

+ (NSString *) NativeToCode: (NSString *) native;
{
    if (native == nil)
        return @"";
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->nativeName isEqualToString: native])
            return l->code;
    }
    return @"";
}

+ (Lang *) getLangByCode: (NSString *) code
{
    if (code == nil)
        return nil;
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->code isEqualToString: code])
            return l;
    }
    return nil;
}

+ (Lang *) getLangByName: (NSString *) name
{
    if (name == nil)
        return nil;
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->name isEqualToString: name])
            return l;
    }
    return nil;
}

+ (bool) isGroup: (NSString *) name
{
    if (name == nil)
        return false;
    for (int i=0; i < Langs.count; i++) {
        Lang *l = [Langs objectAtIndex:i];
        if ([l->name isEqualToString: name])
            return l->isGroup;
    }
    return false;
}

+ (int) getAvgPrice: (NSString *) code
{
    if (code == nil)
        return 0;
    Lang *l = [Lang getLangByCode: code];
    if (l == nil)
        return 0;
    return l->avg_price;
    
}

@end
