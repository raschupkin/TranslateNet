//
//  Country.m
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Country.h"
#import "Lang.h"

@implementation Country
static NSMutableArray *Countries;
+ (NSMutableArray *) Countries {    return Countries;   }
static NSMutableDictionary *MCC;
+ (NSMutableDictionary *) MCC {  return MCC; }


+ (void) init
{
    Countries = [[NSMutableArray alloc] init];
    MCC = [[NSMutableDictionary alloc] init];
    [self initMCC];
}

+ (bool) isCountry: (NSString *) code
{
    if (code == nil)
        return false;
    for (int i=0; i < Countries.count; i++) {
        Country *c = [Countries objectAtIndex:i];
        if ([c->code isEqualToString: code])
            return true;
    }
    return false;
}

+ (NSString *) CodeToCountry: (NSString *) code
{
        if (code == nil)
            return @"";
        for (int i=0; i < Countries.count; i++) {
            Country *c = [Countries objectAtIndex:i];
            if ([c->code isEqualToString: code])
                return c->name;
        }
        return @"";
}

+ (NSString *) CountryToCode: (NSString *) name
{
    if (name == nil)
        return @"";
    for (int i=0; i < Countries.count; i++) {
        Country *c = [Countries objectAtIndex:i];
        if ([c->name isEqualToString: name])
            return c->code;
    }
    return @"";
}

+ (Country *) getCountryByCode: (NSString *) code
{
    if (code == nil)
        return nil;
    for (int i=0; i < Countries.count; i++) {
        Country *c = [Countries objectAtIndex:i];
        if ([c->code isEqualToString: code])
            return c;
    }
    return nil;
}

+ (Country *) getCountryByName: (NSString *) name
{
    if (name == nil)
        return nil;
    for (int i=0; i < Countries.count; i++) {
        Country *c = [Countries objectAtIndex:i];
        if ([c->name isEqualToString: name])
            return c;
    }
    return nil;
}

+ (NSString *) getCountryLang:(NSString *)country
{
    if (country == nil)
        return LANG_DEFAULT;
    Country *c = [[self class] getCountryByCode: country];
    if (c == nil)
        return LANG_DEFAULT;
    return c->lang;
}

+ (NSString *) mccToCountry: (int) mcc
{
    if (MCC == nil)
        return COUNTRY_UNKNOWN;
    if ([MCC objectForKey: [NSNumber numberWithInt:mcc]] != nil)
        return MCC[[NSNumber numberWithInt:mcc]];
    else
        return COUNTRY_UNKNOWN;
}

