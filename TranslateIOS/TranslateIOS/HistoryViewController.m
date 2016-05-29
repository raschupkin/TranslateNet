//
//  HistoryViewController.m
//  Translate-Net
//
//  Created by Admin on 09.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HistoryViewController.h"
#import "SettingsViewController.h"
#import "TNApp.h"
#import "RatingDialogViewController.h"

@implementation CallInfo
@end

@implementation MarkInfo
@end

@implementation HistoryViewController

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if ([packet isKindOfClass:[Packet_Error class]]) {
        [TNApp onPacketError: (Packet_Error *)packet];
    } else if ([packet isKindOfClass:[Packet_MarkList class]]) {
        [self onPacket_MarkHistory:((Packet_MarkList *)packet)->markList];
    } else if ([packet isKindOfClass:[Packet_CallList class]]) {
        [self onPacket_CallHistory:((Packet_CallList *)packet)->callList];
    }
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

-(int)getLastPos
{
    int lastPos = 0;
    for (int i=0; i<[MarkInfoList count]; i++) {
        MarkInfo *mark = [MarkInfoList objectAtIndex:i];
        CGRect frame;
        int pos;
        if (mark->L_Name != nil) {
            frame = [mark->L_Name frame];
            pos = frame.origin.y + frame.size.height;
            if (pos > lastPos)
                lastPos = pos;
        }
        if (mark->RatingBar != nil) {
            frame = [mark->RatingBar frame];
            pos = frame.origin.y + frame.size.height;
            if (pos > lastPos)
                lastPos = pos;
        }
        if (mark->B_Rate != nil) {
            frame = [mark->B_Rate frame];
            pos = frame.origin.y + frame.size.height;
            if (pos > lastPos)
                lastPos = pos;
        }
        
        for (int i=0; i<[mark->callInfoList count]; i++) {
            CallInfo *call = [mark->callInfoList objectAtIndex:i];
            CGRect frame;
            int pos;
            if (call->L_Cost != nil) {
                frame = [call->L_Cost frame];
                pos = frame.origin.y + frame.size.height;
                if (pos > lastPos)
                    lastPos = pos;
            }
            if (call->L_Length != nil) {
                frame = [call->L_Length frame];
                pos = frame.origin.y + frame.size.height;
                if (pos > lastPos)
                    lastPos = pos;
            }
            if (call->L_Start != nil) {
                frame = [call->L_Start frame];
                pos = frame.origin.y + frame.size.height;
                if (pos > lastPos)
                    lastPos = pos;
            }
            if (call->L_Translate != nil) {
                frame = [call->L_Translate frame];
                pos = frame.origin.y + frame.size.height;
                if (pos > lastPos)
                    lastPos = pos;
            }
        }

    }
    return lastPos;
}

-(MarkInfo *)addMarkItem:(User *)t
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    CGRect frame;
    MarkInfo *ti = [[MarkInfo alloc] init];
    ti->user = [t copy];
    int base_x = PADDING_HOR;
    int x;
    int y = [self getLastPos];
    
    x = base_x;
    ti->L_NameLabel = [[UILabel alloc] init];
    [TNApp setUILabelText:ti->L_NameLabel text:[TNApp getString:@"name"] font:font];
    frame = [ti->L_NameLabel frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ti->L_NameLabel setFrame:frame];
    [_SV_History addSubview:ti->L_NameLabel];
    x += frame.size.width;
    
    ti->L_Name = [[UILabel alloc] init];
    [TNApp setUILabelText:ti->L_Name text:ti->user->name font:font];
    frame = [ti->L_Name frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ti->L_Name setFrame:frame];
    [_SV_History addSubview:ti->L_Name];
    x = base_x;
    y += frame.size.height;
    
    x = base_x;
    ti->L_RatingLabel = [[UILabel alloc] init];
    [TNApp setUILabelText:ti->L_RatingLabel text:[TNApp getString:@"last_rating"] font:font];
    frame = [ti->L_RatingLabel frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ti->L_RatingLabel setFrame:frame];
    [_SV_History addSubview:ti->L_RatingLabel];
    x += frame.size.width;

    int height = frame.size.height;
    ti->RatingBar = [[TNRateView alloc] init];
    [ti->RatingBar setNotSelectedImage:[UIImage imageNamed:IMAGE_RATING_EMPTY]];
    [ti->RatingBar setHalfSelectedImage:[UIImage imageNamed:IMAGE_RATING_HALF]];
    [ti->RatingBar setFullSelectedImage:[UIImage imageNamed:IMAGE_RATING_FULL]];
    [ti->RatingBar setMaxRating:5];
    ti->RatingBar.editable = false;
    frame = [ti->RatingBar frame];
    frame.origin.x = x;
    frame.origin.y = y;
    frame.size.height = height;
    frame.size.width = RATING_WIDTH;
    [ti->RatingBar setFrame:frame];
    ti->RatingBar.rating = ti->user->rating/20;
    [_SV_History addSubview:ti->RatingBar];
    x += frame.size.width;
    
    ti->L_RatingNum = [[UILabel alloc] init];
    NSString *text = [NSString stringWithFormat:@"(%d)", ti->user->rating_num];
    [TNApp setUILabelText:ti->L_RatingNum text:text font:font];
    frame = [ti->L_RatingNum frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ti->L_RatingNum setFrame:frame];
    [_SV_History addSubview:ti->L_RatingNum];
    x += frame.size.width;
    
    ti->B_Rate = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [TNApp setUIButtonText:ti->B_Rate text:[TNApp getString:@"rate"] font:font];
    frame = [ti->B_Rate frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ti->B_Rate setFrame:frame];
    [ti->B_Rate addTarget:self action:@selector(onButtonPressed_Rate:) forControlEvents: UIControlEventTouchDown];
    [_SV_History addSubview:ti->B_Rate];

    y += frame.size.height;
    
    return ti;
}

