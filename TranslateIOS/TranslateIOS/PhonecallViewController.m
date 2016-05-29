//
//  PhonecallViewController.m
//  Translate-Net
//
//  Created by Admin on 31.12.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PhonecallViewController.h"
#import "SettingsViewController.h"
#import "RatingDialogViewController.h"
#import "UserSettingsViewController.h"
#import "TNApp.h"

@implementation PhonecallViewController

-(void)setTListViewController:(TListViewController *)_TListVC
{
    TListVC = _TListVC;
}

-(int)onPacket_MarkRequest:(Packet_MarkRequest *)p
{
    if (p == nil)
        return -1;
    if (p->translator <= 0)
        return -1;
    ratingTranslator = [[User alloc] init];
    ratingTranslator->ID = p->translator;
    ratingTranslator->name = [p->name copy];
    ratingCallTime = p->time;
    [self performSegueWithIdentifier:@"PCallToRatingDialog" sender:self];
    return 0;
}

-(void)onError_Phonecall:(int)code peer:(int)peer
{
    if (cur_call == nil)
        return;
    if (MakingCall) {
        if (cur_call->translator != nil && peer == cur_call->translator->ID) {
            /*            Toast toast = Toast.makeText(getActivity(), activity.getErrorMessage(getActivity().getApplicationContext(), code), Toast.LENGTH_LONG);
             toast.show();
             */
            [TNApp setUILabelText:L_Length text:[TNApp getString:@"error"] font:Font];
//            [self ClosePhonecallFragment];    // in android
        }
    } else ;
}

-(void)onPacket_PhonecallStatus:(Packet_PhonecallStatus *)p
{
    if (p == nil)
        return;
    Call *call = [[Call alloc] init];
    if (p->active) {
        call->client = [[User alloc] init];
        call->translator = [[User alloc] init];
        [call initFromPacket:p user:[TNApp.app getUser]];
        if (TListVC != nil)
            [TListVC onCallStarted: p->peer];
    } else {
        call->active = false;
        call->length = p->time;
    }
    if (call->active ||	[self isCallActive] || phonecallStartEvent!= nil)
        [self setCall:call];
}

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if ([TNApp.app getUser] == nil)
        return 0;
   if ([packet isKindOfClass:[Packet_PhonecallStatus class]]) {
        Packet_PhonecallStatus 	*p = (Packet_PhonecallStatus *)packet;
        [self onPacket_PhonecallStatus:p];
    } else if ([packet isKindOfClass:[Packet_Error class]]) {
        Packet_Error *p = (Packet_Error *)packet;
        if ([p->command compare:@"phonecall_request"] == 0) {
            if (TListVC != nil)
                [TListVC onPacket_PhonecallError:p->code translator:p->phonecall_request_translator];
            [self onError_Phonecall: p->code peer:p->phonecall_request_translator];
        } else
            [TNApp onPacketError:p];
    } else if ([packet isKindOfClass:[User class]]) {
        User *u = (User *)packet;
        [TNApp.app setUser:[u copy]];
    } else if ([packet isKindOfClass:[Packet_TList class]]) {
        // in case was disconnected after call and reconnected
    } else if ([packet isKindOfClass:[Packet_MarkRequest class]])
        return [self onPacket_MarkRequest:(Packet_MarkRequest *)packet];
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
            //           [self Relogin];
            [TNApp UserMessage: [TNApp getString: @"error_network"]
                       message: [TNApp getString: @"network_error_format"]];
            return 0;
        }
    }
    return -1;
}

-(Call *)getCall
{
    return cur_call;
}
    
-(bool)isCallActive
{
    return CallActive;
}

-(bool)getMakingCall
{
    return MakingCall;
}

-(void)setMakingCall:(bool)_makingCall
{
    MakingCall = _makingCall;
}

-(bool)SameCall:(Call *)call1 call2:(Call *)call2
{
    return (call1 == nil || call1->client == nil || call1->translator == nil ||
            call2 == nil || call2->client == nil || call2->translator == nil ||
            (call1->client->ID == call2->client->ID && call1->translator->ID == call2->translator->ID));
}

