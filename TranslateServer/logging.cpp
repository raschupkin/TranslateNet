/*
 * logging.cpp
 *
 *  Created on: 25.04.2014
 *      Author: Raschupkin Roman
 */

#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <openssl/err.h>
#include "TranslateServer.h"
#include "SessionManager.h"

FILE *LogFile=0;
int LogPriority = LOG_ALL;

int log_init()
{
	LogFile = fopen(TRANSLATE_LOG_FILENAME, "a");
	if (!LogFile) {
		fprintf(stderr, "Error opening log file.\n");
		return -1;
	}
	return 0;
}

void log_finit()
{
	if (LogFile)
		fclose(LogFile);
}

void log_priority(int p)
{
	if (p < LOG_FATAL || p > LOG_ALL)
		return;
	LogPriority = p;
}

void log_ssl(int priority)
{
	int err;
	while (err= ERR_get_error()) {
		char *str = ERR_error_string(err, 0);
		if (!str)
			return;
		log(priority, str);
	}
}

void log(int priority, const char *msg, ...)
{
	if (priority > LogPriority)
		return;
	time_t t = time(0);
	struct tm *time = localtime(&t);
	char tbuf[512];
	strftime(tbuf, 512, "%D %H:%M:%S|\t", time);
	va_list args;
	if (priority <= LogPriority) {
		if (LogFile) {
			fprintf(LogFile, "{%d}%s", SessionManager::getCurThreadN(), (char *)tbuf);
			va_start(args, msg);
			vfprintf(LogFile, msg, args);
			va_end(args);
			fprintf(LogFile, "\n");
			fflush(LogFile);
		}
		fprintf(stdout, "{%d}%s", SessionManager::getCurThreadN(), (char *)tbuf);
		va_start(args, msg);
		vfprintf(stdout, msg, args);
		va_end(args);
		fprintf(stdout, "\n");
		fflush(stdout);
	}
}



