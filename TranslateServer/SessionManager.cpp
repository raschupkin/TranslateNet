/*
 * SessionManager.cpp
 *
 *  Created on: 27.04.2014
 *      Author: Raschupkin Roman
 */
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <poll.h>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "TranslateServer.h"
#include "SessionManager.h"
#include "Call.h"
#include "User.h"

Options SessionManager::options;
std::map<pthread_t, int> SessionManager::PThreadMap = std::map<pthread_t,int>();

void SessionManager::Process(int thread)
{
	PThreadMap[pthread_self()] = thread;
	switch (thread) {
	case THREAD_SERVICE:
		return ProcessService();
	case THREAD_RESET_PASS:
		return ProcessResetPass();
	case THREAD_EMAIL:
		return ProcessEmail();
	case THREAD_SMS:
		return ProcessServiceHTTP(&serviceTwilioSMS, getSMSCon());
	case THREAD_IOS_VERIFY:
		return ProcessServiceHTTP(&serviceIOSVerify, getIOSVerifyManagerCon());
	case THREAD_PAYPAL:
		return ProcessServiceHTTP(&servicePayPal, getPayPalCon());
	}
	int threadN = thread - THREAD_SERVICE_NUM;
	while (1) {
		DeleteMarkedSessions(getCon(threadN));

		rdlock();
		int k = 0;
		for (size_t i=0; i<=sessions.size(); i++) {
			int s = 0;
			if (i < sessions.size()) {
				if (sessions[i]->getThread() == threadN)
					s = sessions[i]->getSocket();
			} else
				s = sockpipe[threadN][0];
			if (s) {
				sockets[threadN][k].fd = s;
				sockets[threadN][k].events = POLLIN;// | POLLOUT;
				sockets[threadN][k].revents = 0;
				k++;
			}
		}
		unlock();
		poll(sockets[threadN], k, 100);
		rdlock();
		for (int i=0; i<k; i++) {
			for (size_t j=0; j<sessions.size(); j++) {
				if (sessions[j]->getSocket() == sockets[threadN][i].fd) {
					if (sockets[threadN][i].revents & POLLIN || !sessions[j]->isPacketQueueEmpty()) {
						sessions[j]->lock();
						bool del = false;
/*						if (sessions[j]->getMarkEnd() || sessions[j]->getMarkAbort()) {
							sessions[j]->unlock();
							continue;
						}
						*/
						if (sockets[threadN][i].revents & POLLIN)			// if socket has received data
							if (!sessions[j]->getMarkEnd() && !sessions[j]->getMarkAbort())
								del = sessions[j]->recvPacket() < 0;
						del |= sessions[j]->WritePacketQueue();
						if (del) {
							if (sessions[j]->getUser())
								log(LOG_VERBOSE, "Session disconnected: marking for deletion. (id=%d)", sessions[j]->getUser()->getID());
							else
								log(LOG_VERBOSE, "Session disconnected: marking for deletion.");
							sessions[j]->setMarkEnd();
						}
						sessions[j]->unlock();
						break;
					}
				}
				for (int j=0; j<ThreadNum; j++)
					if (sockpipe[threadN][0] == sockets[threadN][i].fd) {
						char b;
						while (read(sockpipe[threadN][0], &b, 1) == 1) {}
					}
			}
		}
		unlock();
		if (isMaintenanceExit() && isMaintenanceExitReady()) {
			wrlock();
			log(LOG_ALL, "Maintenance exit: thread %d ending sessions.", threadN);
			doMaintenanceExit(thread, k);
			unlock();
			return;
		}
	}
}

void SessionManager::ProcessService()
{
	struct timespec req, rem;
	req.tv_sec = 0;
	req.tv_nsec = 100000000;
	while (1) {
		if (time(0) > timer_sessions + TIMER_SESSION_CHECK) {
			wrlock();
			MaintainSessions(getServiceCon());
			unlock();
			timer_sessions = time(0);
		}

		if (time(0) > timer_calls + TIMER_CALLS_CHECK) {
			cManager->wrlock();
			cManager->MaintainCalls(getServiceCon());
			cManager->unlock();
			timer_calls = time(0);
		}
		if (time(0) > timer_stat + TIMER_STAT_UPDATE) {
			rdlock();
			UpdateStatistic(getServiceCon());
			unlock();
			timer_stat = time(0);
		}
		if (time(0) > timer_db + TIMER_DB_MAINTAIN) {
			if (timer_db != 0)
				WriteStats(getServiceCon());
			MaintainDB(getServiceCon());
			if (MaintainLangsAveragePrice(getServiceCon())) {
				log(LOG_FATAL, "Error in %s file. Starting maintenance exit: waiting for active calls to finish.", options.File_LangsXML);
				startMaintenanceExit();
			}
			timer_db = time(0);
		}

		if (isMaintenanceExit() && isMaintenanceExitReady()) {
			wrlock();
			log(LOG_ALL, "Maintenance exit: SERVICE thread exiting.");
			MaintenanceExitNum++;
			if (MaintenanceExitNum == ThreadNum + THREAD_SERVICE_NUM)
				pthread_kill(main_thread, SIGKILL);
			unlock();
			return;
		}

		nanosleep(&req, &rem);
	}
}

void SessionManager::ProcessResetPass()
{
	struct timespec req, rem;
	req.tv_sec = 0;
	req.tv_nsec = 100000000;
	while (1) {
		while (unsigned int id = findConfirmedResetPassword(getResetServiceCon()))
			doResetPassword(getResetServiceCon(), id);

		nanosleep(&req, &rem);

		if (isMaintenanceExit() && isMaintenanceExitReady()) {
			wrlock();
			log(LOG_ALL, "Maintenance exit: RESET_PASS thread exiting.");
			MaintenanceExitNum++;
			if (MaintenanceExitNum == ThreadNum + THREAD_SERVICE_NUM)
				pthread_kill(main_thread, SIGKILL);
			unlock();
			return;
		}
	}
}

