/*
 * email.cpp
 *
 *  Created on: 29.09.2014
 *      Author: roman
 */
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fstream>
#include <curl/curl.h>
#include "ServiceEmail.h"
#include "TranslateServer.h"
#include "SessionManager.h"

using namespace std;

ServiceEmail::ServiceEmail(SessionManager *_sManager, EmailServiceParams *_params)
:	ServiceAsync(_sManager)
{
	params = *_params;
}
/*
void ServiceEmail::ProcessTasks()
{
	lock();
	lock();
	while (ServiceTask *t = (ServiceTask *)tasks.front()) {
		tasks.pop();
		lock_sent();
		tasks_sent.push_back(t);
		unlock_sent();
		if (ProcessTask(t)) {
			callbackTaskDone(t);
			Email *m = dynamic_cast<Email *>(t);
			if (!m) {
				if (t)
					delete t;
				continue;
			}
			log(LOG_ERROR, "\nSENDING EMAIL\n");
			if (ProcessTask(m)) {
				callbackTaskDone(m);
			}
			delete m;
		}
	}
	unlock();
}
*/
void ServiceEmail::archiveEmail(Email *m)
{
	std::ofstream of;
	of.open(SessionManager::getOptions().paramsEmail.email_archive_file, std::ios_base::app);
	time_t curtime;
	time(&curtime);
	of << "Email Type: " << m->type << std::endl;
	of << "Time: " << ctime(&curtime);
	of << "From: " << m->from << std::endl;
	of << "To: " << m->to << std::endl;
	of << "Topic: " << m->topic << std::endl;
	of << m->message << endl << endl << endl;
}

size_t ServiceEmail::read_callback(char *buffer, size_t size, size_t nitems, void *instream)
{
	Email *m = (Email *)instream;
	if (!m || !m->service)
		return 0;
	if (!m->service->checkSent_Task(m)) {
		return 0;
	}
	if (m->sent >= m->text.length()) {
		return 0;
	}
	int len = m->text.length() - m->sent;
	if (len > size*nitems)
		len = size*nitems;
	memcpy(buffer, m->text.c_str() + m->sent, len);
	m->sent += len;
	return len;
}

int ServiceEmail::sendEmailCurl(Email *m)
{
	m->text = "Content-Type: text/html; charset=\"us-ascii\"\n";
	m->text += "Subject: ";
	m->text += m->topic;
	m->text += "\r\n\r\n";
	m->text += m->message;
    m->sent = 0;
      CURL *curl;
	  CURLcode res;
	  struct curl_slist *recipients = NULL;


	  curl = curl_easy_init();
	  if(curl) {
	    /* this is the URL for your mailserver - you can also use an smtps:// URL
	     * here */
		  string url;
	    curl_easy_setopt(curl, CURLOPT_URL, "smtps://login:password@smtp.mail.com.");

	    /* Note that this option isn't strictly required, omitting it will result in
	     * libcurl will sent the MAIL FROM command with no sender data. All
	     * autoresponses should have an empty reverse-path, and should be directed
	     * to the address in the reverse-path which triggered them. Otherwise, they
	     * could cause an endless loop. See RFC 5321 Section 4.5.5 for more details.
	     */
	    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m->from.c_str());
	    curl_easy_setopt(curl, CURLOPT_MAIL_AUTH, m->auth.c_str());

	    /* Note that the CURLOPT_MAIL_RCPT takes a list, not a char array.  */
	    recipients = curl_slist_append(recipients, m->to.c_str());
log(LOG_VERBOSE, "Sending Email:\nFrom:%s\nTo:%s\n",m->from.c_str(),m->to.c_str());
	    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

	    /* You provide the payload (headers and the body of the message) as the
	     * "data" element. There are two choices, either:
	     * - provide a callback function and specify the function name using the
	     * CURLOPT_READFUNCTION option; or
	     * - just provide a FILE pointer that can be used to read the data from.
	     * The easiest case is just to read from standard input, (which is available
	     * as a FILE pointer) as shown here.
	     */
	    curl_easy_setopt(curl, CURLOPT_READFUNCTION, &read_callback);
	    curl_easy_setopt(curl, CURLOPT_READDATA, m);
	    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	    /* send the message (including headers) */
	    res = curl_easy_perform(curl);
	    /* Check for errors */
	    if(res != CURLE_OK)
	      log(LOG_ERROR, "ERROR curl_easy_perform() failed: %s\n",
	              curl_easy_strerror(res));

	    callbackTaskDone(m);

	    /* free the list of recipients */
	    curl_slist_free_all(recipients);

	    /* curl won't send the QUIT command until you call cleanup, so you should be
	     * able to re-use this connection for additional messages (setting
	     * CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and calling
	     * curl_easy_perform() again. It may not be a good idea to keep the
	     * connection open for a very long time though (more than a few minutes may
	     * result in the server timing out the connection), and you do want to clean
	     * up in the end.
	     */
	    curl_easy_cleanup(curl);
	    return 0;
	  }
		if (!sManager->onEmailSendFailure(m->type, m->to.c_str()))
			archiveEmail(m);
	  return -1;
}

int ServiceEmail::ProcessTask(ServiceTask *task)
{
	Email *m = dynamic_cast<Email *>(task);
	if (!m)
		return -1;
	return sendEmailCurl(m);
}
