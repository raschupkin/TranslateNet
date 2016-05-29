//
//  TListViewController.m
//  Translate-Net
//
//  Created by Admin on 17.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import "TListViewController.h"
#import "TNApp.h"
#import "Parser.h"
#import "Lang.h"
#import "Country.h"
#import "User.h"
#import "Call.h"
#import "SettingsViewController.h"
#import "PhoneSettingsViewController.h"
#import "PhonecallViewController.h"
#import "UserSettingsViewController.h"

@implementation TInfo
@end

@implementation CallState
@end

@implementation TListViewController

-(int)onPacket_PhonecallError:(int)code translator :(int)translator
{
    switch (code) {
    case ERROR_BALANCE:
        [self SetConfirmed:translator await:false accept:false reject:false phone:nil resetRequest:true];
//        [self UserMessageLowBalance];
//        return 0;
        break;
    default:
        [self SetConfirmed:translator await:false accept:false reject:false phone:nil resetRequest:false];
        break;
    }
    [TNApp UserMessage:[TNApp getString:@"request"] message:[TNApp getErrorMessage:code]];
    return 0;
}

-(int)onPacket_TranslatorList:(Packet_TList *)pTList
{
    for (int i=0; i<[pTList->tlist count]; i++) {
        User *t = [pTList->tlist objectAtIndex:i];
        if (t == nil)
            continue;
        [self updateTranslatorItem:t];
        [self SetConfirmed:t->ID await:t->await accept:t->confirmed
                    reject:t->rejected phone:nil resetRequest:t->error];
    }

    if ([TInfoList count] > 0) {
        [L_NoTranslators removeFromSuperview];
    } else {
        [self initL_NoTranslators];
        [SV_TList addSubview:L_NoTranslators];
    }

    int width = [SV_TList frame].size.width;
    int height = [self getScrollViewTListOffset:[TInfoList count]];
    [SV_TList setContentSize:CGSizeMake(width, height)];
    [self.view layoutSubviews];
    [self showProgress:false];

    if (Stat == nil)
        Stat = [[Packet_Statistic alloc] init];
    Stat->translators = pTList->translators;
    [self UpdateStatistic:Stat];
    return 0;
}

-(int)assertUserData
{
    User *user = [TNApp.app getUser];
    if (user->name == nil || [user->name length] == 0) {    // first start
       if (!user->isTranslator)
            user->lang = [TNApp getCurrentLanguage];
        [self performSegueWithIdentifier:@"TListToUserSettings" sender:self];
        return 1;
    } else if (user->phone_status != PHONE_STATUS_CONFIRMED ||
               user->phone == nil || [user->phone length] == 0) {
        [self performSegueWithIdentifier:@"TListToPhoneSettings" sender:self];
        return 1;
    } else
        return 0;
}

-(int)onPacket_UserData:(User *)user
{
    UserDataIsOld = false;
    if (user->isTranslator) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"translator_not_supported"]];
        [self Relogin];
        return 0;
    }
    [self showProgress:false];
    [self setClient:user];
    [TNApp.app setUser:user];
//    [self updateClientView];

    if ([self assertUserData])
        return 0;

    if (![user CheckLangs]) {
        user->lang = LANG_DEFAULT;
        [TNApp sendPacket_UserData:user];
        if (user->name != nil && [user->name length] > 0) { // not first start after register
            NSString *message = [TNApp getString: @"error_lang"];
            message = [message stringByAppendingString:[TNApp getLangNameByCode:user->lang]];
            [TNApp UserMessage:[TNApp getString:@"error"] message:message];
        }
    }
    
    if (TNApp.app->PhoneChanged) {
        [TNApp UserMessage: [TNApp getString:@"note"] message:[TNApp getErrorMessage:ERROR_PHONE_CHANGED]];
        TNApp.app->PhoneChanged = false;
    }
    [self UpdateTranslators];
    return 0;
}

-(int)onPacket_ErrorUserData:(int)code
{
    [self showProgress:false];
    switch (code) {
        case ERROR_NOERROR: {
            [TNApp.app setUser:client];
            [self updateClientView];
            [self UpdateTranslators];
            break;
        }
        default: {
            client = [TNApp.app getUser];
            [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:code]];
            break;
        }
    }
    return 0;
}

-(int)onPacket_ErrorStopTranslatorList:(int)code
{
    RequestedTList = false;
//    if (code == ERROR_NOERROR && ListLang != nil && [Lang isLang:ListLang])
//        [TNApp sendPacket_RequestTranslatorList:ListLang];
    [self ClearTranslators];
    return 0;
}
/*
-(int)onPacket_ErrorPhonecallRequest:(int)code
{
    if (code == ERROR_BALANCE) {
        [self UserMessageLowBalance];
    } else
        [TNApp UserMessage: [TNApp getString:@"error"] message:[TNApp getErrorMessage:code]];
    return 0;
}
 */