-(void)setCall:(Call *)call
{
    if ((cur_call == nil || !cur_call->active) && call != nil) {
        if (call->active) {
            CallActive = true;
            CallDialled = true;
            [self EnableButtons];
        } else
            CallDialled = false;
    } else if (call == nil || (!call->active && [self SameCall:call call2:cur_call])) {
        if (CallActive)
            [self CallFinished:cur_call->length];
        CallActive = false;
    }	
    cur_call = call;
    if (CallActive && callStart == nil && call == nil) {
        callStart = [NSDate date];
    }
    if (call != nil && call->length != 0) {
        callStart = [NSDate dateWithTimeIntervalSinceNow:-call->length];    // not in android
        [self SetLengthTV:call->length];
    }
    [self updatePhonecallData];
}

-(void)CallFinished:(int)time
{
    if (cur_call != nil) {
        if (time > 0)
            cur_call->length = time;
        cur_call->active = false;
        cur_call->length = 0;
    }
    callStart = nil;
    [self StopTimer];
    [self updatePhonecallData];
    CallActive = false;
    MakingCall = false;
    [self EnableButtons];
    [TNApp sendPacket_GetUserData];
}

-(void)updatePhonecallData
{
    if (cur_call == nil)
        return;
//    if (!MakingCall && cur_call->length == 0 /* && !AlertStarted*/)
//        return;
    if (cur_call->active)
        [TNApp setUILabelText:L_Status text:[TNApp getString:@"active"] font:Font];
    else
        [TNApp setUILabelText:L_Status text:[TNApp getString:@"idle"] font:Font];
    if (MakingCall)
        [TNApp setUILabelText:L_Descr text:[TNApp getString:@"phonecall_to_translator"] font:Font];
/*    else if (cur_call->active && cur_call->client != nil)
        [L_Descr setText:[TNApp getString:@"phonecall_from_client"]];
    else if (AlertStarted)
        [L_Descr setText:[TNApp getString:@"phonecall_unknown"];
*/  else
        return;
    if (MakingCall || CallActive) {
        if (MakingCall) {
            if (cur_call->translator != nil && cur_call->translator->name != nil)
                [TNApp setUILabelText:L_Name text:cur_call->translator->name font:Font];
        } else if (cur_call->client != nil && cur_call->client->name != nil)
            [TNApp setUILabelText:L_Name text:cur_call->client->name font:Font];
        if (cur_call->ClientLang != nil)
            [TNApp setUILabelText:L_Lang text:[Lang CodeToLang:cur_call->ClientLang] font:Font];
        if (cur_call->TranslateLang != nil)
            [TNApp setUILabelText:L_Translate text:[Lang CodeToLang:cur_call->TranslateLang] font:Font];
        
        NSString *text = [TNApp FormatPrice:cur_call->price];
        text = [text stringByAppendingString:@"/"];
        text = [text stringByAppendingString:[TNApp getString:@"min"]];
        [TNApp setUILabelText:L_Price text:text font:Font];
        [TNApp setUILabelText:L_Cost text:[TNApp FormatPrice:cur_call->cost] font:Font];
        [TNApp setUILabelText:L_Balance text:[TNApp FormatPrice:[TNApp.app getUser]->balance] font:Font];
    }
    [self EnableButtons];
}

-(void)EnableButtons
{
    if (cur_call == nil)
        return;
    [B_Phonecall setEnabled:(MakingCall && !CallActive && !CallDialled)];
    [B_Back setEnabled:(!CallActive)];
}

-(IBAction)BackClick:(id)sender
{
    [self ClosePhonecallFragment];
}

-(IBAction)PhonecallClick:(id)sender
{
    if (cur_call == nil || cur_call->translator == nil || cur_call->translator->phone == nil)
        return;
    NSString *url = [NSString stringWithFormat:@"tel://%@", cur_call->translator->phone];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
}

-(IBAction)PurchaseCreditsClick:(id)sender
{
    [self performSegueWithIdentifier:@"PCallToUserSettings" sender:self];
}

-(void)SetLengthTV:(int)length
{
    [TNApp setUILabelText:L_Length text:[TNApp FormatTime:(int)length] font:Font];
}

-(void)callTimerHandle
{
    if (callStart == nil)
        return;
    NSDate *now = [NSDate date];
    NSTimeInterval length = [now timeIntervalSinceDate:callStart];
    [self SetLengthTV:length];
}

-(void)StartTimer
{
    callTimer = [NSTimer scheduledTimerWithTimeInterval:1 target:self
                                               selector:@selector(callTimerHandle) userInfo:nil repeats:true];
}

