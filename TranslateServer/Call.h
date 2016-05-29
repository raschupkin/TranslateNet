/*
 * Call.h
 *
 *  Created on: 17.05.2014
 *      Author: Raschupkin Roman
 */

#ifndef CALL_H_
#define CALL_H_
#include <time.h>
#include <cppconn/connection.h>
#include "User.h"
using namespace std;
class SessionManager;

enum call_state	{	ERROR, INIT, AWAIT, CONFIRMED, REJECTED, ACTIVE, END	};

#define PRICE_ERROR 1000000000

class Call {
public:
	Call(SessionManager *_sManager, unsigned int _id, unsigned int _client, unsigned int _translator, string lang);
	virtual ~Call()						{														}
	void lock()							{	pthread_mutex_lock(&mutex);							}
	void unlock()						{	pthread_mutex_unlock(&mutex);						}
	void reset_lock()					{	mutex = PTHREAD_MUTEX_INITIALIZER;		}
	unsigned int getID()				{	return id;											}
	unsigned int getClient()			{	return client;										}
	unsigned int getTranslator()		{	return translator;									}
	call_state getState()				{	return state;										}
	void setTranslateLang(string _lang)	{	if (Langs::isLang(_lang))	translateLang = _lang;	}
	string getTranslateLang()			{	return translateLang;								}
	void setClientLang(string _lang)	{	if (Langs::isLang(_lang))	clientLang = _lang;		}
	string getClientLang()				{	return clientLang;									}
	string getTranslatorName()			{	return translatorName;								}
	string getClientName()			{	return clientName;								}
	long getPrice()						{	return price;										}
	time_t getStart()					{	return start_time;									}
	time_t getEnd()						{	return end_time;									}
	time_t getAccountedTime()			{	return accounted;									}
	int getCost()						{	return cost;										}
	void setState(call_state st);
	void Start();
	void End();
	void Error();
	int storeBalances(Client *c, Translator *t);
	long countCost();
	long account(time_t length);
	int tarificate(Connection *sqlCon);
	unsigned int getPeer(unsigned int id);
	// must be called from client as it locks translator's session
	void setTranslatorBusy(bool busy);

	int getStartTime()					{	return start_time;									}
	int getEndTime()					{	return end_time;									}

	void setBalance(long _balance)		{	client_balance = _balance;							}
	long getBalance()					{	return client_balance;								}

	// CallManager must be locked, SessionManager must be read-locked
	void sendTimeouts();
	// CallManager must be locked, SessionManager must be read-locked
	int sendCallStatuses();
	// CallManager must be locked, SessionManager must be read-locked
	int sendCallRequestError(int error);

	virtual int DBread(Connection *sqlCon);
	virtual int DBwrite(Connection *sqlCon);
	static const char *StateToString(call_state st);
protected:
	unsigned int id;
	call_state state;
	unsigned int client;
	unsigned int translator;
	string translateLang;
	string clientLang;
	string translatorName;
	string clientName;
	long price;
	time_t start_time;
	time_t end_time;
	time_t accounted;
	long cost;
	long client_balance;
	pthread_mutex_t mutex;
	SessionManager *sManager;
};

class PhoneCall : public Call {
public:
	PhoneCall(SessionManager *_sManager, unsigned int _id, unsigned int _client, unsigned int _translator, string lang);
	~PhoneCall()						{														}
	void setClientCountry(string cc)	{	client_country = cc;								}
	string getClientCountry()			{	return client_country;								}
	void setTranslatorCountry(string tc){	translator_country = tc;							}
	string getTranslatorCountry()		{	return translator_country;							}

	void sentRequest();
	void receivedConfirm(bool accept);
	int savePhones();
	char *getClientPhone()				{	return client_phone;								}
	char *getTranslatorPhone()			{	return translator_phone;							}
	time_t getRequestTime()				{	return request_time;								}
	time_t getConfirmTime()				{	return confirm_time;								}

	int DBread(Connection *sqlCon);
	int DBwrite(Connection *sqlCon);
private:
	string client_country;
	string translator_country;
	time_t request_time;
	time_t confirm_time;
	bool accepted;
	char client_phone[MAX_PHONE+1];
	char translator_phone[MAX_PHONE+1];
};

class CallManager {
public:
	CallManager(SessionManager *sManager);
	void rdlock()						{	pthread_rwlock_rdlock(&rwlock);						}
	void wrlock()						{	pthread_rwlock_wrlock(&rwlock);						}
	void unlock()						{	pthread_rwlock_unlock(&rwlock);						}
	Call *getCall(unsigned int call_id);
	unsigned int findCall(unsigned int c, unsigned int t);
	vector <unsigned int> findCalls(unsigned int u);
	unsigned int findActiveConfirmedCall(unsigned int u);
	unsigned int findCall(unsigned int u, char *peer_phone);
	void removeCall(unsigned int call_id);
	unsigned int newPhoneCall(Connection *sqlCon, unsigned int client, unsigned int translator, string lang);
	void EndCall(Connection *sqlCon, unsigned int call_id);
	void MaintainCalls(Connection *sqlCon);
	int currentCallNum();
	void updateCallState(vector<Translator> &tlist, int client);
	vector<Call> getClientList(unsigned int translator);
private:
	unsigned int storeCall(Connection *sqlCon, bool phone, unsigned int client, unsigned int translator);
	int addCall(Call *call);
	vector<Call *> calls;
	pthread_rwlock_t	rwlock;
	SessionManager *sManager;
};

#endif /* CALL_H_ */