void SessionManager::ProcessEmail()
{
	struct timespec req, rem;
	req.tv_sec = 0;
	req.tv_nsec = 100000000;
	while (1) {
		serviceEmail.ProcessTasks();
		nanosleep(&req, &rem);

		if (isMaintenanceExit() && isMaintenanceExitReady()) {
			wrlock();
			log(LOG_ALL, "Maintenance exit: EMAIL thread exiting.");
			MaintenanceExitNum++;
			if (MaintenanceExitNum == ThreadNum + THREAD_SERVICE_NUM)
				pthread_kill(main_thread, SIGKILL);
			unlock();
			return;
		}
	}
}

void SessionManager::ProcessServiceHTTP(ServiceAsync *service, Connection *sqlCon)
{
	struct timespec req, rem;
	req.tv_sec = 0;
	req.tv_nsec = 100000000;
	service->setCon(sqlCon);
	while (1) {
		service->ProcessTasks();
		service->ProcessTimeouts();
		service->ProcessUnfinishedTasks();
		nanosleep(&req, &rem);

		if (isMaintenanceExit() && isMaintenanceExitReady()) {
			wrlock();
			log(LOG_ALL, "Maintenance exit: %s service thread exiting.", service->getName());
			MaintenanceExitNum++;
			if (MaintenanceExitNum == ThreadNum + THREAD_SERVICE_NUM)
				pthread_kill(main_thread, SIGKILL);
			unlock();
			return;
		}
	}
}

// must be write-locked
int SessionManager::MaintainSessions(Connection *sqlCon)
{
	for (size_t i=0; i<sessions.size(); i++) {
		Session *s = sessions[i];
		if (time(0) > s->getTimer() + getOptions().TimerSessionTimeout)
			s->setMarkEnd();
	}
	return 0;
}

int SessionManager::MaintainDB(Connection *sqlCon)
{
	log(LOG_ALL, "MaintainDB");
	PreparedStatement *pstmt = sqlCon->prepareStatement("DELETE FROM passwords WHERE user IN (SELECT id FROM users WHERE ISNULL(email_ack_time) AND DATEDIFF(NOW(), reg_time)>=(?))");
	pstmt->setInt(1, getOptions().TimerSessionRegTimeout);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

	pstmt = sqlCon->prepareStatement("DELETE FROM users WHERE ISNULL(email_ack_time) AND DATEDIFF(NOW(), reg_time)>=(?)");
	pstmt->setInt(1, getOptions().TimerSessionRegTimeout);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;

/*	pstmt = sqlCon->prepareStatement("DELETE FROM phones WHERE ISNULL(from_time) AND DATEDIFF(NOW(), reg_time)>=(?)");
	pstmt->setInt(1, getOptions().TimerSessionRegTimeout);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
	*/
	return 0;
}

// Langs::langs_lock must be locked
int SessionManager::MaintainLangsAveragePrice(Connection *sqlCon)
{
	log(LOG_VERBOSE, "MaintainLangsAveragePrice: Loading %s...", options.File_LangsXML);
	Langs::AllLangsLock();
	string LangsXML = Langs::loadLangsFile(options.File_LangsXML, sqlCon);
	if (LangsXML.length() == 0) {
		Langs::AllLangsUnlock();
		return -1;
	}
	Langs::AllLangsUnlock();
	return 0;
}

bool SessionManager::isMaintenanceExitReady()
{
	if (cManager->currentCallNum() > 0)
		return false;
	if (serviceEmail.TaskPending())
		return false;
	if (serviceIOSVerify.TaskPending())
		return false;
	if (serviceTwilioSMS.TaskPending())
		return false;
	return true;
}

void SessionManager::setLangsXML(string _LangsXML)
{
	Langs::AllLangsLock();
	LangsXML = _LangsXML;
	Langs::AllLangsUnlock();
}

string SessionManager::getLangsXML()
{
	Langs::AllLangsLock();
	string _LangsXML = LangsXML;
	Langs::AllLangsUnlock();
	return _LangsXML;
}

// SessionManager must be write-locked
void SessionManager::doMaintenanceExit(int thread, int socket_num)
{
	int threadN = thread - THREAD_SERVICE_NUM;
	for (size_t i=0; i<socket_num; i++) {
		for (size_t j=0; j<sessions.size(); j++) {
			Session *s = sessions[i];
			if (s->getSocket() == sockets[threadN][i].fd) {
				s->setMarkEnd();
			}
		}
	}
	MaintenanceExitNum++;
	if (MaintenanceExitNum == ThreadNum + THREAD_SERVICE_NUM)
		pthread_kill(main_thread, SIGKILL);
}

// SessionManager must be write-locked
int SessionManager::EndSession(Connection *sqlCon, Session *s, bool send_tlist, bool abort_calls)
{
	UpdateStatistic(sqlCon, true);
	if (s->getUser()) {
		cManager->wrlock();
		vector<unsigned int> calls = cManager->findCalls(s->getUser()->getID());
		for (size_t i=0; i<calls.size(); i++) {
			Call *call = cManager->getCall(calls[i]);
			if (!call)
				continue;
			unsigned int peer_id = call->getPeer(s->getUser()->getID());
			Session *ps = findSessionByUser(peer_id);
			if (ps && call->getState() != ACTIVE && call->getState() != CONFIRMED && call->getState() != AWAIT/* && call->getClient() == s->getUser()->getID()*/) {
				log(LOG_VERBOSE, "Ended inactive call %d[%d->%d] of disconnected user. (peer %d). (id=%d)",
							calls[i], call->getClient(), call->getTranslator(), peer_id, s->getUser()->getID());
				ps->sendPacket_Error(ERROR_PEER_DISCON, s->getUser()->getName(), s->getUser()->getID());
				call->End();
				cManager->EndCall(sqlCon, calls[i]);
				continue;
			}
			if (abort_calls || !ps || (call->getState() == ACTIVE && !ps->get_DetectPeerPhone())) {
				call->Error();
				if (!ps)
					log(LOG_VERBOSE, "Ending call %d[%d->%d] of disconnected user: peer is already disconnected. (id=%d)",
							calls[i], call->getClient(), call->getTranslator(), peer_id, s->getUser()->getID());
				else {
					log(LOG_VERBOSE, "Ending call %d[%d->%d] of disconnected user: peer %d of disconnected user seems to be unable to detect peer phone number. (id=%d)",
							calls[i], call->getClient(), call->getTranslator(), peer_id, s->getUser()->getID());
					ps->sendPacket_Error(ERROR_PEER_DISCON, s->getUser()->getName(), s->getUser()->getID());
				}
				cManager->EndCall(sqlCon, calls[i]);
			} else
				log(LOG_VERBOSE, "Keeping %s call %d[%d->%d] of disconnected user: peer %d is able to detect peer phone number or state is not ACTIVE. (id=%d)",
						Call::StateToString(call->getState()), calls[i], call->getClient(), call->getTranslator(), peer_id, s->getUser()->getID());
		}
		s->getUser()->updateUserLoginTime(sqlCon, false);
		cManager->unlock();

		if (send_tlist) {
			User *user = s->getUser();
			if (user)
				s->update_TLists(true);
		}
		log(LOG_VERBOSE, "Session(id=%d, email='%s') deleted.", s->getUser()->getID(), s->getUser()->getEmail());
	} else
		log(LOG_VERBOSE, "Session deleted.");
	removeSession(s);
	s->Close();
	delete s;
}

