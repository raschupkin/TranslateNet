//
//  Lang.h
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef TranslateIOS_Lang_h
#define TranslateIOS_Lang_h

#define LANG_UNKNOWN    @"--"
#define LANG_DEFAULT    @"en"
#define LANG_DEFAULT2   @"fr"
@interface Lang : NSObject {
@public
    bool isGroup;
    Lang *group;
    NSString *name;
    NSString *code;
    NSString *iso3;
    NSString *country;
    NSString *nativeName;
    int avg_price;
}
+ (void) init;
+ (NSMutableArray *) Langs;

+ (bool) isLang: (NSString *) code;
+ (NSString *) CodeToLang: (NSString *) code;
+ (NSString *) CodeToNative: (NSString *) code;
+ (NSString *) LangToCode: (NSString *) name;
+ (NSString *) NativeToCode: (NSString *) native;
+ (Lang *) getLangByCode: (NSString *) code;
+ (Lang *) getLangByName: (NSString *) name;
+ (bool) isGroup: (NSString *) name;

@end


#endif
