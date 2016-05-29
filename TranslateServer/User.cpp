/*
 * User.cpp
 *
 *  Created on: 05.05.2014
 *      Author: Raschupkin Roman
 */

#include <stdlib.h>
#include "TranslateServer.h"
#include "SessionManager.h"
#include "User.h"
#include "Call.h"


int User::storeBilling(int result_code, int money)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO billing SET user=(?), money=(?), date=NOW(), code=(?), os=(?)");
	pstmt->setInt(1, id);
	pstmt->setInt(2, money);
	pstmt->setInt(3, result_code);
	pstmt->setString(4, getClientOS());
	ResultSet *res;
	try {
		pstmt->execute();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

// must be locked, CallManager must be locked
int User::changeBalance(CallManager *cManager, long BalanceChange)
{
	if (storeBilling(0, BalanceChange)) {
		log(LOG_FATAL, "Error Billing: unable to storeBilling (user=%d).", getID());
		return -1;
	}

	if (balance + BalanceChange < 0) {
		log(LOG_FATAL, "Error Billing: balance(%s) + BalanceChange(%d) < 0 (user=%d).", balance, BalanceChange, getID());
		return -1;
	}
	balance += BalanceChange;
	return 0;
}

int User::changeBalanceDyn(Connection *sqlCon, SessionManager *sManager, unsigned int id, long BalanceChange)
{
	if (!sqlCon)
		return -1;
	Session *s = sManager->findSessionByUser(id);
	User *u = 0;
	if (s)
		u = s->getUser();
	bool Dyn = (u == 0);
	if (Dyn) {
		sManager->DynUserLock();
		u = User::readUser(sqlCon, id);
		if (!u/*TRANSLATORS_CALL*/) {
			log(LOG_ERROR, "User::changeBalanceDyn Error: tarificating unknown user (user=%d).", id);
			sManager->DynUserUnlock();
			return -1;
		}
	} else

	if (u->getBalance() + BalanceChange < 0) {
		log(LOG_FATAL, "Error: user(%d) balance=%d + BalanceChange=%d < 0).", id, u->getBalance(), BalanceChange);
		BalanceChange = -u->getBalance();
	}

	u->lock();
	u->changeBalance(sManager->getCallManager(), BalanceChange);
	u->DBWrite();
	u->unlock();

	if (Dyn) {
		delete u;
		sManager->DynUserUnlock();
	}
	return 0;
}

void User::setLangs(vector<pair<string, long>> _langs)
{
	langs.clear();
	for (size_t i=0; i<_langs.size(); i++)
		langs.add(_langs[i].first, _langs[i].second);
}

int User::readPhone()
{
	phone_status = PHONE_STATUS_NONE;
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT phone FROM phones WHERE user=(?) AND !ISNULL(from_time) AND ISNULL(to_time)");
	pstmt->setInt(1, id);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() > 1) {
		delete res;
		return -1;
	}

	if (res->rowsCount() == 1) {
		res->first();
		strncpy(phone, res->getString("phone").c_str(), MAX_PHONE+1);
		phone_status = PHONE_STATUS_CONFIRMED;
		delete res;
	} else {
		delete res;
		pstmt = sqlCon->prepareStatement(
				"SELECT phone FROM phones WHERE user=(?) AND ISNULL(from_time) AND ISNULL(to_time) AND !ISNULL(reg_time)");
		pstmt->setInt(1, id);
		try {
			res = pstmt->executeQuery();
		} catch (SQLException &ex) {
			log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
			delete pstmt;
			return -1;
		}
		delete pstmt;
		if (res->rowsCount() >= 1) {
			phone_status = PHONE_STATUS_AWAIT;
			res->first();
			strncpy(await_phone, res->getString("phone").c_str(), MAX_PHONE+1);
		}
		delete res;
	}
	return 0;
}

int User::readLangs()
{
	if (!id || !sqlCon)
		return -1;
	langs.clear();
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT lang, price, pref FROM langs WHERE user=(?)");
	pstmt->setInt(1, id);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	int max_pref = 0;
	if (res->rowsCount() == 0) {
		delete res;
		return 0;
	}
	res->first();
	do {
		int pref = res->getInt("pref");
		if (max_pref < pref)
			max_pref = pref;
	} while (res->next());
	for (int i=0; i<=max_pref; i++) {
		res->first();
		do {
			int pref = res->getInt("pref");
			if (pref == i)
				langs.add(res->getString("lang"), res->getInt("price"));
		} while (res->next());
	}
	delete res;
	return 0;
}

int User::writeLangs()
{
	if (!id || !sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"DELETE FROM langs WHERE user=(?)");
	pstmt->setInt(1, id);
	try {
		pstmt->execute();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	for (int i=0; i<langs.num(); i++) {
		pstmt = sqlCon->prepareStatement(
				"INSERT INTO langs SET user=(?), pref=(?), lang=(?), price=(?)");
		pstmt->setInt(1, id);
		pstmt->setInt(2, i);
		pstmt->setString(3, langs.get(i));
		pstmt->setInt(4, langs.getPrice(i));
		try {
			pstmt->execute();
		} catch (SQLException &ex) {
			log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
			delete pstmt;
			return -1;
		}
		delete pstmt;
	}
}

bool User::checkNameUnique(char *_name, Connection *sqlCon)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement("SELECT COUNT(id) FROM users WHERE name=(?)");
	pstmt->setString(1, _name);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	res->first();
	int count = res->getInt(1);
	if (count == 0) {
		delete res;
		return true;
	}
	delete res;
	return false;
}