void SessionManager::markAbortSessionByPhone(Connection *sqlCon, Session *session)
{
	if (!sqlCon || !session || !session->getUser() || !session->getUser()->getPhone())
		return;
	for (size_t i=0; i<sessions.size(); i++) {
		Session *s = sessions[i];
		if (!s)
			continue;
		User *u = s->getUser();
		if (!u)
			continue;
		if (!strcmp(u->getPhone(), session->getUser()->getPhone()) && u->getPhoneStatus() == PHONE_STATUS_CONFIRMED) {
			bool sameIP = session->getIP().sin_addr.s_addr == s->getIP().sin_addr.s_addr;
			s->setMarkAbort(true, sameIP);
		}
	}
}

// SessionManager must be read-locked, User must be locked
vector<Translator> SessionManager::getTranslatorList(User *u)
{
	vector<Translator> tlist;
	for (size_t i=0; i<sessions.size(); i++) {
		Session *s = sessions[i];
		Translator *t = dynamic_cast<Translator *>(s->getUser());
		if (!t || !strlen(t->getPhone()) || t->getCountry() == COUNTRY_UNKNOWN)
			continue;
		if (t->getLangs().get(u->getListLang()) >= 0 && t->getCommonLang(u).length()) {
			t->lock();
			Translator tc = *t;
			tc.reset_lock();
			tlist.push_back(tc);
			t->unlock();
		}
	}
	return tlist;
}

// SessionManager must be read-locked
void SessionManager::Lists_processUpdate(vector<Translator> tlist, bool del)
{
	for (size_t i=0; i<sessions.size(); i++) {
		Session *s = sessions[i];
		if (!s)
			continue;
		User *u = s->getUser();
		if (!u)
			continue;
		vector<Translator> reslist;
		u->lock();
		for (int j=0; j<tlist.size(); j++) {
			Translator t = tlist[j];
//			Client *c = dynamic_cast<Client *>(u);
			string listlang = u->getListLang();
			if (listlang.size() &&
					t.getLangs().get(listlang) >= 0 &&
					t.getCommonLang(u).size()) {
				bool send_del = del;
				if (!t.getPhone() || !strlen(t.getPhone()) || t.getPhoneStatus() != PHONE_STATUS_CONFIRMED)
					send_del = true;
				t.setTListDel(send_del);

				reslist.push_back(t);
			}
		}
		u->unlock();
		if (reslist.size() > 0) {
			cManager->rdlock();
			cManager->updateCallState(reslist, u->getID());
			cManager->unlock();
			s->sendPacket_TranslatorList(reslist, del);
		}
	}
}

int SessionManager::addSession(SSL *ssl, BIO *bio)
{
	Session *s;
	int threadN = chooseThread();
	try {
		s = new Session(this, cManager, threadN, ssl, bio);
	} catch (exception ex) {
		log(LOG_ERROR, "Unable to create session.");
		return -1;
	}
	wrlock();
	if (sessions.size() >= max_sockets) {
		unlock();
		log(LOG_ERROR, "Too many sessions.");
		return -1;
	}
	sessions.push_back(s);
	write(sockpipe[threadN][1], "0", 1);
	unlock();
	return 0;
}

// sessions must be write-locked
int SessionManager::removeSession(Session *s)
{
	size_t i;
	write(sockpipe[s->getThread()][1], "0", 1);
	for (i=0; i<sessions.size(); i++)
		if (sessions[i] == s)
			sessions.erase(sessions.begin()+i);
	return 0;
}

// sessions_lock must be read-locked
Session *SessionManager::findSessionByUser(unsigned int id)
{
	if (!id)
		return 0;
	for (vector<Session *>::iterator it=sessions.begin(); it<sessions.end(); it++)
		if ((*it)->getUser())
			if ((*it)->getUser()->getID() == id) {
				Session *s = *it;
//				if (!s->getMarkAbort() && !s->getMarkEnd())
					return	s;
			}
	return 0;
}

// sessions_lock must be read-locked
Session *SessionManager::findSessionByPayPalEmail(const char *paypal_email)
{
	if (!paypal_email)
		return 0;
	for (vector<Session *>::iterator it=sessions.begin(); it<sessions.end(); it++)
		if ((*it)->getUser()) {
			Translator *t = dynamic_cast<Translator *>((*it)->getUser());
			if (t && !strcasecmp(t->getPayPalEmail(), paypal_email))
				return	*it;
		}
	return 0;
}

