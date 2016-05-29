/*
 * Call.cpp
 *
 *  Created on: 17.05.2014
 *      Author: Raschupkin Roman
 */

#include <time.h>
#include "TranslateServer.h"
#include "SessionManager.h"
#include "Call.h"

long Call::countCost()
{
	int sec = accounted;
	if (sec >= SessionManager::getOptions().Call_TimeFree)
		sec -= SessionManager::getOptions().Call_TimeFree;
	else
		sec = 0;
	float minutes = (float)sec / 60;
	return price * minutes;
}

// SessionManager must be read-locked, must be locked
long Call::account(time_t length)
{
	if (state != CONFIRMED && state != ACTIVE) {
		log(LOG_ERROR, "Error: Call::account in state %d (call=%d).", state, getID());
		return -1;
	}
	if (length > time(0) - start_time + SessionManager::getOptions().TimeoutNetLag)
		length = time(0) - start_time + SessionManager::getOptions().TimeoutNetLag;
	if (length > accounted)
		accounted = length;
	long cost = countCost();
	Session *cs = sManager->findSessionByUser(client);
	Client *c = 0;
	if (cs)
		c = dynamic_cast<Client *>(cs->getUser());
	long balance = 0;
	if (c) {
//		c->lock();
		balance = c->getBalance() - cost;
//		c->unlock();
		return balance;
	} else {
		balance = getBalance() - cost;
		return balance;
	}
}

// SessionManager must be read-locked
int Call::tarificate(Connection *sqlCon)
{
	if (price == PRICE_ERROR)
		return -1;
	if (state != ACTIVE && state != END && state != ERROR) {
//		log(LOG_ERROR, "Call::tarificate Error: wrong state %s (call=%d).", State2String(state), getID());
		return 0;
	}

	cost = countCost();

	log(LOG_ERROR, "doing Call::tarificate cost=%d state=%s (call=%d).", cost, StateToString(state), getID());

	Translator *ServerAccount = sManager->getServerAccount();
//	long fee = cost * t->getFeePercent();
	if (cost > 0) {
		User::changeBalanceDyn(sqlCon, sManager, getClient(), -cost);
		User::changeBalanceDyn(sqlCon, sManager, getTranslator(), cost);
		/*
		if (!ServerAccount) {		// done during billing purchase
			log(LOG_FATAL, "Call->tarificate: ServerAccount not initialized.");
		} else {
			ServerAccount->lock();
			ServerAccount->changeBalance(sManager->getCallManager(), cost);
			ServerAccount->DBWrite();
			ServerAccount->unlock();
		}
		*/
	}
//	accounted = 0;
	return 0;
}

unsigned int Call::getPeer(unsigned int id)
{
	if (id == getTranslator())
		return getClient();
	else if (id == getClient())
		return getTranslator();
	else
		return 0;
}

const char *Call::StateToString(call_state st)
{
	switch (st) {
	case INIT:
		return "INIT";
	case AWAIT:
		return "REQUESTED";
	case CONFIRMED:
		return "CONFIRMED";
	case ACTIVE:
		return "ACTIVE";
	case END:
		return "END";
	case ERROR:
		return "ERROR";
	}
	return "UNKOWN";
}

void Call::setState(call_state st)
{
	log(LOG_ALL, "call %d[%d->%d] state: %s->%s", getID(), getClient(), getTranslator(), StateToString(state), StateToString(st));
	state = st;
}

void Call::Start()
{
	if (price == PRICE_ERROR) {
		setState(ERROR);
		return;
	}
	if (getState() == CONFIRMED) {
		start_time = time(0);
		setState(ACTIVE);
	}
}

void Call::End()
{
	if (getState() == ACTIVE) {
		end_time = time(0);
		setState(END);
	}
}

void Call::Error()
{
	setState(ERROR);
}

void Call::setTranslatorBusy(bool busy)
{
	Session *ts = sManager->findSessionByUser(getTranslator());
	if (ts) {
		ts->lock();
		Translator *t = dynamic_cast<Translator *>(ts->getUser());
		if (t)
/*			if (t->setBusy(busy)) {
				vector<Translator> tlist;
				t->lock();
				tlist.push_back(*t);
				t->unlock();
				sManager->Lists_processUpdate(tlist, false);
			}
*/
		ts->SetTranslatorBusy(busy);
		ts->unlock();
	}
}

