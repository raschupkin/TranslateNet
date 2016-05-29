/*
 * SessionManager.h
 *
 *  Created on: 22.05.2014
 *      Author: Raschupkin Roman
 */

#ifndef SESSIONMANAGER_H_
#define SESSIONMANAGER_H_

#include <map>
#include "Session.h"
#include "ServiceEmail.h"
#include "ServiceSMSTwilio.h"
#include "ServicePayPal.h"
#include "ServiceIOSVerify.h"
#include "base64.h"
class CallManager;

#define	TIMER_SESSION_CHECK	10000		// milliseconds
#define TIMER_CALLS_CHECK	10000		// milliseconds
#define TIMER_STAT_UPDATE	5000		// milliseconds
#define TIMER_DB_MAINTAIN	3600000		// milliseconds

typedef struct _Options {
#define OPTIONS_DEBUG_EMAIL		0x01
	int debug;
	const char *ServerAddress;
	int	ServerPort;
	const char *File_DHparam;
	const char *File_Cert;
	const char *File_Key;
	const char *File_GooglePubKey;
	const char *DBHost;
	const char *DBUser;
	const char *DBPassword;

#define PHONE_REGISTER_NONE		0
#define PHONE_REGISTER_SETUP	1
#define PHONE_REGISTER_BALANCE	2
#define PHONE_REGISTER_BALANCE_MIN	100
#define PHONE_REGISTER_ANY		3
	int PhoneRegisterMode;

	const char *File_LangsXML;
	const char *Server_Email;

	EmailServiceParams paramsEmail;

	TwilioSMSServiceParams paramsTwilioSMS;
	PayPalServiceParams paramsPayPal;
	IOSVerifyServiceParams paramsIOSVerify;
	int CurlTimeout;				// seconds

	int FeeMarket;					// 0-100
	int FeeApp;						// 0-100		Not using now as ServerAccount is not used
	int FeeApp_Reduced;				// 0-100
	int FeeApp_TimeReduced;			// seconds

	int TimerSessionTimeout;		// seconds
	int TimerSessionRegTimeout;		// days

	int Stat_statLangs;				// 0/1
	int Stat_MinCallDuration;		// seconds

	int TimeoutNetLag;				// seconds
	int Timeout_CallStatus;			// seconds
	int Timeout_CallRequest;		// seconds
	int Timeout_CallConfirm;		// seconds

	int Call_TimeFree;				// seconds
	int Call_MinLength;				// seconds
	int Call_MinTimeRating;			// seconds
	int SMS_MaxNum;					// maximum number of SMS in last SMS_MaxDays (not counting resended when status="undelivered")
	int SMS_MaxDays;				// days			Period of accounting last SMSes num
	int SMS_BlockDays;				// days			Period of blocking from last SMS
	int ActiveTSearch;				// 0 or 1

	int CallHistoryLimit;
	bool DEBUG_EMAIL;
	bool DEBUG_SMS;
} Options;


struct Statistic {
	int clients;
	int translators;
	map<std::string, int> langnum;
	int calls_hour;
	int users_hour;
	int calls_day;
	int users_day;
	int stats_calls_hour;		// stored in database possibly from last runs
	int stats_users_hour;
};

#define THREAD_SERVICE		0
#define THREAD_RESET_PASS	1
#define THREAD_EMAIL		2
#define THREAD_SMS			3
#define THREAD_IOS_VERIFY	4		// must be 1 thread
#define THREAD_PAYPAL		5		// must be 1 thread
#define THREAD_SERVICE_NUM		(THREAD_PAYPAL+1)
class SessionManager {
public:
	void LogOptions();
	SessionManager(int _ThreadN, pthread_t _main_thread, Driver *_sqlDriver, Options _options);
	~SessionManager();
	static Options getOptions()							{	return options;										}
	int LoadGooglePubKey(string fname);
	void InitSQLConnection(Connection *sqlCon);
	Connection *getCon(int threadN)						{	return sqlCon[THREAD_SERVICE_NUM + threadN];		}
	Connection *getServiceCon()							{	return sqlCon[THREAD_SERVICE];						}
	Connection *getResetServiceCon()					{	return sqlCon[THREAD_RESET_PASS];					}
	Connection *getSMSCon()								{	return sqlCon[THREAD_SMS];							}
	Connection *getEmailCon()							{	return sqlCon[THREAD_EMAIL];						}
	Connection *getPayPalCon()							{	return sqlCon[THREAD_PAYPAL];						}
	Connection *getIOSVerifyManagerCon()				{	return sqlCon[THREAD_IOS_VERIFY];					}
	int getThreadNum()									{	return ThreadNum;									}
	xercesc::XercesDOMParser *getParser(int threadN)	{	return parser[threadN];								}