void SessionManager::DeleteMarkedSessions(Connection *sqlCon)
{
	bool needDelete = false;
	rdlock();
	for (size_t i=0; i<sessions.size(); i++)
		if (sessions[i]->getMarkEnd() || sessions[i]->getMarkAbort()) {
			needDelete = true;
			break;
		}
	unlock();
	if (needDelete) {
		wrlock();
		for (size_t i=0; i<sessions.size(); i++) {
			Session *s = sessions[i];
			if (!s->isPacketQueueEmpty())
				continue;
			if (s->getMarkEnd()) {
				if (s->getUser())
					log(LOG_ALL, "Deleting disconnected session (id=%d).", s->getUser()->getID());
				else
					log(LOG_ALL, "Deleting disconnected session (id=-).");
				EndSession(sqlCon, s, true, false);
			} else if (s->getMarkAbort()) {
				if (s->getAbortSamePhone()) {
					if (s->getUser())
						log(LOG_ALL, "Aborting same phone session (id=%d).", s->getUser()->getID());
					else
						log(LOG_ALL, "Aborting same phone session (id=-).");
					if (!s->getAbortSameIP())
						s->sendPacket_LoginError(ERROR_ANOTHER_PHONE,  Langs::getLangsVersion());
				} else {
					if (s->getUser())
						log(LOG_ALL, "Aborting same user session (id=%d).", s->getUser()->getID());
					else
						log(LOG_ALL, "Aborting same user session (id=-).");
					if (!s->getAbortSameIP())
						s->sendPacket_LoginError(ERROR_ANOTHER_LOGIN,  Langs::getLangsVersion());
				}
				EndSession(sqlCon, s, false, false);
			}
		}
		unlock();
	}
}

int SessionManager::markAbortSessionByUser(Session *session, unsigned int id)
{
	if (!session || id <= 0)
		return 0;
	for (vector<Session *>::iterator it=sessions.begin(); it<sessions.end(); it++) {
		if ((*it)->getUser())
			if ((*it)->getUser()->getID() == id) {
				Session *s = *it;
				log(LOG_VERBOSE, "Marking same user session for deletion (id=%d).", id);
				write(sockpipe[s->getThread()][1], "0", 1);
				bool sameIP = session->getIP().sin_addr.s_addr == s->getIP().sin_addr.s_addr;
				s->setMarkAbort(false, sameIP);
			}
	}
	return 0;
}

int SessionManager::CountCallsStat(Connection *sqlCon, int hours)
{
	if (!sqlCon)
		return 0;
	float period = 60*60*hours;
	float k = 1;
	if (time(0) - period < ServerStartTime)
		k = (time(0) - ServerStartTime) / period;
	string query = "SELECT COUNT(id) FROM calls WHERE start_time+accounted>NOW()-INTERVAL (?) HOUR";
	PreparedStatement *pstmt = sqlCon->prepareStatement(query);
	pstmt->setInt(1, hours);

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
	int calls = res->getInt(1);
	delete res;
	if (stat.stats_calls_hour == 0)
		return calls;
	return calls*k + stat.stats_calls_hour*(1-k);
}

int SessionManager::CountUsersStat(Connection *sqlCon, int hours)
{
	if (!sqlCon)
		return 0;
	float period = 60*60*hours;
	float k = 1;
	if (time(0) - period < ServerStartTime)
		k = (time(0) - ServerStartTime) / period;
/*	string query = "SELECT COUNT(DISTINCT user) FROM logins WHERE (login_time>NOW()-INTERVAL 1 HOUR OR \
				logout_time>NOW()-INTERVAL 1 HOUR) AND \
				(SELECT isTranslator FROM users WHERE users.id = logins.user) = 0";
				*/
	string query = "SELECT COUNT(DISTINCT user) FROM logins WHERE (login>NOW()-INTERVAL (?) HOUR OR \
					logout>NOW()-INTERVAL (?) HOUR) AND \
					(SELECT isTranslator FROM users WHERE users.id = logins.user) = 0";
	PreparedStatement *pstmt = sqlCon->prepareStatement(query);
	pstmt->setInt(1, hours);
	pstmt->setInt(2, hours);

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
	int users = res->getInt(1);
	delete res;
	if (stat.stats_users_hour == 0)
		return users;
	return users*k + stat.stats_users_hour*(1-k);
}

int SessionManager::ReadLastStats(Connection *sqlCon)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT users_hour, calls_hour FROM stats WHERE time = (SELECT MAX(time) FROM stats)");
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
		stat.stats_users_hour = 0;
		stat.stats_calls_hour = 0;
	} else { 
		res->first();
		stat.stats_users_hour = res->getInt("users_hour");
		stat.stats_calls_hour = res->getInt("calls_hour");
	}
	delete res;
}

int SessionManager::WriteStats(Connection *sqlCon)
{
	if (!sqlCon)
		return -1;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"INSERT INTO stats SET time=NOW(), translators=(?), users_hour=(?), calls_hour=(?)");
	pstmt->setInt(1, stat.translators);
	pstmt->setInt(2, stat.users_hour);
	pstmt->setInt(3, stat.calls_hour);
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
		delete pstmt;
		return -1;
	}
	delete pstmt;
}

unsigned int SessionManager::findConfirmedResetPassword(Connection *sqlCon)
{
	if (!sqlCon)
		return false;
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT user FROM reset_req JOIN users ON reset_req.user=users.id WHERE !ISNULL(confirmed) AND \
			request>(SELECT MAX(date) FROM passwords WHERE user=reset_req.user) AND ISNULL(del_time) AND !ISNULL(email_ack_time)");

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
		delete res;
		return 0;
	}
	res->first();
	unsigned int id = res->getInt("user");
	delete res;
	return id;
}

