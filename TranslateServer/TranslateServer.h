/*
 * TranslateServer.h
 *
 *  Created on: 25.04.2014
 *      Author: Raschupkin Roman
 */

#ifndef TRANSLATESERVER_H_
#define TRANSLATESERVER_H_

#define DATABASE		"Translate"
#define XML_LANGS		"langs.xml"

#define REGISTER_PHONE_BLOCK_ATTEMPTS	5		// 5 attempts to register phone
#define REGISTER_PHONE_BLOCK_PERIOD		365/2	// in days
#define REGISTER_PHONE_TIMEOUT			365/2	// in days

#define MAX_RATING		10

#define TRANSLATE_LOG_FILENAME	"TranslateServer.log"
#define LOG_ALL	5
#define LOG_VERBOSE	4
#define	LOG_WARNING	2
#define	LOG_NORMAL	3
#define LOG_ERROR	1
#define LOG_FATAL	0
int log_init();
void log_finit();
void log_priority(int p);
void log(int priority, const char *msg, ...);
void log_ssl(int priority);

#endif /* TRANSLATESERVER_H_ */
