//
//  IAPHelper.h
//  In App Rage
//
//  Created by Ray Wenderlich on 9/5/12.
//  Copyright (c) 2012 Razeware LLC. All rights reserved.
//

#import <StoreKit/StoreKit.h>

UIKIT_EXTERN NSString *const IAPHelperProductPurchasedNotification;

#define PURCHASE_PRODUCT_1CREDIT        0
#define PURCHASE_PRODUCT_2CREDITS       1
#define PURCHASE_PRODUCT_5CREDITS       2
#define PURCHASE_PRODUCT_10CREDITS      3
#define PURCHASE_PRODUCT_25CREDITS      4
@interface PurchaseData : NSObject {
@public
    NSString *productIdentifier;
    bool purchased;
    NSData *data;
//    NSString *signature;
}
@end

typedef void (^RequestProductsCompletionHandler)(BOOL success, NSArray * products);

@interface IAPHelper : NSObject

-(void)removeTransactionObserver;
- (id)initWithProductIdentifiers:(NSSet *)productIdentifiers;
- (void)requestProductsWithCompletionHandler:(RequestProductsCompletionHandler)completionHandler;
- (void)buyProduct:(SKProduct *)product;
- (BOOL)productPurchased:(NSString *)productIdentifier;
- (void)restoreCompletedTransactions;

@end