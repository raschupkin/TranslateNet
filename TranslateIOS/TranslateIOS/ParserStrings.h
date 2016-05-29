//
//  ParserStrings.h
//  Translate-Net
//
//  Created by Admin on 13.12.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_ParserStrings_h
#define Translate_Net_ParserStrings_h


@interface ParserStrings : NSObject <NSXMLParserDelegate> {
    NSXMLParser *parser;
    NSMutableString *currentData;
    bool error;
    NSMutableDictionary *strings;
    NSString *_cur_name;
    NSString *_cur_value;
}
- (NSMutableDictionary *) parseStrings: (NSData *)xmlStrings;
@end

#endif
