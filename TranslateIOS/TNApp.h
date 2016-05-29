//
//  TNApp.h
//  Translate-Net
//
//  Created by Admin on 17.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_TNApp_h
#define Translate_Net_TNApp_h

#import <UIKit/UIKit.h>
#import <UIKit/UIImage.h>
#import "User.h"
#import "Call.h"
#import "Options.h"
#import "Parser.h"
@import CoreTelephony;

#define APP_VERSION 1

#define PREFERENCE_LISTLANG @"ListLang"

#define PADDING_HOR 5
#define PADDING_VERT 5
#define SMALL_FONT_SIZE 11
#define BASE_FONT_SIZE 14
#define LARGE_FONT_SIZE 17
#define PROGRESS_INDICATOR_SIZE 30
#define RATING_WIDTH 60
#define IMAGE_RATING_EMPTY    @"kermit_empty.png"
#define IMAGE_RATING_HALF    @"kermit_half.png"
#define IMAGE_RATING_FULL    @"kermit_full.png"



// global static data structure
@interface TNApp : NSObject {
@public
    BOOL PhoneChanged;              // in case receieved ERROR_PHONE_CHANGED
    int SMS_BlockDays;
    int SMS_SentNum;
    
    bool inForeground;
@protected
    User *user;
    NSMutableDictionary *Strings;
    NSArray *Countries_EU;
}
+(TNApp *) app;
+(NSMutableDictionary *) Strings;
+(NSArray *) Countries_EU;
+(Options *) options;

-(void) instance_init;
+(void) init: (NSString *) lang;
+(int)onPacketError: (Packet_Error *)p;
+(int)sendPacket_GetLanguages;
+(int)sendPacket_GetUserData;
+(int)sendPacket_Login: (NSString *)email password:(NSString *)password;
+(int)sendPacket_RegisterUser:(NSString *)email isT:(BOOL)isT;
+(int)sendPacket_ResetPassword:(NSString *)email;
+(int)sendPacket_SetCountry:(NSString *)country;
+(int)sendPacket_UserData: (User *)user;
+(int)sendPacket_StopTranslatorList;
+(int)sendPacket_RequestTranslatorList:(NSString *)ListLang;
+(int)sendPacket_PhonecallRequest:(int)translator translate_lang:(NSString *)translate_lang;
+(int)sendPacket_PhonecallStatus:(bool)active time:(int)time;
+(int)sendPacket_RegisterPhone:(NSString *)phone user_input:(bool)user_input;
+(int)sendPacket_ResendSMS;
+(int)sendPacket_ConfirmRegisterPhone:(NSString *)code;
+(int)sendPacket_Billing:(int)money data:(NSString *)data signature:(NSString *)signature;
+(int)sendPacket_GetCallHistory;
+(int)sendPacket_GetMarkHistory;
+(int)sendPacket_MarkRating:(int)id rating:(int)rating;
+(int)sendPacket_GetStatistic;

+(NSString *)getString:(NSString *) name;
+(NSString *)getErrorMessage:(int) code;
+(NSString *)getLangCodeByName:(NSString *)name;
+(NSString *)getLangNameByCode:(NSString *)code;
+(UIImage *)getCountryImageRes:(NSString *)country;
+(NSString *)getCurrentLanguage;
+(NSString *)FormatPrice:(int)money;
+(NSString *)FormatDate:(NSDate *)date;
+(NSString *)FormatTime:(int)time;
+(bool)checkEmail:(NSString *)email;
+(NSString *)removeTrailingSpaces:(NSString *)email;
#define PHONE_MIN   6
#define PHONE_MAX   15
+(bool)checkPhoneNumber:(NSString *)phone;
#define CODE_MIN    4

-(User *)getUser;
-(void) setUser: (User *)_user;
//-(Call *)getCall;
//-(void) setCall: (User *)_call;

+(NSString *)getCurrentCountry;
+(void) UserMessage: (NSString *) header message:(NSString *)msg;
+(void)setUILabelText:(UILabel *)label text:(NSString *)text font:(UIFont *)font;
+(void)setUIButtonText:(UIButton *)buttin text:(NSString *)text font:(UIFont *)font;
@end

#endif