bool User::setName(char *_name, Connection *sqlCon)
{
//	if (!checkNameUnique(_name, sqlCon))
//		return false;
	if (strlen(_name) <= MAX_NAME) {
		strcpy(name, _name);
		return true;
	}
	return false;
}

void User::setPhone(char *_phone, int status)
{
	if (!_phone)
		strcpy(phone, "");
	else if (strlen(_phone) <= MAX_PHONE) {
		strcpy(phone, _phone);
	}
	phone_status = status;
}

void User::setLoginParams(string os, int version, string _device_id)
{
	ClientOS = os;
	ClientVersion = version;
	ClientDeviceID = _device_id;
}

void User::setCountry(string _country)
{
	country = _country;
}

void User::formatEmail(char *email) {
	if (!email)
		return;
	for (int i=0; i<strlen(email); i++)
		if (email[i] == ' ' || email[i] == '\t') {
			strcpy(email + i, email + i + 1);
			i--;
		} else
			break;
	for (int i=strlen(email); i>0; i--)
		if (email[i] == ' ' || email[i] == '\t')
			email[i] = 0;
		else
			break;
	for (int i=0; i<strlen(email); i++) {
		if (email[i] >= 'A' && email[i] <= 'F')
			email[i] -= ('A' - 'a');
		if (email[i] == ' ' || email[i] == '\t') {
			strcpy(email + i, email + i + 1);
			i--;
		}
	}
}

bool User::checkEmail(char *email)
{
	if (!email)
		return false;
	int i;
	for (i=0; i<strlen(email); i++)
		if (email[i] == '@')
			break;
	if (i == 0 || i == strlen(email))
		return false;
	for (; i<strlen(email); i++)
		if (email[i] == '.')
			break;
	if (i == strlen(email))
		return false;
	return true;
}

string User::formatDeviceID(char *device_id)
{
	string ret = "";
	if (!device_id)
		return ret;
	for (int i=0; i<strlen(device_id); i++) {
		if (device_id[i] >= 'A' && device_id[i] <= 'F')
			ret.append(1, device_id[i] - ('A' - 'a'));
		if ((device_id[i] >= '0' && device_id[i] <= '9') || (device_id[i] >= 'a' && device_id[i] <= 'f'))
			ret.append(1, device_id[i]);
	}
}

int User::determineSMSCodeTimeout(Connection *sqlCon)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement("SELECT COUNT(sms_twilio.id), DATEDIFF(NOW(), MAX(sms_twilio.time)) FROM sms_twilio \
			JOIN phones ON phones.id = sms_twilio.phone_id JOIN users ON users.id=phones.user WHERE (users.id=(?) OR phones.device_id=(?)) \
			AND sms_twilio.time>NOW()-INTERVAL (?) DAY");
	pstmt->setInt(1, getID());
	pstmt->setString(2, getClientDeviceID());
	pstmt->setInt(3, SessionManager::getOptions().SMS_MaxDays + SessionManager::getOptions().SMS_BlockDays);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	if (res->rowsCount() == 0) {
		SMSSentNum = 0;
		SMSBlockDays = 0;
		delete res;
		return 0;
	} else {
		res->first();
		SMSSentNum = res->getInt(1);
		if (SMSSentNum < SessionManager::getOptions().SMS_MaxNum) {
			delete res;
			SMSBlockDays = 0;
			return 0;
		}
		int diff = res->getInt(2);
		delete res;
		SMSBlockDays = SessionManager::getOptions().SMS_BlockDays - diff;
		return 0;
	}
}

unsigned int User::storeSMSCode(Connection *sqlCon, char *phone, char *sms_code, string device_id)
{
	if (!sqlCon)
		return -1;

	if (resetUserPhoneAwait(sqlCon))
		return -1;

	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO phones SET phone=(?), sms_code=(?), user=(?), reg_time=NOW(), device_id=(?)");
	pstmt->setString(1, phone);
	pstmt->setString(2, sms_code);
	pstmt->setInt(3, getID());
	pstmt->setString(4, device_id);

	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	unsigned int id = 0;
	pstmt = sqlCon->prepareStatement("SELECT id FROM phones WHERE user=(?) and \
										reg_time=(SELECT MAX(reg_time) FROM phones WHERE user=(?))");
	pstmt->setInt(1, getID());
	pstmt->setInt(2, getID());

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	if (res->rowsCount() < 1)
		return 0;
	res->first();
	id = res->getInt("id");
	delete res;
	return id;
}

int User::getLastSMS(int type, TwilioMessage *msg)
{
	if (!msg)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement("SELECT phone_id, phone, sms_code FROM sms_twilio JOIN phones \
			ON sms_twilio.phone_id=phones.id JOIN users ON phones.user=users.id WHERE sms_twilio.type=(?) AND users.id=(?) AND \
			sms_twilio.time=(SELECT MAX(sms_twilio.time) FROM sms_twilio JOIN phones on phone_id=phones.id JOIN users \
			ON phones.user=users.id WHERE users.id=(?))");
	pstmt->setInt(1, type);
	pstmt->setInt(2, getID());
	pstmt->setInt(3, getID());

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	if (res->rowsCount() < 1)
		return -1;
	res->first();
	switch (type) {
	case SMS_TYPE_PHONE_CODE:
		msg->phone_id = res->getInt(1);
		msg->phone = res->getString(2);
		msg->sms_code = res->getString(3);
		break;
	}
	delete res;
	return 0;
}

