//
//  TLisTFiewController.h
//  Translate-Net
//
//  Created by Admin on 17.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "Parser.h"
#import "NetworkClient.h"
#import "TNRateView.h"

@interface TInfo : NSObject {
    @public
    User *translator;
    NSString *UserLang;
    
    UILabel *NameLabelL;
    UILabel *NameL;
    UILabel *statusLabelL;
    UILabel *statusL;
    UILabel *priceLabelL;
    UILabel *priceL;
    UILabel *countryLabelL;
    UILabel *countryL;
    UIImageView *countryIV;
    UILabel *ratingLabelL;
    TNRateView *ratingBar;
    UILabel *ratingNumL;
    UIButton *pcallReqButton;
    UIButton *pcallButton;
    UIButton *callButton;
}
@end

@interface CallState : NSObject {
@public
    bool await;
    bool confirmed;
    bool rejected;
    NSString *phone;
}
@end

@interface TListViewController : UIViewController <NetworkReceiver, UITableViewDelegate, UITableViewDataSource, UIAlertViewDelegate> {
    User *client;
    bool UserDataIsOld;
    NSString *ListLang;
    bool TranslatorsLoaded;
    bool RequestedTList;
    NSMutableArray *TInfoList;
    NSMutableArray *MissedConfirm;
    NSMutableDictionary *CallStates;

    NSString *curCountry;
    NSString *SelectedLang;
    bool selectingTranslateLang;
    UITableView *langsTV;
    
    Packet_Statistic *Stat;
    NSTimer *statTimer;
    UILabel *L_NoTranslators;
     
    int CalledTranslatorID;                     // to pass to PhonecallViewController
    
    bool ReceivedError_PhoneChanged;
    Packet_PhonecallStatus *PhonecallStatusAfterDisconnect;
    
    UIFont *fontSmall;
    UIFont *fontBase;
    UIActivityIndicatorView *activityIndicator;
    __weak IBOutlet UILabel *L_TranslatorsLabel;
    __weak IBOutlet UILabel *L_Translators;
    __weak IBOutlet UIButton *B_Settings;
    __weak IBOutlet UILabel *L_NativeLabel;
    __weak IBOutlet UILabel *L_TranslateLabel;
    __weak IBOutlet UIButton *B_Native;
    __weak IBOutlet UIButton *B_Translate;
    __weak IBOutlet UIScrollView *SV_TList;
    __weak IBOutlet UILabel *L_BalanceLabel;
    __weak IBOutlet UILabel *L_Balance;
    __weak IBOutlet UIButton *B_PurchaseCredits;
}
//-(int)get_CalledTranslatorID;
-(int)onPacket_PhonecallError:(int)code translator:(int)translator;
-(void)onCallStarted:(int)translator;
-(void)Relogin;
-(int)onNetwork_Error: (int) code;
-(int)onNetwork_PacketReceived:(ParsedPacket *)packet;
-(void)setClient:(User *)_client;
-(User *)getClient;
-(bool)setListLang:(NSString *)_ListLang;
-(NSString *)getListLang;

@end
