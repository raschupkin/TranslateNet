/*
 * ServiceSMS.h
 *
 *  Created on: 27.02.2015
 *      Author: roman
 */

#ifndef SERVICESMS_TWILIO_H_
#define SERVICESMS_TWILIO_H_

#include "ServiceAsync.h"

typedef struct _TwilioSMSServiceParams : ServiceParams {
	const char *Addr;
	const char *Account;
	const char *AuthToken;
	const char *From;
	const char *StatusAddr;
	int Send_AttemptNum;
	int Send_Timeout;			// seconds
	bool Log;
} TwilioSMSServiceParams;

#define SMS_STATUS_UNKNOWN		0
#define SMS_STATUS_QUEUED		1
#define SMS_STATUS_SENDING		2
#define SMS_STATUS_SENT			3
#define SMS_STATUS_DELIVERED	4
#define SMS_STATUS_UNDELIVERED	-1
#define SMS_STATUS_FAILED		-2

#define TWILIO_ERROR_UNKNOWN	30008

class TwilioMessage : public ServiceTask {
public:
	string text;
	string id; 			// id in DB table sms_twilio

#define SMS_TYPE_PHONE_CODE		1
	int type;

	string phone;
	string sms_code;
	void GenText_SMSCode();
	int phone_id;		// id in DB table phones, phone_id in DB table sms_twilio

	~TwilioMessage() {}
	string curl_response;
};


class ServiceSMSTwilio : public ServiceAsync {
public:
	ServiceSMSTwilio(SessionManager *_sManager, TwilioSMSServiceParams *_params);
	virtual const char *getName()					{	return "TwilioSMS";						}
	virtual ~ServiceSMSTwilio() {}
	void ProcessUnfinishedTasks();
	static int StatusStrToCode(string str);
private:
	static size_t curl_output(void *ptr, size_t size, size_t nmemb, void *userdata);
	int ProcessTask(ServiceTask *task);
	TwilioSMSServiceParams params;
};


#endif /* SERVICESMS_TWILIO_H_ */
