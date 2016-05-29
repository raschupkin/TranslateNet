//
//  Options.h
//  Translate-Net
//
//  Created by Admin on 23.01.15.
//  Copyright (c) 2015 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_Options_h
#define Translate_Net_Options_h

@interface Options : NSObject<NSCopying> {
@public
    int CallTimeFree;
    int CallMinBalance;
    int CallMinTimeRating;
    int ActiveTSearch;
};
-(void)init_default;
@end

#endif
