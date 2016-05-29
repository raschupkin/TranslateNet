#include "ServicePayPal.h"
#include "SessionManager.h"
#include "TranslateServer.h"
#include <sstream>
#include <iomanip>
#include <curl_easy.h>
#include <curl_form.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
using namespace xercesc;
using curl::curl_form;
using curl::curl_easy;
PayPalTransfer ServicePayPal::cur_transfer;
xercesc::XercesDOMParser *ServicePayPal::parser;

ServicePayPal::ServicePayPal(SessionManager *_sManager, PayPalServiceParams *_params)
: ServiceAsync(_sManager)
{
	params = *_params;
	parser = new XercesDOMParser();
}

size_t ServicePayPal::curl_output(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	int ret = size*nmemb;
	if (ret == 0)
		return ret;
	PayPalTransfer *transfer = (PayPalTransfer *)userdata;
	if (!transfer || !transfer->service)
		return ret;

	if (!transfer->service->checkSent_Task(transfer)) {
		return ret;
	}

	transfer->curl_response.append((char *)ptr, size*nmemb);
	if (SessionManager::getOptions().paramsPayPal.Log && nmemb > 0)
		log(LOG_NORMAL, "PayPal response: %s", (char *)ptr);
	MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)transfer->curl_response.c_str(), nmemb, "Protocol", false);
	try {
		parser->parse(*memIS);
	} catch (const XMLException& ex){
		char *msg = XMLString::transcode(ex.getMessage());
//		log(LOG_ERROR, "Error in data from PayPal: %s", msg);
		XMLString::release(&msg);
		delete memIS;
		return ret;
	} catch (const DOMException& ex){
		char *msg = XMLString::transcode(ex.getMessage());
//		log(LOG_ERROR, "Error in data from PayPal DOM: %s", msg);
		XMLString::release(&msg);
		delete memIS;
		return ret;
	} catch (...) {
//		log(LOG_ERROR, "An error occurred during XML parsing of data from PayPal.");
		delete memIS;
		return ret;
	}
	delete	memIS;
	DOMDocument *doc = parser->getDocument();
	DOMNode *node = doc->getDocumentElement();
	if (!node) {
		return ret;
	}
	string nodeName = XMLString::transcode(node->getNodeName());
	node = node->getFirstChild();
	if (!node) {
		log(LOG_ERROR, "Empty data from PayPal <%s>.", nodeName.c_str());
		return ret;
	}
	nodeName = XMLString::transcode(node->getNodeName());
	node = node->getFirstChild();
	if (!node) {
		log(LOG_ERROR, "Empty nested data from PayPal <%s>.", nodeName.c_str());
		return ret;
	}
	bool success = false;
	int error = 0;
	do {
		char *nodeVal = 0;
		char *nodeName = XMLString::transcode(node->getNodeName());
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_ERROR, "Incorrect data from PayPal.");
			XMLString::release(&nodeName);
			transfer->service->unlock_sent();
			return ret;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "ack")) {
			success = !strcasecmp(nodeVal, "success");
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());

	transfer->service->getsManager()->onPayPalTransferEnd(*transfer, success ? BILLING_VERIFY_OK : BILLING_VERIFY_ERROR_SIGNATURE);
	return ret;
}

