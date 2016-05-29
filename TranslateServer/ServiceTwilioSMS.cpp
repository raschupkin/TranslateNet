/*
 * ServiceSMS.cpp
 *
 *  Created on: 27.02.2015
 *      Author: roman
 */

#include <curl_easy.h>
#include <curl_form.h>
#include "ServiceTwilioSMS.h"
#include "SessionManager.h"
#include "TranslateServer.h"

using namespace std;
using curl::curl_form;
using curl::curl_easy;

ServiceTwilioSMS::ServiceTwilioSMS(SessionManager *_sManager, TwilioSMSServiceParams *_params)
: ServiceAsync(_sManager)
{
	params = *_params;
}

size_t ServiceTwilioSMS::curl_output(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	if (SessionManager::getOptions().paramsTwilioSMS.Log && nmemb > 0)
		log(LOG_NORMAL, "TwilioSMS response: %s", (char *)ptr);
	return size*nmemb;
}

int ServiceTwilioSMS::ProcessTask(ServiceTask *task)
{
	TwilioMessage *msg = dynamic_cast<TwilioMessage *>(task);
	if (!msg)
		return -1;
	if (params.Log) {
		log(LOG_NORMAL, "Sending TwilioSMS: phone=%s", msg->phone.c_str());
		log(LOG_NORMAL, "        TwilioSMS: text=%s", msg->text.c_str());
	}

	string _from = params.From;
	string from = _from.replace(_from.find("+"), 1, "%2B");
	string phone = msg->phone.replace(msg->phone.find("+"), 1, "%2B");
    try {
		curl_easy easy;
		curl_form form;
		easy.add(curl_pair<CURLoption, string>(CURLOPT_USERNAME, params.Account));
		easy.add(curl_pair<CURLoption, string>(CURLOPT_PASSWORD, params.AuthToken));
		easy.add(curl_pair<CURLoption, int>(CURLOPT_HTTPAUTH, CURLAUTH_BASIC));
		easy.add(curl_pair<CURLoption,  size_t(*)(void*,size_t,size_t,void*)>(CURLOPT_WRITEFUNCTION, &(ServiceTwilioSMS::curl_output)));
		easy.add(curl_pair<CURLoption, string>(CURLOPT_URL, params.Addr));
		easy.add(curl_pair<CURLoption, curl_form>(CURLOPT_HTTPPOST, form));
		string postFields = string("From=")+from+"&To="+phone+"&Body="+msg->text;
		easy.add(curl_pair<CURLoption, string>(CURLOPT_POSTFIELDS, postFields.c_str()));
		easy.perform();
    } catch (curl_easy_exception ex) {
    	vector<pair<string,string>> tb = ex.what();
        for_each(tb.begin(), tb.end(),[](const pair<string,string> &value) {
            log(LOG_ERROR,"ERROR TwilioSMS: %s ::::: FUNCTION: %s", value.first.c_str(), value.second.c_str());
        });
    	return -1;
    }
}
