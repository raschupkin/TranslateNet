/*
 * Session.cpp
 *
 *  Created on: 25.04.2014
 *      Author: Raschupkin Roman
 */
#include <unistd.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
using namespace xercesc;
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <time.h>
#include "TranslateServer.h"
#include "SessionManager.h"
#include "User.h"
#include "Call.h"

Session::CommandEntry Session::CommandTable[] = {
	{	"pulse",		 			true,	ERROR_LOGIN_REQUIRED,	0												},
	{	"login", 					false, 	ERROR_ALREADY_LOGIN,	&Session::processPacket_Login					},
	{	"register_user"	, 			false, 	ERROR_ALREADY_LOGIN,	&Session::processPacket_RegisterUser			},
	{	"reset_password", 			false,	ERROR_ALREADY_LOGIN,	&Session::processPacket_ResetPassword			},
	{	"get_languages",	 		true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_getLanguages			},
//	{	"change_password", 			true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_ChangePassword			},
	{	"delete_user", 				true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_DeleteUser				},
	{	"set_country", 				true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_SetCountry				},
	{	"register_phone", 			true, 	ERROR_LOGIN_REQUIRED,	&Session::processPacket_RegisterPhone			},
	{	"confirm_register_phone",	true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_ConfirmRegisterPhone	},
	{	"resend_sms",				true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_ResendSMS				},
	{	"set_busy",					true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_SetBusy					},
	{	"get_user_data", 			true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_getUserData				},
	{	"user_data", 				true, 	ERROR_LOGIN_REQUIRED,	&Session::processPacket_UserData				},
	{	"billing", 					true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_Billing					},
	{	"paypal_transfer", 			true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_PayPalTransfer			},
	{	"request_translator_list",	true, 	ERROR_LOGIN_REQUIRED,	&Session::processPacket_RequestTranslatorList	},
	{	"stop_translator_list",		true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_StopTranslatorList		},
	{	"request_client_list",		true, 	ERROR_LOGIN_REQUIRED,	&Session::processPacket_RequestClientList		},
	{	"phonecall_request",		true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_PhoneCallRequest		},
	{	"phonecall_confirm",		true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_PhoneCallConfirm		},
	{	"phonecall_status", 		true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_CallStatus				},
	{	"get_call_history",			true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_getCallHistory			},
	{	"get_mark_history",			true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_GetMarkHistory			},
	{	"mark_rating",				true, 	ERROR_LOGIN_REQUIRED,	&Session::processPacket_MarkRating				},
	{	"get_statistic", 			true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_GetStatistic			},
	{	"get_translator_statistic", true,	ERROR_LOGIN_REQUIRED,	&Session::processPacket_GetTranslatorStatistic	},
	{	0, 							false,	0,						0												}
};

int Session::ProcessPacket()
	{
	unsigned int id = 0;	
	if (getUser())
		id = getUser()->getID();
	PacketCount++;
	Header *hdr = (Header *)packet;
	memcpy(recvxml, (char *)packet + sizeof(Header), hdr->len);
	recvxml[hdr->len] = 0;
log(LOG_ALL, "[%d]recv(%d):\t%s", id, hdr->len, recvxml);
/*	if (data[hdr->len-1] != 0) 	{
 log(LOG_ALL, "Incoming packet contains wrong XML data.");
 return -1;
	}
*/
	current_command = "";
	MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)recvxml, hdr->len, "Protocol", false);
	try {
		getParser()->parse(*memIS);
	} catch (const XMLException& ex){
		char *msg = XMLString::transcode(ex.getMessage());
		log(LOG_ALL, "Error in incoming packet XML data: %s", msg);
		XMLString::release(&msg);
		delete memIS;
		sendPacket_Error(ERROR_OTHER);
		return -1;
	} catch (const DOMException& ex){
		char *msg = XMLString::transcode(ex.getMessage());
		log(LOG_ALL, "Error in incoming packet XML DOM: %s", msg);
		XMLString::release(&msg);
		delete memIS;
		sendPacket_Error(ERROR_OTHER);
		return -1;
	} catch (...) {
		log(LOG_ERROR, "An error occurred during incoming packet XML parsing.");
		delete memIS;
		sendPacket_Error(ERROR_OTHER);
		return -1;
	}
	delete	memIS;
	DOMDocument *doc = getParser()->adoptDocument();
	DOMNode *node = doc->getDocumentElement();
	if (!node) {
		sendPacket_Error(ERROR_FORMAT);
		delete doc;
		return -1;
	}
	timeout = time(0);
	char *nodeName = XMLString::transcode(node->getNodeName());
	current_command = nodeName;
	// only phonecall_status must be processed after maintenance exit is initiated
	if (sManager->isMaintenanceExit() &&
			strcasecmp(nodeName, "phonecall_status")) {
		sendPacket_Error(ERROR_MAINTENANCE);
		XMLString::release(&nodeName);
		delete doc;
		return 0;
	}

	CommandEntry* cmd = CommandTable;
	while (cmd->command) {
		if (!strcasecmp(cmd->command, nodeName)) {
			if ((bool)getUser() != cmd->require_login) {
				sendPacket_Error(cmd->error);
				XMLString::release(&nodeName);
				delete doc;
				return 0;
			}
			XMLString::release(&nodeName);
			if (!cmd->process) {
				delete doc;
				return 0;
			}
			int ret = (this->*(cmd->process))(node);
			delete doc;
			return ret;
		}
		cmd++;
	}
	XMLString::release(&nodeName);
	delete doc;
	log(LOG_ALL, "Received packet of wrong type.");
	sendPacket_Error(ERROR_FORMAT);
	return 0;
/*	printf("ptype=%d, plen=%d: ", p->type, p->len);
	for (int i=0;i<p->len;i++)
		printf("%x ",packet[sizeof(Header)+i]);
//	int bits;SSL_get_cipher_bits(ssl,&bits);
//	printf("\n%d",bits);
	printf("");fflush(stdout);
*/
}

int Session::sendPacket_Error(int error, char *message, int id)
{
	return sendPacket_CommandError(error, current_command, message, id);
}

int Session::sendPacket_CommandError(int error, string command, char *message, int id)
{
	string msg = "<error>";
	msg += "<code>";	msg += std::to_string(error);	msg += "</code>";
	msg += "<command>";	msg += command;	msg += "</command>";
	if (message) {
		msg += "<message>";	msg += message;	msg += "</message>";
	}
	if (id > 0) {
		msg += "<id>";	msg += std::to_string(id);	msg += "</id>";
	}
	msg += "</error>";
	return sendPacket(msg.c_str());
}

int Session::sendPacket_LoginError(int error, int langs_version)
{
	string msg = "<error>";
	msg += "<code>";	msg += std::to_string(error);	msg += "</code>";
	msg += "<command>";	msg += current_command;	msg += "</command>";
	msg += "<langs_version>";	msg += std::to_string(langs_version);	msg += "</langs_version>";
	msg += "</error>";
	return sendPacket(msg.c_str());
}

int Session::sendPacket_SMSError(int error)
{
	if (!getUser())
		return -1;
	string msg = "<error>";
	msg += "<code>";	msg += std::to_string(error);	msg += "</code>";
	msg += "<command>";	msg += current_command;	msg += "</command>";
	msg += "<sms_sent_num>";	msg += std::to_string(getUser()->getSMSSentNum());	msg += "</sms_sent_num>";
	msg += "<sms_block_days>";	msg += std::to_string(getUser()->getSMSBlockDays());	msg += "</sms_block_days>";
	msg += "<sms_max_num>";	msg += std::to_string(SessionManager::getOptions().SMS_MaxNum);	msg += "</sms_max_num>";
	msg += "</error>";
	return sendPacket(msg.c_str());
}

int Session::sendPacket_BillingError(int error, int money)
{
	string msg = "<error>";
	msg += "<code>";	msg += std::to_string(error);	msg += "</code>";
	msg += "<command>";	msg += "billing";	msg += "</command>";
	msg += "<money>";	msg += std::to_string(money);	msg += "</money>";
	msg += "</error>";
	return sendPacket(msg.c_str());
}

// CallManager must be locked
int Session::sendPacket_CallRequestError(int error, bool confirm, unsigned int call_id)
{
	if (!getUser())
		return -1;
	string msg = "<error>";
	msg += "<code>";	msg += std::to_string(error);	msg += "</code>";
	if (confirm) {
		msg += "<command>";	msg += "phonecall_confirm";	msg += "</command>";
	} else {
		msg += "<command>";	msg += "phonecall_request";	msg += "</command>";
	}
	Call *call = cManager->getCall(call_id);
	if (!call)
		return -1;
	if (call->getClient() == getUser()->getID()) {
		msg += "<translator>"; 	msg += std::to_string(call->getTranslator());	msg += "</translator>";
	} else if (call->getTranslator() == getUser()->getID()) {
		msg += "<client>"; 	msg += std::to_string(call->getClient());	msg += "</client>";
	} else
		return -1;
	msg += "</error>";
	return sendPacket(msg.c_str());
}

int Session::sendPacket_PayPalTransferError(int error)
{
	string msg = "<error>";
	msg += "<code>";	msg += std::to_string(error);	msg += "</code>";
	msg += "<command>";	msg += "paypal_transfer";	msg += "</command>";
	msg += "</error>";
	return sendPacket(msg.c_str());
}

// SessionManager must be read-locked
User *Session::doLogin(unsigned int id)
{
	if (!sManager || sManager->markAbortSessionByUser(this, id)) {
		log(LOG_ERROR, "Login: unable to delete existing session (user=%d).", id);
		return 0;
	}
	User *u = User::readUser(getCon(), id);
	if (!u) {
		log(LOG_ERROR, "Unable to read user data (user=%d).", id);
		return 0;
	}
	return u;
}

void Session::setMarkAbort(bool _SamePhone, bool _SameIP)
{
	AbortSamePhone = _SamePhone;
	AbortSameIP = _SameIP;
	markAbort = true;
}

bool Session::isOS(char *os_string)
{
	return !(strcmp(os_string, SESSION_OS_ANDROID) && strcmp(os_string, SESSION_OS_IOS) && strcmp(os_string, SESSION_OS_TIZEN));
}