void SessionManager::doResetPassword(Connection *sqlCon, unsigned int id)
{
	log(LOG_ALL, "Doing doResetPassword (id=%d).", id);
	if (id == 0 || !sqlCon)
		return;
	char password[MAX_PASSWORD+1] = {0};
	User::genPassword(password, MAX_PASSWORD);
	User *u = User::readUser(sqlCon, id);
	if (!u) {
		log(LOG_ERROR, "Error: doResetPassword: user not found (id=%d).", id);
		return;
	}
	if (sendEmail_Password(u->getEmail(), password, u->getLangs().getBaseLang())) {
		log(LOG_ERROR, "Error: doResetPassword: unable to send email with password after reset (id=%d, email='%s').", id, u->getEmail());
		delete u;
		return;
	}
	if (User::storePassword(sqlCon, id, password)) {
		log(LOG_ERROR, "Error: doResetPassword: unable to store password after reset (id=%d, email='%s').", id, u->getEmail());
		delete u;
		return;
	}
	log(LOG_VERBOSE, "Done doResetPassword for user '%s'(id=%d).", u->getEmail(), id);
	delete u;
}

int SessionManager::sendEmail_Password(char *email, char *password, string lang)
{
	string topic = "Translate-Net account password";
	string message = "Translate-Net<br>Account: ";
	message += email;
	message += "<br>";
	message += "Password for your account: ";
	message += password;
	message += "Account will can be activated during ";
	message += SessionManager::getOptions().TimerSessionRegTimeout;
	message += " days after creation.";
	return sendEmail(EMAIL_TYPE_REG_PASSWORD, email, topic, message);
}

int SessionManager::sendEmail_ResetPassword(char *email, char *reset_uid, string lang)
{
	string topic = "Translate-Net account password reset";
	string message = "<body>Translate-Net<br>Account password reset was requested.<br>";
	message += "To confirm resetting password for account ";
	message += email;
	message += ": <a href=\"http://translate-net.com/reset=";
	message += reset_uid;
	message += "\">Click here</a>";
	message += "<br>A new password will be sent to you.</body>";
	return sendEmail(EMAIL_TYPE_RESET_PASSWORD, email, topic, message);
}

int SessionManager::sendEmail_NewPassword(char *email, char *password, string lang)
{
	string topic = "Translate-Net new password";
	string message = "Translate-Net<br>";
	message += "Account: ";
	message += email;
	message += "New password for your account: ";
	message += password;
	return sendEmail(EMAIL_TYPE_NEW_PASSWORD, email, topic, message);
}

int SessionManager::sendEmail(int type, char *email, string topic, string message)
{
	Email *m = new Email();
	m->type = type;
	m->helo = getOptions().paramsEmail.SMTP_helo;
	m->auth = getOptions().paramsEmail.SMTP_auth;
	m->from = getOptions().paramsEmail.SMTP_from;
	m->to = email;
	m->topic = topic;
	m->message = message;
	serviceEmail.EnqueueTask(m);
	return 0;
}

bool SessionManager::onEmailSendFailure(int email_type, const char *email)
{
	log(LOG_WARNING, "Error: onEmailSendFailure (email=%s).", email);
if (SessionManager::getOptions().debug & OPTIONS_DEBUG_EMAIL) {
	log(LOG_FATAL, "***DEBUG: OPTIONS_DEBUG_EMAIL");
	return true;
}
	if (email_type == EMAIL_TYPE_REG_PASSWORD) {
		bool deleted;
		unsigned int id = User::checkUserExist(getEmailCon(), (char *)email, deleted);
		if (!id) {
			log(LOG_ERROR, "onEmailSendFailure Error: user not found (email=%s).\n", email);
			return false;
		}
		if (deleted) {
			log(LOG_WARNING, "onEmailSendFailure: user already deleted (email=%s).\n", email);
			return true;
		}
		if (User::deleteUser(getEmailCon(), id)) {
			log(LOG_ERROR, "onEmailSendFailure Error: unable to delete user (id=%d, email=%s).\n", id, email);
			return false;
		}
		return true;
	}
	return false;
}

int SessionManager::sendSMSMessage(TwilioMessage *msg)
{
	serviceTwilioSMS.EnqueueTask(msg);
}

int SessionManager::PayPal_Transfer(char *account, int sum)
{
	PayPalTransfer *t = new PayPalTransfer();
	t->account = account;
	t->sum = sum;
	servicePayPal.EnqueueTask(t);
	return 0;
}

bool SessionManager::onPayPalTransferEnd(PayPalTransfer transfer, int result_code)
{
	log(LOG_VERBOSE, "onPayPalTransferEnd (paypal_email=%s).", transfer.account.c_str());
	rdlock();
	Session *s = findSessionByPayPalEmail(transfer.account.c_str());
	Translator *t = 0;
	if (!s) {
		DynUserLock();
		bool deleted = false;
		unsigned int id = Translator::checkUserExistPayPal(getPayPalCon(), transfer.account.c_str(), deleted);
		if (deleted) {
			log(LOG_FATAL, "onPayPalTransferEnd: ERROR: unable to find translator (already deleted). (paypal_email=%s)", transfer.account.c_str());
			DynUserUnlock();
			unlock();
			return true;
		}
		if (!id) {
			log(LOG_FATAL, "onPayPalTransferEnd: unable to find translator. (paypal_email=%s)", transfer.account.c_str());
			DynUserUnlock();
			unlock();
			return false;
		}
		User *user = User::readUser(getPayPalCon(), id);
		if (!user) {
			log(LOG_FATAL, "onPayPalTransferEnd: unable to read translator. (id=%d, paypal_email=%s)", id, transfer.account.c_str());
			DynUserUnlock();
			unlock();
			return false;
		}
		t = dynamic_cast<Translator *>(user);
		if (!t) {
			log(LOG_FATAL, "onPayPalTransferEnd: ERROR: user is not translator. (paypal_email=%s)", transfer.account.c_str());
			DynUserUnlock();
			unlock();
			return false;
		}
		t->StoreTransfer(getPayPalCon(), result_code, -transfer.sum);
		if (result_code == BILLING_VERIFY_OK)
			t->onPurchaseVerified(getPayPalCon(), this, cManager, -transfer.sum);
		t->FinalizeTransfer(getPayPalCon());
		DynUserUnlock();
	} else {
		t = dynamic_cast<Translator *>(s->getUser());
		if (!t) {
			log(LOG_FATAL, "onPayPalTransferEnd: ERROR: user is not translator. (paypal_email=%s)", transfer.account.c_str());
			unlock();
			s->sendPacket_PayPalTransferError(ERROR_NO_USER);
			return false;
		}
		t->StoreTransfer(getPayPalCon(), result_code, -transfer.sum);
		if (result_code == BILLING_VERIFY_OK)
			t->onPurchaseVerified(getPayPalCon(), this, cManager, -transfer.sum);
		t->FinalizeTransfer(getPayPalCon());
		s->onPayPalPurchaseVerified(result_code, -transfer.sum);
	}
	unlock();
	return 0;
}

