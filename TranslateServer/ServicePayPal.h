#ifndef PAYPAL_H_
#define PAYPAL_H_

#include "ServiceAsync.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

typedef struct _PayPalServiceParams : ServiceParams {
	const char *Account;
	const char *UserID;
	const char *Password;
	const char *Signature;
	const char *AppID;
	const char *AdaptivePaymentURL;
	const char *CancelURL;
	const char *ReturnURL;
	bool Log;
} PayPalServiceParams;

class PayPalTransfer : public ServiceTask {
public:
	string account;
	int sum;
	~PayPalTransfer() {}
	string curl_response;
};

class ServicePayPal : public ServiceAsync {
public:
	ServicePayPal(SessionManager *_sManager, PayPalServiceParams *_params);
	virtual const char *getName()					{	return "PayPal";						}
	virtual ~ServicePayPal() {}
private:
	int ProcessTask(ServiceTask *task);
	PayPalServiceParams params;
	static PayPalTransfer cur_transfer;
	static size_t curl_output(void *ptr, size_t size, size_t nmemb, void *userdata);
	static xercesc::XercesDOMParser *parser;
};

#endif