int User::storeSMSTwilio(Connection *sqlCon, string id, unsigned int phone_id, int status, int error, int type)
{
	if (!sqlCon)
		return -1;

	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO sms_twilio SET id=(?), phone_id=(?), status=(?), error=(?), time=NOW(), type=(?) ON DUPLICATE KEY \
			UPDATE status=(?), error=(?)");
	pstmt->setString(1, id);
	pstmt->setInt(2, phone_id);
	pstmt->setInt(3, status);
	pstmt->setInt(4, error);
	pstmt->setInt(5, type);
	pstmt->setInt(6, status);
	pstmt->setInt(7, error);

	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

int User::updateSMSTwilio(Connection *sqlCon, string sms_id, int status, int error)
{
	if (!sqlCon)
		return -1;

	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE sms_twilio SET status=(?), error=(?) WHERE id=(?)");
	pstmt->setInt(1, status);
	pstmt->setInt(2, error);
	pstmt->setString(3, sms_id);

	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

vector<TwilioMessage> User::getUnsentSMSTwilio(Connection *sqlCon)
{
	vector<TwilioMessage> ret;
	if (!sqlCon)
		return ret;

	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT COUNT(sms_twilio.id), phones.phone, phones.sms_code, sms_twilio.type, sms_twilio.phone_id FROM sms_twilio JOIN phones \
			ON sms_twilio.phone_id=phones.id WHERE sms_twilio.status=(?) AND sms_twilio.error=(?) AND \
			(SELECT COUNT(id) FROM sms_twilio WHERE phone_id=phones.id AND STATUS=(?))=0 AND \
			(SELECT MAX(sms_twilio.time) FROM sms_twilio where phone_id=phones.id) > NOW() - INTERVAL (?) MINUTE GROUP BY phone_id");
	pstmt->setInt(1, SMS_STATUS_UNDELIVERED);
	pstmt->setInt(2, TWILIO_ERROR_UNKNOWN);
	pstmt->setInt(3, SMS_STATUS_DELIVERED);
	pstmt->setInt(4, SessionManager::getOptions().paramsTwilioSMS.Send_Timeout);

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return ret;
	}
	delete pstmt;

	if (res->rowsCount() > 0) {
		res->first();
		do {
			int count = res->getInt(1);
			if (count > SessionManager::getOptions().paramsTwilioSMS.Send_AttemptNum)
				continue;
			TwilioMessage msg;
			msg.phone = res->getString(2);
			msg.sms_code = res->getString(3);
			if (res->getInt(4) == SMS_TYPE_PHONE_CODE)
				msg.type = SMS_TYPE_PHONE_CODE;
			if (msg.type == SMS_TYPE_PHONE_CODE)
				msg.GenText_SMSCode();
			msg.phone_id = res->getInt(6);
			ret.push_back(msg);
		} while (res->next());
	}
	delete res;
	return ret;
}

int User::resetUserPhoneAwait(Connection *sqlCon)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE phones SET to_time = NOW() WHERE user=(?) AND isNULL(from_time)");
	pstmt->setInt(1, getID());
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

int User::resetUserPhone(Connection *sqlCon)
{
	if (!sqlCon)
		return -1;

	if (resetUserPhoneAwait(sqlCon))
		return -1;

	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE phones SET to_time=NOW() WHERE user=(?) AND ISNULL(to_time) AND !ISNULL(from_time)");
	pstmt->setInt(1, getID());
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

bool User::checkSMSCode(Connection *sqlCon, char *sms_code, char *phone)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT phone FROM phones WHERE user=(?) AND ISNULL(from_time) AND sms_code=(?) AND ISNULL(to_time)");
	pstmt->setInt(1, getID());
	pstmt->setString(2, sms_code);
	ResultSet * res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	bool ret = false;
	if (res->rowsCount() == 1) {
		res->first();
		ret = true;
		strncpy(phone, res->getString("phone").c_str(), MAX_PHONE + 1);
	}
	delete res;
	delete pstmt;
	return ret;
}

int User::resetOtherUsersPhones(Connection *sqlCon, char *phone) 
{
	if (!sqlCon || !phone)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE phones SET to_time=NOW() WHERE phone=(?) AND user!=(?) AND ISNULL(to_time) AND !ISNULL(from_time)");
	pstmt->setString(1, phone);
	pstmt->setInt(2, getID());
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

int User::updateUserPhone(Connection *sqlCon)
{
	if (!sqlCon)
		return -1;

	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE phones SET from_time=NOW() WHERE user=(?) AND ISNULL(from_time) AND ISNULL(to_time)");
	pstmt->setInt(1, getID());
	int updCnt;
	try {
		updCnt = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return updCnt != 1;
}

string User::getLastDeviceID(Connection *sqlCon)
{
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT device_id FROM logins WHERE user=(?)");
	pstmt->setInt(1, getID());

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return "";
	}
	delete pstmt;
	if (res->rowsCount() != 1)
		return "";
	res->first();
	string device_id = res->getString("device_id");
	delete res;
	return device_id;
}

int User::updateUserLoginTime(Connection *sqlCon, bool login)
{
	if (!sqlCon)
		return -1;
	if (login) {
		PreparedStatement *pstmt = sqlCon->prepareStatement(
				"INSERT INTO logins SET user=(?), os=(?), version=(?), device_id=(?), login=NOW() ON DUPLICATE KEY UPDATE \
				 os=(?), version=(?), device_id=(?), login=NOW()");
		pstmt->setInt(1, id);
		pstmt->setString(2, getClientOS());
		pstmt->setInt(3, getClientVersion());
		pstmt->setString(4, getClientDeviceID());
		pstmt->setString(5, getClientOS());
		pstmt->setInt(6, getClientVersion());
		pstmt->setString(7, getClientDeviceID());
	/*		PreparedStatement *pstmt = sqlCon->prepareStatement(
				"INSERT INTO logins SET user=(?), login_time=NOW(), os=(?), version=(?), device_id=(?)");
		pstmt->setInt(1, id);
		pstmt->setString(2, getClientOS());
		pstmt->setInt(3, getClientVersion());
		pstmt->setString(4, getClientDeviceID());
*/
		int ret;
		try {
			ret = pstmt->executeUpdate();
		} catch (SQLException &ex) {
			log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
			delete pstmt;
			return -1;
		}
		delete pstmt;
		return 0;
	} else {
/*        PreparedStatement *pstmt = sqlCon->prepareStatement(
                         "SELECT MAX(login_time) FROM logins WHERE user=(?)");
         pstmt->setInt(1, getID());
         */
		PreparedStatement *pstmt = sqlCon->prepareStatement(
				"INSERT INTO logins SET user=(?), os=(?), version=(?), device_id=(?), logout=NOW() ON DUPLICATE KEY UPDATE \
				 os=(?), version=(?), device_id=(?), logout=NOW()");
		pstmt->setInt(1, id);
		pstmt->setString(2, getClientOS());
		pstmt->setInt(3, getClientVersion());
		pstmt->setString(4, getClientDeviceID());
		pstmt->setString(5, getClientOS());
		pstmt->setInt(6, getClientVersion());
		pstmt->setString(7, getClientDeviceID());

		ResultSet *res;
		try {
			pstmt->executeUpdate();
		} catch (SQLException &ex) {
			log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
			delete pstmt;
			return -1;
		}
		delete pstmt;
		return 0;
/*		if (res->rowsCount() != 1)
			return -1;
		res->first();
		string last = res->getString(1);
		pstmt = sqlCon->prepareStatement(
						"UPDATE logins SET logout_time=NOW() WHERE user=(?) AND login_time=(?)");
		pstmt->setInt(1, getID());
		pstmt->setDateTime(2, last);

		int ret;
		try {
			ret = pstmt->executeUpdate();
		} catch (SQLException &ex) {
			log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
			delete pstmt;
			return -1;
		}
		delete pstmt;
		if (ret != 1)
			return -1;
*/	}
}

time_t User::getUserLoginsLength(Connection *sqlCon)
{
return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT SUM(logout_time - login_time) FROM logins WHERE user=(?) AND !isNULL(logout_time) AND login_time>NOW()-INTERVAL 1 MONTH");
	pstmt->setInt(1, getID());

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	if (res->rowsCount() != 1)
		return 0;
	res->first();
	time_t length = res->getInt(1);
	return length;
}

int User::deleteUser(Connection *sqlCon, unsigned int id)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE phones SET to_time=NOW() WHERE user=(?) AND ISNULL(to_time)");
	pstmt->setInt(1, id);

	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	pstmt = sqlCon->prepareStatement(
			"UPDATE users SET del_time=NOW() WHERE id=(?) AND ISNULL(del_time)");
	pstmt->setInt(1, id);

	int updCnt;
	try {
		updCnt = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return updCnt!=1;
}

unsigned int User::checkUserExist(Connection *sqlCon, char *email, bool &deleted)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT id, !ISNULL(del_time) AS deleted FROM users WHERE email=(?) AND ISNULL(del_time)");
	pstmt->setString(1, email);

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	if (res->rowsCount() == 0) {
		deleted = false;
		return 0;
	}
	res->first();
	unsigned int id = res->getInt("id");
	deleted = res->getInt("deleted");
	delete res;
	return id;
}

User *User::readUser(Connection *sqlCon, unsigned int id)
{
	if (!sqlCon)
		return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT isTranslator FROM users WHERE id=(?) AND !ISNULL(email_ack_time) AND ISNULL(del_time)");
	pstmt->setInt(1, id);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	if (res->rowsCount() < 1) {
		delete res;
		return 0;
	}
	res->first();
	bool isTranslator = res->getBoolean("isTranslator");
	delete res;

	User *u;
	if (isTranslator)
		u = new Translator(sqlCon, id);
	else
		u = new Client(sqlCon, id);
	if (u->DBRead()) {
		log(LOG_ERROR, "Unable to read user (id=%d).", id);
		delete u;
		return 0;
	}
	return u;
}

int User::updateUserEmailAck(Connection *sqlCon, unsigned int id)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE users SET email_ack_time=NOW() WHERE id=(?) AND ISNULL(email_ack_time) AND ISNULL(del_time)");
	pstmt->setInt(1, id);
	int updCnt;
	try {
		updCnt = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return updCnt>1;
}

int User::storePassword(Connection *sqlCon, unsigned int id, char *password)
{
	if (!sqlCon || !id || !password)
		return -1;
	unsigned char salt[DIGEST_LEN];
	genNonce(salt, DIGEST_LEN);
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int len = computeDigest(salt, DIGEST_LEN, password, strlen(password), hash);
	if (len < 0)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO passwords SET user=(?), hash=(?), salt=(?), date=NOW() ON DUPLICATE KEY UPDATE hash=(?), salt=(?), date=NOW()");
	pstmt->setInt(1, id);
	pstmt->setString(2, NonceToString(hash, len).c_str());
	pstmt->setString(3, NonceToString(salt, DIGEST_LEN).c_str());
	pstmt->setString(4, NonceToString(hash, len).c_str());
	pstmt->setString(5, NonceToString(salt, DIGEST_LEN).c_str());
	try {
		pstmt->execute();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}

unsigned int User::storeUser(Connection *sqlCon, char *email, bool isTranslator)
{
	if (!sqlCon || !email)
		return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO users SET email=(?), isTranslator=(?), reg_time=NOW()");
	pstmt->setString(1, email);
	pstmt->setBoolean(2, isTranslator);
	try {
		pstmt->execute();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;

	pstmt = sqlCon->prepareStatement(
			"SELECT id FROM users WHERE email=(?) AND ISNULL(del_time)");
	pstmt->setString(1, email);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return 0;
	}
	res->first();
	unsigned int user_id = res->getInt("id");
	delete res;
	return user_id;
}

bool User::checkPassword(Connection *sqlCon, char *email, char *password, unsigned int &id)
{
	id = 0;
	if (!email || !password || !sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT id, salt FROM passwords JOIN users ON passwords.user=users.id WHERE email=(?) AND ISNULL(del_time) \
			AND date=(SELECT MAX(date) FROM passwords WHERE user=users.id)");
	pstmt->setString(1, email);

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return false;
	}
	res->first();
	if (res->getString("salt")->length() != DIGEST_LEN*2) {
		delete res;
		return false;
	}
	id = res->getInt("id");

	unsigned char salt[DIGEST_LEN];
	StringToNonce(res->getString("salt").c_str(), salt, DIGEST_LEN);
	delete res;
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int len = computeDigest(salt, DIGEST_LEN, password, strlen(password), hash);
	if (len < 0)
		return false;

	pstmt = sqlCon->prepareStatement(
			"SELECT user FROM passwords WHERE user=(?) AND hash=(?)");
	pstmt->setInt(1, id);
	pstmt->setString(2, NonceToString(hash, len));

	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	if (res->rowsCount() == 1) {
		res->first();
		id = res->getInt(1);
		delete res;
		return true;
	}
	delete res;
	return false;
}

bool User::checkUserEmailAck(Connection *sqlCon, unsigned int id)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT id FROM users WHERE id=(?) AND !ISNULL(email_ack_time) AND ISNULL(del_time)");
	pstmt->setInt(1, id);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	bool confirmed = false;
	if (res->rowsCount() > 0)
		confirmed = true;
	delete res;
	return confirmed;
}

int User::genResetPasswordRequestUID(Connection *sqlCon, char *request_uid)
{
	if (!sqlCon || !request_uid)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT user FROM reset_req WHERE uid=(?)");
	while (true) {
		genUID(request_uid, MAX_RESET_PASS_REQEST_UID);

		pstmt->setString(1, request_uid);

		ResultSet *res;
		try {
			res = pstmt->executeQuery();
		} catch (SQLException &ex) {
			log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
			delete pstmt;
			return -1;
		}
		if (res->rowsCount() == 0) {
			delete res;
			break;
		}
		delete res;
	}
	delete pstmt;
	return 0;
}

int User::storeResetPasswordRequest(Connection *sqlCon, unsigned int id, char *request_uid)
{
	if (!sqlCon || !id || !request_uid)
		return false;
	if (genResetPasswordRequestUID(sqlCon, request_uid)) {
		log(LOG_ERROR, "Unable to generate request_uid (id=%d).", id);
		return -1;
	}
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO reset_req SET user=(?), request=NOW(), uid=(?) ON DUPLICATE KEY \
			UPDATE request=NOW(), CONFIRMED=NULL, uid=(?)");
	pstmt->setInt(1, id);
	pstmt->setString(2, request_uid);
	pstmt->setString(3, request_uid);

	try {
		pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
/*	if (res->rowsCount() != 1) {
		delete res;
		return false;
	}
	*/
	return 0;
}

unsigned int User::getConfirmResetPasswordID(Connection *sqlCon, char *request_uid)
{
	if (!sqlCon || !request_uid)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT user FROM reset_req WHERE uid=(?) AND !ISNULL(request)");
	pstmt->setString(1, request_uid);

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return false;
	}
	res->first();
	unsigned int user = res->getInt("user");
	if (user <= 0)
		return -1;

	pstmt = sqlCon->prepareStatement(
				"UPDATE reset_req SET confirmed=NOW() WHERE uid=(?)");
	pstmt->setString(1, request_uid);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	return user;
}

int User::CalcAveragePrice(Connection *sqlCon, string lang)
{
	if (!sqlCon)
		return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT AVG(price) FROM calls WHERE lang = (?) && accounted > (?)");
	pstmt->setString(1, lang);
	pstmt->setInt(2, SessionManager::getOptions().Stat_MinCallDuration);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return 0;
	}
	res->first();
	unsigned int price = res->getInt(1);
	delete res;
	return price;
}