int SessionManager::IOSPurchaseVerify(IOSPurchase p)
{
	IOSPurchase *purchase = new IOSPurchase();
	*purchase = p;
	serviceIOSVerify.EnqueueTask(purchase);
}

int SessionManager::AndroidPurchaseVerify(int money, string data, string signature)
{
    std::shared_ptr<EVP_MD_CTX> mdctx = std::shared_ptr<EVP_MD_CTX>(EVP_MD_CTX_create(), EVP_MD_CTX_destroy);
    const EVP_MD* md = EVP_get_digestbyname("SHA1");
    if(NULL == md)
    {
        return BILLING_VERIFY_FAILURE;
    }
    if(0 == EVP_VerifyInit_ex(mdctx.get(), md, NULL))
    {
        return BILLING_VERIFY_FAILURE;
    }

    if(0 == EVP_VerifyUpdate(mdctx.get(), (void*)data.c_str(), data.length()))
    {
        return BILLING_VERIFY_FAILURE;
    }

    std::shared_ptr<BIO> b64 = std::shared_ptr<BIO>(BIO_new(BIO_f_base64()), BIO_free);
    BIO_set_flags(b64.get(),BIO_FLAGS_BASE64_NO_NL);

    std::shared_ptr<BIO> bPubKey = std::shared_ptr<BIO>(BIO_new(BIO_s_mem()), BIO_free);
    BIO_puts(bPubKey.get(), GooglePubKey);
    BIO_push(b64.get(), bPubKey.get());

    std::shared_ptr<EVP_PKEY> pubkey = std::shared_ptr<EVP_PKEY>(d2i_PUBKEY_bio(b64.get(), NULL), EVP_PKEY_free);

    string decoded_signature = base64_decode(signature);

    if (EVP_VerifyFinal(mdctx.get(), (const unsigned char *)decoded_signature.c_str(), decoded_signature.length(), pubkey.get()))
    	return BILLING_VERIFY_OK;
    else
    	return BILLING_VERIFY_ERROR_SIGNATURE;
}

void SessionManager::Purchase_VerifyHandler(IOSPurchase p, int result_code)
{
	rdlock();
	Connection *sqlCon = getIOSVerifyManagerCon();
	Session *s = findSessionByUser(p.user);
	if (!s) {
		if (result_code == BILLING_VERIFY_OK) {
			DynUserLock();
			User *u = User::readUser(getIOSVerifyManagerCon(), p.user);
			if (!u) {
				DynUserUnlock();
				unlock();
				log(LOG_ERROR, "IOSPurchase_VerifyHandler: Unable to read user data (user=%d).", p.user);
				return;
			}
			Client *c = dynamic_cast<Client *>(u);
			if (!c) {
				DynUserUnlock();
				unlock();
				return;
			}
			c->StoreTransfer(sqlCon, result_code, p.money);
			if (result_code == BILLING_VERIFY_OK)
				c->onPurchaseVerified(sqlCon, this, cManager, p.money);
			c->FinalizeTransfer(sqlCon);
			delete c;
			DynUserUnlock();
		}
	} else {
		if (result_code == BILLING_VERIFY_OK) {
			Client *c = dynamic_cast<Client *>(s->getUser());
			if (!c) {
				unlock();
				s->onPurchaseVerified(BILLING_VERIFY_FAILURE, p.money);
				return;
			}
			c->StoreTransfer(sqlCon, result_code, p.money);
			if (result_code == BILLING_VERIFY_OK)
				c->onPurchaseVerified(sqlCon, this, cManager, p.money);
			c->FinalizeTransfer(sqlCon);
		}
		s->onPurchaseVerified(result_code, p.money);
	}
	unlock();
	return;
}


Statistic SessionManager::getStatistic(User *u)					
{	
	struct Statistic _stat;
	pthread_mutex_lock(&stat_lock);
	_stat = stat;
	pthread_mutex_unlock(&stat_lock);
	return _stat;
}

// SessionManager must be read-locked
void SessionManager::UpdateStatistic(Connection *sqlCon, bool OnlyUsers)
{
	if (MaintenanceExit)
		return;
	pthread_mutex_lock(&stat_lock);
	stat.clients=0;
	stat.translators=0;
	stat.langnum.clear();
	for (size_t i=0; i<sessions.size(); i++) {
		if (!sessions[i]->getUser())
			continue;
		if (sessions[i]->getMarkEnd() || sessions[i]->getMarkAbort())
			continue;
		Translator *t = dynamic_cast<Translator *>(sessions[i]->getUser());
		if (!t)
			stat.clients++;
		else {
			stat.translators++;
			if (getOptions().Stat_statLangs) {
				t->lock();
				for (size_t j=0; j<t->getLangs().num(); j++) {
					string l = t->getLangs().get(j);
					int cur_num = 1;
					if (stat.langnum.find(l) != stat.langnum.end()) {
						cur_num = stat.langnum[l];
						stat.langnum.erase(l);
					}
					stat.langnum.insert(pair<string, int>(l, cur_num));
				}
				t->unlock();
			}
		}
	}
	if (OnlyUsers) {
		pthread_mutex_unlock(&stat_lock);
		return;
	}
	ReadLastStats(sqlCon);
	stat.calls_hour = CountCallsStat(sqlCon, 1);
	stat.users_hour = CountUsersStat(sqlCon, 1);
	stat.calls_day = CountCallsStat(sqlCon, 24);
	stat.users_day = CountUsersStat(sqlCon, 24);
	pthread_mutex_unlock(&stat_lock);
}

