/*
 * ServiceAsync.h
 *
 *  Created on: 27.02.2015
 *      Author: roman
 */

#ifndef SERVICEASYNC_H_
#define SERVICEASYNC_H_

#include <string>
#include <queue>
#include <time.h>
#include "Session.h"

typedef struct _ServiceParams {
} ServiceParams;

class ServiceAsync;
class ServiceTask {
public:
	ServiceTask()	: start(0)			 	{														}
	virtual ~ServiceTask()					{														}
	time_t start;
	ServiceAsync *service;
};

class ServiceAsync {
public:
	ServiceAsync(SessionManager *_sManager);
	virtual const char *getName()			{	return "ASYNC";										}
	void EnqueueTask(ServiceTask *task);
	bool checkSent_Task(ServiceTask *task);
	SessionManager *getsManager()			{	return sManager;									}
	void callbackTaskDone(ServiceTask *task);
	virtual void ProcessTasks();
	virtual void ProcessTimeouts();
	virtual void ProcessUnfinishedTasks()	{														}
	bool TaskPending()						{	return tasks.size() > 0 || tasks_sent.size() > 0;	}
	Connection *getCon()					{	return sqlCon;										}
	void setCon(Connection *_sqlCon)		{	sqlCon = _sqlCon;									}
	void lock_sent()						{	pthread_mutex_lock(&tasks_sent_lock);				}
	void unlock_sent()						{	pthread_mutex_unlock(&tasks_sent_lock);				}
protected:
	virtual int ProcessTask(ServiceTask *task) = 0;				// return -1 if task not enqueued to curl
	SessionManager *sManager;
	Connection *sqlCon;
	queue<ServiceTask *> tasks;
	void lock()								{	pthread_mutex_lock(&tasks_lock);					}
	void unlock()							{	pthread_mutex_unlock(&tasks_lock);					}
	list<ServiceTask *> tasks_sent;

private:
	pthread_mutex_t tasks_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t tasks_sent_lock = PTHREAD_MUTEX_INITIALIZER;
};

#endif
