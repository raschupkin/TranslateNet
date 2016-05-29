#ifndef IOS_VERIFY_H_
#define IOS_VERIFY_H_

#include "ServiceAsync.h"

typedef struct _IOSVerifyServiceParams : ServiceParams {
	const char *addr;
	int port;
	bool Log;
} IOSVerifyServiceParams;

class IOSPurchase : public ServiceTask {
public:
	string data;
	int len;
	unsigned int user;
	int money;

	~IOSPurchase()	{}
	string curl_response;
};

class ServiceIOSVerify : public ServiceAsync {
typedef void (SessionManager::*IOSVerifyHandler)(IOSPurchase p, int result_code);
public:
	ServiceIOSVerify(SessionManager *_sManager, IOSVerifyServiceParams *_params, IOSVerifyHandler _VerifyHandler);
	virtual ~ServiceIOSVerify() {}
	virtual const char *getName()					{	return "IOSVerify";						}
private:
	IOSVerifyServiceParams params;
	static size_t curl_output(void *ptr, size_t size, size_t nmemb, void *userdata);
	int ProcessTask(ServiceTask *task);
	void archiveIOSPurchase(IOSPurchase p);
	static IOSVerifyHandler VerifyHandler;
	static IOSPurchase curPurchase;
};

#endif
