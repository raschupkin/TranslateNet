/*
 * ServiceSMS.h
 *
 *  Created on: 27.02.2015
 *      Author: roman
 */

#ifndef SERVICESMS_H_
#define SERVICESMS_H_

#include "ServiceAsync.h"

typedef struct _TwilioSMSServiceParams : ServiceParams {
	const char *Addr;
	const char *Account;
	const char *AuthToken;
	const char *From;
	bool Log;
} TwilioSMSServiceParams;

class TwilioMessage : public ServiceTask {
public:
	~TwilioMessage() {}
	string phone;
	string text;
};

class ServiceTwilioSMS : public ServiceAsync {
public:
	ServiceTwilioSMS(SessionManager *_sManager, TwilioSMSServiceParams *_params);
	virtual const char *getName()					{	return "TwilioSMS";						}
	virtual ~ServiceTwilioSMS() {}
private:
	static size_t curl_output(void *ptr, size_t size, size_t nmemb, void *userdata);
	int ProcessTask(ServiceTask *task);
	TwilioSMSServiceParams params;
};




#endif /* SERVICESMS_H_ */
