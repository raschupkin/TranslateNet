//
//  TNApp.m
//  Translate-Net
//
//  Created by Admin on 17.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "TNApp.h"
#import "NetworkClient.h"
#import "ParserStrings.h"

@implementation TNApp
static TNApp *app;
+(TNApp *) app {    return app;   }
static NSMutableDictionary *Strings;
+(NSMutableDictionary *) Strings { return Strings;  }
static NSArray *Countries_EU;
+(NSArray *) Countries_EU { return Countries_EU;    }

-(void) instance_init
{ 
}

+(int) loadStrings:(NSString *)lang
{
    NSArray *files = @[@"strings", @"strings_activity_login", @"strings_settings", @"strings_error", @"langs"];
    NSString *dirName;
    if (lang == nil || [lang isEqualToString:@"en"])
        dirName = @"values";
    else {
        dirName = @"values-";
        dirName = [dirName stringByAppendingString:lang];
    }
    ParserStrings *parserStrings = [[ParserStrings alloc] init];
    for (int i=0; i<[files count]; i++) {
        NSString *stringsFileName = files[i];
        NSString *stringsFile = [[NSBundle mainBundle] pathForResource: stringsFileName ofType: @"xml" inDirectory: dirName];
        NSData *stringsData = [NSData dataWithContentsOfFile:stringsFile];
        if (stringsData != nil) {
            NSMutableDictionary *strings = [parserStrings parseStrings:stringsData];
            if (strings == nil) {
                [self UserMessage:[self getString:@"error"] message:[NSString stringWithFormat:@"Error loading data (%d).", ERROR_LOAD]];
                return -1;
            } else
                [Strings addEntriesFromDictionary:strings];
        } else
            return -1;
    }
    return 0;
}

+(void) init: (NSString *) lang
{
    app = [[TNApp alloc] init];
    [app instance_init];
    
    Strings = [[NSMutableDictionary alloc] init];

    lang = [self getCurrentLanguage];
    if ([self loadStrings:lang] < 0)
        [self loadStrings:LANG_DEFAULT];
    Countries_EU = [NSArray arrayWithObjects:
                             @"be", @"bg", @"cz", @"dk", @"de", @"ee", @"ie", @"el", @"es", @"fr",
                             @"hr", @"it", @"cy", @"lv", @"lt", @"lu", @"hu", @"mt", @"nl", @"at",
                             @"pl", @"pt", @"ro", @"si", @"sk", @"fi", @"se", @"uk", nil];
}

+(int)onPacketError: (Packet_Error *)p
{
    if (p == nil)
        return 0;
    switch 	(p->code) {
        case ERROR_NOERROR: {
            if ([p->command isEqualToString:@"billing"]) {
/*                if (p->money <= 0)
                    return 0;
                User *u = [app getUser];
                u->balance += p->money;
                [app setUser:u];
 */
            }
            return 0;
        }
        case ERROR_NO_USERDATA:
            break;
        case ERROR_PHONE_CHANGED:
            app->PhoneChanged = true;
            return 0;
//        case ERROR_PURCHASE_SIGNATURE:
//        case ERROR_ANOTHER_LOGIN:
        default:
            [self UserMessage:[self getString:@"error"] message:[self getErrorMessage:p->code]];
            break;
    }
    return 0;
}

+(int)sendPacket_GetLanguages
{
    NSString *xml = @"<get_languages></get_languages>";
    return [NetworkClient.client sendPacket: xml];
}

+(int)sendPacket_GetUserData
{
    NSString *xml = @"<get_user_data></get_user_data>";
    return [NetworkClient.client sendPacket: xml];
}

+(NSString *)getDeviceID
{
    NSUUID *oNSUUID = [[UIDevice currentDevice] identifierForVendor];
    return [oNSUUID UUIDString];
}

