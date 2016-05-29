//
//  ParserStrings.m
//  Translate-Net
//
//  Created by Admin on 13.12.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ParserStrings.h"

@implementation ParserStrings : NSObject


- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
    [currentData setString:@""];
    if (error)
        return;
    _cur_name = [attributeDict valueForKey:@"name"];;
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName
{
    if (error)
        return;
    _cur_value = [currentData copy];
    if ([_cur_value characterAtIndex:0] == '\"')
        _cur_value = [_cur_value substringFromIndex:1];
    unsigned long len = [_cur_value length];
    if ([_cur_value characterAtIndex:len-1] == '\"')
        _cur_value = [_cur_value substringToIndex:len-1];
    if ([strings objectForKey:_cur_name] == nil)
        strings[_cur_name] = _cur_value;
    [currentData setString: @""];
}

- (void)parser:(NSXMLParser *)parser foundCharacters:(NSString *)string
{
    [currentData appendString:string];
}

- (NSMutableDictionary *) parseStrings: (NSData *)xmlStrings
{
    parser = [[NSXMLParser alloc] initWithData: xmlStrings];
    [parser setDelegate: self];
    currentData = [[NSMutableString alloc] init];
    error = false;
    strings = [[NSMutableDictionary alloc] init];
    if (![parser parse])
        return NULL;
    if (error) {
        error = false;
        return NULL;
    }
    return strings;
}

@end