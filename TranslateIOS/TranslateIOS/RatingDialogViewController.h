//
//  RatingViewController.h
//  Translate-Net
//
//  Created by Admin on 25.05.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_RatingDialogViewController_h
#define Translate_Net_RatingDialogViewController_h

#import <UIKit/UIKit.h>
#import "TNRateView.h"
#import "PhonecallViewController.h"
#import "HistoryViewController.h"

@interface RatingDialogViewController : UIViewController {
    User *translator;
    NSDate *time;

    PhonecallViewController *PCallVC;
    HistoryViewController *HistoryVC;
}
@property (weak, nonatomic) IBOutlet UIButton *B_Back;
@property (weak, nonatomic) IBOutlet UILabel *L_Descr;
@property (weak, nonatomic) IBOutlet UILabel *L_Time;
@property (weak, nonatomic) IBOutlet UIButton *B_Rate;
@property (strong, nonatomic) IBOutlet TNRateView *RatingBar;
-(void)setPCallVC:(PhonecallViewController *)vc;
-(void)setHistoryVC:(HistoryViewController *)vc;
-(void)setTranslator:(User *)_translator _time:(NSDate *)_time;
-(void)Relogin;
@end

#endif