// CallManager must be locked, SessionManager must be read-locked
int Call::sendCallStatuses()
{
	Session *cs = sManager->findSessionByUser(getClient());
	if (cs)
		cs->sendPacket_CallStatus(getID());
	Session *ts = sManager->findSessionByUser(getTranslator());
	if (ts)
		ts->sendPacket_CallStatus(getID());
	return 0;
}

// CallManager must be locked, SessionManager must be read-locked
int Call::sendCallRequestError(int error)
{
	Session *cs = sManager->findSessionByUser(getClient());
	if (cs)
		cs->sendPacket_CallRequestError(error, getID(), false);
	Session *ts = sManager->findSessionByUser(getTranslator());
	if (ts)
		ts->sendPacket_CallRequestError(error, getID(), true);
	return 0;
}

// SessionManager must be read-locked
void Call::sendTimeouts()
{
	Session *cs = sManager->findSessionByUser(getClient());
	if (cs)
		cs->sendPacket_PhoneCallTimeout(false, getTranslator());
	Session *ts = sManager->findSessionByUser(getTranslator());
	if (ts)
		ts->sendPacket_PhoneCallTimeout(true, getClient());
}

int Call::DBread(Connection *sqlCon)
{
	if (!sqlCon || !id)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT phone, client, translator, client_country, translator_country, lang, price, start_time, accounted, cost, error FROM calls WHERE id=(?)");
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
	if (res->rowsCount() != 1)
		return -1;
	res->first();
	bool isPhone = dynamic_cast<PhoneCall *>(this) != 0;
	if (res->getInt("phone") != isPhone) {
		delete res;
		return -1;
	}
	client = res->getInt("client");
	translator = res->getInt("translator");
	translateLang = res->getString("lang").c_str();
	price = res->getInt("price");
	const char *time = res->getString("start_time").c_str();
	if (strlen(time) > 0)
		start_time = mktime(getdate(time));
	time = res->getString("end_time").c_str();
	if (strlen(time) > 0)
		end_time = mktime(getdate(time));
	accounted = res->getInt("accounted");
	cost = res->getInt("cost");
	if (res->getInt("error"))
		state = ERROR;
//	setClientCountry(res->getString("client_country").c_str());
//	setTranslatorCountry(res->getString("translator_country").c_str());
	delete res;
	return 0;
}

