/*
 * email.cpp
 *
 *  Created on: 29.09.2014
 *      Author: roman
 */
#include <curl_easy.h>
#include <curl_form.h>
#include <json/json.h>
#include "ServiceIOSVerify.h"
#include "SessionManager.h"
#include "TranslateServer.h"

using namespace std;
using curl::curl_form;
using curl::curl_easy;

IOSPurchase ServiceIOSVerify::curPurchase;
ServiceIOSVerify::IOSVerifyHandler ServiceIOSVerify::VerifyHandler;

typedef struct _IOSProduct {
	string name;
	int money;
} IOSProduct;
IOSProduct IOSProducts[] = {{"credit1", 100},
							{"credit2", 200},
							{"credit5", 500},
							{"credit10", 1000},
							{"credit25", 2500},
							{"", 0}};

ServiceIOSVerify::ServiceIOSVerify(SessionManager *_sManager, IOSVerifyServiceParams *_params, IOSVerifyHandler _VerifyHandler)
: ServiceAsync(_sManager)
{
	params = *_params;
	VerifyHandler = _VerifyHandler;
}

void ServiceIOSVerify::archiveIOSPurchase(IOSPurchase p)
{
	/*
	std::ofstream of;
	of.open(SessionManager::getOptions().email_archive_file, std::ios_base::app);
	time_t curtime;
	time(&curtime);
	of << "Email Type: " << m.type << std::endl;
	of << "Time: " << ctime(&curtime);
	of << "From: " << m.from << std::endl;
	of << "To: " << m.to << std::endl;
	of << "Topic: " << m.topic << std::endl;
	of << m.message << endl << endl << endl;
	*/
}

size_t ServiceIOSVerify::curl_output(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	int ret = size*nmemb;
	if (ret == 0)
		return ret;
	IOSPurchase *p = (IOSPurchase *)userdata;
	if (!p || !p->service)
		return ret;

	if (!p->service->checkSent_Task(p)) {
		return ret;
	}

	p->curl_response.append((char *)ptr, size*nmemb);
	Json::Reader jr;
	Json::Value root;
	if (!jr.parse(p->curl_response, root)) {
		return ret;
	}

	if (SessionManager::getOptions().paramsIOSVerify.Log && nmemb > 0)
		log(LOG_NORMAL, "[%d]IOSVerify parsed response: %s", p->user, (char *)ptr);
	Json::Value v = root["receipt"];
	string product_id = v["product_id"].asCString();
	int ios_status = root["status"].asInt();
	int product_money = 0;
	for (int i=0; IOSProducts[i].money != 0; i++)
		if (product_id.find(IOSProducts[i].name) != string::npos) {
			product_money = IOSProducts[i].money;
			break;
		}

	int status;
	if (ios_status == 0)
		status = BILLING_VERIFY_OK;
	else
		status = BILLING_VERIFY_ERROR_SIGNATURE;
	if (product_money != p->money)
		status = BILLING_VERIFY_ERROR_SIGNATURE;

	(p->service->getsManager()->*VerifyHandler)(*p, status);
	return ret;
}

int ServiceIOSVerify::ProcessTask(ServiceTask *task)
{
	IOSPurchase *p = dynamic_cast<IOSPurchase *>(task);
	if (!p)
		return -1;
	string request;
	request = "{\"receipt-data\":\"";
	request += p->data;
	request += "\"}";
	if (params.Log)
		log(LOG_NORMAL, "IOSVerify: user=%d", p->user);

    try {
		curl_easy easy;
		curl_form form;
		easy.add(curl_pair<CURLoption,  size_t(*)(void*,size_t,size_t,void*)>(CURLOPT_WRITEFUNCTION, &(ServiceIOSVerify::curl_output)));
		easy.add(curl_pair<CURLoption, string>(CURLOPT_URL, params.addr));
		easy.add(curl_pair<CURLoption, void *>(CURLOPT_WRITEDATA, (void *)p));
		easy.add(curl_pair<CURLoption, int>(CURLOPT_TIMEOUT, SessionManager::getOptions().CurlTimeout));
		easy.add(curl_pair<CURLoption, curl_form>(CURLOPT_HTTPPOST, form));
		easy.add(curl_pair<CURLoption, string>(CURLOPT_POSTFIELDS, request));
		easy.perform();
		callbackTaskDone(p);
		return 0;
    } catch (curl_easy_exception ex) {
		callbackTaskDone(p);
    	vector<pair<string,string>> tb = ex.what();
        for_each(tb.begin(), tb.end(),[](const pair<string,string> &value) {
            log(LOG_ERROR,"ERROR IOSVerify: %s ::::: FUNCTION: %s", value.first.c_str(), value.second.c_str());
        });
    	return -1;
    }
}
