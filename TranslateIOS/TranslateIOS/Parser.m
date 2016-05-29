//
//  Parser.m
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//


#import <Foundation/Foundation.h>
#import "Parser.h"
#import "User.h"
#import "Call.h"
#import "Lang.h"
#import "Country.h"

@implementation ParsedPacket
@end
@implementation Packet_AwaitLoginConfirm
@end
@implementation Packet_Error
@end
@implementation Packet_TList
@end
@implementation Packet_PhonecallConfirm
@end
@implementation Packet_PhonecallTimeout
@end
@implementation Packet_PhonecallStatus
@end
@implementation Packet_AwaitPhoneConfirm
@end
@implementation Packet_Statistic
@end
@implementation Packet_Languages
@end
@implementation Packet_MarkList
@end
@implementation Packet_CallList
@end
@implementation Packet_MarkRequest
@end

@implementation Parser : NSObject

-(void)parseEnd_Error: (NSString *)elementName
{
    if ([elementName isEqualToString:@"error"]) {
        if (pError->command.length == 0)
            error = true;
    } else if ([elementName isEqualToString: @"code"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pError->code = currentData.intValue;
    } else if ([elementName isEqualToString: @"command"]) {
        pError->command = [currentData copy];
    } else if ([elementName isEqualToString: @"translator"]) {
        pError->phonecall_request_translator = currentData.intValue;
    } else if ([elementName isEqualToString: @"money"]) {
        pError->money = currentData.intValue;
    } else if ([elementName isEqualToString: @"id"]) {
        pError->ID = currentData.intValue;
    } else if ([elementName isEqualToString: @"sms_sent_num"]) {
        pError->sms_sent_num = currentData.intValue;
    } else if ([elementName isEqualToString: @"sms_block_days"]) {
        pError->sms_block_days = currentData.intValue;
    }
}

-(void)parseEnd_UserData: (NSString *)elementName
{
    if ([elementName isEqualToString: @"user_data"]) {
        if (pUserData->email.length == 0 || !pUserData->_exist_isTranslator)
            error = 1;
        if (!pUserData->isTranslator) {
            pUserData->lang = LANG_DEFAULT;
            if (pUserData->translate.count > 0)
                pUserData->lang = [[pUserData->translate allKeys] objectAtIndex: 0];
        }
    } else if ([elementName isEqualToString: @"is_translator"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->isTranslator = currentData.intValue;
        pUserData->_exist_isTranslator = true;
    } else if ([elementName isEqualToString: @"email"]) {
        pUserData->email = [currentData copy];
    } else if ([elementName isEqualToString: @"name"]) {
        pUserData->name = [currentData copy];
    } else if ([elementName isEqualToString: @"phone_status"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->phone_status = currentData.intValue;
    } else if ([elementName isEqualToString: @"phone"]) {
        pUserData->phone = [currentData copy];
    } else if ([elementName isEqualToString: @"await_phone"]) {
        pUserData->await_phone = [currentData copy];
    } else if ([elementName isEqualToString: @"balance"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->balance = currentData.intValue;
    } else if ([elementName isEqualToString: @"translate"]) {
        if (pUserData->_cur_lang.length > 0 &&  pUserData->_cur_price >= 0)
            [pUserData->translate
             setObject: [NSNumber numberWithInt: pUserData->_cur_price]
             forKey: pUserData->_cur_lang];
        else
            error = true;
        pUserData->_cur_lang = @"";
        pUserData->_cur_price = -1;
    } else if ([elementName isEqualToString: @"lang"]) {
        if (![Lang isLang: currentData])
            error = 1;
        else
            pUserData->_cur_lang = [currentData copy];
    } else if ([elementName isEqualToString: @"price"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->_cur_price = currentData.intValue;
    } else if ([elementName isEqualToString: @"rating"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->rating = currentData.intValue;
    } else if ([elementName isEqualToString: @"rating_num"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->rating_num = currentData.intValue;
    } else if ([elementName isEqualToString: @"fee_market"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->fee_market = currentData.intValue;
    } else if ([elementName isEqualToString: @"fee_app"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->fee_app = currentData.intValue;
    } else if ([elementName isEqualToString: @"call_time_free_sec"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->options->CallTimeFree = currentData.intValue;
    } else if ([elementName isEqualToString: @"call_min_balance_sec"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->options->CallMinBalance = currentData.intValue;
    } else if ([elementName isEqualToString: @"call_min_time_rating"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->options->CallMinTimeRating = currentData.intValue;
    } else if ([elementName isEqualToString: @"active_tsearch"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pUserData->options->ActiveTSearch = currentData.intValue > 0;
    }
}

-(void)parseStart_TranslatorList:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    if ([elementName isEqualToString:@"translator_list"]) {
        _cur_t = nil;
    } else if ([elementName isEqualToString:@"translator"]) {
        _cur_t = [[User alloc] init];
        _cur_t->options = [[Options alloc] init];
        _cur_t->translate = [[NSMutableDictionary alloc] init];
        _cur_t->_cur_lang = @"";
        _cur_t->_cur_price = -1;
    }
}

-(void)parseEnd_TranslatorList: (NSString *) elementName
{
    if ([elementName isEqualToString: @"translator_list"]) {
    } else if ([elementName isEqualToString:@"translator"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        if (_cur_t->ID == 0 || !_cur_t->_exist_Delete ||
            (!_cur_t->Delete && (_cur_t->name.length == 0 || _cur_t->translate == nil ||
                                _cur_t->country.length == 0 || !_cur_t->_exist_Busy))) {
            error = true;
            return;
        }
        [pTList->tlist addObject:_cur_t];
        _cur_t = nil;
    } else if ([elementName isEqualToString:@"id"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->ID = currentData.intValue;
    } else if ([elementName isEqualToString:@"delete"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->Delete = currentData.intValue;
        _cur_t->_exist_Delete = true;
    } else if ([elementName isEqualToString:@"busy"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->Busy = currentData.intValue;
        _cur_t->_exist_Busy = true;
    } else if ([elementName isEqualToString:@"name"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->name = [currentData copy];
    } else if ([elementName isEqualToString:@"country"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->country = [currentData copy];
    } else if ([elementName isEqualToString: @"lang"]) {
        if (![Lang isLang: currentData])
            error = 1;
        else
            _cur_t->_cur_lang = [currentData copy];
    } else if ([elementName isEqualToString: @"price"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->_cur_price = currentData.intValue;
    } else if ([elementName isEqualToString: @"translate"]) {
        if (_cur_t->_cur_lang.length > 0 && _cur_t->_cur_price >= 0)
            [_cur_t->translate
             setObject: [NSNumber numberWithInt: _cur_t->_cur_price]
             forKey: _cur_t->_cur_lang];
        else
            error = true;
        _cur_t->_cur_lang = @"";
        _cur_t->_cur_price = -1;
    } else if ([elementName isEqualToString: @"rating"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->rating = currentData.intValue;
    } else if ([elementName isEqualToString: @"rating_num"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->rating_num = currentData.intValue;
    } else if ([elementName isEqualToString: @"await"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->await = currentData.intValue;
    } else if ([elementName isEqualToString: @"confirmed"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->confirmed = currentData.intValue;
    } else if ([elementName isEqualToString: @"rejected"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->rejected = currentData.intValue;
    } else if ([elementName isEqualToString: @"error"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            _cur_t->error = currentData.intValue;
    } else if ([elementName isEqualToString:@"translators"]) {
        pTList->translators = currentData.intValue;
    } else if ([elementName isEqualToString:@"phone"]) {
        _cur_t->phone = [currentData copy];
    }
}

-(void)parseEnd_PhonecallConfirm: (NSString *) elementName
{
    if ([elementName isEqualToString: @"phonecall_confirm"]) {
    } else if ([elementName isEqualToString:@"translator"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallConfirm->translator = currentData.intValue;
    } else if ([elementName isEqualToString:@"phone"]) {
        pPhonecallConfirm->phone = [currentData copy];
    } else if ([elementName isEqualToString:@"accept"]) {
        pPhonecallConfirm->accept = currentData.intValue;
    }
}

-(void)parseEnd_PhonecallTimeout: (NSString *) elementName
{
    if ([elementName isEqualToString: @"phonecall_timeout"]) {
    } else if ([elementName isEqualToString:@"translator"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallTimeout->translator = currentData.intValue;
    }
}

-(void)parseEnd_PhonecallStatus: (NSString *) elementName
{
    if ([elementName isEqualToString: @"phonecall_status"]) {
    } else if ([elementName isEqualToString:@"peer"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->peer = currentData.intValue;
    } else if ([elementName isEqualToString:@"error"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->error = currentData.intValue;
    } else if ([elementName isEqualToString:@"active"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->active = currentData.intValue;
    } else if ([elementName isEqualToString:@"time"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->time = currentData.intValue;
    } else if ([elementName isEqualToString:@"cost"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->cost = currentData.intValue;
    } else if ([elementName isEqualToString:@"balance"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->balance = currentData.intValue;
    } else if ([elementName isEqualToString:@"translate_lang"]) {
        if (![Lang isLang: currentData])
            error = true;
        else
            pPhonecallStatus->translate_lang = [currentData copy];
    } else if ([elementName isEqualToString:@"client_lang"]) {
        if (![Lang isLang: currentData])
            error = true;
        else
            pPhonecallStatus->client_lang = [currentData copy];
    } else if ([elementName isEqualToString:@"client_name"]) {
        pPhonecallStatus->client_name = [currentData copy];
    } else if ([elementName isEqualToString:@"translator_name"]) {
        pPhonecallStatus->translator_name = [currentData copy];
    } else if ([elementName isEqualToString:@"price"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pPhonecallStatus->price = currentData.intValue;
    }
}

-(void)parseEnd_Statistic: (NSString *) elementName
{
    if ([elementName isEqualToString: @"clients"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pStatistic->clients = currentData.intValue;
    } else if ([elementName isEqualToString:@"translators"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pStatistic->translators = currentData.intValue;
    } else if ([elementName isEqualToString:@"users_hour"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pStatistic->users_hour = currentData.intValue;
    } else if ([elementName isEqualToString:@"calls_hour"]) {
        if (![self isNumeric: currentData])
            error = true;
        else
            pStatistic->calls_hour = currentData.intValue;
    }
}

-(void)parseStart_Languages:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    if ([elementName isEqualToString:@"languages"]) {
        _cur_group = nil;
        _cur_lang = nil;
    } else if ([elementName isEqualToString:@"lang"]) {
        _cur_lang = [[Lang alloc] init];
        _cur_lang->group = _cur_group;
    } else if ([elementName isEqualToString: @"group"]) {
        Lang *__cur_group = _cur_group;
        _cur_group = [[Lang alloc] init];
        _cur_group->group = __cur_group;
        _cur_group->isGroup = true;
        _cur_group->code = [attributeDict valueForKey:@"code"];
        if (_cur_group->code == nil) {
            error = true;
            return;
        }
        _cur_group->name = [attributeDict valueForKey:@"name"];
        if (_cur_group->code == nil) {
            error = true;
            return;
        }
//        [Lang.Langs addObject:_cur_group];
        _cur_lang = nil;
    }
}

-(void)parseEnd_Languages: (NSString *) elementName
{
    if ([elementName isEqualToString: @"languages"]) {
        pLanguages->LangNum = [Lang.Langs count];
    } else if ([elementName isEqualToString: @"group"]) {
        if (_cur_group == nil || _cur_group->code.length == 0 || _cur_group->name.length == 0) {
            error = true;
            return;
        }
        [Lang.Langs addObject:_cur_group];
        if (_cur_group != nil)
            _cur_group = _cur_group->group;
    } else if ([elementName isEqualToString: @"lang"]) {
        if (_cur_lang == nil) {
            error = true;
            return;
        }
        if (_cur_lang->code.length == 0 || _cur_lang->name.length == 0 ||
            _cur_lang->iso3.length == 0) {
            error = true;
            return;
        }
        if (_cur_lang->nativeName.length == 0)
            _cur_lang->nativeName = [_cur_lang->name copy];
        [Lang.Langs addObject:_cur_lang];
        _cur_lang = nil;
    } else if ([elementName isEqualToString:@"code"]) {
        if (_cur_lang == nil) {
            error = true;
            return;
        }
        _cur_lang->code = [currentData copy];
    } else if ([elementName isEqualToString:@"name"]) {
        if (_cur_lang == nil) {
            error = true;
            return;
        }
        _cur_lang->name = [currentData copy];
    } else if ([elementName isEqualToString:@"iso3"]) {
        if (_cur_lang == nil) {
            error = true;
            return;
        }
        _cur_lang->iso3 = [currentData copy];
    } else if ([elementName isEqualToString:@"country"]) {
        if (_cur_lang == nil) {
            error = true;
            return;
        }
        _cur_lang->country = [currentData copy];
    } else if ([elementName isEqualToString:@"native"]) {
        if (_cur_lang == nil) {
            error = true;
            return;
        }
        _cur_lang->nativeName = [currentData copy];
    } else if ([elementName isEqualToString:@"avg_price"]) {
        if (_cur_lang == nil || ![self isNumeric: currentData]) {
            error = true;
            return;
        }
        _cur_lang->avg_price = currentData.intValue;
    }
}

-(void)parseStart_Countries:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    if ([elementName isEqualToString:@"countries"]) {
        _cur_country = nil;
    } if ([elementName isEqualToString:@"country"]) {
        _cur_country = [[Country alloc] init];
    }
}

-(void)parseEnd_Countries: (NSString *) elementName
{
    if ([elementName isEqualToString: @"countries"]) {
    } else if ([elementName isEqualToString: @"country"]) {
        if (_cur_country == nil) {
            error = true;
            return;
        }
        if (_cur_country->code.length == 0 || _cur_country->name.length == 0 ||
            _cur_country->lang.length == 0) {
            error = true;
            return;
        }
        [Country.Countries addObject:_cur_country];
        _cur_country = nil;
    } else if ([elementName isEqualToString:@"code"]) {
        if (_cur_country == nil) {
            error = true;
            return;
        }
        _cur_country->code = [currentData copy];
    } else if ([elementName isEqualToString:@"name"]) {
        if (_cur_country == nil) {
            error = true;
            return;
        }
        _cur_country->name = [currentData copy];
    } else if ([elementName isEqualToString:@"iso3"]) {
        if (_cur_country == nil) {
            error = true;
            return;
        }
        _cur_country->iso3 = [currentData copy];
    } else if ([elementName isEqualToString:@"lang"]) {
        if (_cur_country == nil) {
            error = true;
            return;
        }
        _cur_country->lang = [currentData copy];
    } else if ([elementName isEqualToString:@"langs"]) {
        if (_cur_country == nil) {
            error = true;
            return;
        }
        _cur_country->langs = [currentData copy];
    }
}

-(void)parseStart_MarkHistory:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    if ([elementName isEqualToString:@"mark_history"]) {
        _cur_t = nil;
    } else if ([elementName isEqualToString:@"translator"]) {
        _cur_t = [[User alloc] init];
    }
}

-(void)parseEnd_MarkHistory:(NSString *)elementName
{
    if ([elementName isEqualToString: @"mark_history"]) {
    } else if ([elementName isEqualToString:@"translator"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
/*        if (_cur_t->ID == 0 || !_cur_t->_exist_Delete ||
            (!_cur_t->Delete && (_cur_t->name.length == 0 || _cur_t->translate == nil ||
                                 _cur_t->country.length == 0 || !_cur_t->_exist_Busy))) {
            error = true;
            return;
        }
 */
        [pMarkList->markList addObject:_cur_t];
        _cur_t = nil;
    } else if ([elementName isEqualToString:@"id"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->ID = currentData.intValue;
    } else if ([elementName isEqualToString:@"name"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->name = [currentData copy];
    } else if ([elementName isEqualToString:@"rating"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->rating = currentData.intValue;
    } else if ([elementName isEqualToString:@"rating_num"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->rating_num = currentData.intValue;
    } else if ([elementName isEqualToString:@"deleted"]) {
        if (_cur_t == nil) {
            error = true;
            return;
        }
        _cur_t->Delete = currentData.intValue != 0;
    }
}

-(void)parseStart_CallHistory:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    if ([elementName isEqualToString:@"call_history"]) {
        _cur_call = nil;
    } else if ([elementName isEqualToString:@"call"]) {
        _cur_call = [[Call alloc] init];
        _cur_call->translator = [[User alloc] init];
        _cur_call->client = [[User alloc] init];
    }
}

-(void)parseEnd_CallHistory:(NSString *)elementName
{
    if ([elementName isEqualToString: @"mark_history"]) {
    } else if ([elementName isEqualToString:@"call"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        /*        if (_cur_t->ID == 0 || !_cur_t->_exist_Delete ||
         (!_cur_t->Delete && (_cur_t->name.length == 0 || _cur_t->translate == nil ||
         _cur_t->country.length == 0 || !_cur_t->_exist_Busy))) {
         error = true;
         return;
         }
         */
        [pCallList->callList addObject:_cur_call];
        _cur_call = nil;
    } else if ([elementName isEqualToString:@"translator"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->translator->ID = currentData.intValue;
    } else if ([elementName isEqualToString:@"name"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->translator->name = [currentData copy];
    } else if ([elementName isEqualToString:@"lang"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->TranslateLang = [currentData copy];
    } else if ([elementName isEqualToString:@"price"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->price = currentData.intValue;
    } else if ([elementName isEqualToString:@"cost"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->cost = currentData.intValue != 0;
    } else if ([elementName isEqualToString:@"start"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        int time = [currentData intValue];
        _cur_call->start = [NSDate dateWithTimeIntervalSince1970:time];
/*        NSDateFormatter *df = [[NSDateFormatter alloc] init];
        [df setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
        _cur_call->start = [df dateFromString:currentData];
 */
    } else if ([elementName isEqualToString:@"length"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->length = currentData.intValue;
    } else if ([elementName isEqualToString:@"client_country"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->client->country = [currentData copy];
    } else if ([elementName isEqualToString:@"translator_country"]) {
        if (_cur_call == nil) {
            error = true;
            return;
        }
        _cur_call->translator->country = [currentData copy];
    }
}

-(void)parseEnd_MarkRequest:(NSString *)elementName
{
    if ([elementName isEqualToString: @"mark_request"]) {
    } else if ([elementName isEqualToString:@"translator"]) {
        if (pMarkRequest == nil) {
            error = true;
            return;
        }
        pMarkRequest->translator = currentData.intValue;
    } else if ([elementName isEqualToString:@"name"]) {
        if (pMarkRequest == nil) {
            error = true;
            return;
        }
        pMarkRequest->name = [currentData copy];
    } else if ([elementName isEqualToString:@"time"]) {
        if (pMarkRequest == nil) {
            error = true;
            return;
        }
        int time = [currentData intValue];
        pMarkRequest->time = [NSDate dateWithTimeIntervalSinceReferenceDate:time];
    }
}

-(void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    [currentData setString:@""];
    if (error)
        return;
    if ([elementName isEqualToString: @"error"] &&          // important sub-field cases
        ![type isEqualToString:@"translator_list"] && ![type isEqualToString:@"phonecall_status"]) {
        pError = [[Packet_Error alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString: @"await_login_confirm"]) {
        pAwaitLoginConfirm = [[Packet_AwaitLoginConfirm alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString: @"user_data"]) {
        pUserData = [[User alloc] init];
        pUserData->options = [[Options alloc] init];
        pUserData->translate = [[NSMutableDictionary alloc] init];
        pUserData->_cur_lang = @"";
        pUserData->_cur_price = -1;
        type = elementName;
    } else if ([elementName isEqualToString: @"translator_list"]) {
        pTList = [[Packet_TList alloc] init];
        pTList->tlist = [[NSMutableArray alloc] init];
        type = elementName;
    } else if ([type isEqualToString:@"translator_list"]) {
        [self parseStart_TranslatorList:elementName namespaceURI:namespaceURI qualifiedName:qualifiedName attributes:attributeDict];
    } else if ([elementName isEqualToString: @"phonecall_confirm"]) {
        pPhonecallConfirm = [[Packet_PhonecallConfirm alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString: @"phonecall_timeout"]) {
        pPhonecallTimeout = [[Packet_PhonecallTimeout alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString: @"phonecall_status"]) {
        pPhonecallStatus = [[Packet_PhonecallStatus alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString: @"await_phone_confirm"]) {
        pAwaitPhoneConfirm = [[Packet_AwaitPhoneConfirm alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString: @"statistic"]) {
        pStatistic = [[Packet_Statistic alloc] init];
        type = elementName;
    } else if ([elementName isEqualToString:@"languages"]) {
        pLanguages = [[Packet_Languages alloc] init];
        [Lang.Langs removeAllObjects];
        type = elementName;
    } else if ([type isEqualToString:@"languages"]) {
        [self parseStart_Languages:elementName namespaceURI:namespaceURI qualifiedName:qualifiedName attributes:attributeDict];
    } else if ([elementName isEqualToString:@"countries"]) {
        [Country.Countries removeAllObjects];
        type = elementName;
    } else if ([type isEqualToString:@"countries"]) {
        [self parseStart_Countries:elementName namespaceURI:namespaceURI qualifiedName:qualifiedName attributes:attributeDict];
    } else if ([elementName isEqualToString:@"mark_history"]) {
        pMarkList = [[Packet_MarkList alloc] init];
        pMarkList->markList = [[NSMutableArray alloc] init];
        type = elementName;
    } else if ([type isEqualToString:@"mark_history"]) {
        [self parseStart_MarkHistory:elementName namespaceURI:namespaceURI qualifiedName:qualifiedName attributes:attributeDict];
    } else if ([elementName isEqualToString:@"call_history"]) {
        pCallList = [[Packet_CallList alloc] init];
        pCallList->callList = [[NSMutableArray alloc] init];
        type = elementName;
    } else if ([type isEqualToString:@"call_history"]) {
        [self parseStart_CallHistory:elementName namespaceURI:namespaceURI qualifiedName:qualifiedName attributes:attributeDict];
    } else if ([elementName isEqualToString:@"mark_request"]) {
        pMarkRequest = [[Packet_MarkRequest alloc] init];
        type = elementName;
    }
}

-(void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName
{
    if (error)
        return;
    if ([type isEqualToString: @"error"]) {
        [self parseEnd_Error:elementName];
    } else if ([type isEqualToString: @"user_data"]) {
        [self parseEnd_UserData:elementName];
    } else if ([type isEqualToString: @"translator_list"]) {
        [self parseEnd_TranslatorList:elementName];
    } else if ([type isEqualToString: @"phonecall_confirm"]) {
        [self parseEnd_PhonecallConfirm:elementName];
    } else if ([type isEqualToString: @"phonecall_timeout"]) {
        [self parseEnd_PhonecallTimeout:elementName];
    } else if ([type isEqualToString: @"phonecall_status"]) {
        [self parseEnd_PhonecallStatus:elementName];
    } else if ([type isEqualToString: @"statistic"]) {
        [self parseEnd_Statistic:elementName];
    } else if ([type isEqualToString: @"languages"]) {
        [self parseEnd_Languages:elementName];
    } else if ([type isEqualToString: @"countries"]) {
        [self parseEnd_Countries:elementName];
    } else if ([type isEqualToString: @"mark_history"]) {
        [self parseEnd_MarkHistory:elementName];
    } else if ([type isEqualToString: @"call_history"]) {
        [self parseEnd_CallHistory:elementName];
    } else if ([type isEqualToString: @"mark_request"]) {
        [self parseEnd_MarkRequest:elementName];
    }
    [currentData setString: @""];
}

-(void)parser:(NSXMLParser *)parser foundCharacters:(NSString *)string
{
    [currentData appendString:string];
}

-(bool) isNumeric:(NSString *) str {
    NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
    NSNumber *number = [formatter numberFromString:str];
    return !!number; // If the string is not numeric, number will be nil
}

-(ParsedPacket *) parsePacket: (NSData *)xmlPacket
{
    parser = [[NSXMLParser alloc] initWithData: xmlPacket];
    [parser setDelegate: self];
    currentData = [[NSMutableString alloc] init];
    type = nil;
    error = false;
    if (![parser parse])
        return NULL;
    if (error) {
        error = false;
        return NULL;
    }
    if ([type isEqualToString: @"error"])
        return pError;
    else if ([type isEqualToString:@"await_login_confirm"])
        return pAwaitLoginConfirm;
    else if ([type isEqualToString:@"user_data"])
        return pUserData;
    else if ([type isEqualToString:@"translator_list"])
        return pTList;
    else if ([type isEqualToString:@"phonecall_confirm"])
        return pPhonecallConfirm;
    else if ([type isEqualToString:@"phonecall_timeout"])
        return pPhonecallTimeout;
    else if ([type isEqualToString:@"phonecall_status"])
        return pPhonecallStatus;
    else if ([type isEqualToString:@"await_phone_confirm"])
        return pAwaitPhoneConfirm;
    else if ([type isEqualToString:@"statistic"])
        return pStatistic;
    else if ([type isEqualToString:@"languages"])
        return pLanguages;
    else if ([type isEqualToString:@"mark_history"])
        return pMarkList;
    else if ([type isEqualToString:@"call_history"])
        return pCallList;
    else if ([type isEqualToString:@"mark_request"])
        return pMarkRequest;
    return NULL;
}
@end