-(int)onPacket_PhonecallConfirm:(Packet_PhonecallConfirm *)p
{
    if (p == nil)
        return -1;
    int pos = [self SetConfirmed:p->translator await:false accept:p->accept reject:!p->accept phone:p->phone resetRequest:false];
    if (pos >= 0) {
        [SV_TList setContentOffset:CGPointMake(0, [self getScrollViewTListOffset:pos])];
    }
    if (![self isResumed] && MissedConfirm != nil) {
        [MissedConfirm addObject:[NSNumber numberWithInt:p->translator]];
    }
    
    // play sound
    return 0;
}

-(int)onPacket_PhonecallTimeout:(Packet_PhonecallTimeout *)p
{
    if (p == nil)
        return -1;
    [self SetConfirmed:p->translator await:false accept:false reject:true phone:nil resetRequest:true];
    return 0;
}

-(int)onPacket_PhonecallStatus:(Packet_PhonecallStatus *)p
{
    if (p == nil)
        return -1;
    PhonecallStatusAfterDisconnect = p;
    if (p->active)
        [self OpenPhonecallFragment];
    return 0;
}

-(int)onPacket_Statistic:(Packet_Statistic *)p
{
    if (p == nil)
        return -1;
    Stat = p;
    [self UpdateStatistic:Stat];
    return 0;
}

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if ([packet isKindOfClass:[User class]])
        return [self onPacket_UserData: (User *)packet];
    else if ([TNApp.app getUser] == nil)        // exiting
        return 0;
    if ([packet isKindOfClass:[Packet_Error class]]) {
        Packet_Error *p = (Packet_Error *)packet;
        if (p->code == ERROR_PHONE_CHANGED)
            TNApp.app->PhoneChanged = true;
        else if ([p->command compare:@"user_data"] == 0)
            return [self onPacket_ErrorUserData:p->code];
        else if ([p->command compare:@"stop_translator_list"] == 0)
            return [self onPacket_ErrorStopTranslatorList:p->code];
        else if ([p->command compare:@"phonecall_request"] == 0)
            return [self onPacket_PhonecallError:p->code translator:p->phonecall_request_translator];
        else if ([p->command compare:@"billing"] == 0) {
            return [TNApp sendPacket_GetUserData];
        }
        else
            [TNApp onPacketError: p];
    } else if ([packet isKindOfClass:[Packet_TList class]])
        return [self onPacket_TranslatorList:(Packet_TList *)packet];
    else if ([packet isKindOfClass:[Packet_PhonecallConfirm class]])
        return [self onPacket_PhonecallConfirm:(Packet_PhonecallConfirm *)packet];
    else if ([packet isKindOfClass:[Packet_PhonecallTimeout class]])
        return [self onPacket_PhonecallTimeout:(Packet_PhonecallTimeout *)packet];
    else if ([packet isKindOfClass:[Packet_PhonecallStatus class]])
        return [self onPacket_PhonecallStatus:(Packet_PhonecallStatus *)packet];
    else if ([packet isKindOfClass:[Packet_Statistic class]])
        return [self onPacket_Statistic:(Packet_Statistic *)packet];
    return 0;
}

-(int)onNetwork_Error:(int)code
{
    switch (code) {	
        case NETWORK_ERROR_OTHER: {
            [self Relogin];
            [TNApp UserMessage: [TNApp getString: @"error_network"]
                             message: [TNApp getErrorMessage:ERROR_OTHER]];
//            [self dismissViewControllerAnimated:true completion:nil];
            return -1;
        }
        case NETWORK_ERROR_CONNECTION: {
            [self Relogin];
            if (TNApp.app->inForeground)
                [TNApp UserMessage: [TNApp getString: @"error_network"]
                             message: [TNApp getString: @"network_error_connection"]];
//            [self dismissViewControllerAnimated:true completion:nil];
            return -1;
        }
        case NETWORK_ERROR_FORMAT: {
            //[self Relogin];
            [TNApp UserMessage: [TNApp getString: @"error_network"]
                             message: [TNApp getString: @"network_error_format"]];
            return 0;
        }
    }
    return -1;
}

-(void)setClient:(User *)_client
{
    client = _client;
    if (client != nil) {
        /*
        if (ListLang == nil)
            [self InitListLang];
        else
            [self setListLang:[self AssertListLang:ListLang]];
         */
        if (ListLang == nil) {
            NSString *saved_ListLang = [[NSUserDefaults standardUserDefaults] stringForKey:PREFERENCE_LISTLANG];
            if (![Lang isLang:saved_ListLang])
                [self setListLang:LANG_DEFAULT];
            else
                [self setListLang:saved_ListLang ];

        }
        [self updateClientView];
    }
}

-(User *)getClient
{
    return client;
}

-(void)UpdateStatistic:(Packet_Statistic *)stat
{
    if (stat == nil)
        return;
    NSString *str = [NSString stringWithFormat:@"%d", stat->translators];
    [TNApp setUILabelText:L_Translators text:str font:fontSmall];
    [L_Translators sizeToFit];
}

-(bool)setNativeLang:(NSString *)_lang
{
    if (ListLang != nil && [ListLang compare:_lang] == 0) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"same_lang"]];
        return false;
    }
    client->lang = _lang;
//    [self updateClientView];
    [self showProgress:true];
    [TNApp sendPacket_UserData:client];
    return true;
}


