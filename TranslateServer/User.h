/*
 * user.h
 *
 *  Created on: 02.05.2014
 *      Author: Raschupkin Roman
 */

#ifndef USER_H_
#define USER_H_

#include <stdlib.h>
#include <vector>
#include <cppconn/connection.h>
#include "Langs.h"
using namespace std;

#define TRANSLATORS_CALL	1

#define MAX_PASSWORD				8
#define MIN_PHONE					6
#define MAX_PHONE					16		// 15 and '+' sign
#define MAX_SMS_CODE				5
#define	MAX_NAME					30
#define	MAX_EMAIL					254
#define MAX_OS						16
#define MAX_DEVICE_ID				128
#define MAX_RESET_PASS_REQEST_UID	32

class TwilioMessage;

class TranslatorStatistic
{
public:
	unsigned int all_rating_num;
	unsigned int higher_rating_num;
	unsigned int money_sum;
	unsigned int call_num;
	unsigned int call_time_sum;
};

class User {
public:
	User(Connection *_sqlCon, unsigned int _id);
	virtual ~User()						{	pthread_mutex_destroy(&mutex);			}
	void lock()							{	pthread_mutex_lock(&mutex);				}
	void unlock()						{	pthread_mutex_unlock(&mutex);			}
	void reset_lock()					{	mutex = PTHREAD_MUTEX_INITIALIZER;		}
	unsigned int getID()				{	return id;								}
	char *getEmail()					{	return email;							}
	char *getName()						{	return name;							}
	bool checkNameUnique(char *_name, Connection *sqlCon);
	bool setName(char *_name, Connection *sqlCon);
	char *getPhone()					{	return phone;							}
	void setPhone(char *_phone, int status);
	char *getAwaitPhone()				{	return await_phone;						}
	int getPhoneStatus()				{	return phone_status;					}
	void setLoginParams(string _os, int _version, string _device_id);
	void setCountry(string _country);
	string getCountry()					{	return Langs::isCountry(country) ? country : COUNTRY_UNKNOWN;	}
	string getClientOS()				{	return ClientOS;						}
	int getClientVersion()				{	return ClientVersion;					}
	string getClientDeviceID()			{	return ClientDeviceID;					}

	// session must be locked
	long getBalance()					{	return balance;							}
	int storeBilling(int result_code, int money);

	string getListLang()				{	return ListLang;						}
	void setListLang(string _ListLang)	{	ListLang = _ListLang;					}
	void setLangs(vector<pair<string, long>> _langs);
	Langs getLangs()					{	return langs;							}

	virtual int DBRead() = 0;
	virtual int DBWrite() = 0;
	static string formatDeviceID(char *device_id);

	int determineSMSCodeTimeout(Connection *sqlCon);
	unsigned int storeSMSCode(Connection *sqlCon, char *phone, char *sms_code, string device_id);
	int getLastSMS(int type, TwilioMessage *msg);
	static int storeSMSTwilio(Connection *sqlCon, string id, unsigned int phone_id, int status, int error, int type);
	static int updateSMSTwilio(Connection *sqlCon, string sms_id, int status, int error);
	static vector<TwilioMessage> getUnsentSMSTwilio(Connection *sqlCon);
	int getSMSSentNum() 				{	return SMSSentNum;						}
	int getSMSBlockDays() 				{	return SMSBlockDays;					}

	int resetUserPhoneAwait(Connection *sqlCon);
	int resetUserPhone(Connection *sqlCon);
	bool checkSMSCode(Connection *sqlCon, char *sms_code, char *phone);
	int resetOtherUsersPhones(Connection *sqlCon, char *phone);
	int updateUserPhone(Connection *sqlCon);
	string getLastDeviceID(Connection *sqlCon);

	int updateUserLoginTime(Connection *sqlCon, bool login);
	time_t getUserLoginsLength(Connection *sqlCon);
	static int deleteUser(Connection *sqlCon, unsigned int id);
	static unsigned int checkUserExist(Connection *sqlCon, char *email, bool &deleted);
	static User *readUser(Connection *sqlCon, unsigned int id);
	static int updateUserEmailAck(Connection *sqlCon, unsigned int id);
	static int storePassword(Connection *sqlCon, unsigned int id, char *password);
	static unsigned int storeUser(Connection *sqlCon, char *email, bool isTranslator);
	static bool checkPassword(Connection *sqlCon, char *email, char *password, unsigned int &id);
	static bool checkUserEmailAck(Connection *sqlCon, unsigned int id);
	static int genResetPasswordRequestUID(Connection *sqlCon, char *request_uid);
	static int storeResetPasswordRequest(Connection *sqlCon, unsigned int id, char *request_uid);
	static unsigned int getConfirmResetPasswordID(Connection *sqlCon, char *request_uid);