int Call::DBwrite(Connection *sqlCon)
{
	if (!sqlCon || !id)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE calls SET phone=(?), client=(?), translator=(?), client_country=(?), translator_country=(?), lang=(?), price=(?), start_time=(?), accounted=(?), cost=(?), error=(?) WHERE id=(?)");
	pstmt->setInt(2, client);
	pstmt->setInt(3, translator);
	pstmt->setInt(1, false);
	pstmt->setString(4, COUNTRY_UNKNOWN);
	pstmt->setString(5, COUNTRY_UNKNOWN);
	pstmt->setString(6, translateLang.c_str());
	pstmt->setInt(7, price);
	char *time = asctime(localtime(&start_time));
	if (start_time)
		pstmt->setDateTime(8, time);
	else
		pstmt->setNull(8, 0);
	pstmt->setInt(9, accounted);
	pstmt->setInt(10, cost);
	pstmt->setInt(11, getState() == ERROR);
	pstmt->setInt(12, id);

	int ret;
	try {
		ret = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	return ret == 1;
}

// SessionManager must be read-locked
Call::Call(SessionManager *_sManager, unsigned int _id, unsigned int _client, unsigned int _translator, string lang)
: sManager(_sManager),
  id(_id),
  client(_client),
  translator(_translator),
  state(INIT),
  start_time(0), end_time(0), accounted(0),
  cost(0),
  price(0),
  client_balance(0)
{
	pthread_mutex_init(&mutex, 0);
	Session *cs = sManager->findSessionByUser(_client);
	if (!cs)
		throw logic_error("Client session not found.");
	if (!dynamic_cast<Client *>(cs->getUser()) || 
		(!TRANSLATORS_CALL && dynamic_cast<Translator *>(cs->getUser())))
		throw logic_error("Client not found.");
	Client *c = dynamic_cast<Client *>(cs->getUser());
	clientName = c->getName();
	Session *ts = sManager->findSessionByUser(_translator);
	if (!ts)
		throw logic_error("Translator session not found.");
	Translator *t = dynamic_cast<Translator *>(ts->getUser());
	if (!t)
		throw logic_error("Translator not found.");
	setTranslateLang(lang);
	setClientLang(t->getCommonLang(c));
	price = t->getClientPrice(c, lang);
	translatorName = t->getName();
}

void PhoneCall::sentRequest()
{
	request_time = time(0);
	if (state == INIT)
		setState(AWAIT);
	else
		setState(ERROR);
}

void PhoneCall::receivedConfirm(bool accept)
{
	if (state == AWAIT) {
		accepted = accept;
		confirm_time = time(0);
		if (accept) {
			setState(CONFIRMED);
		} else
			setState(REJECTED);
	} else
		setState(ERROR);
}

int PhoneCall::DBread(Connection *sqlCon)
{
	if (!sqlCon || !id)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT phone, client, translator, client_country, translator_country, lang, price, start_time, accounted, cost, error, request_time, confirm_time, accepted FROM calls WHERE id=(?)");
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
	if (res->rowsCount() != 1)
		return -1;
	res->first();
	if (!res->getInt("phone")) {
		delete res;
		return -1;
	}
	client = res->getInt("client");
	translator = res->getInt("translator");
	translateLang = res->getString("lang").c_str();
	price = res->getInt("price");
	const char *time = res->getString("start_time").c_str();
	if (strlen(time) > 0)
		start_time = mktime(getdate(time));
	accounted = res->getInt("accounted");
	cost = res->getInt("cost");
	if (res->getInt("error"))
		state = ERROR;
	time = res->getString("request_time").c_str();
	if (strlen(time) > 0)
		request_time = mktime(getdate(time));
	time = res->getString("confirm_time").c_str();
	if (strlen(time) > 0)
		confirm_time = mktime(getdate(time));
	accepted = res->getBoolean("accepted");
	setClientCountry(res->getString("client_country").c_str());
	setTranslatorCountry(res->getString("translator_country").c_str());
	delete res;
	return 0;
}

int PhoneCall::DBwrite(Connection *sqlCon)
{
	if (!sqlCon || !id)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"UPDATE calls SET phone=(?), client=(?), translator=(?), client_country=(?), translator_country=(?), lang=(?), price=(?), start_time=(?), accounted=(?), cost=(?), error=(?), request_time=(?), confirm_time=(?), accepted=(?) WHERE id=(?)");
	pstmt->setInt(1, true);
	pstmt->setInt(2, client);
	pstmt->setInt(3, translator);
	pstmt->setString(4, ((PhoneCall *)this)->getClientCountry().c_str());
	pstmt->setString(5, ((PhoneCall *)this)->getTranslatorCountry().c_str());
	pstmt->setString(6, translateLang.c_str());
	pstmt->setInt(7, price);
	char time[512];
	strftime(time, 512, SQLTIME_FMT, localtime(&start_time));
	if (start_time)
		pstmt->setDateTime(8, time);
	else
		pstmt->setNull(8, 0);
	pstmt->setInt(9, getAccountedTime());
	pstmt->setInt(10, cost);
	pstmt->setInt(11, getState() == ERROR);
	strftime(time, 512, SQLTIME_FMT, localtime(&request_time));
	if (request_time)
		pstmt->setString(12, time);
	else
		pstmt->setNull(12, 0);
	strftime(time, 512, SQLTIME_FMT, localtime(&confirm_time));
	if (confirm_time)
		pstmt->setDateTime(13, time);
	else
		pstmt->setNull(13, 0);
	pstmt->setBoolean(14, accepted);
	pstmt->setInt(15, id);
	int ret;
	try {
		ret = pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	return ret == 1;
}

// save phones to use in case of loosing connection by one of peers
// SessionManager must be read-locked
int PhoneCall::savePhones()
{
	Session *cs = sManager->findSessionByUser(getClient());
	if (!cs || !cs->getUser())
		return -1;
	Client *c = dynamic_cast<Client *>(cs->getUser());
	if (c &&
		!(!TRANSLATORS_CALL && dynamic_cast<Translator *>(cs->getUser()))) {
		c->lock();
		strncpy(client_phone, c->getPhone(), MAX_PHONE+1);
		c->unlock();
	}
	Session *ts = sManager->findSessionByUser(getTranslator());
	if (!ts || !ts->getUser())
		return -1;
	Translator *t = dynamic_cast<Translator *>(ts->getUser());
	if (t) {
		t->lock();
		strncpy(translator_phone, t->getPhone(), MAX_PHONE+1);
		t->unlock();
	}
	return 0;
}

PhoneCall::PhoneCall(SessionManager *_sManager, unsigned int _id, unsigned int _client, unsigned int _translator, string lang)
: Call(_sManager, _id, _client, _translator, lang),
  accepted(false),
  request_time(0), confirm_time(0)
{
	client_phone[0] = 0;
	translator_phone[0] = 0;
	Session *cs = sManager->findSessionByUser(client);
	if (!cs)
		throw std::logic_error("Client session not found.");
	Client *c = dynamic_cast<Client *>(cs->getUser());
	if (!c ||
		(!TRANSLATORS_CALL && dynamic_cast<Translator *>(cs->getUser())))
		throw std::logic_error("Client not found.");
	setClientCountry(c->getCountry());
	Session *ts = sManager->findSessionByUser(translator);
	if (!ts)
		throw std::logic_error("Translator session not found.");
	Translator *t = dynamic_cast<Translator *>(ts->getUser());
	if (!t)
		throw std::logic_error("Translator not found.");
	setTranslatorCountry(t->getCountry());
}

// must be locked
Call *CallManager::getCall(unsigned int call_id)
{
	if (!call_id)
		return 0;
	for (size_t i=0; i<calls.size(); i++)
		if (calls[i]->getID() == call_id) {
			return calls[i];
		}
	return 0;
}

// must be locked
unsigned int CallManager::findCall(unsigned int c, unsigned int t)
{
	for (size_t i=0; i<calls.size(); i++) {
		if (calls[i]->getClient() != c)
			continue;
		if (calls[i]->getTranslator() != t)
			continue;
		return calls[i]->getID();
	}
	return 0;
}

// must be locked
vector <unsigned int> CallManager::findCalls(unsigned int u)
{
	vector <unsigned int> callv;
	for (size_t i=0; i<calls.size(); i++) {
		if (calls[i]->getClient() == u || calls[i]->getTranslator() == u)
			callv.push_back(calls[i]->getID());
	}
	return callv;
}

// must be locked
unsigned int CallManager::findActiveConfirmedCall(unsigned int u)
{
	for (size_t i=0; i<calls.size(); i++) {
		if (calls[i]->getClient() == u || calls[i]->getTranslator() == u)
			if (calls[i]->getState() == ACTIVE || calls[i]->getState() == CONFIRMED)
				return calls[i]->getID();
	}
	return 0;
}

// must be locked
unsigned int CallManager::findCall(unsigned int u, char *peer_phone)
{
	if (!peer_phone || !u)
		return 0;
	for (size_t i=0; i<calls.size(); i++) {
		if (calls[i]->getClient() == u || calls[i]->getTranslator() == u) {
			Session *s = sManager->findSessionByUser(calls[i]->getPeer(u));
			if (s) {
				User *u = s->getUser();
				if (u)
					if (User::equalPhone(u->getPhone(), peer_phone))
						return calls[i]->getID();
			}
		}
	}
	return 0;
}

// must be locked
void CallManager::removeCall(unsigned int call_id)
{
	for (vector<Call *>::iterator it=calls.begin(); it<calls.end(); it++)
		if ((*it)->getID() == call_id)
			calls.erase(it);
}

// must be write-locked, SessionManager must be read-locked
void CallManager::EndCall(Connection *sqlCon, unsigned int call_id)
{
	Call *call = getCall(call_id);
	if (!call)
		return;
	call->lock();

	//call->sendCallStatuses();

	if (call->tarificate(sqlCon)) {
		log(LOG_FATAL, "Error tarficating call[%d].", call_id);
	}
	call->DBwrite(sqlCon);

	if (call->getState() == ACTIVE || call->getState() == END) {
		if (User::CheckNeedMark(sqlCon, call->getClient(), call->getTranslator())) {
			Session *cs = sManager->findSessionByUser(call->getClient());
			if (cs) {
				struct timeval tv;
				gettimeofday(&tv, 0);
				unsigned long time = tv.tv_sec*1000 + tv.tv_usec/1000;
				cs->sendPacket_MarkRequest(call->getTranslator(), call->getTranslatorName(), time);
			}
		}
	}
	call->unlock();
	log(LOG_VERBOSE, "EndCall[%d->%d]: accounted=%d cost=%d.", call->getClient(), call->getTranslator(),
			call->getAccountedTime(), call->getCost());
	removeCall(call_id);
	delete call;
}

unsigned int CallManager::storeCall(Connection *sqlCon, bool phone, unsigned int client, unsigned int translator)
{
	if (!sqlCon)
		return 0;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO calls SET phone=(?), client=(?), translator=(?), request_time=NOW()");
	pstmt->setInt(1, phone);
	pstmt->setInt(2, client);
	pstmt->setInt(3, translator);

	try {
		pstmt->execute();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;

	pstmt = sqlCon->prepareStatement("SELECT LAST_INSERT_ID()");

	ResultSet *res;
	try {
		res = pstmt->executeQuery();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return 0;
	}
	delete pstmt;
	res->first();
	unsigned int id = res->getInt(1);
	delete res;
	return id;
}

// CallManager must be locked
unsigned int CallManager::newPhoneCall(Connection *sqlCon, unsigned int client, unsigned int translator, string lang)
{
	if (!sqlCon || !Langs::isLang(lang))
		return 0;
	unsigned int id = storeCall(sqlCon, true, client, translator);
	if (!id)
		return 0;
	try {
		PhoneCall *call = new PhoneCall(sManager, id, client, translator, lang);
		if (!call)
			return 0;
		calls.push_back(call);
	} catch (exception ex) {
		return 0;
	}
	return id;
}

// remove timeouted and unconfirmed calls
// must be locked;
void CallManager::MaintainCalls(Connection *sqlCon)
{
	for (size_t i=0; i<calls.size(); i++) {
		Call *call = calls[i];
		if (PhoneCall *pcall = dynamic_cast<PhoneCall *>(call))
			if ((call->getState() == ACTIVE && call->getStartTime() + SessionManager::getOptions().Timeout_CallStatus < time(0)) ||
					(pcall->getState() == AWAIT && pcall->getRequestTime() + SessionManager::getOptions().Timeout_CallRequest < time(0)) ||
					(pcall->getState() == CONFIRMED && pcall->getConfirmTime() + SessionManager::getOptions().Timeout_CallConfirm < time(0))) {
				log(LOG_VERBOSE, "MaintainCalls: deleting call %d[%d->%d] with timeouted confirmation.", call->getID(), call->getClient(), call->getTranslator());
				call->sendTimeouts();
				EndCall(sqlCon, call->getID());
			}
	}
}

int CallManager::currentCallNum()
{
	int num = 0;
	for (int i=0; i<calls.size(); i++) {
		Call *call = calls[i];
		if (call->getState() == ACTIVE)
			num++;
	}
	return num;
}

// must be locked
void CallManager::updateCallState(vector<Translator> &tlist, int client)
{
	vector<unsigned int>calls = findCalls(client);
	for (int i=0; i<tlist.size(); i++) {
		tlist[i].setAwait(false); tlist[i].setConfirmed(false);	tlist[i].setRejected(false);
		for (int j=0; j<calls.size(); j++) {
			PhoneCall *c = dynamic_cast<PhoneCall *>(getCall(calls[j]));
			if (!c)
				continue;
			if (c->getTranslator() == tlist[i].getID()) {
				switch (c->getState()) {
				case INIT:
					break;
				case AWAIT:
					tlist[i].setAwait(true);
					break;
				case CONFIRMED:
					tlist[i].setConfirmed(true);
					break;
				case REJECTED:
					tlist[i].setRejected(true);
					break;
				case ACTIVE:
					break;
				case ERROR:
					tlist[i].setError(true);
					break;
				}
				break;
			}
		}
	}
}

// must be locked
vector<Call> CallManager::getClientList(unsigned int translator)
{
	vector<Call> calllist;
	for (int i=0; i<calls.size(); i++) {
		Call *call = calls[i];
		PhoneCall *pcall = dynamic_cast<PhoneCall *>(call);
		if (pcall) {
			if (pcall->getTranslator() != translator)
				continue;

			pcall->lock();
			Call callc = *pcall;
			pcall->unlock();
			callc.reset_lock();
			calllist.push_back(callc);
		}
	}
	return calllist;
}

CallManager::CallManager(SessionManager *_sManager)
: sManager(_sManager)
{
	pthread_rwlock_init(&rwlock, 0);
}
