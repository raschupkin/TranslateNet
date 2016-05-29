/*
 * ServiceAsync.cpp
 *
 *  Created on: 27.02.2015
 *      Author: roman
 */
#include "ServiceAsync.h"
#include "TranslateServer.h"
#include "SessionManager.h"

ServiceAsync::ServiceAsync(SessionManager *_sManager)
: sManager(_sManager),
  sqlCon(0)
{
}

void ServiceAsync::EnqueueTask(ServiceTask *task)
{
	lock();
	task->service = this;
	tasks.push(task);
	unlock();
}

// lock_sent must be locked
bool ServiceAsync::checkSent_Task(ServiceTask *task)
{
	for (std::list<ServiceTask *>::iterator it=tasks_sent.begin(); it!=tasks_sent.end(); it++) {
		if (*it == task)
			return true;
	}
	return false;
}

// lock_sent must be locked
void ServiceAsync::callbackTaskDone(ServiceTask *task)
{
	for (std::list<ServiceTask *>::iterator it=tasks_sent.begin(); it!=tasks_sent.end(); it++) {
		ServiceTask *t = *it;
		if (!t)
			continue;
		if (t == task) {
			tasks_sent.erase(it);
			delete t;
			break;
		}
	}
}

void ServiceAsync::ProcessTasks()
{
	lock();
	while (ServiceTask *t = (ServiceTask *)tasks.front()) {
		tasks.pop();
		lock_sent();
		t->start = time(0);
		tasks_sent.push_back(t);
		ProcessTask(t);
		unlock_sent();
	}
	unlock();
}

void ServiceAsync::ProcessTimeouts()
{
	lock_sent();
	for (std::list<ServiceTask *>::iterator it=tasks_sent.begin(); it!=tasks_sent.end(); it++) {
		ServiceTask *t = *it;
		if (!t)
			continue;
		if (time(0) > t->start + SessionManager::getOptions().CurlTimeout + 5) {			// +5 for curl_output to work
			tasks_sent.erase(it);
			delete t;
		}
	}
	unlock_sent();
}