int Session::processPacket_Login(DOMNode *node)
{
	log(LOG_ALL, "Received LOGIN (id=-).");
	char email[MAX_EMAIL+1] = {0};
	char password[MAX_PASSWORD+1] = {0};
	string device_id;
	int version = VERSION_DEFAULT;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (id=-).");
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeVal = 0;
		char *nodeName = XMLString::transcode(node->getNodeName());
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			if (!strcmp(nodeName, "password"))
				sendPacket_Error(ERROR_WRONG_PASSWORD);
			else
				sendPacket_Error(ERROR_FORMAT);
			log(LOG_NORMAL, "Incorrect LOGIN.");
			XMLString::release(&nodeName);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "email")) {
			if (strlen(nodeVal) <= MAX_EMAIL)
				strcpy(email, nodeVal);
			else
				error = 1;
		} else if (!strcasecmp(nodeName, "password")) {
			if (strlen(nodeVal) <= MAX_PASSWORD)
            	strcpy(password, nodeVal);
            else
            	error = 1;
		} else if (!strcasecmp(nodeName, "os")) {
			if (strlen(nodeVal) > MAX_OS)
				error = 1;
			if (isOS(nodeVal))
				OS = nodeVal;
			else
				error = 1;
		} else if (!strcasecmp(nodeName, "version")) {
			if (!sscanf(nodeVal, "%d", &version))
				error = 1;
		} else if (!strcasecmp(nodeName, "device_id")) {
			if (strlen(nodeVal) < MAX_DEVICE_ID)
				device_id = nodeVal;
		} else
			error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	User::formatEmail(email);
	if (error || !strlen(email) || !User::checkEmail(email) || !OS.size() || !device_id.size()) {
		log(LOG_NORMAL, "Incorrect LOGIN (email='%s').", email);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	if (User::checkPassword(getCon(), email, password, login_id)) {
		log(LOG_ALL, "Login: password verified (user=%d, email='%s').", login_id, email);
		if (User::updateUserEmailAck(getCon(), login_id)) {
			sendPacket_Error(ERROR_OTHER);
			log(LOG_ERROR, "Unable to update email_ack_time (user=%d, email='%s').", login_id, email);
			return -1;
		}
		sManager->DynUserLock();
		if (user = doLogin(login_id)) {
			sManager->DynUserUnlock();
			log(LOG_ALL, "Login successful (user=%d, email='%s').", login_id, email);
			getUser()->lock();
			getUser()->setLoginParams(OS, version, device_id);
			if (SessionManager::getOptions().PhoneRegisterMode == PHONE_REGISTER_SETUP || (dynamic_cast<Translator *>(getUser()) != 0)) {
				if (!strlen(getUser()->getPhone())) {
//					getUser()->unlock();
					log(LOG_VERBOSE, "Logged in user has no phone (user=%d).", getUser()->getID());
					sendPacket_LoginError(ERROR_NO_PHONE, Langs::getLangsVersion());
//				return 0;
				} else if (/*SessionManager::getOptions().PhoneRegisterMode != PHONE_REGISTER_NONE && */getUser()->getPhoneStatus() != PHONE_STATUS_CONFIRMED) {
					log(LOG_VERBOSE, "Logged in user phone is not confirmed (user=%d)", getUser()->getID());
					sendPacket_LoginError(ERROR_PHONE_AWAITING, Langs::getLangsVersion());
				} else
					sendPacket_LoginError(ERROR_NOERROR, Langs::getLangsVersion());
			} else
				sendPacket_LoginError(ERROR_NOERROR, Langs::getLangsVersion());

			string LastDeviceID = getUser()->getLastDeviceID(getCon());
			if (LastDeviceID.size() > 0)
				if (LastDeviceID.compare(device_id) != 0)
					sendPacket_Error(ERROR_PHONE_CHANGED);
			getUser()->updateUserLoginTime(getCon(), true);

/*	LANGS not received by client apps yet
 	 	 	cManager->rdlock();
			unsigned int call_id = cManager->findActiveCall(login_id);
			if (call_id) {
				Call *call = cManager->getCall(call_id);
				if (call) {
					call->lock();
					sendPacket_CallStatus(call_id);
					call->unlock();
				}
			}
			cManager->unlock();
*/
			getUser()->unlock();
//			update_TLists(false);
			sManager->rdlock();
			sManager->UpdateStatistic(getCon(), true);
			sManager->unlock();
			return 0;
		} else {
			sManager->DynUserUnlock();
			sendPacket_Error(ERROR_OTHER);
			log(LOG_ERROR, "Unable to login (user=%d, email='%s').", login_id, email);
			return -1;
		}
	}
	bool deleted;
	if (!User::checkUserExist(getCon(), email, deleted) || deleted) {
		log(LOG_ALL, "User not exist (email='%s').", email);
		sendPacket_Error(ERROR_NO_USER);
		return -1;
	}
	sendPacket_Error(ERROR_WRONG_PASSWORD);
	log(LOG_ALL, "Login failed (user=%d, email='%s').", login_id, email);
	return -1;
}

int Session::sendPacket_Challenge()
{
	string msg = "<challenge>";
/*	string str_nonce = NonceToString(nonce);
	msg += "<nonce>";	msg += str_nonce;	msg += "</nonce>";
	*/
	msg += "</challenge>";
	return sendPacket(msg.c_str());
}

int Session::sendPacket_AwaitLogin()
{
	const char *msg = "<await_login_confirm></await_login_confirm>";
	return sendPacket(msg);
}