int SessionManager::chooseThread()
{
	unsigned int *sessionNum = new unsigned int[ThreadNum];
	for (int i=0; i<ThreadNum; i++)
		sessionNum[i] = 0;
	for (int i=0; i<ThreadNum; i++)
		for (vector<Session *>::iterator it=sessions.begin(); it<sessions.end(); it++)
			sessionNum[(*it)->getThread()]++;
	int thread = 0;
	for (int i=0; i<ThreadNum; i++)
		if (sessionNum[i] < sessionNum[thread])
			thread = i;
	delete[] sessionNum;
	return thread;
}

SessionManager::~SessionManager()
{
	for (int i=0; i<ThreadNum; i++) {
		delete[] sockets[i];
		delete[] sockpipe[i];
		delete parser[i];
	}
	for (int i=0; i<ThreadNum-1+THREAD_SERVICE_NUM; i++) {
		delete sqlCon[i];
	}
	delete[] sockets;
	delete[] sockpipe;
	delete[] sqlCon;
	delete[] parser;
	pthread_rwlock_destroy(&sessions_lock);
}

Translator *SessionManager::readServerAccount(Connection *sqlCon, const char *Email_Server)
{
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SELECT salt, hash FROM passwords JOIN users ON passwords.user=users.id WHERE email=(?)");
	pstmt->setString(1, Email_Server);
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
	if (!res->getString("salt").length() || !res->getString("hash").length()) {
		log(LOG_FATAL, "Error: Server account (email='%s') is without password.", Email_Server);
		delete res;
		return 0;
	}
	delete res;
	return dynamic_cast<Translator *>(User::readUser(sqlCon, User::readAccountID(sqlCon, Email_Server)));
}

int SessionManager::getCurThreadN()
{
	return PThreadMap[pthread_self()];
}

void SessionManager::InitSQLConnection(Connection *sqlCon)
{
	PreparedStatement *pstmt = sqlCon->prepareStatement(
			"SET wait_timeout=2147483");
	try {
		pstmt->executeUpdate();
	} catch (SQLException &ex) {
		log(LOG_ERROR, "[%s] MySQL error(%d): %s", __func__, ex.getErrorCode(), ex.what());
	}
	delete pstmt;
	return;
}

int SessionManager::LoadGooglePubKey(string fname)
{
	try {
		std::ifstream in(fname);
		std::stringstream buffer;
		buffer << in.rdbuf();
		std::string contents(buffer.str());
		GooglePubKey = new char[contents.length()+1];
		strcpy(GooglePubKey, contents.c_str());
		return 0;
	} catch (std::exception ex) {
		log (LOG_FATAL, ex.what());
		GooglePubKey = new char[1];
		strcpy(GooglePubKey, "");
		return -1;
	}
}

void SessionManager::LogOptions()
{
	log(LOG_NORMAL, "Options:");
	log(LOG_NORMAL,"debug=%d", options.debug);
	log(LOG_NORMAL,"ServerAddress=%s", options.ServerAddress);
	log(LOG_NORMAL,"ServerPort=%d", options.ServerPort);
	log(LOG_NORMAL,"File_DHparam=%s", options.File_DHparam);
	log(LOG_NORMAL,"File_Cert=%s", options.File_Cert);
	log(LOG_NORMAL,"File_Key=%s", options.File_Key);
	log(LOG_NORMAL,"File_GooglePubKey=%s", options.File_GooglePubKey);
	log(LOG_NORMAL,"DBHost=%s", options.DBHost);
	log(LOG_NORMAL,"DBUser=%s", options.DBUser);
	log(LOG_NORMAL,"PhoneRegisterMode=%d", options.	PhoneRegisterMode);
	log(LOG_NORMAL,"File_LangsXML=%s", options.	File_LangsXML);
	log(LOG_NORMAL,"Server_Email=%s", options.Server_Email);
	log(LOG_NORMAL,"paramsPayPal.Account=%s", options.paramsPayPal.Account);
	log(LOG_NORMAL,"paramsPayPal.UserID=%s", options.paramsPayPal.UserID);
	log(LOG_NORMAL,"paramsPayPal.Password=%s", options.paramsPayPal.Password);
	log(LOG_NORMAL,"paramsPayPal.Signature=%s", options.paramsPayPal.Signature);
	log(LOG_NORMAL,"paramsPayPal.AppID=%s", options.paramsPayPal.AppID);
	log(LOG_NORMAL,"paramsPayPal.AdaptivePaymentURL=%s", options.paramsPayPal.AdaptivePaymentURL);
	log(LOG_NORMAL,"paramsPayPal.CancelURL=%s", options.paramsPayPal.CancelURL);
	log(LOG_NORMAL,"paramsPayPal.ReturnURL=%s", options.paramsPayPal.ReturnURL);
	log(LOG_NORMAL,"paramsPayPal.Log=%d", options.paramsPayPal.Log);
	log(LOG_NORMAL,"paramsEmail.SMTP_addr=%s", options.paramsEmail.SMTP_addr);
	log(LOG_NORMAL,"paramsEmail.SMTP_port=%d", options.paramsEmail.SMTP_port);
	log(LOG_NORMAL,"paramsEmail.SMTP_helo=%s", options.paramsEmail.SMTP_helo);
	log(LOG_NORMAL,"paramsEmail.SMTP_from=%s", options.paramsEmail.SMTP_from);
	log(LOG_NORMAL,"paramsEmail.email_archive_file=%s", options.paramsEmail.email_archive_file);
	log(LOG_NORMAL,"paramsEmail.Log_Email=%d", options.paramsEmail.Log_Email);
	log(LOG_NORMAL,"paramsIOSVerify.addr=%s", options.paramsIOSVerify.addr);
	log(LOG_NORMAL,"paramsIOSVerify.port=%d", options.paramsIOSVerify.port);
	log(LOG_NORMAL,"paramsIOSVerify.Log=%d", options.paramsIOSVerify.Log);
	log(LOG_NORMAL,"paramsTwilioSMS.Addr=%s", options.paramsTwilioSMS.Addr);
	log(LOG_NORMAL,"paramsTwilioSMS.Account=%s", options.paramsTwilioSMS.Account);
	log(LOG_NORMAL,"paramsTwilioSMS.From=%s", options.paramsTwilioSMS.From);
	log(LOG_NORMAL,"paramsTwilioSMS.Send_AttemptNum=%d", options.paramsTwilioSMS.Send_AttemptNum);
	log(LOG_NORMAL,"paramsTwilioSMS.Send_Timeout=%d", options.paramsTwilioSMS.Send_Timeout);
	log(LOG_NORMAL,"paramsTwilioSMS.Log=%d", options.paramsTwilioSMS.Log);
	log(LOG_NORMAL,"CurlTimeout=%d", options.CurlTimeout);
	log(LOG_NORMAL,"FeeMarket=%d", options.FeeMarket);
	log(LOG_NORMAL,"FeeApp=%d", options.FeeApp);
	log(LOG_NORMAL,"FeeApp_Reduced=%d", options.FeeApp_Reduced);
	log(LOG_NORMAL,"FeeApp_TimeReduced=%d", options.FeeApp_TimeReduced);
	log(LOG_NORMAL,"TimerSessionTimeout=%d", options.	TimerSessionTimeout);
	log(LOG_NORMAL,"TimerSessionRegTimeout=%d", options.TimerSessionRegTimeout);
	log(LOG_NORMAL,"Stat_statLangs=%d", options.	Stat_statLangs);
	log(LOG_NORMAL,"Stat_MinCallDuration=%d", options.Stat_MinCallDuration);
	log(LOG_NORMAL,"TimeoutNetLag=%d", options.	TimeoutNetLag);
	log(LOG_NORMAL,"Timeout_CallStatus=%d", options.Timeout_CallStatus);
	log(LOG_NORMAL,"Timeout_CallRequest=%d", options.Timeout_CallRequest);
	log(LOG_NORMAL,"Timeout_CallConfirm=%d", options.Timeout_CallConfirm);
	log(LOG_NORMAL,"Call_TimeFree=%d", options.	Call_TimeFree);
	log(LOG_NORMAL,"Call_MinLength=%d", options.	Call_MinLength);
	log(LOG_NORMAL,"Call_MinTimeRating=%d", options.Call_MinTimeRating);
	log(LOG_NORMAL,"SMS_MaxNum=%d", options.SMS_MaxNum);
	log(LOG_NORMAL,"SMS_MaxDays=%d", options.SMS_MaxDays);
	log(LOG_NORMAL,"ActiveTSearch=%d", options.ActiveTSearch);
	log(LOG_NORMAL,"CallHistoryLimit=%d", options.CallHistoryLimit);
	log(LOG_NORMAL,"DEBUG_EMAIL=%d", options.DEBUG_EMAIL);
	log(LOG_NORMAL,"DEBUG_SMS=%d", options.DEBUG_SMS);
}