bool User::CheckNeedMark(Connection *sqlCon, unsigned int client, unsigned int translator)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT COUNT(id) FROM calls WHERE client=(?) AND translator=(?) AND !ISNULL(start_time) AND \
			start_time = (SELECT MAX(start_time) FROM calls WHERE client=(?) && translator=(?)) AND accounted>(?)");
	pstmt->setInt(1, client);
	pstmt->setInt(2, translator);
	pstmt->setInt(3, client);
	pstmt->setInt(4, translator);
	pstmt->setInt(5, SessionManager::getOptions().Call_MinTimeRating);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return false;
	}
	res->first();
	unsigned int call_num = res->getInt(1);
	delete res;
	if (call_num == 0)
		return false;
	return true;
/*
	pstmt = sqlCon->prepareStatement(
			"SELECT COUNT(mark) FROM marks WHERE client=(?) AND translator=(?)");
	pstmt->setInt(1, client);
	pstmt->setInt(2, translator);
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return false;
	}
	res->first();
	unsigned int mark_num = res->getInt(1);
	delete res;
	if (mark_num == 0)
		return true;
	return false;
	*/
}

unsigned int User::readAccountID(Connection *sqlCon, const char *EmailServer)
{
	if (!sqlCon)
		return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT id FROM users WHERE email=(?)");
	pstmt->setString(1, EmailServer);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return 0;
	}
	res->first();
	unsigned int id = res->getInt("id");
	delete res;
	return id;
}