	static int CalcAveragePrice(Connection *sqlCon, string lang);
	static bool CheckNeedMark(Connection *sqlCon, unsigned int client, unsigned int translator);
	static unsigned int readAccountID(Connection *sqlCon, const char *EmailServer);

	void onPurchaseVerified(Connection *sqlCon, CallManager *cManager, int money);
	int StoreTransfer(Connection *sqlCon, int result_code, int money);
	void FinalizeTransfer(Connection *sqlCon);
	int changeBalance(CallManager *cManager, long BalanceChange);
	static int changeBalanceDyn(Connection *sqlCon, SessionManager *sManager, unsigned int id, long BalanceChange);

	static void formatEmail(char *email);
	static bool checkEmail(char *email);
	static int formatPhone(char *phone);
	static bool checkPhone(char *phone);
	static bool equalPhone(char *phone1, char *phone2);
	static void genSMSCode(char *code, int len);
	static void genPassword(char *password, int len);
	static void genUID(char *uid, int len);


	void setAwait(bool _await)			{	await = _await;			}
	bool getAwait()						{	return await;			}
	void setConfirmed(bool _confirmed)	{	confirmed = _confirmed;	}
	bool getConfirmed()					{	return confirmed;		}
	void setRejected(bool _rejected)	{	rejected = _rejected;	}
	bool getRejected()					{	return rejected;		}
	void setError(bool _error)			{	error = _error;			}
	bool getError()						{	return error;			}
protected:
	int readLangs();
	int writeLangs();
#define DIGEST_LEN	512/8
	static void genNonce(unsigned char *nonce, int len);
	static string NonceToString(unsigned char *nonce, int len);
	static int StringToNonce(string str, unsigned char *nonce, int len);
	static unsigned int computeDigest(unsigned char *nonce, int nonce_len, char *pwd, int pwd_len, unsigned char *digest);

	Connection *sqlCon;
	unsigned int id;
	char name[MAX_NAME+1];
	char email[MAX_EMAIL+1];
	Langs langs;
	char phone[MAX_PHONE+1];
	int phone_status;					// defined in protocol.h
	char await_phone[MAX_PHONE+1];
	int readPhone();
	int SMSSentNum, SMSBlockDays;

	long balance;						// also stored in call->balance
	string ListLang;

	string country;
	string ClientOS;					// defined in protocol.h
	int ClientVersion;
	string ClientDeviceID;

	pthread_mutex_t mutex;				// used for balance, list_lang and rating
										// client can lock with translators locked at sendPacket_TranslatorList

	bool await, confirmed, rejected, error;	// CallState for translator and client lists
						// needed because can't task with nettask in background in iOS
						// and because of killing app in background for android
};


class Client : public User {
public:
	Client(Connection *sqlCon, unsigned int _id);
	~Client()							{											}
	int	DBRead();
	int DBWrite();
	bool CheckMark(Connection *sqlCon, unsigned int translator);
	void AddMark(Connection *sqlCon, unsigned int translator, int rating);
	void onPurchaseVerified(Connection *sqlCon, SessionManager *sManager, CallManager *cManager, int money);
	int StoreTransfer(Connection *sqlCon, int result_code, int money);
	int changeBalance(CallManager *cManager, long BalanceChange);
};


class Translator : public Client {
public:
	Translator(Connection *sqlCon, unsigned int _id);
	~Translator()						{											}
	int	DBRead();
	int DBWrite();
	string getCommonLang(User *u);
	long getPrice(string lang)			{	return langs.getPrice(lang);			}
	long getClientPrice(User *u, string l);
	float getFeePercent();

	bool isBusy()						{	return busy;							}
	bool setBusy(bool _busy);

	float CountRating(int &rating_num);
	void UpdateRating();
	float getRating()					{	return rating;							}
	int getRatingNum()					{	return rating_num;						}

	int getTranslatorStatistic(Connection *sqlCon, TranslatorStatistic &tstat);

	static unsigned int checkUserExistPayPal(Connection *sqlCon, const char *paypal_email, bool &deleted);
	char *getPayPalEmail()				{	return PayPalEmail;						}
	int setPayPalEmail(char *_PayPalEmail);
	bool isPayPalTransferActive()		{	return PayPalTransferActive;			}
	bool setPayPalTransferActive(bool active)	{	PayPalTransferActive = active;	}
	void onPurchaseVerified(Connection *sqlCon, SessionManager *sManager, CallManager *cManager, int money);
	int StoreTransfer(Connection *sqlCon, int result_code, int money);
	int changeBalance(CallManager *cManager, long BalanceChange);

	void setTListDel(bool _del)			{	TListDel = _del;						}
	bool getTListDel()					{	return TListDel;						}
private:
	bool busy;
	float rating;
	int rating_num;
	char PayPalEmail[MAX_EMAIL+1];
	bool PayPalTransferActive;
	time_t LoginLength;

	bool TListDel;	//	for tlist delete flag
};

#endif /* USER_H_ */