int ServicePayPal::ProcessTask(ServiceTask *task)
{
	PayPalTransfer *transfer = dynamic_cast<PayPalTransfer *>(task);
	if (!transfer)
		return -1;
	string request = "<PayRequest><actionType>PAY</actionType>";
	request += "<senderEmail>" + string(params.Account) + "</senderEmail>";
	request += "<cancelUrl>" + string(params.CancelURL) + "</cancelUrl>";
	request += "<returnUrl>" + string(params.ReturnURL) +"</returnUrl>";
	request += "<currencyCode>USD</currencyCode>";
	request += "<receiverList><receiver><email>" + transfer->account  + "</email>";
	std::ostringstream buf;
	buf.precision(3);
	buf << std::setprecision(2) << fixed << (float)transfer->sum/100;
	request += "<amount>" + buf.str() + "</amount></receiver></receiverList>";
	request += "<requestEnvelope><errorLanguage>en_US</errorLanguage></requestEnvelope></PayRequest>";
	if (params.Log)
		log(LOG_NORMAL, "PayPal request: %s", request.c_str());

    try {
    	curl_form form;
    	curl_easy easy;
    	curl_pair<CURLformoption,string> userid_form(CURLFORM_COPYNAME, "X-PAYPAL-SECURITY-USERID");
        curl_pair<CURLformoption,string> userid_cont(CURLFORM_COPYCONTENTS, params.UserID);
        curl_pair<CURLformoption,string> pass_form(CURLFORM_COPYNAME, "X-PAYPAL-SECURITY-PASSWORD");
        curl_pair<CURLformoption,string> pass_cont(CURLFORM_COPYCONTENTS, params.Password);
        curl_pair<CURLformoption,string> sign_form(CURLFORM_COPYNAME, "X-PAYPAL-SECURITY-SIGNATURE");
        curl_pair<CURLformoption,string> sign_cont(CURLFORM_COPYCONTENTS, params.Signature);
        curl_pair<CURLformoption,string> appid_form(CURLFORM_COPYNAME, "X-PAYPAL-APPLICATION-ID");
        curl_pair<CURLformoption,string> appid_cont(CURLFORM_COPYCONTENTS, params.AppID);
        curl_pair<CURLformoption,string> reqfmt_form(CURLFORM_COPYNAME, "X-PAYPAL-REQUEST-DATA-FORMAT");
        curl_pair<CURLformoption,string> reqfmt_cont(CURLFORM_COPYCONTENTS, "xml");
        curl_pair<CURLformoption,string> respfmt_form(CURLFORM_COPYNAME, "X-PAYPAL-RESPONSE-DATA-FORMAT");
        curl_pair<CURLformoption,string> respfmt_cont(CURLFORM_COPYCONTENTS, "xml");
    	form.add(userid_form, userid_cont);
    	form.add(pass_form, pass_cont);
    	form.add(sign_form, sign_cont);
    	form.add(appid_form, appid_cont);
    	form.add(reqfmt_form, reqfmt_cont);
    	form.add(respfmt_form, respfmt_cont);

    	string opt_userid = "X-PAYPAL-SECURITY-USERID: " + string(params.UserID);
    	string opt_pass = "X-PAYPAL-SECURITY-PASSWORD: " + string(params.Password);
    	string opt_sign = "X-PAYPAL-SECURITY-SIGNATURE: " + string(params.Signature);
    	string opt_appid = "X-PAYPAL-APPLICATION-ID: " + string(params.AppID);
    	string opt_reqfmt = "X-PAYPAL-REQUEST-DATA-FORMAT: xml";
    	string opt_respfmt = "X-PAYPAL-RESPONSE-DATA-FORMAT: xml";
    	curl_slist slist_userid;
    	slist_userid.data = (char *)opt_userid.c_str();
    	curl_slist slist_pass;
    	slist_pass.data = (char *)opt_pass.c_str();
    	slist_userid.next = &slist_pass;
    	curl_slist slist_sign;
    	slist_sign.data = (char *)opt_sign.c_str();
    	slist_pass.next = &slist_sign;
    	curl_slist slist_appid;
    	slist_appid.data = (char *)opt_appid.c_str();
    	slist_sign.next = &slist_appid;
    	curl_slist slist_reqfmt;
    	slist_reqfmt.data = (char *)opt_reqfmt.c_str();
    	slist_appid.next = &slist_reqfmt;
    	curl_slist slist_respfmt;
    	slist_respfmt.data = (char *)opt_respfmt.c_str();
    	slist_reqfmt.next = &slist_respfmt;
    	slist_respfmt.next = 0;

    	easy.add(curl_pair<CURLoption, curl_slist*>(CURLOPT_HTTPHEADER, &slist_userid));
    	easy.add(curl_pair<CURLoption,  size_t(*)(void*,size_t,size_t,void*)>(CURLOPT_WRITEFUNCTION, &(ServicePayPal::curl_output)));
		easy.add(curl_pair<CURLoption, void *>(CURLOPT_WRITEDATA, (void *)transfer));
		easy.add(curl_pair<CURLoption, int>(CURLOPT_TIMEOUT, SessionManager::getOptions().CurlTimeout));
		easy.add(curl_pair<CURLoption, string>(CURLOPT_URL, params.AdaptivePaymentURL));
    	easy.add(curl_pair<CURLoption, curl_form>(CURLOPT_HTTPPOST, form));
    	easy.add(curl_pair<CURLoption, string>(CURLOPT_POSTFIELDS, request));

    	cur_transfer = *transfer;
    	easy.perform();
    	callbackTaskDone(transfer);
		return 0;
   } catch (curl_easy_exception ex) {
    	vector<pair<string,string>> tb = ex.what();
        for_each(tb.begin(), tb.end(),[](const pair<string,string> &value) {
            log(LOG_ERROR,"ERROR PayPal: %s ::::: FUNCTION: %s", value.first.c_str(), value.second.c_str());
        });
    	callbackTaskDone(transfer);
    	return -1;
    }
	return 0;
}