-(bool)setListLang:(NSString *)_ListLang
{
    if (client != nil && client->lang != nil && [client->lang compare:_ListLang] == 0) {
        [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getString:@"same_lang"]];
//        ListLang = LANG_DEFAULT;
        if (ListLang == nil ||
            (client != nil && client->lang != nil && [client->lang compare:ListLang] == 0))
            ListLang = LANG_DEFAULT;
        if (ListLang == nil ||
            (client != nil && client->lang != nil && [client->lang compare:ListLang] == 0))
            ListLang = LANG_DEFAULT2;
    } else
        ListLang = _ListLang;	
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    [userDefaults setObject:ListLang forKey:PREFERENCE_LISTLANG];
    [userDefaults synchronize];
    [self updateClientView];
    return true;
}

-(NSString *)getListLang
{
    return ListLang;
}

-(void)updateClientView
{
    if (client != nil) {
        [B_Native setTitle:[TNApp getLangNameByCode: client->lang] forState:UIControlStateNormal];
        [B_Translate setTitle:[TNApp getLangNameByCode:ListLang] forState:UIControlStateNormal];
        [L_Balance setText:[TNApp FormatPrice:client->balance]];
    }
}

-(Call *)getCallData:(int)translator
{
    for (int j=0; j<[TInfoList count]; j++) {
        TInfo *tinfo = [TInfoList objectAtIndex:j];
        if (tinfo == nil || tinfo->translator == nil)
            continue;
        if (tinfo->translator->ID == translator) {
            Call *call = [[Call alloc] init];
            call->client = [self getClient];
            call->translator = tinfo->translator;
            call->translator->phone = [tinfo->translator->phone copy];
            call->translator->name = tinfo->translator->name;
            call->ClientLang = client->lang;
            call->TranslateLang = [self getListLang];
            if (![tinfo->translator->translate objectForKey:call->TranslateLang])
                return nil;
            call->price = [[tinfo->translator->translate objectForKey:call->TranslateLang] intValue];
            return call;
        }
    }
    return nil;
}

-(void)NativeLangSelected:(NSString *)lang
{
    if (![Lang isLang:lang])
        return;
    if (client != nil && ![client->lang isEqualToString:lang])
        if ([self setNativeLang:lang])
            [self UpdateTranslators];
}

-(void)TranslateLangSelected:(NSString *)lang
{
    if (![Lang isLang:lang])
        return;
    if (![ListLang isEqualToString:lang])
        if ([self setListLang:lang])
            [self UpdateTranslators];
}

-(int)doRequestTranslatorList
{
    RequestedTList = true;
    return [TNApp sendPacket_RequestTranslatorList:ListLang];
}

-(int)SearchTranslators
{
    if (client == nil)
        return 0;
    if (RequestedTList)
        return -1;
    if (ListLang == nil || ![Lang isLang:ListLang])
        return -1;
    [self showProgress:true];
    
    TInfoList = [[NSMutableArray alloc] init];
/*
    if (RequestedTList)
        return [TNApp sendPacket_StopTranslatorList];
    else
 */
        return [self doRequestTranslatorList];
}

-(void)updateTranslatorStatus:(TInfo *)tinfo resetRequest:(bool)resetRequest
{
    if (tinfo == nil || tinfo->translator == nil)
        return;
    User *t = tinfo->translator;
    [TNApp setUILabelText:tinfo->statusL text:[self getStatusText:tinfo->translator] font:fontSmall];
/*    if (cs == nil) {
        [tinfo->pcallButton setEnabled:false];
        if (tinfo->translator != nil)
            [tinfo->pcallReqButton setEnabled:!tinfo->translator->Busy];
    } else {
 */
        [tinfo->pcallButton setEnabled:(t->confirmed && !t->Busy)];
        [tinfo->pcallReqButton setEnabled:
                            (resetRequest || (!t->await && !t->confirmed && !t->rejected)) &&
                            !t->Busy];
//    }
}

-(int)SetConfirmed:(int)translator await:(bool)await accept:(bool)accept reject:(bool)reject phone:(NSString *)phone resetRequest:(bool)resetRequest
{
    int j;
    for (j=0; j<[TInfoList count]; j++) {
        TInfo *tinfo = [TInfoList objectAtIndex:j];
        if (tinfo == nil || tinfo->translator == nil)
            continue;
        if (tinfo->translator->ID == translator) {
            if (accept && phone != nil)
                tinfo->translator->phone = [phone copy];
            // resume toasts
            tinfo->translator->await = await;
            tinfo->translator->confirmed = accept;
            tinfo->translator->rejected = reject;
            [self updateTranslatorStatus:tinfo /*cs:cs*/ resetRequest:resetRequest];
            break;
        }
    }
    if (j == [TInfoList count])
        return -1;
    return j;
}

-(void)onButtonPhonecallRequest:(TInfo *)tinfo
{
    tinfo->translator->await = true;
    tinfo->translator->confirmed = false;
    tinfo->translator->rejected = false;
    [tinfo->pcallButton setEnabled:false];
    [tinfo->pcallReqButton setEnabled:false];
    [TNApp setUILabelText:tinfo->statusL text:[self getStatusText:tinfo->translator] font:fontSmall];
    [TNApp sendPacket_PhonecallRequest:tinfo->translator->ID translate_lang:ListLang];
}