	CallManager *getCallManager()						{	return cManager;									}
	Translator *getServerAccount()						{	return ServerAccount;								}
	static Translator *readServerAccount(Connection *sqlCon, const char *Email_Server);
	void startMaintenanceExit()							{	MaintenanceExit = true;								}
	bool isMaintenanceExit()							{	return MaintenanceExit;								}
	bool isMaintenanceExitReady();

	void rdlock()										{	pthread_rwlock_rdlock(&sessions_lock);				}
	void wrlock()										{	pthread_rwlock_wrlock(&sessions_lock);				}
	void unlock()										{	pthread_rwlock_unlock(&sessions_lock);				}
	void setLangsXML(string _LangsXML);
	string getLangsXML();

	void Process(int cpu);
	void ProcessService();
	void ProcessResetPass();
	void ProcessEmail();
	void ProcessServiceHTTP(ServiceAsync *service, Connection *sqlCon);
	Session *findSessionByUser(unsigned int id);
	Session *findSessionByPayPalEmail(const char *paypal_email);
	int addSession(SSL *ssl, BIO *bio);
	int removeSession(Session *s);
	int EndSession(Connection *sqlCon, Session *s, bool send_tlist, bool abort_calls = false);
	int markAbortSessionByUser(Session *session, unsigned int id);
	void markAbortSessionByPhone(Connection *sqlCon, Session *session);		// same user, no need to resend tlists
	void DeleteMarkedSessions(Connection *sqlCon);
	void DynUserLock()							{	pthread_mutex_lock(&DynUser_lock);							}
	void DynUserUnlock()						{	pthread_mutex_unlock(&DynUser_lock);						}
	vector<Translator> getTranslatorList(User *u);
	void Lists_processUpdate(vector<Translator> tlist, bool del);

	unsigned int findConfirmedResetPassword(Connection *sqlCon);
	void doResetPassword(Connection *sqlCon, unsigned int id);

	int sendEmail_Password(char *email, char *password, string lang);
    int sendEmail_ResetPassword(char *email, char *reset_uid, string lang);
    int sendEmail_NewPassword(char *email, char *password, string lang);
	int sendEmail(int type, char *email, string topic, string body);
	bool onEmailSendFailure(int email_type, const char *email);

	int sendSMSMessage(TwilioMessage *msg);

#define BILLING_VERIFY_OK				0
#define BILLING_VERIFY_ERROR_SIGNATURE	-1
#define BILLING_VERIFY_FAILURE			-999
	int IOSPurchaseVerify(IOSPurchase p);
	int DecodeBase64(const char *b64, unsigned char *data, int data_len);
	int AndroidPurchaseVerify(int money, string data, string signature);
	void Purchase_VerifyHandler(IOSPurchase p, int result_code);

	int PayPal_Transfer(char *account, int sum);
	bool onPayPalTransferEnd(PayPalTransfer transfer, int result_code);

	void test_email();
	Statistic getStatistic(User *u);
	void UpdateStatistic(Connection *sqlCon, bool OnlyUsers = false);

	static std::map<pthread_t, int> PThreadMap;
	static int getCurThreadN();
private:
	void doMaintenanceExit(int thread, int socket_num);
	int MaintainSessions(Connection *sqlCon);
	int MaintainDB(Connection *sqlCon);
	int MaintainLangsAveragePrice(Connection *sqlCon);
	int ReadLastStats(Connection *sqlCon);
	int WriteStats(Connection *sqlCon);
	int CountCallsStat(Connection *sqlCon, int hours);
	int CountUsersStat(Connection *sqlCon, int hours);
	static Options options;
	int chooseThread();
	Driver *sqlDriver;
	Connection **sqlCon;
	xercesc::XercesDOMParser **parser;
	string LangsXML;					// Langs::AllLangsLock/AllLangsUnlock
	CallManager *cManager;
	volatile bool MaintenanceExit;
	pthread_t main_thread;
	int MaintenanceExitNum;

	vector<Session *> sessions;
	pthread_rwlock_t sessions_lock;
	int ThreadNum;
	size_t max_sockets;
	struct pollfd **sockets;
	int **sockpipe;
	pthread_mutex_t DynUser_lock;		// lock for dynamically reading User not assigned to Session(Call::tarificate, processPacket_MarkRating)

	ServiceEmail serviceEmail;
	ServiceSMSTwilio serviceTwilioSMS;
	ServicePayPal	servicePayPal;
	ServiceIOSVerify serviceIOSVerify;
	char *GooglePubKey;				// for in-app purchase verifing
	Translator *ServerAccount;
	time_t	timer_sessions;
	time_t	timer_calls;
	time_t	timer_db;
	time_t	timer_stat;
	struct Statistic stat;
	pthread_mutex_t stat_lock;
	time_t ServerStartTime;
};

#endif /* SESSIONMANAGER_H_ */
