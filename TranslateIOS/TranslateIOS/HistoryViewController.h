//
//  HistoryViewController.h
//  Translate-Net
//
//  Created by Admin on 09.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_HistoryViewController_h
#define Translate_Net_HistoryViewController_h

#import <UIKit/UIKit.h>
#import "NetworkClient.h"
#import "Call.h"
#import "User.h"
#import "TNRateView.h"

@interface CallInfo : NSObject {
@public
    Call *call;
    UILabel *L_TranslateLabel;
    UILabel *L_Translate;
    UILabel *L_StartLabel;
    UILabel *L_Start;
    UILabel *L_LengthLabel;
    UILabel *L_Length;
    UILabel *L_CostLabel;
    UILabel *L_Cost;
}
@end

@interface MarkInfo : NSObject {
@public
    User *user;
    UILabel *L_NameLabel;
    UILabel *L_Name;
    UILabel *L_RatingLabel;
    TNRateView *RatingBar;
    UILabel *L_RatingNum;
    UIButton *B_Rate;
    NSMutableArray *callInfoList;
}
@end

@class RatingDialogViewController;
@class SettingsViewController;

@interface HistoryViewController : UIViewController <NetworkReceiver> {
    NSMutableArray *markList;
    NSMutableArray *callList;
    NSMutableArray *MarkInfoList;
    
    bool isShown;
    User *RatingTranslator;
    NSDate *RatingCallTime;
    RatingDialogViewController *RatingDialogVC;
    
    UIActivityIndicatorView *activityIndicator;
@public
    SettingsViewController *SettingsVC;
}
@property (weak, nonatomic) IBOutlet UIButton *B_Back;
@property (weak, nonatomic) IBOutlet UILabel *L_Descr;
@property (weak, nonatomic) IBOutlet UIScrollView *SV_History;
-(int)onNetwork_Error: (int) code;
-(int)onNetwork_PacketReceived:(ParsedPacket *)packet;
-(int)getLastPos;
-(MarkInfo *)addMarkItem:(User *)t;
-(CallInfo *)addCallItem:(MarkInfo *)ti call:(Call *)call;
-(MarkInfo *)addTranslatorItem:(int)translator;
-(User *)findTranslator:(int)translator;
-(void)SortCallList;
-(void)UpdateCallList;
-(void)ClearCallList;
-(void)onPacket_CallHistory:(NSMutableArray *)_callList;
-(void)onPacket_MarkHistory:(NSMutableArray *)_markList;
-(void)ReloadMarkLists;
-(void)FragmentOpened;
@end

#endif
