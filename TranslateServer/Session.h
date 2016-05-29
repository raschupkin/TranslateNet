/*
 * Session.h
 *
 *  Created on: 01.05.2014
 *      Author: roman
 */

#ifndef SESSION_H_
#define SESSION_H_

#include <stdlib.h>
#include <vector>
#include <queue>
#include <pthread.h>
#include <openssl/ssl.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "protocol.h"
class TwilioMessage;

using namespace std;
using namespace sql;
class SessionManager;
class User;
class Client;
class Translator;
class Call;
class CallManager;

#define VERSION_DEFAULT	0

class Session {
public:
	Session(SessionManager *_sManager, CallManager *_cManager, int _ThreadN, SSL *_ssl, BIO *bio);
	~Session();
	void lock()								{	pthread_mutex_lock(&mutex);		}
	void unlock()							{	pthread_mutex_unlock(&mutex);	}
	int getSocket()							{	return socket;					}
	int getThread()							{	return ThreadN;					}
	struct sockaddr_in getIP();
	User *getUser();
	time_t getTimer()						{	return timeout;					}
	int ProcessPacket();
	int recvPacket();
	int sendPacket(const char *xml);
	int WritePacketQueue();
	bool isSSL_WANT_WRITE()					{	return ssl_want_write;			}
	bool isPacketQueueEmpty()				{	return out_packets.empty();		}

#define	SQLTIME_FMT	"%y-%m-%d %H:%M:%S"
    int sendPacket_Error(int error, char *message = 0, int id=0);
    int sendPacket_CommandError(int error, string command, char *message = 0, int id=0);
    int sendPacket_SMSError(int error);
    int sendPacket_BillingError(int error, int money);
    int sendPacket_LoginError(int error, int langs_version);
    int sendPacket_CallRequestError(int error, bool confirm, unsigned int call_id);
    int sendPacket_PayPalTransferError(int error);
    User *doLogin(unsigned int id);
    int processPacket_Login(xercesc::DOMNode *node);
    int sendPacket_Challenge();
    bool checkUserExist(char *email, bool &deleted);
    int sendPacket_AwaitLogin();
    int processPacket_RegisterUser(xercesc::DOMNode *node);
    int resetPassword(unsigned int id, char *password);
    int processPacket_ResetPassword(xercesc::DOMNode *node);
    int processPacket_ChangePassword(xercesc::DOMNode *node);
    int processPacket_DeleteUser(xercesc::DOMNode *node);
    int processPacket_SetCountry(xercesc::DOMNode *node);
    int sendSMSCode(TwilioMessage *msg);
    int processPacket_RegisterPhone(xercesc::DOMNode *node);
    int sendPacket_AwaitPhoneConfirm();
    int processPacket_ResendSMS(xercesc::DOMNode *node);
    int processPacket_ConfirmRegisterPhone(xercesc::DOMNode *node);
    int processPacket_getLanguages(xercesc::DOMNode *node);
    void SetTranslatorBusy(bool busy);
    int processPacket_SetBusy(xercesc::DOMNode *node);
    void update_TLists(bool del);
    int processPacket_getUserData(xercesc::DOMNode *node);
    int sendPacket_UserData(long balance);
    int processPacket_UserData(xercesc::DOMNode *node);
    int sendPacket_UserData();
    int onPayPalPurchaseVerified(int result_code, int money);
    int onPurchaseVerified(int result_code, int money);
    int processPacket_Billing(xercesc::DOMNode *node);
    int processPacket_PayPalTransfer(xercesc::DOMNode *node);
    int sendPacket_TranslatorList(vector<Translator> tlist, bool del);
    int processPacket_RequestTranslatorList(xercesc::DOMNode *node);
    int processPacket_StopTranslatorList(xercesc::DOMNode *node);
    int sendPacket_ClientList(vector<Call> calllist, bool del);
    int processPacket_RequestClientList(xercesc::DOMNode *node);
    int processPacket_PhoneCallRequest(xercesc::DOMNode *node);
    int sendPacket_PhoneCallRequest(unsigned int call_id);
    int processPacket_PhoneCallConfirm(xercesc::DOMNode *node);
    int sendPacket_PhoneCallConfirm(unsigned int call_id, bool accept);
    int sendPacket_PhoneCallTimeout(bool client, int user);
    int processPacket_CallStatus(xercesc::DOMNode *node);
    int sendPacket_CallStatus(unsigned int call_id);
    int processPacket_getCallHistory(xercesc::DOMNode *node);
    int sendPacket_CallHistory();
    int processPacket_GetMarkHistory(xercesc::DOMNode *node);
    int sendPacket_MarkHistory();
    void AddMark(unsigned int translator, int rating);
    int processPacket_MarkRating(xercesc::DOMNode *node);
    int sendPacket_MarkRequest(unsigned int translator, string name, unsigned long time);
    int processPacket_GetStatistic(xercesc::DOMNode *node);
    int sendPacket_Statistic();
    int processPacket_GetTranslatorStatistic(xercesc::DOMNode *node);
    int sendPacket_TranslatorStatistic();

	Connection *getCon();
	xercesc::XercesDOMParser *getParser();
	void Close();
	void setMarkEnd()							{	markEnd = true;			}
	bool getMarkEnd()							{	return markEnd;			}
	void setMarkAbort(bool _SamePhone, bool _SameIP);
	bool getMarkAbort()							{	return markAbort;		}
	bool getAbortSamePhone()					{	return AbortSamePhone;		}
	bool getAbortSameIP()						{	return AbortSameIP;			}
	bool get_DetectPeerPhone()					{	return DetectPeerPhone;	}
	bool isOS(char *os_string);
	int getClientVersion()						{	return ClientVersion;	}
private:
	struct CommandEntry {
		const char *command;
		bool require_login;
		unsigned int error;
		int (Session::*process)(xercesc::DOMNode *node);
	};
	static CommandEntry CommandTable[];
	pthread_mutex_t mutex;		// lock for session and user
	SessionManager *sManager;
	CallManager *cManager;
	int ThreadN;
	SSL *ssl;
	BIO *bio;
	int socket;

	uint8_t *packet;
	uint8_t *recvxml;
	size_t data_len;
	string current_command;
	struct out_packet {
		Header *data;
		int len;
		int written;
	};
	queue<struct out_packet *> out_packets;
	bool ssl_want_write;
	pthread_mutex_t	out_mutex;
	void out_lock()		{	pthread_mutex_lock(&out_mutex);		}
	void out_unlock()	{	pthread_mutex_unlock(&out_mutex);	}
	bool markEnd;
	bool markAbort, AbortSamePhone, AbortSameIP;

	unsigned int login_id;
	string OS;
	int ClientVersion;
	bool DetectPeerPhone;
	string country;
	User *user;						// 0 until receiving correct password

	time_t timeout;
	unsigned long PacketCount;
};

#endif /* SESSION_H_ */