int Session::processPacket_RegisterUser(DOMNode *node)
{
	log(LOG_ALL, "Received REGISTER_USER (id=-).");
	char email[MAX_EMAIL+1] = {0};
	string lang = LANG_DEFAULT;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (id=-).");
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	int isTranslator = -1;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect REGISTER_USER.");
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "email")) {
			if (strlen(nodeVal) <= MAX_EMAIL)
				strncpy(email, nodeVal, MAX_EMAIL);
			else
				error = 1;
		} else if (!strcasecmp(nodeName, "is_translator")) {
			try {
				isTranslator = std::stoi(nodeVal);
			} catch (std::exception ex) {
				error = 1;
			}
		} else if (!strcasecmp(nodeName, "lang")) {
			if (!Langs::isLang(nodeVal))
				lang = LANG_DEFAULT;
			else
				lang = nodeVal;
		} else error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	User::formatEmail(email);
	if (error || !strlen(email) || !User::checkEmail(email) || isTranslator < 0) {
		log(LOG_NORMAL, "Incorrect REGISTER_USER (email='%s').", email);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	bool deleted;
//	if (User::checkUserExist(getCon(), email, deleted) || deleted) {
	if (User::checkUserExist(getCon(), email, deleted) && !deleted) {
		sendPacket_Error(ERROR_USER_ALREADY_EXIST);
		log(LOG_ALL, "Received REGISTER_USER from existing user (email='%s').", email);
		return 0;
	}
	char password[MAX_PASSWORD+1];
	User::genPassword(password, MAX_PASSWORD);
	if (sManager->sendEmail_Password(email, password, lang)) {
		log(LOG_ERROR, "Unable to enqueue email with password (email='%s').", email);
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	unsigned int user_id = User::storeUser(getCon(), email, isTranslator);
	if (user_id <= 0) {
		log(LOG_ERROR, "Unable to store user(email='%s').", email);
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	if (User::storePassword(getCon(), user_id, password)) {
		log(LOG_ERROR, "Unable to set password (id=%d, email='%s').", user_id, email);
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	sendPacket_AwaitLogin();
	log(LOG_ALL, "Awaiting registration confirm (id=%d, email='%s').", user_id, email);
	return 0;
}

int Session::processPacket_ResetPassword(DOMNode *node)
{
	log(LOG_ALL, "Received RESET_PASSWORD.");
	char email[MAX_EMAIL+1] = {0};
	int error = 0;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (id=-).");
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect DELETE_USER (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "email")) {
			if (strlen(nodeVal) <= MAX_EMAIL)
				strncpy(email, nodeVal, MAX_EMAIL);
			else
				error = 1;
		} else
			error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !strlen(email)) {
		log(LOG_NORMAL, "Incorrect RESET_PASSWORD (email='%s').", strlen(email)>0 ? email : "-");
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	unsigned int id;
	bool deleted;
	if (!(id = User::checkUserExist(getCon(), email, deleted)) || deleted) {
		sendPacket_Error(ERROR_NO_USER);
		log(LOG_ERROR, "Received RESET_PASSWORD from non-existing user (user=%d, email='%s').", id, email);
		return 0;
	}
	if (!User::checkUserEmailAck(getCon(), id)) {
		sendPacket_Error(ERROR_NO_USER);
		log(LOG_ERROR, "Received RESET_PASSWORD from non-confirmed EmailAck user (user=%d, email='%s').", id, email);
		return 0;
	}
	char reset_uid[MAX_RESET_PASS_REQEST_UID+1];
	if (User::storeResetPasswordRequest(getCon(), id, reset_uid)) {
		log(LOG_VERBOSE, "Unable to store ResetPasswordRequest (user=%d).", email, id);
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	User *user = User::readUser(getCon(), id);
	if (!user) {
		log(LOG_ERROR, "Unable to readUser for RESET_PASSWORD (user=%d, email='%s').", id, email);
		sendPacket_Error(ERROR_NO_USER);
		return 0;
	}
	log(LOG_WARNING, "reset_uid: %s (user=%d, email='%s').", reset_uid, id, email);
	if (sManager->sendEmail_ResetPassword(email, reset_uid, user->getLangs().getBaseLang())) {
		log(LOG_ERROR, "Unable to send reset password email with confirmation link (user=%d, email='%s').", id, email);
		sendPacket_Error(ERROR_OTHER);
		delete user;
		return 0;
	}
	delete user;
	log(LOG_VERBOSE, "Sent reset password email with confirmation link (user=%d, email='%s').", id, email);
	sendPacket_Error(ERROR_NOERROR);
	return 0;
}

int Session::processPacket_ChangePassword(DOMNode *node)
{
	log(LOG_ALL, "Received CHANGE_PASSWORD (user=%d).", getUser()->getID());
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	char old_password[MAX_PASSWORD+1] = {0};
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect DELETE_USER (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "old_password")) {
			if (strlen(nodeVal) <= MAX_PASSWORD)
				strcpy(old_password, nodeVal);
			else
				error = 1;
		} else
			error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !strlen(old_password)) {
		log(LOG_NORMAL, "Incorrect CHANGE_PASSWORD (user=%d, email='%s').", getUser()->getID(), getUser()->getEmail());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	unsigned int id;
	if (!User::checkPassword(getCon(), getUser()->getEmail(), old_password, id)) {
		log(LOG_VERBOSE, "Wrong old_password in CHANGE_PASSWORD (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_WRONG_PASSWORD);
		return 0;
	}
	char password[MAX_PASSWORD+1] = {0};
	User::genPassword(password, MAX_PASSWORD);
	if (sManager->sendEmail_Password(getUser()->getEmail(), password, getUser()->getLangs().getBaseLang())) {
		log(LOG_ERROR, "Error: unable to send email with changed password (user=%d, email='%s').", getUser()->getID(), getUser()->getEmail());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	if (User::storePassword(getCon(), getUser()->getID(), password)) {
		log(LOG_ERROR, "Error: unable to store password after change (user=%d, email='%s').", getUser()->getID(), getUser()->getEmail());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	log(LOG_VERBOSE, "Changed password for user '%s'(user=%d).", getUser()->getEmail(), getUser()->getID());
	sendPacket_Error(ERROR_NOERROR);
	return 0;
}

int Session::processPacket_DeleteUser(DOMNode *node)
{
	log(LOG_ALL, "Received DELETE_USER (user=%d).", getUser()->getID());

	Translator *t = dynamic_cast<Translator *>(getUser());
	if (t && t->isPayPalTransferActive()) {
		log(LOG_NORMAL, "Discarding DELETE_USER because of active PayPalTransfer (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_PAYPAL_TRANSFER_ACTIVE);
		return 0;
	}

	char email[MAX_EMAIL+1] = {0};
	char password[MAX_PASSWORD+1] = {0};
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	if (!strlen(user->getEmail())) {
		log(LOG_ERROR, "Unable to delete uninitialized user. (user=%d/%d).", login_id, user->getID());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect DELETE_USER (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "email")) {
			if (strlen(nodeVal) <= MAX_EMAIL)
				strcpy(email, nodeVal);
			else
				error = 1;
		} else if (!strcasecmp(nodeName, "password")) {
			if (strlen(nodeVal) <= MAX_PASSWORD)
				strcpy(password, nodeVal);
			else
				error = 1;
		} else
			error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !strlen(email) || !strlen(password) || strcmp(email, user->getEmail())) {
		log(LOG_NORMAL, "Incorrect DELETE_USER (user=%d, email='%s').", login_id, email);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	bool deleted;
	if (!User::checkUserExist(getCon(), getUser()->getEmail(), deleted) || deleted) {
		sendPacket_Error(ERROR_NO_USER);
		log(LOG_ERROR, "Received DELETE_USER from non-existing user (user=%d, email='%s').", login_id, email);
		return 0;
	}
	if (!User::checkPassword(getCon(), email, password, login_id)) {
		log(LOG_ALL, "Login: Received DELETE_USER with wrong password (user=%d, email='%s').", login_id, email);
		sendPacket_Error(ERROR_WRONG_PASSWORD);
		return 0;
	}
	if (User::deleteUser(getCon(), login_id)) {
		sendPacket_Error(ERROR_OTHER);
		log(LOG_ERROR, "Unable to delete user (user=%d, email='%s').", login_id, email);
		return 0;
	}
	sendPacket_Error(ERROR_NOERROR);
	log(LOG_ALL, "Deleted user (email='%s').", email);
	return -1;
}

int Session::processPacket_SetCountry(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received SET_COUNTRY (user=%d).", getUser()->getID());

	char country[MAX_COUNTRY+1] = {0};
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect SET_COUNTRY (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "country")) {
			if (strlen(country) <= MAX_COUNTRY)
				strcpy(country, nodeVal);
			else
				error = 1;
		} else
			error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !strlen(country)) {
		log(LOG_NORMAL, "Incorrect SET_COUNTRY (user=%d).", login_id);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	if (!Langs::isCountry(country)) {
		log(LOG_NORMAL, "Wrong country in SET_COUNTRY (user=%d).", login_id);
		sendPacket_Error(ERROR_UNKNOWN_COUNTRY);
		return 0;
	}

	user->setCountry(country);

	if (dynamic_cast<Translator *>(getUser()))
			update_TLists(false);

	sendPacket_Error(ERROR_NOERROR);
	log(LOG_ALL, "Set country '%s' (user=%d).", country, login_id);
	return 0;
}

int Session::sendSMSCode(TwilioMessage *msg)
{
	msg->type = SMS_TYPE_PHONE_CODE;
	msg->GenText_SMSCode();
char buf[512];
time_t now = time(0);
struct tm tstruct;
tstruct = *localtime(&now);
strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
msg->text += "\nTimer: ";
msg->text += buf;

	bool do_send = false;
	if (SessionManager::getOptions().DEBUG_SMS) {
		if (!msg->phone.compare("+79096545914") || !msg->phone.compare("+79032729056")||
				!msg->phone.compare("+79295332980") || !msg->phone.compare("+79261816725")) {
			do_send = true;
		} else
			log(LOG_ERROR, "DEBUG_SMS enabled: not sending SMS (user=%d).", getUser()->getID());
	} else
		do_send = true;
	if (do_send)
		sManager->sendSMSMessage(msg);
	return 0;
}

int Session::processPacket_RegisterPhone(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received REGISTER_PHONE (user=%d).", getUser()->getID());
	if (SessionManager::getOptions().PhoneRegisterMode == PHONE_REGISTER_BALANCE) {
		if (getUser()->getBalance() < PHONE_REGISTER_BALANCE_MIN) {
			log(LOG_NORMAL, "balance < PHONE_REGISTER_BALANCE_MIN (user=%d).", getUser()->getID());
			return sendPacket_Error(ERROR_BALANCE);
		}
	}
	if (SessionManager::getOptions().PhoneRegisterMode == PHONE_REGISTER_NONE) {
		log(LOG_NORMAL, "Discarding REGISTER_PHONE as PHONE_REGISTER_NONE (user=%d).", getUser()->getID());
		return sendPacket_Error(ERROR_OTHER);
	}
	char phone[MAX_PHONE+1] = {0};
	int user_input = -1;
	string device_id;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect REGISTER_PHONE (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "phone")) {
			if (strlen(nodeVal) <= MAX_PHONE)
				strncpy(phone, nodeVal, MAX_PHONE+1);
			else
				error = 1;
		} else if (!strcasecmp(nodeName, "user_input")) {
			try {
				user_input = std::stoi(nodeVal);
			} catch (std::exception ex) {
				error = 1;
			}
		} else if (!strcasecmp(nodeName, "device_id")) {
			device_id = User::formatDeviceID(nodeVal);
		}  else error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	User::formatPhone(phone);
	if (error || !strlen(phone) || !User::checkPhone(phone) || (user_input != 0 && user_input != 1)/* || device_id.length() == 0*/) {
		log(LOG_NORMAL, "Incorrect REGISTER_PHONE (user=%d, phone='%s').", login_id, phone);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	bool must_confirm = user_input || !OS.compare(SESSION_OS_IOS);
	must_confirm &= (SessionManager::getOptions().PhoneRegisterMode != PHONE_REGISTER_NONE);
	
	if (must_confirm) {
		if (getUser()->determineSMSCodeTimeout(getCon()) < 0) {
			log(LOG_ERROR, "Error: determineSMSCodeTimeout failed (user=%d).", getUser()->getID());
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
		if (getUser()->getSMSBlockDays() > 0) {
			log(LOG_VERBOSE, "Limit exeeded: register phone='%s' from device_id='%s'. Discarding REGISTER_PHONE. (user=%d).", phone, device_id.c_str(), login_id);
			sendPacket_SMSError(ERROR_TEMP_BLOCKED);
			return 0;
		}
	}

	getUser()->lock();
	user->setPhone((char *)"", PHONE_STATUS_AWAIT);

	getUser()->resetUserPhone(getCon());
	char sms_code[MAX_SMS_CODE + 1] = {0};
	User::genSMSCode(sms_code, MAX_SMS_CODE);
	unsigned int phone_id = getUser()->storeSMSCode(getCon(), phone, sms_code, getUser()->getClientDeviceID());
	if (phone_id <= 0) {
		getUser()->unlock();
		update_TLists(true);
		log(LOG_ERROR, "Error storing SMS code (user=%d, phone='%s').", login_id, phone);
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	if (must_confirm) {
		TwilioMessage *msg = new TwilioMessage();
		msg->phone = phone;
		msg->sms_code = sms_code;
		msg->phone_id = phone_id;
		if (sendSMSCode(msg)) {
			getUser()->unlock();
			update_TLists(true);
			log(LOG_ERROR, "Error sending SMS code (user=%d, phone='%s').", login_id, phone);
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
		sendPacket_AwaitPhoneConfirm();
	} else {
//		getUser()->resetOtherUsersPhones(getCon(), phone);
		getUser()->updateUserPhone(getCon());
		sendPacket_Error(ERROR_NOERROR);
	}
	getUser()->unlock();
	update_TLists(true);
	return 0;
}

int Session::sendPacket_AwaitPhoneConfirm()
{
	const char *msg = "<await_phone_confirm></await_phone_confirm>";
	return sendPacket(msg);
}

int Session::processPacket_ConfirmRegisterPhone(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received CONFIRM_REGISTER_PHONE (user=%d).", getUser()->getID());
	char sms_code[MAX_SMS_CODE+1] = {0};
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect CONFIRM_REGISTER_PHONE (user=%d).", user->getID());
			sendPacket_Error(ERROR_FORMAT);
			XMLString::release(&nodeName);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "sms_code")) {
			if (strlen(nodeVal) <= MAX_SMS_CODE)
				strcpy(sms_code, nodeVal);
			else
				error = 1;
		} else error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !strlen(sms_code)) {
		log(LOG_NORMAL, "Incorrect CONFIRM_REGISTER_PHONE (user=%d).", login_id);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	char phone[MAX_PHONE+1] = {0};
	getUser()->lock();
	if (user->checkSMSCode(getCon(), sms_code, phone)) {
		sManager->markAbortSessionByPhone(getCon(), this);
		user->setPhone(phone, PHONE_STATUS_CONFIRMED);
/*		if (user->resetOtherUsersPhones(getCon(), phone)) {
			log(LOG_ERROR, "Unable to reset other users phone (user=%d).", login_id);
			getUser()->unlock();
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
*/		if (user->updateUserPhone(getCon())) {
			log(LOG_ERROR, "Unable to update phone (user=%d).", login_id);
			getUser()->unlock();
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
		sendPacket_Error(ERROR_NOERROR);

		getUser()->unlock();
		update_TLists(false);
		return 0;
	} else {
		getUser()->unlock();
		sendPacket_Error(ERROR_WRONG_SMSCODE);
		log(LOG_ALL, "Wrong sms_code  (user=%d).", login_id);
		return 0;
	}
}

int Session::processPacket_ResendSMS(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received RESEND_SMS (user=%d).", getUser()->getID());
	if (SessionManager::getOptions().PhoneRegisterMode == PHONE_REGISTER_BALANCE) {
		if (getUser()->getBalance() < PHONE_REGISTER_BALANCE_MIN) {
			log(LOG_NORMAL, "balance < PHONE_REGISTER_BALANCE_MIN (user=%d).", getUser()->getID());
			return sendPacket_Error(ERROR_BALANCE);
		}
	}
	if (SessionManager::getOptions().PhoneRegisterMode == PHONE_REGISTER_NONE) {
		log(LOG_NORMAL, "Discarding REGISTER_PHONE as PHONE_REGISTER_NONE (user=%d).", getUser()->getID());
		return sendPacket_Error(ERROR_OTHER);
	}

	bool must_confirm = (user->getPhoneStatus() == PHONE_STATUS_AWAIT);
	must_confirm &= (SessionManager::getOptions().PhoneRegisterMode != PHONE_REGISTER_NONE);
	if (!must_confirm) {
		log(LOG_WARNING, "Client error: attempt to resend SMS with code for autodetermined phone number (user=%d).", getUser()->getID());
		sendPacket_SMSError(ERROR_FORMAT);
		return 0;
	} else {
		getUser()->determineSMSCodeTimeout(getCon());
		if (getUser()->getSMSBlockDays() > 0) {
			log(LOG_VERBOSE, "Too many attempts to send SMS from device_id='%s'. Discarding RESEND_SMS. (user=%d).", getUser()->getClientDeviceID().c_str(), getUser()->getID());
			sendPacket_SMSError(ERROR_TEMP_BLOCKED);
			return 0;
		}
		TwilioMessage *msg = new TwilioMessage();
		if (getUser()->getLastSMS(SMS_TYPE_PHONE_CODE, msg)) {
			log(LOG_WARNING, "Client error: attempt to resend SMS without any SMSes sent (user=%d).", getUser()->getID());
			sendPacket_SMSError(ERROR_FORMAT);
			return 0;
		}
		log(LOG_VERBOSE, "Resending SMS (user=%d).", getUser()->getID());
		if (sendSMSCode(msg)) {
			getUser()->unlock();
			update_TLists(true);
			log(LOG_ERROR, "Error sending SMS code (user=%d, phone='%s').", getUser()->getID(), msg->phone.c_str());
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
		sendPacket_Error(ERROR_NOERROR);
	}
	return 0;
}

int Session::processPacket_getLanguages(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received GET_LANGS");
	return sendPacket(sManager->getLangsXML().c_str());
}

// user must be locked
int Session::sendPacket_UserData()
{
	Translator *t = dynamic_cast<Translator *>(getUser());
	string msg = "<user_data>";
	msg += "<is_translator>";	msg += std::to_string((bool)t);	msg += "</is_translator>";
	msg += "<email>";	msg += getUser()->getEmail();	msg += "</email>";
	msg += "<name>";	msg += getUser()->getName();	msg += "</name>";
	msg += "<phone_status>";	msg += std::to_string(getUser()->getPhoneStatus());	msg += "</phone_status>";
	if (getUser()->getPhone() && strlen(getUser()->getPhone())) {
		msg += "<phone>";	msg += getUser()->getPhone();	msg += "</phone>";
	}
	if (getUser()->getAwaitPhone() && strlen(getUser()->getAwaitPhone())) {
		msg += "<await_phone>";	msg += getUser()->getAwaitPhone();	msg += "</await_phone>";
	}
	Langs langs = getUser()->getLangs();
	for (size_t i=0; i<langs.num(); i++) {
		msg += "<translate>";
		msg += "<lang>";	msg += langs.get(i);	msg += "</lang>";
		msg += "<price>";	msg += std::to_string(langs.getPrice(i));	msg += "</price>";
		msg += "</translate>";
	}
	msg += "<balance>";	msg += std::to_string(getUser()->getBalance());	msg += "</balance>";
	if (t) {
		msg += "<rating>";	msg += std::to_string(10*(int)t->getRating());	msg += "</rating>";
		msg += "<rating_num>";	msg += std::to_string(t->getRatingNum());	msg += "</rating_num>";
		msg += "<paypal_email>";	msg += t->getPayPalEmail();	msg += "</paypal_email>";
	}
	msg += "<options>";
	Options options = SessionManager::getOptions();
	msg += "<phone_register_mode>";	msg += std::to_string(options.PhoneRegisterMode);	msg += "</phone_register_mode>";
	msg += "<fee_market>";	msg += std::to_string(options.FeeMarket);	msg += "</fee_market>";
	msg += "<fee_app>";	msg += std::to_string(options.FeeApp);	msg += "</fee_app>";
	msg += "<call_time_free_sec>";	msg += std::to_string(options.Call_TimeFree);	msg += "</call_time_free_sec>";
	msg += "<call_min_balance_sec>";	msg += std::to_string(options.Call_MinLength);	msg += "</call_min_balance_sec>";
	msg += "<call_min_time_rating_sec>";	msg += std::to_string(options.Call_MinTimeRating);	msg += "</call_min_time_rating_sec>";
	msg += "<active_tsearch>";	msg += std::to_string(options.ActiveTSearch);	msg += "</active_tsearch>";
	msg += "</options>";
	msg += "</user_data>";
	return sendPacket(msg.c_str());
}

void Session::SetTranslatorBusy(bool busy)
{
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (!t)
		return;
	if (t->setBusy(busy))
		update_TLists(false);
}

int Session::processPacket_SetBusy(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received SET_BUSY (user=%d).", getUser()->getID());
	int busy = -1;
	bool exist_busy = false;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect SET_BUSY (user=%d).", user->getID());
			sendPacket_Error(ERROR_FORMAT);
			XMLString::release(&nodeName);
			return 0;
		}
		if (!nodeVal) {
			error = 1;
		} else if (!strcasecmp(nodeName, "busy")) {
			try {
				busy = std::stoi(nodeVal);
				exist_busy = true;
			} catch (std::exception ex) {
				error = 1;
			}
		} else error = 1;
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !exist_busy) {
		log(LOG_NORMAL, "Incorrect SET_BUSY (user=%d).", login_id);
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	sManager->rdlock();
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (t)
		SetTranslatorBusy(busy);
	sManager->unlock();
	return sendPacket_Error(ERROR_NOERROR);
}

void Session::update_TLists(bool del)
{
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (t) {
		t->lock();
		vector<Translator> tlist;
		Translator tc = *t;
		t->unlock();
		tc.reset_lock();
		tlist.push_back(tc);
		sManager->Lists_processUpdate(tlist, del);
	}
}

int Session::processPacket_getUserData(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received GET_USER_DATA (user=%d)", getUser()->getID());
	getUser()->lock();
	sendPacket_UserData();
	getUser()->unlock();
	return 0;
}

int Session::processPacket_UserData(DOMNode *node)
{
	log(LOG_ALL, "Received USER_DATA (user=%d).", getUser()->getID());
	vector<unsigned int> calls = cManager->findCalls(getUser()->getID());
	if (calls.size() > 0) {
		sendPacket_Error(ERROR_CALL_EXIST);
		return 0;
	}

	Translator *t = dynamic_cast<Translator *>(getUser());
	if (t && t->isPayPalTransferActive()) {
		log(LOG_NORMAL, "Discarding USER_DATA because of active PayPalTransfer (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_PAYPAL_TRANSFER_ACTIVE);
		return 0;
	}

	char name[MAX_NAME+1] = {0};
	char paypal_email[MAX_EMAIL+1] = {0};
	vector<pair<string, long>> langs;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect USER_DATA (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "name")) {
			if (strlen(nodeVal) > MAX_NAME)
				error = 1;
			else
				strcpy(name, nodeVal);
		} else if (!strcasecmp(nodeName, "paypal_email")) {
			if (strlen(nodeVal) > MAX_NAME)
				error = 1;
			else
				strcpy(paypal_email, nodeVal);
		} else if (!strcasecmp(nodeName, "translate")) {
			DOMNode *tnode = node->getFirstChild();
			if (!tnode) {
				log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
				return 0;
			}
			string lang = "";
			long price = -1;
			do {
				char *tnodeName = XMLString::transcode(tnode->getNodeName());
				char *tnodeVal = 0;
				if (tnode->getFirstChild())
					tnodeVal = XMLString::transcode(tnode->getFirstChild()->getNodeValue());
				else {
					log(LOG_NORMAL, "Incorrect USER_DATA translate (user=%d).", user->getID());
					XMLString::release(&tnodeName);
					sendPacket_Error(ERROR_FORMAT);
					return 0;
				}
				if (!strcasecmp(tnodeName, "lang")) {
					if (!Langs::isLang(tnodeVal))
						error = 1;
					else
						lang = tnodeVal;
				} else if (!strcasecmp(tnodeName, "price")) {
					try {
						price = std::stoi(tnodeVal);
					} catch (std::exception ex) {
						error = 1;
					}
				} else {
					log(LOG_NORMAL, "Incorrect fields in USER_DATA translate packet.");
					error = 1;
				}
				XMLString::release(&tnodeName);
				XMLString::release(&tnodeVal);
				if (error)
					break;
			} while (tnode = tnode->getNextSibling());
			if (!dynamic_cast<Translator *>(getUser()))
				price = 0;
			else if (price < 0)
				error = 1;
			if (!error && lang.size() > 0)
				langs.push_back(make_pair(lang, price));
			else
				error = 1;
		} else {
			log(LOG_NORMAL, "Incorrect fields in USER_DATA packet.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !strlen(name) || (t && (strlen(paypal_email) > 0 && !User::checkEmail(paypal_email)))) {
		log(LOG_NORMAL, "Incorrect USER_DATA (user=%d).", user->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
/*	if (!langs.size()) {
		log(LOG_ALL, "No langs in USER_DATA (user=%d).", user->getID());
		sendPacket_Error(ERROR_NO_LANG);
		return 0;
	}
*/
	getUser()->lock();
	if (!getUser()->setName(name, getCon())) {
		getUser()->unlock();
		sendPacket_Error(ERROR_NAME_EXIST);
		return 0;
	}
	if (t) {
		User::formatEmail(paypal_email);
		t->setPayPalEmail(paypal_email);
	}
	getUser()->setLangs(langs);
	getUser()->DBWrite();
	getUser()->unlock();
	update_TLists(false);
	log(LOG_ALL, "Updated user_data (user=%d).", user->getID());
	sendPacket_Error(ERROR_NOERROR);
	return 0;
}

int Session::onPayPalPurchaseVerified(int result_code, int money)
{
	switch (result_code) {
	case BILLING_VERIFY_FAILURE:
		return sendPacket_PayPalTransferError(ERROR_OTHER);
	case BILLING_VERIFY_ERROR_SIGNATURE:
		return sendPacket_PayPalTransferError(ERROR_PURCHASE_SIGNATURE);
	case BILLING_VERIFY_OK:
		return sendPacket_PayPalTransferError(ERROR_NOERROR);
	default:
		sendPacket_PayPalTransferError(ERROR_OTHER);
		return -1;
	}
}

int Session::onPurchaseVerified(int result_code, int money)
{
	switch (result_code) {
	case BILLING_VERIFY_FAILURE:
		return sendPacket_BillingError(ERROR_OTHER, money);
	case BILLING_VERIFY_ERROR_SIGNATURE:
		return sendPacket_BillingError(ERROR_PURCHASE_SIGNATURE, money);
	case BILLING_VERIFY_OK:
		return sendPacket_BillingError(ERROR_NOERROR, money);
	default:
		sendPacket_BillingError(ERROR_OTHER, money);
		return -1;
	}
}

int Session::processPacket_Billing(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received BILLING (user=%d).", getUser()->getID());
	if (dynamic_cast<Translator *>(getUser())) {
		log(LOG_ERROR, "Received BILLING from translator. (user=%d)", getUser()->getID());
		sendPacket_Error(ERROR_NO_USER);
		return 0;
	}
	int money = 0;
	string data, signature;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect BILLING (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "money")) {
			if (!sscanf(nodeVal, "%d", &money))
				error = 1;
		} else if (!strcasecmp(nodeName, "data")) {
			data = nodeVal;
		} else if (!strcasecmp(nodeName, "signature")) {
			signature = nodeVal;
		} else {
			log(LOG_VERBOSE, "Incorrect fields in BILLING.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || money <= 0) {
		log(LOG_NORMAL, "Incorrect BILLING (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	if (OS.compare(SESSION_OS_IOS) == 0) {
		IOSPurchase p;
		p.data = data;
		p.user = getUser()->getID();
		p.money = money;
		sManager->IOSPurchaseVerify(p);
	} else if (OS.compare(SESSION_OS_ANDROID) == 0) {
		int result_code = sManager->AndroidPurchaseVerify(money, data, signature);
		IOSPurchase p;
		p.money = money;
		p.user = getUser()->getID();
		sManager->Purchase_VerifyHandler(p, result_code);
	}
	return 0;
}

int Session::processPacket_PayPalTransfer(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received PAYPAL_TRANSFER (user=%d).", getUser()->getID());

	Translator *t = dynamic_cast<Translator *>(getUser());
	if (t && t->isPayPalTransferActive()) {
		log(LOG_NORMAL, "Discarding PAYPAL_TRANSFER because of active PayPalTransfer (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_PAYPAL_TRANSFER_ACTIVE);
		return 0;
	}

	if (!t) {
		log(LOG_ERROR, "Received PAYPAL_TRANSFER from client. (user=%d)", getUser()->getID());
		sendPacket_Error(ERROR_NO_USER);
		return 0;
	}

	int money = 0;
	bool exist_money = false;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect PAYPAL_TRANSFER (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "money")) {
			if (!sscanf(nodeVal, "%d", &money))
				error = 1;
			else
				exist_money = true;
		} else {
			log(LOG_VERBOSE, "Incorrect fields in PAYPAL_TRANSFER.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !exist_money) {
		log(LOG_NORMAL, "Incorrect PAYPAL_TRANSFER (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
/*	if (getUser()->changeBalance(cManager, money)) {
		log(LOG_ERROR, "Error updating balance (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	if (getUser()->DBWrite()) {
		log(LOG_ERROR, "Error storing new User balance (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	*/
	if (money == 0) {
		log(LOG_ERROR, "Attempt to transfer 0 money to user PayPal %s (user=%d).", t->getPayPalEmail(), getUser()->getID());
		sendPacket_Error(ERROR_BALANCE);
		return 0;
	}
	if (sManager->PayPal_Transfer(t->getPayPalEmail(), money)) {
		log(LOG_ERROR, "Error transferring money to user PayPal %s (user=%d).", t->getPayPalEmail(), getUser()->getID());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	sendPacket_Error(ERROR_NOERROR);
	return 0;
}

int Session::processPacket_RequestTranslatorList(DOMNode *node)
{
	log(LOG_ALL, "Received REQUEST_TRANSLATOR_LIST (user=%d).", getUser()->getID());
	if (!TRANSLATORS_CALL) {
		Translator *t = dynamic_cast<Translator *>(user);
		if (t) {
			log(LOG_NORMAL, "Received REQUEST_TRANSLATOR_LIST from translator.");
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
	}
	string lang;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect REQUEST_TRANSLATOR_LIST (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "list_lang")) {
			if (!Langs::isLang(nodeVal))
				error = 1;
			else
				lang =  nodeVal;
		} else {
			log(LOG_NORMAL, "Incorrect fields in REQUEST_TRANSLATOR_LIST packet.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
    if (error || !lang.size()) {
		log(LOG_NORMAL, "Incorrect REQUEST_TRANSLATOR_LIST (user=%d).", user->getID());
		sendPacket_Error(ERROR_FORMAT);
		getUser()->setListLang("");
		return 0;
	}
	if (user->getLangs().num() < 1) {
		log(LOG_NORMAL, "REQUEST_TRANSLATOR_LIST from user without langs in USER_DATA (user=%d).", user->getID());
		sendPacket_Error(ERROR_NO_USERDATA);
		return 0;
	}
	getUser()->lock();
	getUser()->setListLang(lang);
	getUser()->unlock();
	// not locking user
	vector<Translator> tlist = sManager->getTranslatorList(getUser());
	cManager->rdlock();
	cManager->updateCallState(tlist, getUser()->getID());
	cManager->unlock();
	getUser()->lock();
	sendPacket_TranslatorList(tlist, false);
	getUser()->unlock();
	return 0;
}

// client must be locked
int Session::sendPacket_TranslatorList(vector<Translator> tlist, bool del)
{
	if (!sManager)
		return -1;
	if (!getUser())
		return -1;
/*
	Client *c = dynamic_cast<Client *>(getUser());
	if (!c)
		return 0;
		*/
	string msg = "<translator_list>";
	for (size_t i=0; i<tlist.size(); i++) {
		Translator t = tlist[i];
		msg += "<translator>";
		msg += "<delete>";	msg += std::to_string(t.getTListDel());	msg += "</delete>";
//		msg += "<busy>";	msg += std::to_string(cManager->findActiveCall(t.getID()) || t.isBusy());	msg += "</busy>";
		msg += "<busy>";	msg += std::to_string(t.isBusy());	msg += "</busy>";
		msg += "<id>";	msg += std::to_string(t.getID());	msg += "</id>";
		if (!del) {
			msg += "<name>";	msg += t.getName();	msg += "</name>";
			msg += "<price>";	msg += std::to_string(t.getClientPrice(getUser(), getUser()->getListLang()));	msg += "</price>";
			Langs l = t.getLangs();
			for (size_t j=0; j<l.num(); j++) {
				msg += "<translate>";
				msg += "<lang>";	msg += l.get(j);	msg += "</lang>";
				msg += "<price>";	msg += std::to_string(l.getPrice(j));	msg += "</price>";
				msg += "</translate>";
			}
			msg += "<client_lang>";	msg += t.getCommonLang(getUser());	msg += "</client_lang>";
			msg += "<country>";	msg += t.getCountry();	msg += "</country>";
			msg += "<rating>";	msg += std::to_string((int)(10*t.getRating()));	msg += "</rating>";
			msg += "<rating_num>";	msg += std::to_string(t.getRatingNum());	msg += "</rating_num>";

			msg += "<await>";	msg += std::to_string(t.getAwait());	msg += "</await>";
			msg += "<confirmed>";	msg += std::to_string(t.getConfirmed());	msg += "</confirmed>";
			msg += "<rejected>";	msg += std::to_string(t.getRejected());	msg += "</rejected>";
			msg += "<error>";	msg += std::to_string(t.getError());	msg += "</error>";
			msg += "<phone>";	msg += t.getPhone();	msg += "</phone>";
		}
		msg += "</translator>";
		if (msg.length() > PACKET_BUF_SIZE-5000)
			break;
	}

	msg += "<translators>";	msg += std::to_string(sManager->getStatistic(getUser()).translators);	msg += "</translators>";
	msg += "<clients>";	msg += std::to_string(sManager->getStatistic(getUser()).clients);	msg += "</clients>";
	msg += "</translator_list>";
	return sendPacket(msg.c_str());
}

int Session::processPacket_StopTranslatorList(DOMNode *node)
{
	/*
	Translator *t = dynamic_cast<Translator *>(user);
	if (t) {
		log(LOG_VERBOSE, "Received STOP_TRANSLATOR_LIST from translator.");
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	*/
	log(LOG_ALL, "Received STOP_TRANSLATOR_LIST (user=%d).", getUser()->getID());
	string lang;

	getUser()->lock();
	getUser()->setListLang("");
	getUser()->unlock();
	sendPacket_Error(ERROR_NOERROR);
	return 0;
}

int Session::processPacket_RequestClientList(DOMNode *node)
{
	log(LOG_ALL, "Received REQUEST_CLIENT_LIST (user=%d).", getUser()->getID());
	Translator *t = dynamic_cast<Translator *>(user);
	if (!t) {
		log(LOG_NORMAL, "Received REQUEST_TRANSLATOR_LIST from client.");
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
/*	string lang;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;

	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect REQUEST_CLIENT_LIST (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "list_lang")) {
			if (!Langs::isLang(nodeVal))
				error = 1;
			else
				lang =  nodeVal;
		} else {
			log(LOG_NORMAL, "Incorrect fields in REQUEST_TRANSLATOR_LIST packet.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
    if (error || !lang.size()) {
		log(LOG_NORMAL, "Incorrect REQUEST_TRANSLATOR_LIST (user=%d).", user->getID());
		sendPacket_Error(ERROR_FORMAT);
		getUser()->setListLang("");
		return 0;
	}
*/
	if (user->getLangs().num() < 1) {
		log(LOG_NORMAL, "REQUEST_CLIENT_LIST from user without langs in USER_DATA (user=%d).", user->getID());
		sendPacket_Error(ERROR_NO_USERDATA);
		return 0;
	}
	cManager->rdlock();
	vector<Call> calllist = cManager->getClientList(getUser()->getID());
	cManager->unlock();
	// not locking user
	getUser()->lock();
	sendPacket_ClientList(calllist, false);
	getUser()->unlock();
	return 0;
}

// client must be locked
int Session::sendPacket_ClientList(vector<Call> calllist, bool del)
{
	if (!sManager)
		return -1;
/*
	Client *c = dynamic_cast<Client *>(getUser());
	if (!c)
		return 0;
		*/
	string msg = "<client_list>";
	for (size_t i=0; i<calllist.size(); i++) {
		Call call = calllist[i];
		Session *cs = sManager->findSessionByUser(call.getClient());
		if (!cs)
			continue;
		Client *c = dynamic_cast<Client *>(cs->getUser());
		if (!c)
			continue;

		msg += "<client>";
		msg += "<id>";	msg += std::to_string(c->getID());	msg += "</id>";
		msg += "<delete>";	msg += std::to_string((int)del);	msg += "</delete>";
		if (!del) {
			msg += "<name>";	msg += c->getName();	msg += "</name>";
			msg += "<client_lang>";	msg += call.getClientLang();	msg += "</client_lang>";
			msg += "<translate_lang>";	msg += call.getTranslateLang();	msg += "</translate_lang>";
			msg += "<country>";	msg += c->getCountry();	msg += "</country>";
			msg += "<balance>";	msg += std::to_string(call.getBalance());	msg += "</balance>";
			msg += "<price>";	msg += std::to_string(call.getPrice());	msg += "</price>";
			msg += "<confirmed>";	msg += std::to_string(call.getState() == CONFIRMED || call.getState() == ACTIVE);	msg += "</confirmed>";
			msg += "<rejected>";	msg += std::to_string(call.getState() == REJECTED);	msg += "</rejected>";
			msg += "<error>";	msg += std::to_string(call.getState() == ERROR);	msg += "</error>";
		}
		msg += "</client>";
		if (msg.length() > PACKET_BUF_SIZE-5000)
			break;
	}
	msg += "</client_list>";
	return sendPacket(msg.c_str());
}

int Session::processPacket_PhoneCallRequest(xercesc::DOMNode *node)
{
	if (!TRANSLATORS_CALL) {
		Translator *t = dynamic_cast<Translator *>(user);
		if (t) {
			log(LOG_ALL, "Error: Received PHONECALL_REQUEST from translator (user=%d).", user->getID());
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
	}

	log(LOG_ALL, "Received PHONECALL_REQUEST (user=%d).", getUser()->getID());
	unsigned int translator = 0;
	string translate_lang;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect PHONECALL_REQUEST (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "translator")) {
			if (!sscanf(nodeVal, "%d", &translator))
				error = 1;
		} else if (!strcasecmp(nodeName, "translate_lang")) {
			if (!Langs::isLang(nodeVal))
				error = 1;
			else
				translate_lang =  nodeVal;
		} else {
			log(LOG_VERBOSE, "Incorrect fields in PHONECALL_REQUEST.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !translate_lang.size()) {
		log(LOG_NORMAL, "Incorrect PHONECALL_REQUEST (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}

	if (!sManager->findSessionByUser(translator)) {
		User *u = User::readUser(getCon(), translator);
		if (!u) {
			log(LOG_NORMAL, "Dismissing phonecall request to unexisting user %d (user=%d).", translator, getUser()->getID());
			sendPacket_Error(ERROR_NO_USER);
			return 0;
		}
		if (dynamic_cast<Translator *>(user)) {
			log(LOG_VERBOSE, "Dismissing phonecall request to offline translator %d (user=%d).", translator, getUser()->getID());
			sendPacket_Error(ERROR_USER_OFFLINE);
		} else {
			log(LOG_VERBOSE, "Dismissing phonecall request to (offline)client %d (user=%d).", translator, getUser()->getID());
			sendPacket_Error(ERROR_NO_USER);
		}
		delete u;
		return 0;
	}
	cManager->wrlock();
	if (cManager->findActiveConfirmedCall(getUser()->getID())) {
		log(LOG_NORMAL, "Dismissing phonecall from already calling client (user=%d)", getUser()->getID());
		sendPacket_Error(ERROR_CALL_EXIST);
		cManager->unlock();
		return 0;
	}

	getUser()->lock();
	if (!strlen(getUser()->getPhone())/* && !os.compare(SESSION_OS_IOS)*/) {
		getUser()->unlock();
		cManager->unlock();
		log(LOG_VERBOSE, "Requesting phone number for making a phonecall (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_NO_PHONE);
		return 0;
	} else if (SessionManager::getOptions().PhoneRegisterMode != PHONE_REGISTER_NONE &&
			getUser()->getPhoneStatus() != PHONE_STATUS_CONFIRMED) {
		getUser()->unlock();
		cManager->unlock();
		log(LOG_VERBOSE, "Requesting phone number confirmation for making a phonecall (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_PHONE_AWAITING);
		return 0;
	}
	getUser()->unlock();

	if (int call_id = cManager->findActiveConfirmedCall(translator)) {
		Call *call = cManager->getCall(call_id);
		if (call && call->getState() == ACTIVE) {
			log(LOG_VERBOSE, "Dismissing call to busy translator %d (user=%d)", translator, getUser()->getID());
			sendPacket_Error(ERROR_CALL_EXIST);
			cManager->unlock();
			return 0;
		}
	}
	if (unsigned int call_id = cManager->findCall(getUser()->getID(), translator)) {
		Call *call = cManager->getCall(call_id);
		if (call->getState() == CONFIRMED) {
			log(LOG_VERBOSE, "Sending PHONECALL_CONFIRM for already confirmed call (user=%d).", getUser()->getID());
			if (sendPacket_PhoneCallConfirm(call_id, true)) {
				log(LOG_NORMAL, "Warning: Unable to send PHONECALL_CONFIRM (user=%d).", getUser()->getID());
//				sendPacket_Error(ERROR_CALL_STATE);
				cManager->unlock();
				return 0;
			}
		} else {
			log(LOG_NORMAL, "Sending ERROR_CALL_EXIST for confirm on existing call (user=%d).", getUser()->getID());
			sendPacket_Error(ERROR_CALL_EXIST);
		}
		cManager->unlock();
		return 0;
	}

	unsigned int call_id = cManager->newPhoneCall(getCon(), getUser()->getID(), translator, translate_lang);
	if (!call_id) {
		cManager->unlock();
		log(LOG_ERROR, "Error: unable to create PhoneCall to user %d (user=%d).", translator, getUser()->getID());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}

	PhoneCall *pcall = (PhoneCall *)cManager->getCall(call_id);
	pcall->lock();
	pcall->savePhones();
	getUser()->lock();
	pcall->setBalance(getUser()->getBalance());
	if (getUser()->getBalance() < ((float)SessionManager::getOptions().Call_MinLength/60)*pcall->getPrice()) {
		getUser()->unlock();
		sendPacket_CallRequestError(ERROR_BALANCE, false, pcall->getID());
		pcall->unlock();
		cManager->EndCall(getCon(), call_id);
		cManager->unlock();
		log(LOG_VERBOSE, "Denying call of client to %d because of low balance. (user=%d).", pcall->getTranslator(), getUser()->getID());
		return 0;
	}
	if (!sendPacket_PhoneCallRequest(call_id))
		pcall->sentRequest();
	else {
		getUser()->unlock();
		pcall->Error();
//		cManager->sendCallStatus(call_id);
		sendPacket_CallRequestError(ERROR_PHONECALL_ERROR, false, pcall->getID());
		pcall->unlock();
		cManager->EndCall(getCon(), call_id);
		cManager->unlock();
		log(LOG_ERROR, "Error: unable to send PHONECALL_REQUEST (call_user=%d).", call_id);
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	getUser()->unlock();
	if (pcall->DBwrite(getCon())) {
		pcall->unlock();
		cManager->unlock();
		return 0;
	}
	pcall->unlock();
	cManager->unlock();
	return 0;
}

// CallManager must be locked, SessionManager must be read-locked, user must be locked
int Session::sendPacket_PhoneCallRequest(unsigned int call_id)
{
	if (!sManager)
		return -1;
	Call *call = cManager->getCall(call_id);
	if (!call || call->getState() != INIT)
		return -1;

/*	vector<Call> calllist;						// not tasking for android preprocessing
	Call callc = *call;
	callc.reset_lock();		// user locked
	calllist.push_back(callc);

	Session *ts = sManager->findSessionByUser(call->getTranslator());
	if (!ts)
		return -1;
	return ts->sendPacket_ClientList(calllist, false);
	*/

	Session *cs = sManager->findSessionByUser(call->getClient());
	if (!cs)
		return -1;
	Client *c = dynamic_cast<Client *>(cs->getUser());
	if (!c)
		return -1;
	string msg = "<phonecall_request>";
	msg += "<client>";	msg += std::to_string(c->getID());	msg += "</client>";
	msg += "<name>";	msg += getUser()->getName();	msg += "</name>";
	msg += "<translate_lang>";	msg += call->getTranslateLang();		msg += "</translate_lang>";
	for (int i=0; i<c->getLangs().num(); i++) {
		msg += "<client_lang>";	msg += c->getLangs().get(i);	msg += "</client_lang>";
	}
	msg += "<price>";	msg += std::to_string(call->getPrice());	msg += "</price>";
	msg += "<country>";	msg += c->getCountry();		msg += "</country>";
	msg += "<balance>";	msg += std::to_string(c->getBalance());	msg += "</balance>";
	msg += "</phonecall_request>";
	Session *ts = sManager->findSessionByUser(call->getTranslator());
	if (!ts)
		return -1;
	int ret = ts->sendPacket(msg.c_str());
	return ret;
}

int Session::processPacket_PhoneCallConfirm(xercesc::DOMNode *node)
{
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (!t) {
		log(LOG_ALL, "Error: received PHONECALL_CONFIRM from client (user=%d).", user->getID());
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	log(LOG_ALL, "Received PHONECALL_CONFIRM (user=%d).", getUser()->getID());
	unsigned int client = 0;
	int accept = -1;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect PHONECALL_CONFIRM (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return -1;
		}
		if (!strcasecmp(nodeName, "client")) {
			if (!sscanf(nodeVal, "%d", &client))
				error = 1;
		} else if (!strcasecmp(nodeName, "accept")) {
			if (!sscanf(nodeVal, "%d", &accept))
				error = 1;
		} else {
			log(LOG_NORMAL, "Incorrect fields in PHONECALL_CONFIRM.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !client || (accept != 0 && accept != 1)) {
		log(LOG_NORMAL, "Incorrect PHONECALL_CONFIRM (user=%d).", user->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	cManager->rdlock();
	unsigned int call_id = cManager->findCall(client, getUser()->getID());
	if (!call_id) {
		cManager->unlock();
		log(LOG_NORMAL, "Error: Received PHONECALL_CONFIRM for unknown call (call_user=%d).", call_id);
		sendPacket_Error(ERROR_UNKOWN_CALL);
		return -0;
	}
	Call *call = cManager->getCall(call_id);
	PhoneCall *pcall = dynamic_cast<PhoneCall *>(call);
	if (!pcall) {
		cManager->unlock();
		log(LOG_NORMAL, "Error: Received PHONECALL_CONFIRM for VoIP call (call_user=%d).", call_id);
		sendPacket_Error(ERROR_UNKOWN_CALL);
		return 0;
	}
	pcall->lock();
	pcall->receivedConfirm(accept);
	if (call->getState() == ERROR) {
		pcall->sendCallRequestError(ERROR_PHONECALL_ERROR);
		pcall->unlock();
		cManager->EndCall(getCon(), call_id);
		cManager->unlock();
		log(LOG_NORMAL, "PHONECALL_CONFIRM in wrong state (user=%d).", user->getID());
		sendPacket_Error(ERROR_CALL_STATE);
		return 0;
	}
	pcall->DBwrite(getCon());

	if (sendPacket_PhoneCallConfirm(call->getID(), accept)) {
//		pcall->sendCallRequestError(ERROR_PHONECALL_ERROR);			// connection might be lost temporarily
		pcall->unlock();
//		cManager->EndCall(getCon(), call_id);
		cManager->unlock();
		log(LOG_NORMAL, "Warning: Unable to send PHONECALL_CONFIRM (user=%d).", user->getID());
//		sendPacket_Error(ERROR_CALL_STATE);
		return 0;
	}
	pcall->unlock();
	cManager->unlock();
	return 0;
}

// CallManager must be locked, SessionManager must be read-locked
int Session::sendPacket_PhoneCallConfirm(unsigned int call_id, bool accept)
{
	if (!sManager)
		return -1;
	Call *call = cManager->getCall(call_id);
	if (!call)
		return -1;
	Session *ts = sManager->findSessionByUser(call->getTranslator());
	if (!ts) {
		unlock();
		return -1;
	}
	Translator *t = dynamic_cast<Translator *>(ts->getUser());
	if (!t) {
		unlock();
		return -1;
	}
/*									// not tasking for android preprocessing
	vector<Translator> tlist;
	t->lock();
	Translator tc = *t;
	tc.reset_lock();
	t->unlock();
	if (accept)
		tc.setConfirmed(true);
	else
		tc.setRejected(true);
	tlist.push_back(tc);
	Session *cs = sManager->findSessionByUser(call->getClient());
	if (!cs || !cs->getUser())
		return -1;
	return cs->sendPacket_TranslatorList(tlist, false);
*/
	string msg = "<phonecall_confirm>";
	msg += "<translator>";	msg += std::to_string(getUser()->getID());	msg += "</translator>";
	msg += "<accept>";	msg += std::to_string(accept);	msg += "</accept>";
	if (accept) {
		t->lock();
		msg += "<phone>";	msg += t->getPhone();	msg += "</phone>";
		t->unlock();
	}
	msg += "</phonecall_confirm>";

	Session *cs = sManager->findSessionByUser(call->getClient());
	if (!cs || !cs->getUser())
		return -1;
	return cs->sendPacket(msg.c_str());

}

int Session::sendPacket_PhoneCallTimeout(bool client, int user)
{
	string msg = "<phonecall_timeout>";
	if (client) {
		msg += "<client>";	msg += std::to_string(user);	msg += "</client>";
	} else {
		msg += "<translator>";	msg += std::to_string(user);	msg += "</translator>";
	}
	msg += "</phonecall_timeout>";
	sendPacket(msg.c_str());
}

int Session::processPacket_CallStatus(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received PHONECALL_STATUS (user=%d).", getUser()->getID());
	char peer_phone[MAX_PHONE+1] = {0};
	int active = -1;
	int time = -1;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect phonecall_status (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "peer_phone")) {
			if (strlen(nodeVal) <= MAX_PHONE)
				strcpy(peer_phone, nodeVal);
			else
				error = 1;
		} else if (!strcasecmp(nodeName, "active")) {
			if (!sscanf(nodeVal, "%d", &active))
				error = 1;
		} else if (!strcasecmp(nodeName, "time")) {
			if (!sscanf(nodeVal, "%d", &time))
				error = 1;
		} else {
			log(LOG_NORMAL, "Incorrect fields in phonecall_status.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	User::formatPhone(peer_phone);
	if (error || !(active == 0 || (active = 1 && time>=0))) {
		log(LOG_NORMAL, "Incorrect phonecall_status (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (t)
		SetTranslatorBusy(active);

	cManager->rdlock();
	Call *call = cManager->getCall(cManager->findActiveConfirmedCall(getUser()->getID()));
	if (!call)
		call = cManager->getCall(cManager->findCall(getUser()->getID(), peer_phone));
	if (!call) {
		sendPacket_CallStatus(0);
		cManager->unlock();
		return 0;
	}
	unsigned int call_id = call->getID();

	call->lock();
	if (active) {
		PhoneCall *pcall = dynamic_cast<PhoneCall *>(call);
		if (pcall) {
			bool phone_correct = false;
			if (strlen(peer_phone) > 0) {
				if ((getUser()->getID() == pcall->getClient() && !User::equalPhone(pcall->getTranslatorPhone(), peer_phone)) ||
						(getUser()->getID() == pcall->getTranslator() && !User::equalPhone(pcall->getClientPhone(), peer_phone))) {
					log(LOG_VERBOSE, "phonecall_status(call=%d) with wrong peer_phone='%s' (user=%d).", pcall->getID(), peer_phone, getUser()->getID());
					sendPacket_CallStatus(pcall->getID());
					pcall->sendCallRequestError(ERROR_PHONECALL_ERROR);
					call->unlock();
					cManager->unlock();
					cManager->wrlock();
					Call *c = cManager->getCall(call_id);
					if (c)
						cManager->EndCall(getCon(), pcall->getID());
					cManager->unlock();
					call->unlock();
					return 0;
				}
				DetectPeerPhone = true;
			} else {
				log(LOG_VERBOSE, "phonecall_status(call=%d): peer_phone is absent (user=%d).", pcall->getID(), getUser()->getID());
				DetectPeerPhone = false;
				call->unlock();
				cManager->unlock();
				return 0;
			}
		}
		if (DetectPeerPhone) {
			if (call->getState() != END)
				if (!t)
					call->setTranslatorBusy(true);
			if (call->getState() != CONFIRMED && call->getState() != ACTIVE &&
					call->getState() != END && call->getState() != ERROR) {
				log(LOG_NORMAL, "phonecall_status in wrong(non-ready) state for call %d (user=%d).", call->getID(), getUser()->getID());
				call->sendCallStatuses();
				call->unlock();
				cManager->unlock();
				return 0;
			} else if (call->getState() == CONFIRMED)
				call->Start();
		}
	} else {
		if (call->getState() != END)
			if (!t)
				call->setTranslatorBusy(false);
		call->End();
	}
	if (call->getState() == ACTIVE)// || call->getState() == END)
		call->account(time);

	if (call->sendCallStatuses()) {
		log(LOG_NORMAL, "Unable to send phonecall_status for call %d (user=%d).", call->getID(), getUser()->getID());
		call->Error();
		call->sendCallRequestError(ERROR_PHONECALL_ERROR);
		call->unlock();
		cManager->unlock();
		cManager->wrlock();
		Call *c = cManager->getCall(call_id);
		if (c)
			cManager->EndCall(getCon(), call->getID());
		cManager->unlock();
		sendPacket_Error(ERROR_OTHER);
		return 0;
	}
	call->unlock();
	if (call->getState() == END || call->getState() == ERROR) {
		cManager->unlock();
		cManager->wrlock();
		Call *c = cManager->getCall(call_id);
		if (c)
			cManager->EndCall(getCon(), call->getID());
	}
	cManager->unlock();
	return 0;
}

// CallManager must be read-locked, call must be locked
int Session::sendPacket_CallStatus(unsigned int call_id)
{
	if (call_id == 0) {
		string msg = "<phonecall_status><active>0</active></phonecall_status>";
		return sendPacket(msg.c_str());
	}
	Call *call = cManager->getCall(call_id);
	if (!call)
		return -1;
	string msg = "<phonecall_status>";
	msg += "<peer>";	msg += std::to_string(call->getPeer(getUser()->getID()));	msg += "</peer>";
	msg += "<error>";	msg += std::to_string(call->getState() == ERROR);	msg += "</error>";
	msg += "<active>";	msg += std::to_string(call->getState() == ACTIVE);	msg += "</active>";
	msg += "<time>";	msg += std::to_string(call->getAccountedTime());	msg += "</time>";
	msg += "<cost>";	msg += std::to_string(call->countCost());	msg += "</cost>";
	msg += "<balance>";	msg += std::to_string(call->getBalance());	msg += "</balance>";
	msg += "<translate_lang>";	msg += call->getTranslateLang();	msg += "</translate_lang>";
	msg += "<client_lang>";	msg += call->getClientLang();	msg += "</client_lang>";
	msg += "<client_name>";	msg += call->getClientName();	msg += "</client_name>";
	msg += "<translator_name>";	msg += call->getTranslatorName();	msg += "</translator_name>";
	msg += "<price>";	msg += std::to_string(call->getPrice());	msg += "</price>";
	msg += "</phonecall_status>";

	return sendPacket(msg.c_str());
}

int Session::processPacket_getCallHistory(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received GET_CALL_HISTORY (user=%d).", getUser()->getID());
	sendPacket_CallHistory();
	return 0;
}

int Session::sendPacket_CallHistory()
{
	if (!getCon())
			return -1;
	string query;
	Client *t = dynamic_cast<Translator *>(getUser());
	if (t)
		query = "SELECT calls.client, phone, users.name, lang, price, cost, start_time, accounted, client_country, translator_country FROM calls JOIN users ON calls.client=users.id WHERE calls.translator=(?) AND (phone=0 OR accepted=1) AND !ISNULL(start_time) ORDER BY start_time DESC LIMIT " + std::to_string(SessionManager::getOptions().CallHistoryLimit);
	else
		query = "SELECT calls.translator, phone, users.name, lang, price, cost, start_time, accounted, client_country, translator_country FROM calls JOIN users ON calls.translator=users.id WHERE calls.client=(?) AND (phone=0 OR accepted=1) AND !ISNULL(start_time) ORDER BY start_time DESC LIMIT " + std::to_string(SessionManager::getOptions().CallHistoryLimit);
	PreparedStatement *pstmt = getCon()->prepareStatement(query);
	pstmt->setInt(1, getUser()->getID());
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	string msg = "<call_history>";
	while (res->next()) {
		msg += "<call>";
		msg += "<translator>";	msg += std::to_string(res->getInt(1));	msg += "</translator>";
		msg += "<name>";	msg += res->getString(3);	msg += "</name>";
		msg += "<lang>";	msg += res->getString("lang");	msg += "</lang>";
		msg += "<price>";	msg += std::to_string(res->getInt("price"));	msg += "</price>";
		msg += "<cost>";	msg += std::to_string(res->getInt("cost"));	msg += "</cost>";
		int t;
		struct tm tm;
		if ( strptime(res->getString("start_time").c_str(), "%Y-%m-%d %H:%M:%S", &tm) != NULL )
			t = mktime(&tm);
		else
			t = time(0);
		msg += "<start>";	msg += std::to_string(t);	msg += "</start>";
		msg += "<length>";	msg += std::to_string(res->getInt("accounted"));	msg += "</length>";
		msg += "<client_country>";	msg += res->getString("client_country");	msg += "</client_country>";
		msg += "<translator_country>";	msg += res->getString("translator_country");	msg += "</translator_country>";
		msg += "</call>";
		if (msg.length() > PACKET_BUF_SIZE-2000)
			break;
	}
	msg += "</call_history>";
	delete res;
	return sendPacket(msg.c_str());
}

int Session::processPacket_GetMarkHistory(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received GET_MARK_HISTORY (user=%d).", getUser()->getID());
	if (!TRANSLATORS_CALL)
		if (dynamic_cast<Translator *>(getUser())) {
			log(LOG_VERBOSE, "Received GET_MARK_HISTORY from translator (user=%d).", getUser()->getID());
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}
	sendPacket_MarkHistory();
	return 0;
}

int Session::sendPacket_MarkHistory()
{
	PreparedStatement *pstmt = getCon()->prepareStatement(
			"SELECT calls.translator, users.name, marks.mark, users.rating, users.rating_num, !ISNULL(users.del_time) FROM calls JOIN users ON calls.translator=users.id JOIN marks ON calls.client=marks.client AND calls.translator=marks.translator WHERE calls.client=(?) GROUP BY calls.client");
	pstmt->setInt(1, getUser()->getID());
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	string msg = "<mark_history>";
	while (res->next()) {
		msg += "<translator>";
		msg += "<id>";	msg += std::to_string(res->getInt(1));	msg += "</id>";
		msg += "<name>";	msg += res->getString(2);	msg += "</name>";
		msg += "<mark>";	msg += std::to_string(res->getInt(3));	msg += "</mark>";
		msg += "<rating>";	msg += std::to_string(10*res->getInt(4));	msg += "</rating>";
		msg += "<rating_num>";	msg += std::to_string(res->getInt(5));	msg += "</rating_num>";
		msg += "<deleted>";	msg += std::to_string(res->getInt(6));	msg += "</deleted>";
		msg += "</translator>";
		if (msg.length() > PACKET_BUF_SIZE-2000)
			break;
	}
	msg += "</mark_history>";
	delete pstmt;
	return sendPacket(msg.c_str());
}

int Session::processPacket_MarkRating(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received MARK_RATING (user=%d).", getUser()->getID());
	int translator = 0;
	int mark = -1;
	node = node->getFirstChild();
	if (!node) {
		log(LOG_NORMAL, "Empty packet (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	int error = 0;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = 0;
		if (node->getFirstChild())
			nodeVal = XMLString::transcode(node->getFirstChild()->getNodeValue());
		else {
			log(LOG_NORMAL, "Incorrect MARK_RATING (user=%d).", user->getID());
			XMLString::release(&nodeName);
			sendPacket_Error(ERROR_FORMAT);
			return 0;
		}
		if (!strcasecmp(nodeName, "translator")) {
			if (!sscanf(nodeVal, "%d", &translator))
				error = 1;
		} else if (!strcasecmp(nodeName, "mark")) {
			if (!sscanf(nodeVal, "%d", &mark))
				error = 1;
		} else {
			log(LOG_VERBOSE, "Incorrect fields in MARK_RATING.");
			error = 1;
		}
		XMLString::release(&nodeName);
		XMLString::release(&nodeVal);
		if (error)
			break;
	} while (node = node->getNextSibling());
	if (error || !translator || mark < 0 || mark > MAX_RATING) {
		log(LOG_NORMAL, "Incorrect MARK_RATING (user=%d).", getUser()->getID());
		sendPacket_Error(ERROR_FORMAT);
		return 0;
	}
	if (!TRANSLATORS_CALL)
		if (dynamic_cast<Translator *>(getUser())) {
			log(LOG_VERBOSE, "Received MARK_RATING from translator (user=%d).", getUser()->getID());
			sendPacket_Error(ERROR_OTHER);
			return 0;
		}

	Client *c = dynamic_cast<Client *>(getUser());
	if (!c->CheckMark(getCon(), translator)) {
		log(LOG_VERBOSE, "Received MARK_RATING from client who doesn't have long enough calls to translator %d (user=%d).", translator, getUser()->getID());
		sendPacket_Error(ERROR_RATING_ERROR);
		return 0;
	}

	c->AddMark(getCon(), translator, mark);

	sManager->rdlock();
	Session *ts = sManager->findSessionByUser(translator);
	User *u = 0;
	if (ts)
		u = ts->getUser();
	bool dynamic_user = false;
	if (!u) {
		dynamic_user = true;
		sManager->DynUserLock();
		u = User::readUser(getCon(), translator);
	}
	if (!u) {
		if (dynamic_user)
			sManager->DynUserUnlock();
		sManager->unlock();
		log(LOG_NORMAL, "Received MARK_RATING for unknown user %d (user=%d).", translator, getUser()->getID());
		sendPacket_Error(ERROR_NO_USER);
		return 0;
	}
	Translator *t = dynamic_cast<Translator *>(u);
	if (!t) {
		sManager->unlock();
		log(LOG_NORMAL, "Received MARK_RATING for client %d (user=%d).", translator, getUser()->getID());
		sendPacket_Error(ERROR_NO_USER);
		if (dynamic_user) {
			delete u;
			sManager->DynUserUnlock();
		}
		return 0;
	}
	t->lock();
	t->UpdateRating();
	if (dynamic_user) {
		t->unlock();
		delete t;
		sManager->DynUserUnlock();
	} else {
		ts->sendPacket_UserData();
		t->unlock();
		ts->update_TLists(false);
	}
	sManager->unlock();
	sendPacket_Error(ERROR_NOERROR);
	return 0;
}

int Session::sendPacket_MarkRequest(unsigned int translator, string name, unsigned long time)
{
	if (!translator || name.length() == 0)
		return -1;
	string msg = "<mark_request>";
	msg += "<translator>";	msg += std::to_string(translator);	msg += "</translator>";
	msg += "<name>";	msg += name;	msg += "</name>";
	msg += "<time>";	msg += std::to_string(time);	msg += "</time>";
	msg += "</mark_request>";
	return sendPacket(msg.c_str());
}

int Session::processPacket_GetStatistic(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received GET_STATISTIC (user=%d).", getUser()->getID());
	return sendPacket_Statistic();
}

int Session::sendPacket_Statistic()
{
/*	PreparedStatement *pstmt = getCon()->prepareStatement(
			"SELECT calls.translator, users.name, marks.mark, users.rating, users.rating_num, !ISNULL(users.del_time) FROM calls JOIN users ON calls.translator=users.id JOIN marks ON calls.client=marks.client AND calls.translator=marks.translator WHERE calls.client=(?) GROUP BY calls.client");
	pstmt->setInt(1, getUser()->getID());
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	*/
	Statistic stat = sManager->getStatistic(getUser());
	string msg = "<statistic>";
	msg += "<clients>"; msg += std::to_string(stat.clients);	msg += "</clients>";
	msg += "<translators>"; msg += std::to_string(stat.translators);	msg += "</translators>";
	if (SessionManager::getOptions().Stat_statLangs) {
		getUser()->lock();
		for (size_t i=0; i<getUser()->getLangs().num(); i++) {
			msg += "<language_stat>";
			string lang = getUser()->getLangs().get(i);
			msg += "<lang>";	msg += lang;	msg += "</lang>";
			msg += "<translators>";
			if (stat.langnum.find(lang) != stat.langnum.end())
				msg += std::to_string(stat.langnum[lang]);
			else
				msg += std::to_string(0);
			msg += "</translators>";
			msg += "</language_stat>";
		}
		getUser()->unlock();
	}
	msg += "<calls_hour>";	msg += std::to_string(stat.calls_hour);	msg += "</calls_hour>";
	msg += "<calls_day>";	msg += std::to_string(stat.calls_day);	msg += "</calls_day>";
	msg += "<users_hour>";	msg += std::to_string(stat.users_hour);	msg += "</users_hour>";
	msg += "<users_day>";	msg += std::to_string(stat.users_day);	msg += "</users_day>";
	msg += "</statistic>";
	return sendPacket(msg.c_str());
}

int Session::processPacket_GetTranslatorStatistic(xercesc::DOMNode *node)
{
	log(LOG_ALL, "Received GET_TRANSLATOR_STATISTIC (user=%d).", getUser()->getID());
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (!t) {
		sendPacket_Error(ERROR_OTHER);
		log(LOG_ERROR, "Error: GET_TRANSLATOR_STATISTIC from client (user=%d).", getUser()->getID());
		return 0;
	}
	return sendPacket_TranslatorStatistic();
}

int Session::sendPacket_TranslatorStatistic()
{
	Translator *t = dynamic_cast<Translator *>(getUser());
	if (!t)
		return -1;
	TranslatorStatistic tstat;
	if (t->getTranslatorStatistic(getCon(), tstat)) {
		sendPacket_Error(ERROR_OTHER);
		log(LOG_ERROR, "Error: getTranslatorStatistic failed (user=%d).", getUser()->getID());
		return 0;
	}
	string msg = "<translator_statistic>";
	msg += "<all_rating_num>"; msg += std::to_string(tstat.all_rating_num);	msg += "</all_rating_num>";
	msg += "<higher_rating_num>"; msg += std::to_string(tstat.higher_rating_num);	msg += "</higher_rating_num>";
	msg += "<money_sum>";	msg += std::to_string(tstat.money_sum);	msg += "</money_sum>";
	msg += "<call_num>";	msg += std::to_string(tstat.call_num);	msg += "</call_num>";
	msg += "<call_time_sum>";	msg += std::to_string(tstat.call_time_sum);	msg += "</call_time_sum>";
	msg += "</translator_statistic>";
	return sendPacket(msg.c_str());
}

int DEBUG_IO = true;
int Session::recvPacket()
{
	bool checked_disconnect = false;
	int len;
	while (1) {
		if (data_len + sizeof(Header) > PACKET_BUF_SIZE)
			return -1;
		if (data_len < sizeof(Header)) {
			len = SSL_read(ssl, packet + data_len, sizeof(Header) - data_len);
//			if (DEBUG_IO)	fprintf(stderr, "r%d ", len);
			if (/*!checked_disconnect &&*/ len == 0) {
				if (DEBUG_IO)	fprintf(stderr,"len1 0 disconnect", len);
				return -1;
			}
			checked_disconnect = true;
			if (len > 0)
				data_len += len;
			if (data_len > PACKET_BUF_SIZE)
				return -1;
			if (len < 0) {
				int err = SSL_get_error(ssl, len);
				if (DEBUG_IO)	fprintf(stderr,"rE%d", err);
				if (err == SSL_ERROR_WANT_READ)
					return 0;
				if (err == SSL_ERROR_WANT_WRITE)
					return 0;
				if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL)
					return -1;
				continue;
			}
			if (data_len < sizeof(Header))
				continue;
		}
		Header *hdr = (Header *)packet;
		if (hdr->len < 0 || sizeof(Header) + hdr->len > PACKET_BUF_SIZE) {
			log(LOG_VERBOSE, "Wrong incoming packet: len=%d", hdr->len);
			return -1;
		}
		if (sizeof(Header) + hdr->len > PACKET_BUF_SIZE)
			return -1;
		len = SSL_read(ssl, packet + data_len, sizeof(Header) + hdr->len - data_len);
		if (DEBUG_IO)	fprintf(stderr,"R%d ", len);
		if (/*!checked_disconnect &&*/ len == 0) {
			if (DEBUG_IO)	fprintf(stderr,"len2 0 disconnect", len);
			return -1;
		}
		checked_disconnect = true;
		if (len > 0) {
			data_len += len;
			if (data_len > PACKET_BUF_SIZE)
				return -1;
		}
		if (len < 0) {
			int err = SSL_get_error(ssl, len);
			if (DEBUG_IO)	fprintf(stderr,"rE%d", err);
			switch (err) {
			case SSL_ERROR_WANT_WRITE:
				return 0;
			case SSL_ERROR_WANT_READ:
				return 0;
			case SSL_ERROR_ZERO_RETURN:
			case SSL_ERROR_SYSCALL:
			case SSL_ERROR_SSL:
			default:
				return -1;
			}
		}
		if (data_len < sizeof(Header) + hdr->len)
			continue;
		int ret = ProcessPacket();
		data_len -= sizeof(Header) + hdr->len;
		memcpy(packet, packet + sizeof(Header) + hdr->len, data_len);
		if (ret < 0)
			return ret;
	}
}

int Session::sendPacket(const char *xml)
{
	int len = strlen(xml);
	if (len + sizeof(Header) > PACKET_BUF_SIZE)
		return -1;
unsigned int id=0;
if (getUser())	id = getUser()->getID();
log(LOG_ALL,"[%d]send(%d):\t%.*s", id, len, strlen(xml)<1000?strlen(xml):1000, xml);
	struct out_packet *p = new struct out_packet;
	p->data = (Header *)new char[sizeof(Header) + len + 1];
	((Header *)p->data)->len = len;
	p->len = sizeof(Header)+len;
	p->written = 0;
	strncpy((char *)p->data + sizeof(Header), xml, len + 1);
	out_lock();
	out_packets.push(p);
	out_unlock();
	return WritePacketQueue();
}

int Session::WritePacketQueue()
{
	out_lock();
	ssl_want_write = false;
	while (!out_packets.empty()) {
		struct out_packet *p = out_packets.front();
		if (p->len > PACKET_BUF_SIZE) {
			out_packets.pop();
			delete[] p->data;
			delete p;
			continue;
		}
		int len = SSL_write(ssl, p->data + p->written, p->len - p->written);
		if (len < p->len - p->written) {
			if (len > 0) {
				p->written += len;
				out_unlock();
				return 0;
			}
			int err = SSL_get_error(ssl, len);
			if (DEBUG_IO)	fprintf(stderr,"wE%d", err);
			
			switch (err) {
			case SSL_ERROR_WANT_WRITE:
				ssl_want_write = true;
				out_unlock();
				return 0;
			case SSL_ERROR_WANT_READ:
				out_unlock();
				return 0;
			case SSL_ERROR_ZERO_RETURN:
			case SSL_ERROR_SYSCALL:
			case SSL_ERROR_SSL:
			default:
				while (!out_packets.empty())
					out_packets.pop();
				out_unlock();
				return -1;
			}
		}
		out_packets.pop();
		delete[] p->data;
		delete p;
	}
	out_unlock();
	return 0;
}

Connection* Session::getCon() {
	if (!sManager)
		return 0;
	return sManager->getCon(ThreadN);
}

xercesc::XercesDOMParser *Session::getParser()
{
	if (!sManager)
		return 0;
	return sManager->getParser(ThreadN);
}

struct sockaddr_in Session::getIP()
{
	struct sockaddr addr;
	socklen_t len = sizeof(addr);
	getpeername(SSL_get_fd(ssl), &addr, &len);
	return *(sockaddr_in *)&addr;
}

User *Session::getUser()
{
	return user;
}

void Session::Close()
{
	if (user)
		delete user;
	user = 0;
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(socket);
}

Session::~Session()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&out_mutex);
	delete[] packet;
	delete[] recvxml;
}

Session::Session(SessionManager *_sManager, CallManager *_cManager, int _ThreadN, SSL *_ssl, BIO *_bio)
:sManager(_sManager),
 cManager(_cManager),
 ThreadN(_ThreadN),
 ssl(_ssl),
 bio(_bio),
 data_len(0),
 login_id(0),
 user(0),
 PacketCount(0),
 DetectPeerPhone(false),
 markAbort(false), markEnd(false)
{
	try {
		packet = new uint8_t[PACKET_BUF_SIZE + sizeof(Header)];
		recvxml = new uint8_t[PACKET_BUF_SIZE + 1];
	} catch (exception ex) {
		throw ex;
	}
	pthread_mutex_init(&mutex, 0);
	pthread_mutex_init(&out_mutex, 0);
	socket = SSL_get_fd(ssl);
	timeout = time(0);
}
