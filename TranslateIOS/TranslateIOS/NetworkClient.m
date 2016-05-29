//
//  NetworkClient.m
//  Translate-Net
//
//  Created by Admin on 17.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "NetworkClient.h"

@implementation DataPacket
- (void) initWithString: (NSString *) string
{
    byteIndex = 0;
    len = (unsigned int)string.length + 4;
    *(unsigned int *)buf = (unsigned int)string.length;
    memcpy(buf+4, [string UTF8String], string.length);
}
@end

@implementation  NetworkClient
@synthesize connectionTimer;
@synthesize receiver;
static NetworkClient *client;
+ (NetworkClient *) client {    return client;   }

+ (void) init
{
    client = [[NetworkClient alloc] init];
}

+ (void) finit
{
    if (client != nil)
        [client stopConnectionTimer];
}

- (void)setReceiver:(id)_receiver
{
    receiver = _receiver;
}

- (int)onPacketReceived: (DataPacket *)packet
{
    if (parser == nil)
        return 0;
    NSData *xmlData = [NSData dataWithBytes:packet->buf length:packet->len];
    if (xmlData == nil)
        return [receiver onNetwork_Error:NETWORK_ERROR_OTHER];
    ParsedPacket *p = [parser parsePacket:xmlData];
    if (p == nil)
        return [receiver onNetwork_Error:NETWORK_ERROR_FORMAT];
    [receiver onNetwork_PacketReceived:p];
    return 0;
}

- (int)sendPacket: (NSString *)xml
{
    DataPacket *p = [[DataPacket alloc] init];
    if (p == nil)
        return -1;
    [p initWithString:xml];
    [writeQueueLock lock];
    [writeQueue addObject:p];
    [writeQueueLock unlock];
    if (sendAvailable)
        [self sendPacketQueue:oStream];
    return 0;
}

- (void)sendPacketQueue: (NSOutputStream*) os
{
    [writeQueueLock lock];
    do {
        if ([writeQueue count] == 0) {
            sendAvailable = true;
            [writeQueueLock unlock];
            return;
        }
        DataPacket *p = [writeQueue objectAtIndex:0];
        unsigned long len = ((p->len - p->byteIndex >= 1024) ?
                             1024 : (p->len-p->byteIndex));
        len = p->len - p->byteIndex;
        unsigned long written = [os write: (p->buf + p->byteIndex)
                                maxLength: len];
        p->byteIndex += written;
        if (written < len) {
            sendAvailable = false;
            [writeQueueLock unlock];
            return;
        }
        if (p->byteIndex == p->len)
            [writeQueue removeObject:p];
    } while (true);
}

- (void)recvPacket: (NSInputStream *)is
{
    unsigned long read = [is read: (rdbuf + rdpos) maxLength:(PACKETBUF_SIZE - rdpos)];
    rdpos += read;
    while (true) {
        if (rdpos <= 4)
            return;
        unsigned int len = *(unsigned int *)rdbuf;
        if (len > PACKETBUF_SIZE) {
            [self Disconnect];
            return;
        }
        if (rdpos < 4 + len)
            return;
        readPacket = [[DataPacket alloc] init];
        readPacket->len = len;
        memcpy(readPacket->buf, rdbuf + 4, len);
        int l =[self onPacketReceived: readPacket];
        if (l < 0) {
            [self Disconnect];
            return;
        }
        rdpos -= 4 + len;
        memcpy(rdbuf, rdbuf + 4 + len, rdpos);
    }
    
}

- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode
{
    switch (eventCode) {
        case NSStreamEventOpenCompleted:
        {
            Connected = true;
            informedError = false;
            [self stopConnectionTimer];
            break;
        }
        case NSStreamEventHasBytesAvailable:
        {
            [self recvPacket:(NSInputStream *)stream];
            break;
        }
        case NSStreamEventHasSpaceAvailable:
        {
            [self sendPacketQueue: (NSOutputStream *)stream];
            break;
        }
        case NSStreamEventEndEncountered:
        case NSStreamEventErrorOccurred:
        {
            if (!informedError)
                [receiver onNetwork_Error:NETWORK_ERROR_CONNECTION];
            informedError = true;
            [self Disconnect];
            return;
        }
        case NSStreamEventNone:
        {
            break;
        }
    }
}

- (int) Connect
{
    [self startConnectionTimer];
    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;
    CFStreamCreatePairWithSocketToHost(NULL, (__bridge CFStringRef)SERVER_ADDRESS, SERVER_PORT, &readStream, &writeStream);
    if (readStream) {
        iStream = (__bridge_transfer NSInputStream*) readStream;
        [iStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL forKey:NSStreamSocketSecurityLevelKey];
        [iStream setDelegate:self];
        [iStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [iStream open];
    } else {
        //        CFStreamError  cferr= CFReadStreamGetError(readStream);
        //NSError *err = (__bridge NSError *)cferr;
        [self stopConnectionTimer];
        return -1;
    }
    if (writeStream) {
        oStream = (__bridge_transfer NSOutputStream*) writeStream;
        [oStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL forKey:NSStreamSocketSecurityLevelKey];
        [oStream setDelegate:self];
        [oStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [oStream open];
    } else {
        [self stopConnectionTimer];
        return -1;
    }
    
    NSDictionary *settings = [[NSDictionary alloc] initWithObjectsAndKeys:
                              //[NSNumber numberWithBool:YES],
                              //kCFStreamSSLAllowsExpiredCertificates,
                              //                              [NSNumber numberWithBool:YES],
                              //                              kCFStreamSSLAllowsAnyRoot,
                              //                              [NSNumber numberWithBool:NO],
                              //                              kCFStreamSSLValidatesCertificateChain,
                              //                              kCFNull, kCFStreamSSLPeerName,
                              SERVER_CERT_NAME, kCFStreamSSLPeerName,
                              nil];
    
    CFReadStreamSetProperty((CFReadStreamRef)iStream, kCFStreamPropertySSLSettings, (CFTypeRef)settings);
    CFWriteStreamSetProperty((CFWriteStreamRef)oStream, kCFStreamPropertySSLSettings, (CFTypeRef)settings);
    if (iStream.streamStatus == NSStreamStatusError ||
        oStream.streamStatus == NSStreamStatusError) {
        [self stopConnectionTimer];
        return -1;
    }
    
    parser = [[Parser alloc] init];
    writeQueue = [[NSMutableArray alloc] init];
    writeQueueLock = [[NSLock alloc] init];
    return 0;
}

- (void) Disconnect
{
    Connected = false;
    sendAvailable = false;
    [iStream close];
    [oStream close];
}

- (void)startConnectionTimer
{
    [self stopConnectionTimer];
    self.connectionTimer = [NSTimer scheduledTimerWithTimeInterval: TIMEOUT_CONNECTION
                                                            target:self
                                                          selector:@selector(handleConnectionTimeout)
                                                          userInfo:nil
                  	                                         repeats:NO];
}

- (void)handleConnectionTimeout
{
    if (!Connected)
        [receiver onNetwork_Error:NETWORK_ERROR_CONNECTION];
}

- (void)stopConnectionTimer
{
    if (connectionTimer)
    {
        [connectionTimer invalidate];
        connectionTimer = nil;
    }
}

@end