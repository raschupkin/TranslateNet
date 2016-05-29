#ifndef EMAIL_H_
#define EMAIL_H_

#include <queue>
#include "Session.h"
#include "ServiceAsync.h"

#define EMAIL_TYPE_GENERAL			0
#define EMAIL_TYPE_REG_PASSWORD		1
#define EMAIL_TYPE_RESET_PASSWORD	2
#define EMAIL_TYPE_NEW_PASSWORD		3

class EmailServiceParams : public ServiceParams
{
public:
	const char *SMTP_addr;
	int SMTP_port;
	const char *SMTP_helo;
	const char *SMTP_auth;
	const char *SMTP_from;
	const char *email_archive_file;
	bool Log_Email;
};

class Email : public ServiceTask {
public:
	int type;
	string helo;
	string auth;
	string to;
	string topic;
	string message;
	string from;

	string text;
	int sent;
};

class ServiceEmail : public ServiceAsync {
public:
	ServiceEmail(SessionManager *_sManager, EmailServiceParams *_params);
//	void ProcessTasks();
private:
	static size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream);
	int ProcessTask(ServiceTask *task);
	int sendEmail(Email *m);
	int sendEmailCurl(Email *m);
	void archiveEmail(Email *m);
	EmailServiceParams params;
#define EMAIL_BUF_SIZE 1024
	char rdbuf[EMAIL_BUF_SIZE];
	int rdpos;
	char wrbuf[EMAIL_BUF_SIZE];
	int wrpos;
	bool LogEmail;
	SessionManager *sManager;
};

#endif