void User::FinalizeTransfer(Connection *sqlCon)
{
	if (!sqlCon)
		return;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE transfers t1 INNER JOIN (SELECT MAX(date) AS d FROM transfers WHERE user=(?)) t2 ON t1.date=t2.d SET saved=true");
	pstmt->setInt(1, getID());

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return;
	}
	delete pstmt;
}

User::User(Connection *_sqlCon, unsigned int _id)
: sqlCon(_sqlCon),
  id(_id),
  balance(0),
  await(false), confirmed(false), rejected(false), error(false)
{
	email[0] = 0;
	name[0] = 0;
	phone[0] = 0;
	await_phone[0] = 0;
	pthread_mutex_init(&mutex, 0);
}

bool Client::CheckMark(Connection *sqlCon, unsigned int translator)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT id FROM calls WHERE client=(?) AND translator=(?) AND cost>0");
	pstmt->setInt(1, getID());
	pstmt->setInt(2, translator);
//	pstmt->setInt(3, SessionManager::getOptions().Call_MinTimeRating);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	int cnt = res->rowsCount();
	delete res;
	return cnt > 0;
}

void Client::AddMark(Connection *sqlCon, unsigned int translator, int rating)
{
	if (!sqlCon)
		return;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO marks SET translator=(?), client=(?), mark=(?) ON DUPLICATE KEY UPDATE mark=(?)");
	pstmt->setInt(1, translator);
	pstmt->setInt(2, getID());
	pstmt->setInt(3, rating);
	pstmt->setInt(4, rating);
	int updCnt;
	try {
		updCnt = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return;
	}
	delete pstmt;
}