-(void)onCallStarted:(int)translator
{
    for (int j=0; j<[TInfoList count]; j++) {
        TInfo *tinfo = [TInfoList objectAtIndex:j];
        if (tinfo == nil || tinfo->translator == nil)
            continue;
        if (tinfo->translator->ID == translator) {
            [self updateTListItem:j translator:tinfo->translator];
            return;
        }
    }
}

-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"TListToPCall"]) {
        PhonecallViewController *pcVC = [segue destinationViewController];
        [pcVC setTListViewController:self];
        [pcVC setMakingCall:true];
        if (PhonecallStatusAfterDisconnect == nil)
            [pcVC setCall:(Call *)[self getCallData:CalledTranslatorID]];
        else {
            [pcVC onPacket_PhonecallStatus:PhonecallStatusAfterDisconnect];
            PhonecallStatusAfterDisconnect = nil;
        }
        [pcVC FragmentOpened];
    } else if ([[segue identifier] isEqualToString:@"TListToSettings"]) {
        SettingsViewController *SettingsVC = [segue destinationViewController];
        [SettingsVC setFromTList];
    } else if ([[segue identifier] isEqualToString:@"TListToPhoneSettings"]) {
        PhoneSettingsViewController *PhoneSettingsVC = [segue destinationViewController];
        PhoneSettingsVC->isStarting = true;
    } else if ([[segue identifier] isEqualToString:@"TListToUserSettings"]) {
        UserSettingsViewController *UserSettingsVC = [segue destinationViewController];
        UserSettingsVC->fromTList = true;
    }
}

-(void)OpenPhonecallFragment
{
    [self performSegueWithIdentifier:@"TListToPCall" sender:self];
}

-(void)onButtonPhonecall:(TInfo *)tinfo
{
    if (tinfo->translator == nil)
        return;
    CalledTranslatorID = tinfo->translator->ID;
    [self updateTranslatorItem:tinfo->translator];
    [self installBackgroundNotification:false];
    [self performSegueWithIdentifier:@"TListToPCall" sender:self];
}

-(NSString *)getStatusText:(User *)t
{
    NSString *status = @"";
//    if (cs != nil) {
        if (t->await) {
            if (t->Busy)
                status = [[TNApp getString:@"busy"] stringByAppendingString:@"("];
            status = [TNApp getString:@"await"];
            if (t->Busy)
                status = [status stringByAppendingString:@")"];
        } else if (t->confirmed) {
            if (t->Busy)
                status = [[TNApp getString:@"busy"] stringByAppendingString:@"("];
            status = [TNApp getString:@"confirmed_adjective"];
            if (t->Busy)
                status = [status stringByAppendingString:@")"];
        } else if (t->rejected) {
            if (t->Busy)
                status = [[TNApp getString:@"busy"] stringByAppendingString:@"("];
            status = [TNApp getString:@"rejected_adjective"];
            if (t->Busy)
                status = [status stringByAppendingString:@")"];
        } else if (t->Busy)
            status = [TNApp getString:@"busy"];
        else
            status = [TNApp getString:@"available"];
/*
        return status;
    } else {
        if (t->Busy) {
            status = [TNApp getString:@"busy"];
            return status;
        }
    }
    status = [TNApp getString:@"available"];
 */
    return status;
}

int SV_NumElemsInItem = 5;
-(int)getScrollViewTListOffset:(int)pos
{
    if ([TInfoList count] < pos)
        return 0;
    /*if (pos == 0)
        return 0;
    else if (pos == [TInfoList count]) {
        CGRect bar = [((TInfo *)[TInfoList objectAtIndex:pos-1])->ratingBar frame];
        return bar.origin.y + bar.size.height;
    } else */if (pos <= [TInfoList count]) {
        NSString *text = @"Test";
        CGSize s = [text sizeWithFont:fontSmall constrainedToSize:CGSizeMake(MAXFLOAT, MAXFLOAT) lineBreakMode:NSLineBreakByWordWrapping];
        int height = s.height * SV_NumElemsInItem;
        return (PADDING_VERT + height) * pos;
//        CGRect bar = [((TInfo *)[TInfoList objectAtIndex:pos])->NameL frame];
 //       return bar.origin.y;
    } else
        return -1;
//    int height = size * SV_NumElemsInItem;
 //   return PADDING_VERT + (PADDING_VERT + height) * pos;
}