-(CallInfo *)addCallItem:(MarkInfo *)ti call:(Call *)call
{
    CallInfo *ci = [[CallInfo alloc] init];
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    CGRect frame;
    ci->call= [call copy];
    int base_x = 5*PADDING_HOR;
    int x;
    int y = [self getLastPos];
    
    x = base_x;
    ci->L_TranslateLabel = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_TranslateLabel text:[TNApp getString:@"translate"] font:font];
    frame = [ci->L_TranslateLabel frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_TranslateLabel setFrame:frame];
    [_SV_History addSubview:ci->L_TranslateLabel];
    x += frame.size.width;
    
    ci->L_Translate = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_Translate text:[Lang CodeToLang:call->TranslateLang] font:font];
    frame = [ci->L_Translate frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_Translate setFrame:frame];
    [_SV_History addSubview:ci->L_Translate];
    y += frame.size.height;
    
    x = base_x;
    ci->L_StartLabel = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_StartLabel text:[TNApp getString:@"call_start_noun"] font:font];
    frame = [ci->L_StartLabel frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_StartLabel setFrame:frame];
    [_SV_History addSubview:ci->L_StartLabel];
    x += frame.size.width;
    
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yy-MM-dd HH:mm:ss"];
    NSString *text = [formatter stringFromDate:call->start];
    ci->L_Start = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_Start text:text font:font];
    frame = [ci->L_Start frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_Start setFrame:frame];
    [_SV_History addSubview:ci->L_Start];
    x = base_x;
    y += frame.size.height;

    x = base_x;
    ci->L_LengthLabel = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_LengthLabel text:[TNApp getString:@"length"] font:font];
    frame = [ci->L_LengthLabel frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_LengthLabel setFrame:frame];
    [_SV_History addSubview:ci->L_LengthLabel];
    x += frame.size.width;
    
    formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm:ss"];
    [formatter setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
    text = [formatter stringFromDate:[NSDate dateWithTimeIntervalSince1970:call->length]];
    ci->L_Length = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_Length text:text font:font];
    frame = [ci->L_Length frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_Length setFrame:frame];
    [_SV_History addSubview:ci->L_Length];
    x = base_x;
    y += frame.size.height;
    
    x = base_x;
    ci->L_CostLabel = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_CostLabel text:[TNApp getString:@"cost"] font:font];
    frame = [ci->L_CostLabel frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_CostLabel setFrame:frame];
    [_SV_History addSubview:ci->L_CostLabel];
    x += frame.size.width;
    
    text = [TNApp FormatPrice:call->cost];
    ci->L_Cost = [[UILabel alloc] init];
    [TNApp setUILabelText:ci->L_Cost text:text font:font];
    frame = [ci->L_Cost frame];
    frame.origin.x = x;
    frame.origin.y = y;
    [ci->L_Cost setFrame:frame];
    [_SV_History addSubview:ci->L_Cost];
    x = base_x;
    y += frame.size.height;
    return ci;
}

-(MarkInfo *)addTranslatorItem:(int)translator
{
    User *t = [self findTranslator:translator];
    MarkInfo *ti = [self addMarkItem:t];
    ti->callInfoList = [[NSMutableArray alloc] init];
    [MarkInfoList addObject:ti];
    for (int i=0; i<[callList count]; i++) {
        Call *call = [callList objectAtIndex:i];
        if (call == nil)
            continue;
        if (call->translator->ID != translator /*&& !user.isT*/)
            continue;
        
        CallInfo *ci = [self addCallItem:ti call:call];
        [ti->callInfoList addObject:ci];
        
        call->displayed = true;
    }
    [_SV_History setContentSize:CGSizeMake([_SV_History frame].size.width, [self getLastPos])];
    return ti;
}

