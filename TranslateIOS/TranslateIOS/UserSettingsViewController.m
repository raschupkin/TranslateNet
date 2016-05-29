		//
//  UserSettingsViewController.m
//  Translate-Net
//
//  Created by Admin on 09.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "UserSettingsViewController.h"
#import "SettingsViewController.h"
#import "TNApp.h"

@implementation UserSettingsViewController

-(int)onNetwork_PacketReceived:(ParsedPacket *)packet
{
    if ([packet isKindOfClass:[Packet_Error class]]) {
        Packet_Error *p = (Packet_Error *)packet;
        if ([p->command isEqualToString:@"billing"]) {
/*            if (p->code == ERROR_NOERROR)
 
                if (p->money >= 0)
                    user->balance += p->money;
            [self UpdateUserView:user];
 */
            [_B_Back setEnabled:true];
            [self showProgress:false];
            [self BillingPacketError:p->code];
            return 0;
        } else if ([p->command isEqualToString:@"user_data"]) {
            switch (p->code) {
            case ERROR_NOERROR:
                [TNApp.app setUser:user];
                [self Close];
                return 0;
            case ERROR_NAME_EXIST:
            default:
                user = [[TNApp.app getUser] copy];
                [TNApp onPacketError: p];
                return 0;
            }
        }
    } else if ([packet isKindOfClass:[User class]]) {
        User *u = (User *)packet;
        user = [u copy];
        [[TNApp app] setUser:[user copy]];
        [self UpdateUserView:user];
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

-(void)onPacketError:(int)code
{
    [self showProgress:false];
    switch (code) {
        case ERROR_NOERROR: {
            [TNApp.app setUser:[user copy]];
            if (DonePressed)
                [self Close];
            break;
        }
        default: {
            user = [[TNApp.app getUser] copy];
            [TNApp UserMessage:[TNApp getString:@"error"] message:[TNApp getErrorMessage:code]];
            break;
        }
    }
}

-(void)BillingPacketError:(int)code
{
    [self EnableButtons:true];
    if (code != ERROR_NOERROR) {
        [TNApp UserMessage:[TNApp getString:@"error_billing"] message:[TNApp getErrorMessage:code]];
        return;
    }
    DonePressed = false;
    [TNApp sendPacket_GetUserData];
}

-(void)UpdateUserView:(User *)u
{
    if (u == nil)
        return;
    if (u->name != nil) {
        [_TF_Name setText:u->name];
        if ([u->name length] > 0)
            [_TF_Name setEnabled:false];
    }
    
    if (u->email != nil) {
        [_L_Email setText:u->email];
        [_L_Email sizeToFit];
    }
    [_L_Balance setText:[TNApp FormatPrice:u->balance]];
    [_L_Balance sizeToFit];
    [[self view] layoutSubviews];
}

-(bool)checkName:(NSString *)name
{
    if ([name length] < NameMinLength)
        return false;
    for (int i=0; i<[name length]; i++) {
        if (([name characterAtIndex:i] < 'a' || [name characterAtIndex:i] > 'z') &&
            ([name characterAtIndex:i] < 'A' || [name characterAtIndex:i] > 'Z') &&
            ([name characterAtIndex:i] < '0' || [name characterAtIndex:i] > '9')) {
            int j=0;
            for (; j<[NameSymbols length]; j++)
                if ([name characterAtIndex:i] == [NameSymbols characterAtIndex:j])
                    break;
            if (j == [NameSymbols length])
                return false;
        }
    }
    return true;
}

- (void)productPurchased:(NSNotification *)notification {
    
    PurchaseData *pd = notification.object;
    [_products enumerateObjectsUsingBlock:^(SKProduct * product, NSUInteger idx, BOOL *stop) {
        if ([product.productIdentifier isEqualToString:pd->productIdentifier]) {
            *stop = YES;
            if (!pd->purchased) {
                [self EnableButtons:true];
                [self showProgress:false];
                return;
            }
            switch (idx) {
                case 0:
                    [TNApp sendPacket_Billing:100 data:[pd->data base64EncodedStringWithOptions:0] signature:@""];
                    break;
            }
        }
    }];
}

- (IBAction)credit1Click:(id)sender
{
    if (_products == nil || [_products count] < PURCHASE_PRODUCT_1CREDIT+1)
        return;
    [self EnableButtons:false];
    SKProduct *product = _products[PURCHASE_PRODUCT_1CREDIT];
    [iaph buyProduct:product];
    [_B_Back setEnabled:false];
    [self showProgress:true];
}

- (IBAction)credit2Click:(id)sender
{
    if (_products == nil || [_products count] < PURCHASE_PRODUCT_2CREDITS+1)
        return;
    [self EnableButtons:false];
    SKProduct *product = _products[PURCHASE_PRODUCT_2CREDITS];
    [iaph buyProduct:product];
    [_B_Back setEnabled:false];
    [self showProgress:true];
}

- (IBAction)credit5Click:(id)sender
{
    if (_products == nil || [_products count] < PURCHASE_PRODUCT_5CREDITS+1)
        return;
    [self EnableButtons:false];
    SKProduct *product = _products[PURCHASE_PRODUCT_2CREDITS];
    [iaph buyProduct:product];
    [_B_Back setEnabled:false];
    [self showProgress:true];
}

- (IBAction)credit10Click:(id)sender
{
    if (_products == nil || [_products count] < PURCHASE_PRODUCT_10CREDITS+1)
        return;
    [self EnableButtons:false];
    SKProduct *product = _products[PURCHASE_PRODUCT_10CREDITS];
    [iaph buyProduct:product];
    [_B_Back setEnabled:false];
    [self showProgress:true];
}

- (IBAction)credit25Click:(id)sender
{
    if (_products == nil || [_products count] < PURCHASE_PRODUCT_25CREDITS+1)
        return;
    [self EnableButtons:false];
    SKProduct *product = _products[PURCHASE_PRODUCT_25CREDITS];
    [iaph buyProduct:product];
    [_B_Back setEnabled:false];
    [self showProgress:true];
}

-(NSString *)getName
{
    NSString *name = [_TF_Name text];
    if (![self checkName:name]) {
        NSString *msg = [TNApp getString:@"invalid_name"];
        msg = [msg stringByAppendingString:NameSymbols];
        [TNApp UserMessage:[TNApp getString:@"error"] message:msg];
        return nil;
    }
    return name;
}

-(IBAction)doneClick:(id)sender
{
    if (user == nil)
        [self Close];
    if (user->name == nil || [user->name length] == 0)
        user->name = [self getName];
    if (user->name == nil)
        return;
    if ([user->name compare:[TNApp.app getUser]->name] != 0) {
        [self showProgress:true];
        [TNApp sendPacket_UserData:user];
        DonePressed = true;
    } else
        [self Close];
}

-(void)Close
{
    [iaph removeTransactionObserver];
    if ((SettingsVC != nil && SettingsVC->SegueFromTList) || fromTList) {
        fromTList = false;
        [self performSegueWithIdentifier:@"unwindToTList" sender:self];
    } else if ((SettingsVC != nil && SettingsVC->SegueFromPhonecall) || fromPhonecall) {
        fromPhonecall = false;
        [self performSegueWithIdentifier:@"unwindToPCall" sender:self];
    }
}

-(void)Relogin
{
    [NetworkClient.client Disconnect];
    [iaph removeTransactionObserver];
    [TNApp.app setUser:nil];                // to skip TListVC:assertUserData
    [self performSegueWithIdentifier:@"unwindToLogin" sender:self];
}	

-(void)viewDidDisappear:(BOOL)animated
{
//    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(void)viewDidAppear:(BOOL)animated
{
    [NetworkClient.client setReceiver:self];
    DonePressed = false;
    if (user != nil)
        [self UpdateUserView:user];
//    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(productPurchased:) name:IAPHelperProductPurchasedNotification object:nil];
}

-(void)EnableButtons:(bool)enable
{
    [_B_1credit setEnabled:enable];
    [_B_2credits setEnabled:enable];
    [_B_5credits setEnabled:enable];
    [_B_10credits setEnabled:enable];
    [_B_25credits setEnabled:enable];
    [_B_Done setEnabled:enable];
}

-(IBAction)onBackPressed:(id)sender
{
    [self Close];
}

-(void)InitControls
{
    UIFont *font = [UIFont systemFontOfSize:BASE_FONT_SIZE];
    NSString *text = @"";
    
    text = [TNApp getString:@"back"];
    [TNApp setUIButtonText:_B_Back text:text font:font];
    
    text = [TNApp getString:@"name_descr"];
    [TNApp setUILabelText:_L_NameDescr text:text font:font];
    
    [_TF_Name setAutocapitalizationType: UITextAutocapitalizationTypeSentences];
    [_TF_Name setDelegate:self];
    
    text = [TNApp getString:@"email"];
    [TNApp setUILabelText:_L_EmailLabel text:text font:font];
    
    text = [TNApp getString:@"default_value"];
    [TNApp setUILabelText:_L_Email text:text font:font];
    
    text = [TNApp getString:@"balance"];
    [TNApp setUILabelText:_L_BalanceLabel text:text font:font];
    
    text = [TNApp getString:@"default_value"];
    [TNApp setUILabelText:_L_Balance text:text font:font];
    
    text = [TNApp getString:@"done"];
    [TNApp setUIButtonText:_B_Done text:text font:font];
    
    text = [TNApp getString:@"credit_descr"];
    [TNApp setUILabelText:_L_CreditDescr text:text font:font];
    
    text = [TNApp getString:@"credit1"];
    [TNApp setUIButtonText:_B_1credit text:text font:font];
    
    text = [TNApp getString:@"credit5"];
    [TNApp setUIButtonText:_B_5credits text:text font:font];
    
    text = [TNApp getString:@"credit25"];
    [TNApp setUIButtonText:_B_25credits text:text font:font];
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    NameMinLength = 2;
    NameSymbols = @".,-_";
    [self InitControls];
    [self initProgress];
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
                                   initWithTarget:self
                                   action:@selector(dismissKeyboard)];
    [self.view addGestureRecognizer:tap];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(productPurchased:) name:IAPHelperProductPurchasedNotification object:nil];
  
    user = [[TNApp.app getUser] copy];
    [self UpdateUserView:user];
    
    [self EnableButtons:false];
    NSSet * productIdentifiers = [NSSet setWithObjects:
                                  @"app.TranslateNet.credit1",
                                  nil];
    iaph = [[IAPHelper alloc] initWithProductIdentifiers:productIdentifiers];
    _products = nil;
    [self showProgress:true];
    [iaph requestProductsWithCompletionHandler:^(BOOL success, NSArray *products) {
        if (success) {
            _products = products;
            [self EnableButtons:true];
        }
        [self showProgress:false];
    }];
}

-(BOOL) textFieldShouldReturn:(UITextField *)textField{
    
    [textField resignFirstResponder];
    return YES;
}

-(void)dismissKeyboard {
    [_TF_Name resignFirstResponder];
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

-(bool)isResumed
{
    return [self isViewLoaded] && ([self view].window != nil);
}

@end