-(void)updateTListItem:(int)pos translator:(User *)translator
{
    TInfo *tinfo = [TInfoList objectAtIndex:pos];
    if (tinfo == nil || tinfo->translator == nil)
        return;
    
    NSString *old_phone = nil;
    if (tinfo->translator != nil)
        old_phone = tinfo->translator->phone;
    tinfo->translator = [translator copy];
    if (tinfo->translator->phone == nil)
        tinfo->translator->phone = [old_phone copy];

    CGRect frame;
    int base_x = 0;//PADDING_HOR;
    int x = base_x;
    int base_y = [self getScrollViewTListOffset:pos];
    if (base_y < 0)
        return;
    int y = base_y;
 
    [TNApp setUILabelText:tinfo->NameLabelL text:[TNApp getString:@"name"] font:fontSmall];
    frame = [tinfo->NameLabelL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->NameLabelL setFrame:frame];
    x += frame.size.width;
    [TNApp setUILabelText:tinfo->NameL text:translator->name font:fontSmall];
    frame = [tinfo->NameL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->NameL setFrame:frame];
    y += frame.size.height;
    
    x = base_x;
    [TNApp setUILabelText:tinfo->statusLabelL text:[TNApp getString:@"status"] font:fontSmall];
    frame = [tinfo->statusLabelL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->statusLabelL setFrame:frame];
    x += frame.size.width;
    [TNApp setUILabelText:tinfo->statusL text:[self getStatusText:translator] font:fontSmall];
    frame = [tinfo->statusL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->statusL setFrame:frame];
    y += frame.size.height;
    
    x = base_x;
    if ([translator->translate objectForKey:ListLang] != nil) {
        [TNApp setUILabelText:tinfo->priceLabelL text:[TNApp getString:@"price"] font:fontSmall];
        frame = [tinfo->priceLabelL frame];
        frame.origin.x = x;
        frame.origin.y = y;
        [tinfo->priceLabelL setFrame:frame];
        x += frame.size.width;
        int price_list = [[translator->translate objectForKey:ListLang] intValue];
        int price_native = [[translator->translate objectForKey:client->lang] intValue];
        NSString *text = [TNApp FormatPrice:(price_list>price_native?price_list:price_native)];
        text = [text stringByAppendingString:@"/"];
        text = [text stringByAppendingString:[TNApp getString:@"min"]];
        [TNApp setUILabelText:tinfo->priceL text:text font:fontSmall];
        frame = [tinfo->priceL frame];
        frame.origin.x = x;
        frame.origin.y = y;
        [tinfo->priceL setFrame:frame];
        y += frame.size.height;
    }
    
    x = base_x;
    [TNApp setUILabelText:tinfo->countryLabelL text:[TNApp getString:@"country"] font:fontSmall];
    frame = [tinfo->countryLabelL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->countryLabelL setFrame:frame];
    x += frame.size.width;
    NSString *text = [Country CodeToCountry:translator->country];
    [TNApp setUILabelText:tinfo->countryL text:text font:fontSmall];
    frame = [tinfo->countryL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->countryL setFrame:frame];
    x += frame.size.width;
    [tinfo->countryIV setFrame:CGRectMake(x, y, frame.size.height, frame.size.height)];
    if (![translator->country isEqualToString:COUNTRY_UNKNOWN]) {
        UIImage *countryImage = [TNApp getCountryImageRes:translator->country];
        if (countryImage != nil)
            [tinfo->countryIV setImage:countryImage];
    }
    y += frame.size.height;
    
//rating
    x = base_x;
    
    x = base_x;
    [TNApp setUILabelText:tinfo->ratingLabelL text:[TNApp getString:@"rating"] font:fontSmall];
    frame = [tinfo->ratingLabelL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->ratingLabelL setFrame:frame];
    x += frame.size.width;
    [tinfo->ratingBar setFrame:CGRectMake(x, y, RATING_WIDTH, frame.size.height)];
    tinfo->ratingBar.editable = false;
    tinfo->ratingBar.rating = translator->rating/20;
    x += RATING_WIDTH;
    text = [NSString stringWithFormat:@"(%d)", translator->rating_num];
    [TNApp setUILabelText:tinfo->ratingNumL text:text font:fontSmall];
    frame = [tinfo->ratingNumL frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->ratingNumL setFrame:frame];
    x += frame.size.width;
    y += frame.size.height;

    x = SV_TList.frame.size.width - SV_TList.frame.origin.x;
    y = base_y;
    [TNApp setUIButtonText:tinfo->pcallButton text:[TNApp getString:@"pcall_verb"] font:fontSmall];
    frame = [tinfo->pcallButton frame];
    x -= frame.size.width + PADDING_HOR;
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->pcallButton setFrame:frame];
    
    [TNApp setUIButtonText:tinfo->pcallReqButton text:[TNApp getString:@"pcall_req_verb"] font:fontSmall];
    frame = [tinfo->pcallReqButton frame];
    x -= frame.size.width + PADDING_HOR;
    frame.origin.x = x;
    frame.origin.y = y;
    [tinfo->pcallReqButton setFrame:frame];

/*    CallState *cs = [CallStates objectForKey:[NSNumber numberWithInt:translator->ID]];
    if (cs == nil) {
        [self updateTranslatorStatus:tinfo cs:nil resetRequest:true];
    } else {
*/        [self updateTranslatorStatus:tinfo /*cs:cs*/ resetRequest:false];
//    }
}

-(void)addTranslatorItem:(int)pos translator:(User *)translator
{
    if ([translator->translate objectForKey:ListLang] == nil)
        return;
    TInfo *tinfo = [self createTListItem];
    tinfo->translator = translator;
    [TInfoList insertObject:tinfo atIndex:pos];
    for (int i=pos; i<[TInfoList count]; i++) {
        TInfo *ti = [TInfoList objectAtIndex:i];
        [self updateTListItem:i translator:ti->translator];
    }
}