void Client::onPurchaseVerified(Connection *sqlCon, SessionManager *sManager, CallManager *cManager, int money)
{
//	int fee = money * ((float)SessionManager::getOptions().FeeApp)/100;		// FeeApp==0, not used
	User::onPurchaseVerified(sqlCon, cManager, money);
}

int Client::StoreTransfer(Connection *sqlCon, int result_code, int money)
{
	if (money < 0) {
		log(LOG_FATAL, "StoreTransfer: Error: Wrong sign value %d for client %d.", money, getID());
		return -1;
	}
	return User::StoreTransfer(sqlCon, result_code, money);
}

// must be locked, CallManager must be locked
int Client::changeBalance(CallManager *cManager, long BalanceChange)
{
	User::changeBalance(cManager, BalanceChange);

	vector<unsigned int> calls = cManager->findCalls(getID());
	for (int i=0; i<calls.size(); i++) {
		Call *call = cManager->getCall(calls[i]);
		if (!call)
			continue;
		if (call->getClient() == getID())
			call->setBalance(balance);
	}
	return 0;
}

// must be locked
int	Client::DBRead()
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT isTranslator, email, name, balance FROM users WHERE id=(?) AND !ISNULL(email_ack_time) AND ISNULL(del_time)");
	pstmt->setInt(1, id);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return -1;
	}
	res->first();

	bool isTranslator = res->getBoolean("isTranslator");
	if (isTranslator)
		return -1;
	const char *str = res->getString("email").c_str();
	strncpy(email, str, MAX_EMAIL+1);
	str = res->getString("name").c_str();
	strncpy(name, str, MAX_NAME+1);
	balance = res->getInt("balance");
	delete res;

	if (readLangs())
		return -1;
	if (readPhone())
		return -1;
	return 0;
}

// must be locked
int Client::DBWrite()
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE users SET name=(?), balance=(?) WHERE id=(?)");
	pstmt->setString(1, name);
	pstmt->setInt(2, getBalance());
	pstmt->setInt(3, id);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	writeLangs();
}


Client::Client(Connection *_sqlCon, unsigned int _id)
:User(_sqlCon, _id)
{

}

// must be locked
long Translator::getClientPrice(User *u, string l)
{
	string cl = getCommonLang(u);
	if (getLangs().getPrice(cl) > getLangs().getPrice(l))
		return getLangs().getPrice(cl);
	else
		return getLangs().getPrice(l);
	return 100;
}

float Translator::getFeePercent()
{
	if (LoginLength > SessionManager::getOptions().FeeApp_TimeReduced)
		return ((float)SessionManager::getOptions().FeeApp_Reduced)/100;
	else
		return ((float)SessionManager::getOptions().FeeApp)/100;
}

