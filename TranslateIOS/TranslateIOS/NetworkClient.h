//
//  NetworkClient.h
//  Translate-Net
//
//  Created by Admin on 17.11.14.
//  Copyright (c) 2014 Translate-Net. All rights reserved.
//

#ifndef Translate_Net_NetworkClient_h
#define Translate_Net_NetworkClient_h

#import "Parser.h"
#import "TNApp.h"

#define SERVER_ADDRESS          @"translate-net.com"
//#define SERVER_ADDRESS          @"194.58.108.201"
//#define SERVER_ADDRESS          @"192.168.43.148"

#define SERVER_PORT             45914
#define SERVER_CERT_NAME        @"translate-net.com"
#define TIMEOUT_CONNECTION      30      // seconds

#define NETWORK_ERROR_OK            0
#define NETWORK_ERROR_OTHER        -1
#define NETWORK_ERROR_CONNECTION    -2
#define NETWORK_ERROR_FORMAT        -3
@protocol NetworkReceiver
- (int) onNetwork_PacketReceived: (ParsedPacket *) packet;
- (int) onNetwork_Error: (int) code;
//- (void) onError: (NSString *)msg;
@end

#define PACKETBUF_SIZE  1048576
@interface DataPacket : NSObject {
@public
    uint8_t buf[PACKETBUF_SIZE];
    unsigned int len;
    int byteIndex;
}
- (void) initWithString: (NSString *) string;
@end

@interface NetworkClient : NSObject {
@private
    NSInputStream *iStream;
    NSOutputStream *oStream;
    bool Connected;
    NSTimer* connectionTimer;
    NSMutableArray *writeQueue;
    NSLock *writeQueueLock;
    bool sendAvailable;
    uint8_t rdbuf[PACKETBUF_SIZE];
    unsigned long rdpos;
    DataPacket *readPacket;
    bool informedError;

    Parser *parser;
};
@property (nonatomic, weak) id<NetworkReceiver> receiver;
@property (nonatomic, retain) NSTimer* connectionTimer;
+ (void) init;
+ (void) finit;
+ (NetworkClient *) client;
- (void)setReceiver:(id<NetworkReceiver>)_receiver;
- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode;
- (int)sendPacket: (NSString *)xml;
- (int)Connect;
- (void)Disconnect;
- (void)startConnectionTimer;
- (void)handleConnectionTimeout;
- (void)stopConnectionTimer;
@end

#endif