+ (void) initMCC
{
    if (MCC == nil)
        return;
    MCC[@412] = @"af";
    MCC[@276] = @"al";
    MCC[@603] = @"dz";
    MCC[@213] = @"ad";
    MCC[@631] = @"ao";
    MCC[@365] = @"ai";
    MCC[@344] = @"ag";
    MCC[@722] = @"ar";
    MCC[@283] = @"am";
    MCC[@363] = @"aw";
    MCC[@505] = @"au";
    MCC[@232] = @"at";
    MCC[@400] = @"az";
    MCC[@426] = @"bh";
    MCC[@470] = @"bd";
    MCC[@342] = @"bb";
    MCC[@257] = @"by";
    MCC[@206] = @"be";
    MCC[@702] = @"bz";
    MCC[@616] = @"bj";
    MCC[@402] = @"bt";
    MCC[@218] = @"ba";
    MCC[@736] = @"bo";
    MCC[@652] = @"bw";
    MCC[@724] = @"br";
    MCC[@348] = @"vg";
    MCC[@528] = @"bn";
    MCC[@284] = @"bg";
    MCC[@613] = @"bf";
    MCC[@642] = @"bi";
    MCC[@456] = @"kh";
    MCC[@624] = @"cm";
    MCC[@302] = @"ca";
    MCC[@625] = @"cv";
    MCC[@346] = @"ky";
    MCC[@623] = @"cf";
    MCC[@622] = @"td";
    MCC[@730] = @"cl";
    MCC[@460] = @"cn";
    MCC[@732] = @"co";
    MCC[@654] = @"km";
    MCC[@629] = @"cd";
    MCC[@548] = @"ck";
    MCC[@712] = @"cr";
    MCC[@219] = @"hr";
    MCC[@368] = @"cu";
    MCC[@280] = @"cy";
    MCC[@230] = @"cz";
    MCC[@630] = @"cd";
    MCC[@238] = @"dk";
    MCC[@638] = @"dj";
    MCC[@370] = @"do";
    MCC[@740] = @"ec";
    MCC[@602] = @"eg";
    MCC[@706] = @"sv";
    MCC[@248] = @"es";
    MCC[@636] = @"et";
    MCC[@288] = @"fo";
    MCC[@542] = @"fj";
    MCC[@244] = @"fi";
    MCC[@208] = @"fr";
    MCC[@547] = @"pf";
    MCC[@628] = @"ga";
    MCC[@607] = @"gm";
    MCC[@282] = @"ge";
    MCC[@262] = @"de";
    MCC[@620] = @"gh";
    MCC[@266] = @"gi";
    MCC[@202] = @"gr";
    MCC[@290] = @"gl";
    MCC[@340] = @"gp";
    MCC[@704] = @"gt";
    MCC[@611] = @"gq";
    MCC[@632] = @"gw";
    MCC[@738] = @"gy";
    MCC[@708] = @"hn";
    MCC[@454] = @"hk";
    MCC[@216] = @"hu";
    MCC[@274] = @"is";
    MCC[@404] = @"in";
    MCC[@510] = @"id";
    MCC[@432] = @"ir";
    MCC[@418] = @"iq";
    MCC[@272] = @"ie";
    MCC[@425] = @"il";
    MCC[@222] = @"it";
    MCC[@612] = @"ic";
    MCC[@338] = @"jm";
    MCC[@440] = @"jp";
    MCC[@416] = @"jo";
    MCC[@401] = @"kz";
    MCC[@639] = @"ke";
    MCC[@450] = @"kp";
    MCC[@450] = @"kr";
    MCC[@419] = @"kw";
    MCC[@437] = @"kg";
    MCC[@457] = @"la";
    MCC[@247] = @"lv";
    MCC[@415] = @"lb";
    MCC[@651] = @"ls";
    MCC[@618] = @"lr";
    MCC[@606] = @"ly";
    MCC[@295] = @"li";
    MCC[@246] = @"lt";
    MCC[@270] = @"lu";
    MCC[@455] = @"mo";
    MCC[@294] = @"mk";
    MCC[@646] = @"mg";
    MCC[@650] = @"mw";
    MCC[@502] = @"my";
    MCC[@472] = @"mv";
    MCC[@610] = @"ml";
    MCC[@278] = @"mt";
    MCC[@609] = @"mr";
    MCC[@617] = @"mu";
    MCC[@334] = @"mx";
    MCC[@550] = @"fm";
    MCC[@259] = @"md";
    MCC[@208] = @"mc";
    MCC[@428] = @"mn";
    MCC[@220] = @"me";
    MCC[@604] = @"ma";
    MCC[@643] = @"mz";
    MCC[@414] = @"mm";
    MCC[@649] = @"ma";
    MCC[@429] = @"np";
    MCC[@204] = @"nl";
    MCC[@362] = @"an";
    MCC[@546] = @"nc";
    MCC[@530] = @"nz";
    MCC[@710] = @"ni";
    MCC[@614] = @"ne";
    MCC[@621] = @"ng";
    MCC[@242] = @"no";
    MCC[@422] = @"om";
    MCC[@410] = @"pk";
    MCC[@552] = @"pw";
    MCC[@714] = @"pa";
    MCC[@537] = @"pg";
    MCC[@744] = @"py";
    MCC[@716] = @"pe";
    MCC[@260] = @"pl";
    MCC[@268] = @"pt";
    MCC[@427] = @"qa";
    MCC[@647] = @"re";
    MCC[@226] = @"ro";
    MCC[@250] = @"ru";
    MCC[@635] = @"rw";
    MCC[@308] = @"pm";
    MCC[@549] = @"ws";
    MCC[@292] = @"sm";
    MCC[@222] = @"it";
    MCC[@626] = @"st";
    MCC[@420] = @"sa";
    MCC[@608] = @"sn";
    MCC[@220] = @"cs";
    MCC[@633] = @"sc";
    MCC[@619] = @"sl";
    MCC[@525] = @"sg";
    MCC[@231] = @"sk";
    MCC[@293] = @"si";
    MCC[@655] = @"sa";
    MCC[@214] = @"es";
    MCC[@413] = @"lk";
    MCC[@634] = @"sd";
    MCC[@746] = @"sr";
    MCC[@653] = @"sz";
    MCC[@240] = @"se";
    MCC[@228] = @"ch";
    MCC[@417] = @"sy";
    MCC[@436] = @"tj";
    MCC[@466] = @"tw";
    MCC[@640] = @"tz";
    MCC[@520] = @"th";
    MCC[@615] = @"tg";
    MCC[@539] = @"to";
    MCC[@374] = @"tt";
    MCC[@605] = @"tn";
    MCC[@286] = @"tr";
    MCC[@438] = @"tm";
    MCC[@641] = @"ug";
    MCC[@255] = @"ua";
    MCC[@424] = @"ae";
    MCC[@430] = @"ae";
    MCC[@431] = @"ae";
    MCC[@424] = @"ae";
    MCC[@424] = @"ae";
    MCC[@424] = @"ae";
    MCC[@234] = @"gb";
    MCC[@310] = @"us";
    MCC[@311] = @"us";
    MCC[@312] = @"us";
    MCC[@313] = @"us";
    MCC[@314] = @"us";
    MCC[@315] = @"us";
    MCC[@316] = @"us";
    MCC[@748] = @"uy";
    MCC[@434] = @"uz";
    MCC[@541] = @"vu";
    MCC[@225] = @"va";
    MCC[@734] = @"ve";
    MCC[@452] = @"vn";
    MCC[@421] = @"ye";
    MCC[@645] = @"zm";
    MCC[@648] = @"zw";
    MCC[@235] = @"uk";
    MCC[@234] = @"uk";
}
@end