// returns common lang with minimum price
string Translator::getCommonLang(User *u)
{
	Langs lt = getLangs();
	Langs lc = u->getLangs();
	string lang = "";
	for (size_t i=0; i<lc.num(); i++) {
		long price = 1000000;
		string l = lc.get(i);
		if (lt.get(l) >= 0)
			if (lt.getPrice(l) < price) {
				lang = l;
				price = lt.getPrice(lang);
			}
	}
	return lang;
}

bool Translator::setBusy(bool _busy)
{
	if (busy != _busy) {
		busy = _busy;
		if (busy)
			log(LOG_VERBOSE, "Setting translator busy (id=%d).", getID());
		else
			log(LOG_VERBOSE, "Setting translator non-busy (id=%d).", getID());
		return true;
	}
	return false;
}

float Translator::CountRating(int &rating_num)
{
	if (!sqlCon)
		return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT AVG(mark), COUNT(client) FROM marks WHERE translator=(?) GROUP BY translator");
	pstmt->setInt(1, getID());
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	res->first();
	float rating = res->getDouble(1);
	rating_num = res->getInt(2);
	delete pstmt;
	return rating;
}

// must be locked
void Translator::UpdateRating()
{
	if (!sqlCon)
		return;
	rating = CountRating(rating_num);
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE users SET rating=(?), rating_num=(?) WHERE id=(?)");
	pstmt->setDouble(1, rating);
	pstmt->setInt(2, rating_num);
	pstmt->setInt(3, getID());
	int updCnt;
	try {
		updCnt = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return;
	}
	delete pstmt;
}

int Translator::getTranslatorStatistic(Connection *sqlCon, TranslatorStatistic &tstat)
{
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT SUM(money) FROM billing WHERE user=(?)");
	pstmt->setInt(1, getID());
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() == 0) {
		tstat.money_sum = 0;
	} else {
		res->first();
		tstat.money_sum = -res->getInt(1);
	}
	delete res;

	pstmt = sqlCon->prepareStatement(
			"SELECT COUNT(id), SUM(accounted) FROM calls WHERE translator=(?)");
	pstmt->setInt(1, getID());
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() == 0) {
		tstat.call_num = 0;
		tstat.call_time_sum = 0;
	} else {
		res->first();
		tstat.call_num = res->getInt(1);
		tstat.call_time_sum = res->getInt(2);
	}
	delete res;

	pstmt = sqlCon->prepareStatement(
			"SELECT COUNT(id) FROM users WHERE rating>0");
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() == 0) {
		tstat.all_rating_num = 0;
	} else {
		res->first();
		tstat.all_rating_num = res->getInt(1);
	}
	delete res;

	pstmt = sqlCon->prepareStatement(
			"SELECT COUNT(id) FROM users WHERE rating>(SELECT rating FROM users WHERE id=(?))");
	pstmt->setInt(1, getID());
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() == 0) {
		tstat.higher_rating_num = 0;
	} else {
		res->first();
		tstat.higher_rating_num = res->getInt(1);
	}
	delete res;
	return 0;
}

int Translator::setPayPalEmail(char *_PayPalEmail)
{
	if (!_PayPalEmail)
		return -1;
	strncpy(PayPalEmail, _PayPalEmail, MAX_EMAIL+1);
	return 0;
}

unsigned int Translator::checkUserExistPayPal(Connection *sqlCon, const char *paypal_email, bool &deleted)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT id, !ISNULL(del_time) AS deleted FROM users WHERE paypal_email=(?) AND ISNULL(del_time)");
	pstmt->setString(1, paypal_email);

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return false;
	}
	delete pstmt;
	if (res->rowsCount() == 0) {
		deleted = false;
		return 0;
	}
	res->first();
	unsigned int id = res->getInt("id");
	deleted = res->getInt("deleted");
	delete res;
	return id;
}

void Translator::onPurchaseVerified(Connection *sqlCon, SessionManager *sManager, CallManager *cManager, int money)
{
	int fee = money * ((float)SessionManager::getOptions().FeeApp)/100;		// FeeApp==0, not used
	User::onPurchaseVerified(sqlCon, cManager, money);

	if (fee > 0) {
		Translator *ServerAccount = sManager->getServerAccount();
		if (!ServerAccount) {
			log(LOG_FATAL, "Error Billing: Server Account for fee is not initialized.(user=%d, fee=%d)", getID(), fee);
			return;
		} else {
			ServerAccount->lock();
			ServerAccount->changeBalance(sManager->getCallManager(), money);
			ServerAccount->DBWrite();
			ServerAccount->unlock();
		}
	}
}

int Translator::StoreTransfer(Connection *sqlCon, int result_code, int money)
{
	if (money > 0) {
		log(LOG_FATAL, "StoreTransfer: Error: Wrong sign value %d for translator %d.", money, getID());
		return -1;
	}
	return User::StoreTransfer(sqlCon, result_code, money);
}

// must be locked, CallManager must be locked
int Translator::changeBalance(CallManager *cManager, long BalanceChange)
{
	User::changeBalance(cManager, BalanceChange);
	return 0;
}