-(void)removeTranslatorItem:(int)pos
{
    [self ClearTInfo:pos];
    [TInfoList removeObjectAtIndex:pos];
    for (int i=pos; i<[TInfoList count]; i++) {
        TInfo *ti = [TInfoList objectAtIndex:i];
        [self updateTListItem:i translator:ti->translator];
    }
}

-(void)updateTranslatorItem:(User *)translator
{
    int j;
    for (j=0; j<[TInfoList count]; j++) {
        TInfo *ti = (TInfo*)[TInfoList objectAtIndex:j];
        if (ti == nil || ti->translator == nil)
            continue;
        if (ti->translator->ID == translator->ID) {
            if (!translator->Delete) {
                [self updateTListItem:j translator:translator];
            } else {
                [self removeTranslatorItem:j];
                j--;
            }
            return;
        } else if (!translator->Delete && translator->rating > ti->translator->rating) {
            [self addTranslatorItem: j translator:translator];
            return;
        }
    }
    if (j == [TInfoList count])
        [self addTranslatorItem: j translator:translator];
}

-(void)ClearTranslators
{
    if (client == nil)
        return;
    for (int i=0; i<[TInfoList count]; i++)
        [self ClearTInfo:i];
    [TInfoList removeAllObjects];
}

-(void)UpdateTranslators
{
    if (client == nil)
        return;
    RequestedTList = false;
    [self ClearTranslators];
    [self SearchTranslators];
}


-(void)FragmentShown
{
    
}

-(IBAction) onButtonPressed_Phonecall: (id)sender
{
    for (int i=0; i<[TInfoList count]; i++) {
        TInfo *ti = [TInfoList objectAtIndex:i];
        if (ti->pcallButton == sender)
            [self onButtonPhonecall:ti];
    }
}
		
-(IBAction) onButtonPressed_PhonecallReq: (id)sender
{
    for (int i=0; i<[TInfoList count]; i++) {
        TInfo *ti = [TInfoList objectAtIndex:i];
        if (ti->pcallReqButton == sender)
            [self onButtonPhonecallRequest:ti];
    }
}

-(TInfo *)createTListItem
{
    TInfo *tinfo = [[TInfo alloc] init];
    tinfo->NameLabelL = [[UILabel alloc] init];
/*    if ([tinfo->NameLabelL respondsToSelector:@selector(layoutMargins)]) {
        [tinfo->NameLabelL setPreservesSuperviewLayoutMargins:false];
        [tinfo->NameLabelL setLayoutMargins:UIEdgeInsetsZero];// only iOS8
        // not working anyway
    }
*/
    [SV_TList addSubview:tinfo->NameLabelL];
    tinfo->NameL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->NameL];
    tinfo->statusLabelL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->statusLabelL];
    tinfo->statusL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->statusL];
    tinfo->priceLabelL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->priceLabelL];
    tinfo->priceL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->priceL];
    tinfo->countryLabelL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->countryLabelL];
    tinfo->countryL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->countryL];
    tinfo->countryIV = [[UIImageView alloc] init];
    [SV_TList addSubview:tinfo->countryIV];
    tinfo->ratingLabelL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->ratingLabelL];
    tinfo->ratingBar = [[TNRateView alloc] init];
    [tinfo->ratingBar setNotSelectedImage:[UIImage imageNamed:IMAGE_RATING_EMPTY]];
    [tinfo->ratingBar setHalfSelectedImage:[UIImage imageNamed:IMAGE_RATING_HALF]];
    [tinfo->ratingBar setFullSelectedImage:[UIImage imageNamed:IMAGE_RATING_FULL]];
    [tinfo->ratingBar setMaxRating:5];
    [SV_TList addSubview:tinfo->ratingBar];
    tinfo->ratingNumL = [[UILabel alloc] init];
    [SV_TList addSubview:tinfo->ratingNumL];

    tinfo->pcallButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [tinfo->pcallButton addTarget:self action:@selector(onButtonPressed_Phonecall:) forControlEvents: UIControlEventTouchDown];
    [SV_TList addSubview:tinfo->pcallButton];
    
    tinfo->pcallReqButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [tinfo->pcallReqButton addTarget:self action:@selector(onButtonPressed_PhonecallReq:) forControlEvents: UIControlEventTouchDown];
    [SV_TList addSubview:tinfo->pcallReqButton];
    
    return tinfo;
}

-(void)ClearTInfo:(int)pos
{
    TInfo *tinfo = [TInfoList objectAtIndex:pos];
    if (tinfo == nil)
        return;
    [tinfo->NameLabelL removeFromSuperview];
    [tinfo->NameL removeFromSuperview];
    [tinfo->statusLabelL removeFromSuperview];
    [tinfo->statusL removeFromSuperview];
    [tinfo->priceLabelL removeFromSuperview];
    [tinfo->priceL removeFromSuperview];
    [tinfo->countryLabelL removeFromSuperview];
    [tinfo->countryL removeFromSuperview];
    [tinfo->countryIV removeFromSuperview];
    [tinfo->ratingLabelL removeFromSuperview];
    [tinfo->ratingBar removeFromSuperview];
    [tinfo->ratingNumL removeFromSuperview];
    [tinfo->pcallReqButton removeFromSuperview];
    [tinfo->pcallButton removeFromSuperview];
//    [tinfo->callButton removeFromSuperview];
}

