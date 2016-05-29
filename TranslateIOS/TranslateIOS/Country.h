//
//  Country.h
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef TranslateIOS_Country_h
#define TranslateIOS_Country_h

#define COUNTRY_UNKNOWN @"--"

@interface Country : NSObject {
@public
    NSString *name;
    NSString *code;
    NSString *iso3;
    NSString *lang;
    NSString *langs;
}
+ (void) init;
+ (NSMutableArray *) Countries;
+ (NSMutableDictionary *) MCC;

+ (bool) isCountry: (NSString *) code;
+ (NSString *) CodeToCountry: (NSString *) code;
+ (NSString *) CountryToCode: (NSString *) name;
+ (Country *) getCountryByCode: (NSString *) code;
+ (Country *) getCountryByName: (NSString *) name;
+ (NSString *) getCountryLang: (NSString *) lang;
+ (NSString *) mccToCountry: (int) mcc;
@end

#endif