// must be locked
int	Translator::DBRead()
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT isTranslator, email, name, balance, rating, rating_num, paypal_email FROM users WHERE id=(?) AND !ISNULL(email_ack_time) AND ISNULL(del_time)");
	pstmt->setInt(1, id);
	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	if (res->rowsCount() != 1) {
		delete res;
		return -1;
	}
	res->first();

	bool isTranslator = res->getBoolean("isTranslator");
	if (!isTranslator) {
		delete res;
		return -1;
	}
	const char *str = res->getString("email").c_str();
	strncpy(email, str, MAX_EMAIL+1);
	str = res->getString("name").c_str();
	strncpy(name, str, MAX_NAME+1);
	balance = res->getInt("balance");
	rating = res->getDouble("rating");
	rating_num = res->getInt("rating_num");
	str = res->getString("paypal_email").c_str();
	strncpy(PayPalEmail, str, MAX_EMAIL+1);
	delete res;

	if (readLangs())
		return -1;
	if (readPhone())
		return -1;
	LoginLength = getUserLoginsLength(sqlCon);
	return 0;
}

// must be locked
int Translator::DBWrite()
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE users SET name=(?), balance=(?), rating=(?), rating_num=(?), paypal_email=(?) WHERE id=(?)");
	pstmt->setString(1, name);
	pstmt->setInt(2, getBalance());
	pstmt->setDouble(3, rating);
	pstmt->setInt(4, rating_num);
	pstmt->setString(5, PayPalEmail);
	pstmt->setInt(6, id);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	writeLangs();
}

Translator::Translator(Connection *_sqlCon, unsigned int _id)
: Client(_sqlCon, _id)
{
	rating = 0;
	rating_num = 0;
	PayPalTransferActive = false;
	busy = false;
	TListDel = false;
}

bool User::checkPhone(char *phone)
{
	if (!phone)
		return false;
	if (strlen(phone)<MIN_PHONE || strlen(phone) > MAX_PHONE)
		return false;
	if (phone[0] != '+')
		return false;
	for (int i=1; i<strlen(phone); i++)
		if (phone[i] < '0' || phone[i] > '9')
			return false;
	return true;
}

int User::formatPhone(char *phone) {
	if (!phone)
		return -1;
	for (int i=0; i<strlen(phone); i++) {
		if ((phone[i] < '0' || phone[i] > '9') && !(phone[i] == '+' && i == 0)) {
			strcpy(phone + i, phone + i + 1);
			i--; 
		}
	}
}

bool User::equalPhone(char *phone1, char *phone2)
{
	if (!phone1 || !phone2)
		return false;
	if (!checkPhone(phone1) || !checkPhone(phone2))
		return false;
	return !strcmp(phone1 + strlen(phone1) - MIN_PHONE, phone2 + strlen(phone2) - MIN_PHONE);
}

void User::onPurchaseVerified(Connection *sqlCon, CallManager *cManager, int money)
{
	if (changeBalance(cManager, money)) {
		log(LOG_FATAL, "Error Billing: unable to update balance (user=%d).", getID());
		return;
	}
	if (DBWrite()) {
		log(LOG_FATAL, "Error Billing: unable to store store new User balance (user=%d).", getID());
		return;
	}
}

int User::StoreTransfer(Connection *sqlCon, int result_code, int money)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO transfers SET user=(?), money=(?), date=NOW(), code=(?), os=(?)");
	pstmt->setInt(1, getID());
	pstmt->setInt(2, money);
	pstmt->setInt(3, result_code);
	pstmt->setString(4, getClientOS());

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	return 0;
}


#include <sys/time.h>
void User::genNonce(unsigned char *nonce, int len)
{
	if (!nonce)
		return;
	struct timeval t;
	gettimeofday(&t, 0);
	srandom(t.tv_sec * t.tv_usec);
	for (int i=0; i<len; i++)
		nonce[i] = random();
}

string User::NonceToString(unsigned char *nonce, int len)
{
	string str;
	char hex[2+1];
	for (int i=0; i<len; i++) {
		sprintf(hex, "%02x", nonce[i]);
		str += hex;
	}
	return str;
}

int User::StringToNonce(string str, unsigned char *nonce, int len)
{
	char hex[2+1];
	for (int i=0; i<len; i++) {
		hex[0] = str[2*i];
		hex[1] = str[2*i+1];
		hex[2] = 0;
		char *end=0;
		nonce[i] = strtol(hex, &end, 16);
		if (*end!=0 || end==hex)
			return -1;
	}
	return 0;
}

unsigned int User::computeDigest(unsigned char *nonce, int nonce_len, char *pwd, int pwd_len, unsigned char *digest)
{
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	if (!ctx)
		return -1;
//	EVP_DigestInit(&ctx, EVP_sha512());
	if (EVP_DigestInit_ex(ctx, EVP_sha512(), 0) != 1)
		return -1;
	if (EVP_DigestUpdate(ctx, nonce, nonce_len) != 1)
		return -1;
	if (EVP_DigestUpdate(ctx, pwd, pwd_len) != 1)
		return -1;
	unsigned int len;
	if (EVP_DigestFinal(ctx, digest, &len) != 1)
		return -1;
	EVP_MD_CTX_destroy(ctx);
	return len;
}

void User::genSMSCode(char *code, int len)
{
	for (int i=0; i<len; i++)
		code[i] = '0' + (random() % 10);
	code[len] = 0;
strcpy(code, "12345");
}

void User::genPassword(char *password, int len)
{
	const char *symbols = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,./;'[]\\-=`<>?:\"{}|_+~";
	for (int i=0; i<len; i++)
		password[i] = symbols[random() % strlen(symbols)];
	password[len] = 0;
strcpy(password,"pwd");
}

void User::genUID(char *uid, int len)
{
	const char *symbols = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	for (int i=0; i<len; i++)
		uid[i] = symbols[random() % strlen(symbols)];
	uid[len] = 0;
}