-(int)Langs_getRowNum
{
    Lang *l = [Lang getLangByCode:SelectedLang];
    int num = 0;
    for (int i=0; i<Lang.Langs.count; i++) {
        Lang *cur = [Lang.Langs objectAtIndex:i];
        if (cur->group == l)
            num++;
    }
    return num;
}

-(NSString *)Langs_getRow:(NSIndexPath *)indexPath
{
    Lang *l = [Lang getLangByCode:SelectedLang];
    int r = -1;
    for (int i=0; i<Lang.Langs.count; i++) {
        Lang *cur = [Lang.Langs objectAtIndex:i];
        if (cur->group == l)
            r++;
        if (r == [indexPath indexAtPosition:1]) {
            l = cur;
            break;
        }
    }
    if (l == nil)
        return LANG_DEFAULT;
    return l->code;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // Return the number of rows in the section.
    // If you're serving data from an array, return the length of the array:
    return [self Langs_getRowNum];
}

// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
    }
    
    cell.textLabel.text = [Lang CodeToLang: [self Langs_getRow:indexPath]];
//    cell.detailTextLabel.text = @"More text";
//    cell.imageView.image = [UIImage imageNamed:@"flower.png"];

    cell.accessoryType =  UITableViewCellAccessoryDisclosureIndicator;
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    SelectedLang = [self Langs_getRow:indexPath];
    Lang *l = [Lang getLangByCode:SelectedLang];
    if (l == nil) {
        [langsTV removeFromSuperview];
        langsTV = nil;
    }
    if (l->isGroup)
        [langsTV reloadData];
    else {
        [langsTV removeFromSuperview];
        langsTV = nil;
        if (selectingTranslateLang)
            [self TranslateLangSelected: SelectedLang];
        else
            [self NativeLangSelected: SelectedLang];
    }
}

-(void)selectLang
{
    if (langsTV != nil)
        [langsTV removeFromSuperview];
    SelectedLang = nil;
    CGSize sz = self.view.frame.size;
    langsTV = [[UITableView alloc] initWithFrame:CGRectMake(sz.width/8, sz.height/8, sz.width*6/8, sz.height*6/8)];
    [langsTV setDelegate:self];
    [langsTV setDataSource:self];
    [langsTV reloadData];
    [self.view addSubview:langsTV];
}

-(IBAction)selectNativeClick:(id)sender
{
    selectingTranslateLang = false;
    [self selectLang];
}

-(IBAction)selectTranslateClick:(id)sender
{
    selectingTranslateLang = true;
    [self selectLang];
}

-(IBAction)settingsClick:(id)sender
{
    if ([TNApp.app getUser] == nil)
        return;
    UserDataIsOld = true;
    [self performSegueWithIdentifier:@"TListToSettings" sender:self];
}

-(void)reloadStatistics
{
    [TNApp sendPacket_GetStatistic];
}

-(void)Relogin
{
    [TNApp.app setUser:nil];
    [NetworkClient.client Disconnect];
    [self showProgress:false];
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
    
    TranslatorsLoaded = false;
}

-(NSString *)getDefaultListLang
{
/*    Country *c = [Country getCountryByCode:[TNApp.app getCurrentCountry]];
    if (c != nil)
        return c->lang;
    return LANG_DEFAULT;
*/
    return LANG_DEFAULT;
}

-(NSString *)AssertListLang:(NSString *)llang
{
    if (client == nil)
        return LANG_DEFAULT;
    if (llang == nil)
        llang = LANG_DEFAULT;
    if ([llang compare:client->lang] == 0)
        llang = @"es";
    if ([llang compare:client->lang] == 0)
        llang = @"fr";
    return llang;
}

-(void)InitListLang
{
//    [self setListLang:[self AssertListLang:[self getDefaultListLang]]];
}

-(IBAction) onButtonPressed_PurchaseCredits: (id)sender
{
    [self performSegueWithIdentifier:@"TListToUserSettings" sender:self];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 1) {
        
        [self performSegueWithIdentifier:@"TListToUserSettings" sender:self];
    }
}

-(void)UserMessageLowBalance
{
    UIAlertView* alert = [[UIAlertView alloc] initWithTitle:[TNApp getString:@"low_balance"] message:@"" delegate:self cancelButtonTitle:[TNApp getString:@"ok"] otherButtonTitles: [TNApp getString:@"purchase_credits"], nil];
    [alert show];
}


- (void)ActivityOnStop:(NSNotification *) notification {
    TNApp.app->inForeground = false;
    if ([self isResumed]) {
        RequestedTList = false;
        [TNApp sendPacket_StopTranslatorList];
    }
}

- (void)ActivityOnStart:(NSNotification *) notification {
    TNApp.app->inForeground = true;
    if ([self isResumed]) {
        RequestedTList = false;
        [self UpdateTranslators];
    }
}

-(void)installBackgroundNotification:(bool)install
{
    if (install) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(ActivityOnStop:)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(ActivityOnStart:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    } else {
        [[NSNotificationCenter defaultCenter] removeObserver:self];
    }
}