+(int)sendPacket_Login: (NSString *)email password:(NSString *)password
{
    app->PhoneChanged = false;
    
    NSString *xml = @"<login><os>ios</os>";
    xml = [xml stringByAppendingString: [NSString stringWithFormat:@"<version>%d</version>", APP_VERSION]];
    xml = [xml stringByAppendingString: @"<email>"];
    xml = [xml stringByAppendingString: email];
    xml = [xml stringByAppendingString: @"</email>"];
    xml = [xml stringByAppendingString: @"<password>"];
    xml = [xml stringByAppendingString: password];
    xml = [xml stringByAppendingString: @"</password>"];
    xml = [xml stringByAppendingString: @"<device_id>"];
    xml = [xml stringByAppendingString: [self getDeviceID]];
    xml = [xml stringByAppendingString: @"</device_id>"];
    xml = [xml stringByAppendingString: @"</login>"];
    return [NetworkClient.client sendPacket: xml];
}

+(int)sendPacket_RegisterUser:(NSString *)email isT:(BOOL)isT
{
    if (email == nil)
        return -1;
    NSString *xml = @"<register_user><email>";
    xml = [xml stringByAppendingString:email];
    xml = [xml stringByAppendingString:@"</email><is_translator>"];
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"%d", isT]];
    xml = [xml stringByAppendingString:@"</is_translator></register_user>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_ResetPassword:(NSString *)email
{
    if (email == nil)
        return -1;
    NSString *xml = @"<reset_password><email>";
    xml = [xml stringByAppendingString:email];
    xml = [xml stringByAppendingString:@"</email></reset_password>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_SetCountry:(NSString *)country
{
    if (country == nil)
        return -1;
    NSString *xml = @"<set_country><country>";
    xml = [xml stringByAppendingString:country];
    xml = [xml stringByAppendingString:@"</country></set_country>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_UserData: (User *)user
{
    if (user == nil || user->isTranslator)
        return -1;
    NSString *xml = @"<user_data>";
    if (user->name != nil) {
        xml = [xml stringByAppendingString:@"<name>"];
        xml = [xml stringByAppendingString:user->name];
        xml = [xml stringByAppendingString:@"</name>"];
    }
    xml = [xml stringByAppendingString:@"<translate><lang>"];
    xml = [xml stringByAppendingString:user->lang];
    xml = [xml stringByAppendingString:@"</lang><price>0</price></translate>"];
    xml = [xml stringByAppendingString:@"</user_data>"];
    return [NetworkClient.client sendPacket: xml];
}

+(int)sendPacket_StopTranslatorList
{
    NSString *xml = @"<stop_translator_list></stop_translator_list>";
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_RequestTranslatorList:(NSString *)ListLang
{
    NSString *xml = @"<request_translator_list>";
    xml = [xml stringByAppendingString:@"<list_lang>"];
    xml = [xml stringByAppendingString:ListLang];
    xml = [xml stringByAppendingString:@"</list_lang>"];
    xml = [xml stringByAppendingString:@"</request_translator_list>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_PhonecallRequest:(int)translator translate_lang:(NSString *)translate_lang;
{
    NSString *xml = @"<phonecall_request>";
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<translator>%d</translator>", translator]];
    xml = [xml stringByAppendingString:@"<translate_lang>"];
    xml = [xml stringByAppendingString:translate_lang];
    xml = [xml stringByAppendingString:@"</translate_lang>"];
    xml = [xml stringByAppendingString:@"</phonecall_request>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_PhonecallStatus:(bool)active time:(int)time
{
    NSString *xml = @"<phonecall_status>";
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<active>%d</active>", active]];
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<time>%d</time>", time]];
    xml = [xml stringByAppendingString:@"</phonecall_status>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_RegisterPhone:(NSString *)phone user_input:(bool)user_input
{
    NSString *xml = @"<register_phone>";
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<phone>%@</phone>", phone]];
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<user_input>%d</user_input>", user_input]];
    xml = [xml stringByAppendingString:@"</register_phone>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_ResendSMS
{
    NSString *xml = @"<resend_sms>";
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_ConfirmRegisterPhone:(NSString *)code
{
    NSString *xml = @"<confirm_register_phone>";
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<sms_code>%@</sms_code>", code]];
    xml = [xml stringByAppendingString:@"</confirm_register_phone>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_Billing:(int)money data:(NSString *)data signature:(NSString *)signature
{
    NSString *xml = @"<billing>";
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<money>%d</money>", money]];
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<data>%@</data>", data]];
//    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<signature>%@</signature>", signature]];
    xml = [xml stringByAppendingString:@"</billing>"];
    return [NetworkClient.client sendPacket:xml];
}
	
+(int)sendPacket_GetCallHistory
{
    NSString *xml = @"<get_call_history></get_call_history>";
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_GetMarkHistory
{
    NSString *xml = @"<get_mark_history></get_mark_history>";
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_MarkRating:(int)id rating:(int)rating
{
    NSString *xml = @"<mark_rating>";
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<translator>%d</translator>", id]];
    xml = [xml stringByAppendingString:[NSString stringWithFormat:@"<mark>%d</mark>", rating]];
    xml = [xml stringByAppendingString:@"</mark_rating>"];
    return [NetworkClient.client sendPacket:xml];
}

+(int)sendPacket_GetStatistic
{
    NSString *xml = @"<get_statistic></get_statistic>";
    return [NetworkClient.client sendPacket:xml];
}

+(NSString *) getErrorMessage: (int) code
{
    NSString *msg;
    switch (code) {
        case ERROR_NOERROR:
            msg = [self getString: @"ERROR_NOERROR"];
            break;
        case ERROR_OTHER:
            msg = [self getString: @"ERROR_OTHER"];
            break;
        case ERROR_MAINTENANCE:
            msg = [self getString: @"ERROR_MAINTENANCE"];
            break;
        case ERROR_VERSION:
            msg = [self getString: @"ERROR_VERSION"];
            break;
        case ERROR_LOAD:
            msg = [self getString: @"ERROR_LOAD"];
            break;
        case ERROR_FORMAT:
            msg = [self getString: @"ERROR_FORMAT"];
            break;
        case ERROR_NO_USER:
            msg = [self getString: @"ERROR_NO_USER"];
            break;
        case ERROR_USER_OFFLINE:
            msg = [self getString: @"ERROR_USER_OFFLINE"];
            break;
        case ERROR_USER_ALREADY_EXIST:
            msg = [self getString: @"ERROR_USER_ALREADY_EXIST"];
            break;
        case ERROR_ALREADY_LOGIN:
            msg = [self getString: @"ERROR_ALREADY_LOGIN"];
            break;
        case ERROR_LOGIN_REQUIRED:
            msg = [self getString: @"ERROR_LOGIN_REQUIRED"];
            break;
        case ERROR_WRONG_PASSWORD:
            msg = [self getString: @"ERROR_WRONG_PASSWORD"];
            break;
        case ERROR_WRONG_SMSCODE:
            msg = [self getString: @"ERROR_WRONG_SMSCODE"];
            break;
        case ERROR_NO_USERDATA:
            msg = [self getString: @"ERROR_NO_USERDATA"];
            break;
        case ERROR_NO_PHONE:
            msg = [self getString: @"ERROR_NO_PHONE"];
            break;
        case ERROR_PHONE_CHANGED:
            msg = [self getString: @"ERROR_PHONE_CHANGED"];
            break;
        case ERROR_NO_LANG:
            msg = [self getString: @"ERROR_NO_LANG"];
            break;
        case ERROR_PHONE_AWAITING:
            msg = [self getString: @"ERROR_PHONE_AWAITING"];
            break;
        case ERROR_TEMP_BLOCKED:
            msg = [self getString: @"ERROR_TEMP_BLOCKED_1"];
            msg = [msg stringByAppendingString:[NSString stringWithFormat:@" %d", app->SMS_BlockDays]];
            msg = [msg stringByAppendingString:[self getString: @"ERROR_TEMP_BLOCKED_2"]];
            break;
        case ERROR_UNKOWN_CALL:
            msg = [self getString: @"ERROR_UNKOWN_CALL"];
            break;
        case ERROR_CALL_EXIST:
            msg = [self getString: @"ERROR_CALL_EXIST"];
            break;
        case ERROR_CALL_STATE:
            msg = [self getString: @"ERROR_CALL_STATE"];
            break;
        case ERROR_BALANCE: {
            int min = 1;
            int sec = 0;
            if (app->user != nil && app->user->options != nil &&
                app->user->options->CallMinBalance > 0) {
                min = app->user->options->CallMinBalance / 60;
                sec = app->user->options->CallMinBalance - min * 60;
            }
            msg = [self getString: @"ERROR_BALANCE_1"];
            msg = [msg stringByAppendingString:[self getString: @"ERROR_BALANCE_2"]];
            msg = [msg stringByAppendingString:[NSString stringWithFormat:@" %d:%02d ", min, sec]];
            msg = [msg stringByAppendingString:[self getString: @"ERROR_BALANCE_3"]];
            break;
        }
        case ERROR_PHONECALL_ERROR:
            msg = [self getString: @"ERROR_PHONECALL_ERROR"];
            break;
        case ERROR_PEER_DISCON:
            msg = [self getString: @"ERROR_PEER_DISCON"];
            break;
        case ERROR_RATING_ERROR:
            msg = [self getString: @"ERROR_RATING_ERROR"];
            break;
        case ERROR_PAYPAL_TRANSFER_ACTIVE:
            msg = [self getString: @"ERROR_PAYPAL_TRANSFER_ACTIVE"];
            break;
        case ERROR_PURCHASE_SIGNATURE:
            msg = [self getString: @"ERROR_PURCHASE_SIGNATURE"];
            break;
        case ERROR_ANOTHER_LOGIN:
            msg = [self getString: @"ERROR_ANOTHER_LOGIN"];
            break;
        case ERROR_ANOTHER_PHONE:
            msg = [self getString: @"ERROR_ANOTHER_PHONE"];
            break;
        case ERROR_NAME_EXIST:
            msg = [self getString: @"ERROR_NAME_EXIST"];
            break;
        }
    if (code == ERROR_PHONE_CHANGED || code == ERROR_PEER_DISCON ||
        code == ERROR_WRONG_SMSCODE || code == ERROR_PURCHASE_SIGNATURE ||
        code == ERROR_WRONG_PASSWORD)
        return msg;
    NSString *str = @"(";
    str = [str stringByAppendingString:[NSString stringWithFormat:@"%d", code]];
    str = [str stringByAppendingString:@")"];
    str = [str stringByAppendingString:msg];
    return str;
}

+(NSString *) getLangCodeByName: (NSString *)name
{
    NSArray *codes = [Strings allKeysForObject:name];
    if (codes.count == 0)
        return [Lang LangToCode:name];
    return codes[0];
}

+(NSString *) getLangNameByCode: (NSString *)code
{
    NSString *name = [self getString:code];
    if (name == nil)
        return [Lang CodeToLang:code];
    return name;
}

+(NSString *) getString: (NSString *) name
{		
    if (Strings[name] != nil)
        return Strings[name];
    else
        return @"";
}

+(UIImage *)getCountryImageRes:(NSString *)country
{
    if ([Countries_EU containsObject: country])
        country = @"eu";

    NSString *resName = @"flag_";
    resName = [resName stringByAppendingString:country];
    return [UIImage imageNamed:resName];
}

+(NSString *)getCurrentLanguage
{
    NSString *lang = [[NSLocale preferredLanguages] objectAtIndex:0];
    return lang;
}

+(NSString *)FormatPrice:(int)money
{
    NSString *str = [NSString stringWithFormat:@"%.02f ", (float)money/100];
    return [str stringByAppendingString:[TNApp getString:@"credit"]];
}

+(NSString *)FormatDate:(NSDate *)date
{
    NSDateFormatter *format = [[NSDateFormatter alloc] init];
    [format setDateFormat:@"yy:MM:dd hh:mm"];
    return [format stringFromDate:date];
}

+(NSString *)FormatTime:(int)time
{
    int hours = time/3600;
    int minutes = (time - hours*3600)/60;
    int seconds = time - hours*360 - minutes*60;
    return [NSString stringWithFormat:@"%02d:%02d:%02d", hours, minutes, seconds];
}

+(bool)checkEmail:(NSString *)email
{
    if (email == nil || [email length] == 0)
        return false;
    for (int i=0; i<[email length]; i++)
        if ([email characterAtIndex:i] == ' ' ||
            [email characterAtIndex:i] == '<' ||
            [email characterAtIndex:i] == '>')
            return false;
    int i;
    for (i=0; i<[email length]; i++)
        if ([email characterAtIndex:i] == '@')
            break;
    if (i == [email length] || i == 0)
        return false;
    for (   ; i<[email length]; i++)
        if ([email characterAtIndex:i] == '.')
            break;
    if (i >= [email length]-1)
        return false;
    return true;
}

+(NSString *)removeTrailingSpaces:(NSString *)email
{
    for (int i=0; i<[email length]; i++)
        if ([email characterAtIndex:i] == ' ') {
            if ([email length] > 1)
                email = [email substringFromIndex:1];
            i--;
        } else
            break;
    for (int i=[email length]-1; i>0; i--)
        if ([email characterAtIndex:i] == ' ') {
            if ([email length] > i)
                email = [email substringToIndex:i];
        } else
            break;
    return email;
}


+(bool)checkPhoneNumber:(NSString *)phone;
{
    if (phone == nil || [phone length] < PHONE_MIN || [phone length] > PHONE_MAX)
        return false;
    if ([phone characterAtIndex:0] != '+')
        return false;
    for (int i = 1; i < [phone length]; i++) {
        if ([phone characterAtIndex:i] < '0' || [phone characterAtIndex:i] > '9')
            return false;
    }
    bool allZero = true;
    for (int i = 1; i < [phone length]; i++)
        if ([phone characterAtIndex:i] != '0')
            allZero = false;
    if (allZero)
        return false;
    return true;
}

-(User *)getUser
{
    return user;
}

-(void) setUser: (User *)_user
{
    user = _user;
}


+(NSString *)getCurrentCountry
{
    CTTelephonyNetworkInfo *network_Info = [CTTelephonyNetworkInfo new];
    if (network_Info == nil)
        return COUNTRY_UNKNOWN;
    CTCarrier *carrier = network_Info.subscriberCellularProvider;
    if (carrier == nil)
        return COUNTRY_UNKNOWN;
    NSString *codeStr = carrier.mobileCountryCode;
    if (codeStr == nil)
        return COUNTRY_UNKNOWN;
    int code = codeStr.intValue;
    return [Country mccToCountry:code];
}

+(void)UserMessage: (NSString *) header message:(NSString *)msg
{
    UIAlertView* alert = [[UIAlertView alloc] initWithTitle:header message:msg delegate:nil cancelButtonTitle:[TNApp getString:@"ok"] otherButtonTitles: nil];
    [alert show];
}

+(void)setUILabelText:(UILabel *)label text:(NSString *)text font:(UIFont *)font
{
    if (label == nil || text == nil || font == nil)
        return;
    CGRect frame = [label frame];
    [label setFont: font];
    [label setText: text];
    int maxWidth = label.superview.frame.size.width - label.frame.origin.x;
    CGSize s = [text sizeWithFont:font constrainedToSize:CGSizeMake(maxWidth, MAXFLOAT) lineBreakMode:NSLineBreakByWordWrapping];
    [label setFrame:CGRectMake(frame.origin.x,frame.origin.y,s.width,s.height)];
    [label setPreferredMaxLayoutWidth:s.width];
    [label sizeToFit];
    [label needsUpdateConstraints];
}

+(void)setUIButtonText:(UIButton *)button text:(NSString *)text font:(UIFont *)font
{
    if (button == nil || text == nil || font == nil)
        return;
    CGRect frame = [button frame];
    [[button titleLabel] setFont:font];
    [button setTitle:text forState:UIControlStateNormal];
    CGSize s = [text sizeWithFont:font constrainedToSize:CGSizeMake(MAXFLOAT, MAXFLOAT) lineBreakMode:NSLineBreakByWordWrapping];
    [button setFrame:CGRectMake(frame.origin.x,frame.origin.y,s.width,s.height)];
    [button sizeToFit];
}

@end