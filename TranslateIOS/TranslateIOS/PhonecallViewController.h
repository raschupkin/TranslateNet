//
//  PhonecallViewController.h
//  Translate-Net
//
//  Created by Admin on 31.12.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_PhonecallViewController_h
#define Translate_Net_PhonecallViewController_h

#import <UIKit/UIKit.h>
#import "Call.h"
#import "Parser.h"
#import "NetworkClient.h"
#import "TListViewController.h"

@class RatingDialogViewController;

@interface PhonecallViewController : UIViewController <NetworkReceiver> {
    Call *cur_call;
    NSDate *callStart;
    bool MakingCall;
    bool CallActive;
    bool CallDialled;
    bool AlertStarted;
    NSTimer *callTimer;
    
    NSDate *phonecallStartEvent;
    CTCallCenter *CallCenter;
    
    bool PhonecallClicked;
    
    User *ratingTranslator;
    NSDate *ratingCallTime;
    bool showedRatingDialog;
    
    TListViewController *TListVC;               // to call onPacketError and on CallStarted
    RatingDialogViewController *RatingDialogVC;
    
    UIFont *Font;
    UIActivityIndicatorView *activityIndicator;
    __weak IBOutlet UILabel *L_Descr;
    __weak IBOutlet UILabel *L_NameLabel;
    __weak IBOutlet UILabel *L_Name;
    __weak IBOutlet UILabel *L_LangLabel;
    __weak IBOutlet UILabel *L_Lang;
    __weak IBOutlet UILabel *L_TranslateLabel;
    __weak IBOutlet UILabel *L_Translate;
    __weak IBOutlet UILabel *L_PriceLabel;
    __weak IBOutlet UILabel *L_Price;
    __weak IBOutlet UILabel *L_CostLabel;
    __weak IBOutlet UILabel *L_Cost;
    __weak IBOutlet UILabel *L_BalanceLabel;
    __weak IBOutlet UILabel *L_Balance;
    __weak IBOutlet UIButton *B_PurchaseCredits;
    __weak IBOutlet UILabel *L_StatusLabel;
    __weak IBOutlet UILabel *L_Status;
    __weak IBOutlet UILabel *L_LengthLabel;
    __weak IBOutlet UILabel *L_Length;
    __weak IBOutlet UIButton *B_Phonecall;
    __weak IBOutlet UIButton *B_Back;
    __weak IBOutlet UIButton *B_Settings;
}
-(void)setTListViewController:(TListViewController *)_TListVC;
-(void)FragmentOpened;
-(Call *)getCall;
-(bool)isCallActive;
-(bool)getMakingCall;
-(void)setMakingCall:(bool)_makingCall;
-(void)setCall:(Call *)call;
-(void)onPacket_PhonecallStatus:(Packet_PhonecallStatus *)p;
-(int)onNetwork_Error: (int)code;
-(int)onNetwork_PacketReceived:(ParsedPacket *)packet;
-(void)EnableButtons;
@end
#endif