-(void)viewDidAppear:(BOOL)animated
{
    [NetworkClient.client setReceiver:self];
    /*    if (!TranslatorsLoaded) {
     [self UpdateTranslators];
     TranslatorsLoaded = true;
     }
     */
    TranslatorsLoaded = false;
    
    statTimer = [NSTimer scheduledTimerWithTimeInterval:30 target:self selector:@selector(reloadStatistics) userInfo:nil repeats:YES];
    [statTimer fire];
    [self installBackgroundNotification:true];
    if (/*UserDataIsOld && */[TNApp.app getUser] != nil) {
        if ([self assertUserData])
            return;
        UserDataIsOld = false;
    }
    [self ActivityOnStart:nil];
    [self FragmentShown];
}

-(void)viewDidDisappear:(BOOL)animated
{
    [statTimer invalidate];
}

-(void)initL_NoTranslators
{
    NSString *text = [TNApp getString:@"no_translators"];
    if (client != nil && client->options->ActiveTSearch) {
        text = [text stringByAppendingString:@"\n"];
        text = [text stringByAppendingString:[TNApp getString:@"active_tsearch"]];
        [L_NoTranslators setNumberOfLines:0];
    }
    [TNApp setUILabelText:L_NoTranslators text:text font:fontBase];
}

-(void)initControls
{
    NSString *text = @"";
    
    text = [TNApp getString:@"translators"];
    [TNApp setUILabelText:L_TranslatorsLabel text:text font:fontBase];
    
    text = [TNApp getString:@"settings"];
    [TNApp setUIButtonText:B_Settings text:text font:fontBase];
    
    text = [TNApp getString:@"native_lang"];
    [TNApp setUILabelText:L_NativeLabel text:text font:fontBase];
    
    text = [TNApp getString:@"list_lang"];
    [TNApp setUILabelText:L_TranslateLabel text:text font:fontBase];
    
    text = [TNApp getString:@"native"];
    [TNApp setUIButtonText:B_Native text:text font:fontBase];
    B_Native.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
    
    text = [TNApp getString:@"translate"];
    [TNApp setUIButtonText:B_Translate text:text font:fontBase];
    B_Translate.contentHorizontalAlignment = UIControlContentHorizontalAlignmentRight;
    
    if ([SV_TList respondsToSelector:@selector(layoutMargins)]) {
        [[SV_TList superview] setLayoutMargins:UIEdgeInsetsZero];
        [[SV_TList superview] setPreservesSuperviewLayoutMargins:false];
        [SV_TList setContentOffset:CGPointZero];
        [SV_TList setPreservesSuperviewLayoutMargins:false];
        [SV_TList setLayoutMargins:UIEdgeInsetsZero];// only iOS8
        // not working anyway
    }
    
    L_NoTranslators = [[UILabel alloc] init];
    text = [TNApp getString:@"no_translators"];
    [TNApp setUILabelText:L_NoTranslators text:text font:fontBase];

    text = [TNApp getString:@"balance"];
    [TNApp setUILabelText:L_BalanceLabel text:text font:fontBase];
    
    text = [TNApp getString:@"purchase_credits"];
    [TNApp setUIButtonText:B_PurchaseCredits text:text font:fontBase];
}

- (IBAction)unwindToTList:(UIStoryboardSegue *)unwindSegue
{
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    // Do view setup here.
    CTCallCenter *CallCenter = [[CTCallCenter alloc] init];
    if (CallCenter.currentCalls == nil)
        [TNApp sendPacket_PhonecallStatus:false time:0];
    
    ListLang = nil;
    RequestedTList = false;
    PhonecallStatusAfterDisconnect = nil;
    fontSmall = [UIFont fontWithName:@"Verdana" size:SMALL_FONT_SIZE];
    fontBase = [UIFont fontWithName:@"Verdana" size:BASE_FONT_SIZE];
    [self initControls];
    [self initProgress];
    [NetworkClient.client setReceiver:self];
    [self showProgress: true];
    SV_TList.delegate = self;
    
//    if (curCountry == nil)
        curCountry = [TNApp getCurrentCountry];
        if (![Country isCountry:curCountry])
            curCountry = COUNTRY_UNKNOWN;
        [TNApp sendPacket_SetCountry:curCountry];
//    }
    
  //  CallStates = [[NSMutableDictionary alloc] init];
    [TNApp sendPacket_GetUserData];
}

-(void)initProgress
{
    activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    CGRect MainFrame = [[self view] frame];
    [activityIndicator setFrame:CGRectMake((MainFrame.size.width - PROGRESS_INDICATOR_SIZE)/2,
                                           (MainFrame.size.height - PROGRESS_INDICATOR_SIZE)/2,
                                           PROGRESS_INDICATOR_SIZE, PROGRESS_INDICATOR_SIZE)];
    [self.view addSubview:activityIndicator];
}

-(void)showProgress: (bool)show
{
    if (show)
        [activityIndicator startAnimating];
    else
        [activityIndicator stopAnimating];
}
        
-(bool)isResumed
{
    return [self isViewLoaded] && ([self view].window != nil);
}

@end