-(void)StopTimer
{
    if (callTimer != nil)
        [callTimer invalidate];
    callTimer = nil;
}


-(void)FragmentClosed
{
    [self CallFinished:0];
}

// called in TListVC->prepareForSegue
-(void)FragmentOpened
{
    [self EnableButtons];
    [self SetLengthTV:0];

    [self StopTimer];
    [self StartTimer];
}

-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"PCallToRatingDialog"]) {
        RatingDialogVC = [segue destinationViewController];
        [RatingDialogVC setPCallVC:self];
        [RatingDialogVC setTranslator:ratingTranslator _time:ratingCallTime];
        showedRatingDialog = true;
    } else if ([[segue identifier] isEqualToString:@"PCallToSettings"]) {
        SettingsViewController *SettingsVC = [segue destinationViewController];
        [SettingsVC setFromPhonecall];
    } else if ([[segue identifier] isEqualToString:@"PCallToUserSettings"]) {
        UserSettingsViewController *UserSettingsVC = [segue destinationViewController];
        UserSettingsVC->fromPhonecall = true;
    }
}

-(void)Relogin
{
    [TNApp.app setUser:nil];  // to skip assertUserData in TListVC->viewDidAppear
    [NetworkClient.client Disconnect];
    if (showedRatingDialog) {
        if (RatingDialogVC != nil)
            [RatingDialogVC Relogin];
    } else {
        [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
    }
}

-(void)ClosePhonecallFragment
{
    [self FragmentClosed];
    [self performSegueWithIdentifier:@"unwindToTList" sender:self];
}

- (IBAction)unwindToPCall:(UIStoryboardSegue *)unwindSegue
{
}

-(void)viewDidAppear:(BOOL)animated
{
    [NetworkClient.client setReceiver:self];
//    [self FragmentOpened];
    showedRatingDialog = false;
    [self updatePhonecallData];
}

-(void)InitControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    Font = font;
    NSString *text = @"";
    
    text = [TNApp getString:@"phonecall_unknown"];
    [TNApp setUILabelText:L_Descr text:text font:font];
    
    text = [TNApp getString:@"name"];
    [TNApp setUILabelText:L_NameLabel text:text font:font];
   
    text = [TNApp getString:@"lang"];
    [TNApp setUILabelText:L_LangLabel text:text font:font];
    
    text = [TNApp getString:@"translate"];
    [TNApp setUILabelText:L_TranslateLabel text:text font:font];
    
    text = [TNApp getString:@"price"];
    [TNApp setUILabelText:L_PriceLabel text:text font:font];
    
    text = [TNApp getString:@"cost"];
    [TNApp setUILabelText:L_CostLabel text:text font:font];
    
    text = [TNApp getString:@"balance"];
    [TNApp setUILabelText:L_BalanceLabel text:text font:font];
    
    text = [TNApp getString:@"status"];
    [TNApp setUILabelText:L_StatusLabel text:text font:font];
    
    text = [TNApp getString:@"length"];
    [TNApp setUILabelText:L_LengthLabel text:text font:font];
    
    text = [TNApp getString:@"pcall_verb"];
    [TNApp setUIButtonText:B_Phonecall text:text font:font];
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:B_Back text:text font:font];

    text = [TNApp getString:@"settings"];
    [TNApp setUIButtonText:B_Settings text:text font:font];
}

-(void)sendPhonecallStatus
{
    if (callStart != nil && phonecallStartEvent != nil) {
        NSTimeInterval length = -[phonecallStartEvent timeIntervalSinceNow];
        //NSTimeInterval length = -[callStart timeIntervalSinceNow];
        [TNApp sendPacket_PhonecallStatus:false time:length];
    }
}

-(void)viewDidLoad {
    [super viewDidLoad];
    
    // Do view setup here.
    [NetworkClient.client setReceiver:self];
    [self InitControls];

    phonecallStartEvent = nil;
    CallCenter = [[CTCallCenter alloc] init];
    CallCenter.callEventHandler = ^(CTCall* call) {
        if (call.callState == CTCallStateDisconnected) {
            [self sendPhonecallStatus];
            phonecallStartEvent = nil;
        } else if (call.callState == CTCallStateConnected) {
            phonecallStartEvent = [NSDate date];
        }
    };

}

-(bool)isResumed
{
    return [self isViewLoaded] && ([self view].window != nil);
}

@end