SessionManager::SessionManager(int _ThreadN, pthread_t _main_thread, Driver *_sqlDriver, Options _options)
:	ThreadNum(_ThreadN),
 	sqlDriver(_sqlDriver),
 	serviceEmail(this, &_options.paramsEmail),
 	serviceIOSVerify(this, &_options.paramsIOSVerify, &SessionManager::Purchase_VerifyHandler),
 	servicePayPal(this, &_options.paramsPayPal),
	serviceTwilioSMS(this, &_options.paramsTwilioSMS)
{
	options = _options;
	LogOptions();

	max_sockets = 1000000;
	try {
		sockets = new struct pollfd*[ThreadNum];
		memset(sockets, 0, ThreadNum*sizeof(struct pollfd));
		sockpipe = new int*[ThreadNum];
		parser = new xercesc::XercesDOMParser*[ThreadNum];
		for (int i=0; i<ThreadNum; i++) {
			sockets[i] = new struct pollfd[max_sockets];
			sockpipe[i] = new int[2];
			pipe2(sockpipe[i], O_NONBLOCK);
			parser[i] = new xercesc::XercesDOMParser;
		}
		sqlCon = new Connection*[ThreadNum+THREAD_SERVICE_NUM];
		for (int i=0; i<ThreadNum+THREAD_SERVICE_NUM; i++) {
			sqlCon[i] = sqlDriver->connect(options.DBHost, options.DBUser, options.DBPassword);
			sqlCon[i]->setSchema("Translate");
			InitSQLConnection(sqlCon[i]);
		}
		sqlCon[ThreadNum] = sqlDriver->connect(options.DBHost, options.DBUser, options.DBPassword);
		sqlCon[ThreadNum]->setSchema("Translate");
		InitSQLConnection(sqlCon[ThreadNum]);
	} catch (exception ex) {
		log(LOG_FATAL, "No free memory.");
		throw ex;
	}
	cManager = new CallManager(this);
	pthread_rwlock_init(&sessions_lock, 0);
	timer_sessions = timer_calls = timer_stat = timer_db = 0;
	stat.clients = stat.translators = stat.calls_hour = stat.users_hour = 0;
	MaintenanceExit = false;
	main_thread = _main_thread;
	MaintenanceExitNum = 0;
	ServerStartTime = time(0);
	pthread_mutex_init(&stat_lock, 0);

	if (LoadGooglePubKey(options.File_GooglePubKey)) {
		log(LOG_FATAL, "Error: Error loading Google public key from '%s'", options.File_GooglePubKey);
		throw new exception;
	}

	ServerAccount = readServerAccount(getServiceCon(), options.Server_Email);
	if (!ServerAccount) {
		log(LOG_FATAL, "Error: Error reading server translator account (email='%s').", getOptions().Server_Email);
		throw new exception;
	}
}

void SessionManager::test_email()
{
	Email *m = new Email();
	m->helo = "smtp.mail.ru";
	m->from = "translate-net@mail.ru";
	m->to = "login@mail.ru";
	m->topic = "test3";
	m->message = "Test\n test email 3";
	serviceEmail.EnqueueTask(m);
}
