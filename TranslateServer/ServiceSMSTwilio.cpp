/*
 * ServiceSMS.cpp
 *
 *  Created on: 27.02.2015
 *      Author: roman
 */

#include <curl_easy.h>
#include <curl_form.h>
#include <json/json.h>
#include "ServiceSMSTwilio.h"
#include "SessionManager.h"
#include "TranslateServer.h"
#include "User.h"

using namespace std;
using curl::curl_form;
using curl::curl_easy;

void TwilioMessage::GenText_SMSCode()
{
	text = "Translate-Net code: ";
	text += sms_code;
}

ServiceSMSTwilio::ServiceSMSTwilio(SessionManager *_sManager, TwilioSMSServiceParams *_params)
: ServiceAsync(_sManager)
{
	params = *_params;
}

void ServiceSMSTwilio::ProcessUnfinishedTasks()
{
	lock();
	vector<TwilioMessage> list = User::getUnsentSMSTwilio(getCon());
	for (int i=0; i<list.size(); i++) {
		TwilioMessage msg = list[i];
		log(LOG_WARNING, "Resend SMS: phone_id=%d.", msg.phone_id);
		ProcessTask(&msg);
	}
	unlock();
}

size_t ServiceSMSTwilio::curl_output(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	int ret = size*nmemb;
	if (ret == 0)
		return ret;
	TwilioMessage *msg = (TwilioMessage *)userdata;
	if (!msg || !msg->service)
		return ret;

	if (!msg->service->checkSent_Task(msg)) {
		log(LOG_ERROR,"Error: ServiceSMSTwilio checkSentTask false");
		return ret;
	}

	msg->curl_response.append((char *)ptr, size*nmemb);
	Json::Reader jr;
	Json::Value root;
	if (!jr.parse(msg->curl_response, root))
		return ret;
	if (SessionManager::getOptions().paramsTwilioSMS.Log && nmemb > 0)
		log(LOG_NORMAL, "TwilioSMS parsed response: %s", (char *)ptr);

	Json::Value v = root["sid"];
	string id = v.asCString();
	v = root["status"];
	int status = StatusStrToCode(v.asCString());
	int error = 0;
	if (status == SMS_STATUS_UNDELIVERED) {
		v = root["error_code"];
		error = v.asInt();
	}

	if (User::storeSMSTwilio(msg->service->getCon(), id,  msg->phone_id, status, error, msg->type))
		return ret;
	return ret;
}

int ServiceSMSTwilio::ProcessTask(ServiceTask *task)
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
		easy.add(curl_pair<CURLoption,  size_t(*)(void*,size_t,size_t,void*)>(CURLOPT_WRITEFUNCTION, &(ServiceSMSTwilio::curl_output)));
		easy.add(curl_pair<CURLoption, void *>(CURLOPT_WRITEDATA, (void *)msg));
		easy.add(curl_pair<CURLoption, int>(CURLOPT_TIMEOUT, SessionManager::getOptions().CurlTimeout));
		easy.add(curl_pair<CURLoption, string>(CURLOPT_URL, params.Addr));
		easy.add(curl_pair<CURLoption, curl_form>(CURLOPT_HTTPPOST, form));
		string postFields = string("From=") + from;
		postFields += "&To=" + phone;
		postFields += "&Body=" + msg->text;
		postFields += "&StatusCallback="; postFields += params.StatusAddr;
		easy.add(curl_pair<CURLoption, string>(CURLOPT_POSTFIELDS, postFields.c_str()));
		easy.perform();
		callbackTaskDone(task);
		return 0;
    } catch (curl_easy_exception ex) {
    	vector<pair<string,string>> tb = ex.what();
        for_each(tb.begin(), tb.end(),[](const pair<string,string> &value) {
            log(LOG_ERROR,"ERROR TwilioSMS: %s ::::: FUNCTION: %s", value.first.c_str(), value.second.c_str());
        });
		callbackTaskDone(task);
    	return -1;
    }
}

int ServiceSMSTwilio::StatusStrToCode(string str)
{
	int status = SMS_STATUS_UNKNOWN;
	if (str.compare("queued") == 0)
		status = SMS_STATUS_QUEUED;
	if (str.compare("sending") == 0)
		status = SMS_STATUS_SENDING;
	if (str.compare("sent") == 0)
		status = SMS_STATUS_SENT;
	if (str.compare("delivered") == 0)
		status = SMS_STATUS_DELIVERED;
	if (str.compare("undelivered") == 0)
		status = SMS_STATUS_UNDELIVERED;
	if (str.compare("failed") == 0)
		status = SMS_STATUS_FAILED;
	return status;
}