-(User *)findTranslator:(int)translator
{
    for (int i=0; i<[markList count]; i++) {
        User *t = [markList objectAtIndex:i];
        if (t->ID == translator)
            return t;
    }
    for (int i=0; i<[callList count]; i++) {
        User *t = ((Call *)[callList objectAtIndex:i])->translator;
        if (t->ID == translator)
            return t;
    }
    return nil;
}

-(void)SortCallList
{
    
}

-(void)UpdateCallList
{
    [self SortCallList];
//    if (user.isT){}else
    for (int i=0; i<[callList count]; i++) {
        Call *c = [callList objectAtIndex:i];
        if (c->displayed) continue;
//        if (user == nil)
//            continue;
        [self addTranslatorItem:c->translator->ID];
    }
}

-(void)ClearCallList
{
    [_SV_History.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
}

-(void)onPacket_CallHistory:(NSMutableArray *)_callList
{
    callList = [_callList copy];
    if (markList != nil && callList != nil) {
        [self UpdateCallList];
        [self showProgress: false];
    }
}

-(void)onPacket_MarkHistory:(NSMutableArray *)_markList
{
    markList = [_markList copy];
    if (markList != nil && callList != nil) {
        [self showProgress: false];
        [self UpdateCallList];
    }
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"HistoryToRatingDialog"]) {
        RatingDialogVC = [segue destinationViewController];
        [RatingDialogVC setHistoryVC:self];
        if (RatingTranslator != nil && RatingCallTime != nil)
            [RatingDialogVC setTranslator:RatingTranslator _time:RatingCallTime];
        isShown = false;
    }
}

- (IBAction)unwindToHistory:(UIStoryboardSegue *)unwindSegue
{
}

-(void)onButtonRate:(MarkInfo *)ti
{
    RatingTranslator = ti->user;
    RatingCallTime = nil;
    for (int i=0; i<[ti->callInfoList count]; i++) {
        CallInfo *ci = [ti->callInfoList objectAtIndex:i];
        if (RatingCallTime == nil || [RatingCallTime compare:ci->call->start])
            RatingCallTime = ci->call->start;
    }
    if (RatingCallTime != nil)
        [self performSegueWithIdentifier:@"HistoryToRatingDialog" sender:self];
}

-(IBAction)onButtonPressed_Rate: (id)sender
{
    for (int i=0; i<[MarkInfoList count]; i++) {
        MarkInfo *ti = [MarkInfoList objectAtIndex:i];
        if (ti->B_Rate == sender)
            [self onButtonRate:ti];
    }
}

-(void)ReloadMarkLists
{
    MarkInfoList = [[NSMutableArray alloc] init];
 
    callList = nil;
    markList = nil;
    [self showProgress:true];
    
    [TNApp sendPacket_GetCallHistory];
    [TNApp sendPacket_GetMarkHistory];
}

-(void)FragmentOpened
{
    [self ClearCallList];
    [self ReloadMarkLists];
}

-(void)Close
{
    if (SettingsVC == nil || SettingsVC->SegueFromTList)
        [self performSegueWithIdentifier:@"unwindToTList" sender:self];
    else if (SettingsVC->SegueFromPhonecall)
        [self performSegueWithIdentifier:@"unwindToPCall" sender:self];
}

-(void)Relogin
{
    [NetworkClient.client Disconnect];
    [TNApp.app setUser:nil];                // to skip TListVC:assertUserData
    if (isShown)
        [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
    else {
        if (RatingDialogVC != nil)
            [RatingDialogVC Relogin];
    }
}

-(IBAction)onBackPressed:(id)sender
{
    [self Close];
}

-(void)InitControls
{
//    int FontSize = [UIFont systemFontSize];
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    UIFont *fontLarge = [UIFont systemFontOfSize:LARGE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];

    text = [TNApp getString:@"act_history"];
    [TNApp setUILabelText:_L_Descr text:text font:fontLarge];
    
    [_SV_History setScrollEnabled:true];
}

-(void)viewDidAppear:(BOOL)animated
{
    isShown = true;
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    [NetworkClient.client setReceiver:self];
    [self InitControls];
    [self initProgress];
    isShown = true;
    [self FragmentOpened];
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

-(void)showProgress: (bool) show
{
    if (show)
        [activityIndicator startAnimating];
    else
        [activityIndicator stopAnimating];
}

@end