//
//  Parser.h
//  TranslateIOS
//
//  Created by Admin on 11.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef TranslateIOS_Parser_h
#define TranslateIOS_Parser_h

#import "Lang.h"
#import "Country.h"

#define	ERROR_NOERROR  							0
#define ERROR_OTHER								1
#define ERROR_MAINTENANCE                       2
#define ERROR_VERSION                           3
#define ERROR_LOAD                              4
#define ERROR_FORMAT                            5
#define ERROR_NO_USER                           6
#define ERROR_USER_OFFLINE                      7
#define ERROR_USER_ALREADY_EXIST       			8
#define ERROR_NAME_EXIST                        9
#define ERROR_ALREADY_LOGIN                     10
#define ERROR_LOGIN_REQUIRED            		11
#define ERROR_WRONG_PASSWORD            		12
#define ERROR_WRONG_SMSCODE                     13
#define ERROR_NO_USERDATA                       14
#define ERROR_NO_PHONE                          15
#define ERROR_PHONE_CHANGED                     16
#define ERROR_NO_LANG                           17
#define ERROR_PHONE_AWAITING           			18
#define ERROR_TEMP_BLOCKED                     19
#define ERROR_UNKOWN_CALL                       20
#define ERROR_CALL_EXIST                        21
#define ERROR_CALL_STATE                        22
#define ERROR_BALANCE                           23
#define ERROR_PHONECALL_ERROR           		24
#define ERROR_PEER_DISCON						25
#define ERROR_RATING_ERROR                      26
#define ERROR_PAYPAL_TRANSFER_ACTIVE            27
#define ERROR_PURCHASE_SIGNATURE                28
#define ERROR_ANOTHER_LOGIN                     29
#define ERROR_ANOTHER_PHONE                     30
#define ERROR_UNKNOWN_COUNTRY                   31

@class User;
@class Call;

@interface ParsedPacket : NSObject
@end

@interface Packet_Error : ParsedPacket {
@public
    int code;
    NSString *command;
    int client;
    int langs_version;
    int phonecall_request_translator;
    int money;
    int ID;
    int sms_block_days, sms_sent_num;
};
@end

@interface Packet_AwaitLoginConfirm : ParsedPacket {
};
@end

@interface Packet_TList : ParsedPacket {
@public
    NSMutableArray *tlist;
    int translators;
};
@end

@interface Packet_PhonecallConfirm : ParsedPacket {
@public
    int translator;
    bool accept;
    NSString *phone;
};
@end

@interface Packet_PhonecallTimeout : ParsedPacket {
@public
    int translator;
};
@end

@interface Packet_PhonecallStatus : ParsedPacket {
@public
    int peer;
    int error;
    int cost;
    int balance;
    bool active;
    int time;
    NSString *translate_lang;
    NSString *client_lang;
    NSString *translator_name;
    NSString *client_name;
    int price;
};
@end

@interface Packet_AwaitPhoneConfirm : ParsedPacket {
    
}
@end

@interface Packet_Statistic : ParsedPacket {
@public
    int clients;
    int translators;
    NSMutableDictionary *language_stat;
    int calls_hour;
    int users_hour;
};
@end

@interface Packet_Languages : ParsedPacket {
@public
    int LangNum;
};
@end

@interface Packet_MarkList : ParsedPacket {
@public
    NSMutableArray *markList;
};
@end

@interface Packet_CallList : ParsedPacket {
@public
    NSMutableArray *callList;
};
@end

@interface Packet_MarkRequest : ParsedPacket {
@public
    int translator;
    NSString *name;
    NSDate *time;
};
@end

@interface Parser : NSObject <NSXMLParserDelegate> {
    NSXMLParser *parser;
    NSMutableString *currentData;
    bool error;
    NSString *type;
    Packet_Error *pError;
    Packet_AwaitLoginConfirm *pAwaitLoginConfirm;
    User *pUserData;
    Packet_TList *pTList;
    Packet_PhonecallConfirm *pPhonecallConfirm;
    Packet_PhonecallTimeout *pPhonecallTimeout;
    Packet_PhonecallStatus *pPhonecallStatus;
    Packet_AwaitPhoneConfirm *pAwaitPhoneConfirm;
    Packet_Statistic *pStatistic;
    Packet_Languages *pLanguages;
    Packet_MarkList *pMarkList;
    Packet_CallList *pCallList;
    Packet_MarkRequest *pMarkRequest;
    
    User *_cur_t;
    Call *_cur_call;
    Lang *_cur_lang;
    Lang *_cur_group;
    Country *_cur_country;
}
-(ParsedPacket *) parsePacket: (NSData *)xmlPacket;
@end

#endif
