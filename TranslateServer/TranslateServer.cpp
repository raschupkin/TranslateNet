//============================================================================
// Name        : TranslateServer.cpp
// Author      : Raschupkin Roman
// Version     :
// Copyright   : Your copyright notice
// Description : Translate server
//============================================================================

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <xercesc/util/PlatformUtils.hpp>
#include "TranslateServer.h"
#include "SessionManager.h"
#include "User.h"
#include "ServicePayPal.h"
using namespace sql;
using namespace xercesc;

Driver *sqlDriver;
SessionManager *sManager;


Options *loadOptions(const char *OptionsFile) {
	Options *options = new Options;
options->debug = OPTIONS_DEBUG_EMAIL;

	options->ServerAddress = "*";
	options->ServerPort = 45914;
	options->File_DHparam = "certs//dh_param_1024.pem";
	options->File_Cert = "certs//ssl.pem";
	options->File_Key = "certs//ssl.key";
	options->File_GooglePubKey = "certs//google_pubkey.b64";
	options->DBHost = "tcp://127.0.0.1:3306";
	options->DBUser = "roman";
	options->DBPassword = "";

	options->PhoneRegisterMode					= PHONE_REGISTER_SETUP;

	options->File_LangsXML						= XML_LANGS;
	options->Server_Email						= "company@translate-net.com";

	options->paramsPayPal.Account				= "translatenet1-facilitator@gmail.com";
	options->paramsPayPal.UserID				= "translatenet1-facilitator_api1.gmail.com";
	options->paramsPayPal.Password				= "NTAJMT8DKJ687VP5";
	options->paramsPayPal.Signature				= "AFcWxV21C7fd0v3bYYYRCpSSRl31ANE2xkYRJa5bFyg5.4icPUWOYTAc";
	options->paramsPayPal.AppID					= "APP-80W284485P519543T";
	options->paramsPayPal.AdaptivePaymentURL	= "https://svcs.sandbox.paypal.com/AdaptivePayments/Pay";
	options->paramsPayPal.CancelURL				= "http://cancel_url";
	options->paramsPayPal.ReturnURL				= "http://return_url";
	options->paramsPayPal.Log					= true;

	options->paramsEmail.SMTP_addr				= "smtp.mail.ru";
	options->paramsEmail.SMTP_port				= 465;
	options->paramsEmail.SMTP_helo				= "mail.ru";
	options->paramsEmail.SMTP_from				= "translate-net@mail.ru";
	options->paramsEmail.SMTP_auth				= "dHJhbnNsYXRlLW5ldAB0cmFuc2xhdGUtbmV0AEJhd2lsMTIz";
	options->paramsEmail.email_archive_file		= "unsent.email";
	options->paramsEmail.Log_Email 				= false;

	options->paramsTwilioSMS.Addr				= "https://api.twilio.com/2010-04-01/Accounts/ACc2af327bdfc1b91c351bab341ca25567/Messages.json";
	options->paramsTwilioSMS.Account			= "ACc2af327bdfc1b91c351bab341ca25567";
	options->paramsTwilioSMS.AuthToken			= "bf45266acdb06a348791ac97f997b41c";
	options->paramsTwilioSMS.From				= "+13343924268";
	options->paramsTwilioSMS.StatusAddr			= "http://translate-net.com/twilio-status.php";
	options->paramsTwilioSMS.Send_AttemptNum 	= 1;
	options->paramsTwilioSMS.Send_Timeout		= 2;
	options->paramsTwilioSMS.Log				= true;

	options->paramsIOSVerify.addr				= "https://sandbox.itunes.apple.com/verifyReceipt";
	options->paramsIOSVerify.port				= 80;
	options->paramsIOSVerify.Log 				= true;

	options->CurlTimeout						= 180;

	options->FeeMarket							= 30;
	options->FeeApp								= 0;
	options->FeeApp_Reduced						= 0;
	options->FeeApp_TimeReduced 				= INT_MAX;//120*60*60;	// 120 hours

	options->TimerSessionTimeout				= 300;
	options->TimerSessionRegTimeout				= 7;
	options->TimeoutNetLag						= 100;
	options->Timeout_CallStatus					= 600;
	options->Timeout_CallRequest				= 1200;
	options->Timeout_CallConfirm				= 1200;
	options->Call_TimeFree 						= 60;			// 60 seconds
	options->Call_MinLength 					= 60;			// 60 seconds
	options->Call_MinTimeRating 				= 60;			// 60 seconds
	options->SMS_MaxNum							= 5;
	options->SMS_MaxDays						= 90;			// 90 days
	options->ActiveTSearch						= 1;
	options->Stat_statLangs						= 0;
	options->Stat_MinCallDuration				= options->Call_MinLength;
	
	options->CallHistoryLimit					= 100;

	options->SMS_MaxNum = 10;
	options->SMS_MaxDays = 30;
	options->SMS_BlockDays = 30;

	options->DEBUG_EMAIL = false;
	options->DEBUG_SMS = true;
}

void *Thread_ProcessSessions(void *arg)
{
	unsigned long thread = (unsigned long)arg;
	sManager->Process(thread);
	return 0;
}

int ProcessConnections(SSL_CTX *ssl_ctx, Options *options)
{
	BIO *bio, *ssl_bio;
	char address[512] = {0};
	snprintf(address, 512, "%s:%d", options->ServerAddress, options->ServerPort);
	SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
	ssl_bio = BIO_new_accept(address);
	BIO_set_bind_mode(ssl_bio, BIO_BIND_REUSEADDR);
	BIO_set_nbio(ssl_bio, 1);
	if (BIO_do_accept(ssl_bio) <= 0) {
		log(LOG_VERBOSE, "Error setting up SSL accept.");
		log_ssl(LOG_VERBOSE);
		return -1;
	}
	while (1) {
		if (BIO_do_accept(ssl_bio) <= 0) {
			log(LOG_VERBOSE, "Error accepting SSL connection.");
			log_ssl(LOG_VERBOSE);
			fflush(stdout);
			return -1;
		}
		bio = BIO_pop(ssl_bio);
		SSL *ssl = SSL_new(ssl_ctx);
		SSL_set_accept_state(ssl);
		SSL_set_bio(ssl, bio, bio);
		SSL_set_read_ahead(ssl, 1);

		struct sockaddr addr;
		socklen_t len = sizeof(addr);
		getpeername(SSL_get_fd(ssl), &addr, &len);
		log(LOG_VERBOSE, "New session from %s.", inet_ntoa(((sockaddr_in *)&addr)->sin_addr));
//		SSL_set_options(ssl, SSL_OP_NO_COMPRESSION);
/*	fprintf(stderr,"acc %x %x",bio, ssl);
	SSL_free(ssl);
	continue;
*/
//		int err = SSL_get_error(ssl, 0);
//		printf("e%d",err);
//		SSL_read(ssl,0,0);
		sManager->addSession(ssl, bio);
	}
	return 0;
}

pthread_mutex_t	*ssl_mutex = 0;

void SSL_locking_function(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&ssl_mutex[n]);
	else
		pthread_mutex_unlock(&ssl_mutex[n]);
}

void SSL_id_function(CRYPTO_THREADID *tid)
{
	CRYPTO_THREADID_set_numeric(tid, pthread_self());
}

void SSL_thread_setup(void)
{
	ssl_mutex = new pthread_mutex_t[CRYPTO_num_locks()];
	for (int i=0; i<CRYPTO_num_locks(); i++)
		pthread_mutex_init(&ssl_mutex[i], 0);
	CRYPTO_THREADID_set_callback(SSL_id_function);
	CRYPTO_set_locking_callback(SSL_locking_function);
}

SSL_CTX* InitializeSSL(Options *options)
{
	SSL_library_init();
	SSL_load_error_strings();

	SSL_thread_setup();

	SSL_CTX *ssl_ctx = SSL_CTX_new (SSLv3_server_method());
/*	if (!SSL_CTX_set_cipher_list(ssl_ctx, "aNULL")){
	if (!SSL_CTX_set_cipher_list(ssl_ctx, "ECDHE-RSA-AES256-GCM-SHA384")){
		log(LOG_ERROR, "Error selecting SSL cipher.");
		return 0;
	}
*/
	if (SSL_CTX_use_PrivateKey_file(ssl_ctx, options->File_Key, SSL_FILETYPE_PEM) != 1) {
		log(LOG_ERROR, "Error reading SSL %s.", options->File_Key);
		return 0;
	}
	if (SSL_CTX_use_certificate_chain_file(ssl_ctx, options->File_Cert) != 1) {
		log(LOG_ERROR, "Error reading SSL %s.", options->File_Cert);
		return 0;
	}
	FILE *dhParamFile = fopen(options->File_DHparam, "r");
	if (!dhParamFile) {
		log(LOG_ERROR, "Error reading SSL %s.", options->File_DHparam);
		return 0;
	}
	DH *dh = PEM_read_DHparams(dhParamFile, 0, 0, 0);
	if (!dh) {
		log(LOG_ERROR, "Error reading SSL DH parameters.");
		return 0;
	}
	SSL_CTX_set_tmp_dh(ssl_ctx, dh);
	return ssl_ctx;
}

int InitializeMySQL()
{
	sqlDriver = get_driver_instance();
	if (!sqlDriver)
		return -1;
	return 0;
}

char ServerAccount_Password[MAX_PASSWORD+1] = {0};
char ResetPassword_UID[MAX_RESET_PASS_REQEST_UID+1] = {0};

const char *HelpMessage = "-d\t\tLog priority [%d, %d]\n, -i\t\tpassword to create server account with\n\n";
int parse_cmdline(int argc, char *argv[])
{
	opterr = 0;
	optind = 1;
	int opt;
	while ((opt = getopt(argc, argv, "d:i:R:")) != -1) {
		switch (opt) {
		case 'd': {
			if (!optarg) {
				log(LOG_ALL, "Wrong arguments.");
				return -1;
			}
			int d = atoi(optarg);
			log(LOG_FATAL, "Log priority %d", d);
			log_priority(d);
			break;
		}
		case 'i': {
			strncpy(ServerAccount_Password, optarg, MAX_PASSWORD+1);
			break;
		}
		case 'h': {
			log(LOG_NORMAL, HelpMessage , LOG_FATAL, LOG_ALL);
			return -1;
		}
		case 'R': {
			if (strlen(optarg) > MAX_RESET_PASS_REQEST_UID)
				return -1;
			strncpy(ResetPassword_UID, optarg, MAX_RESET_PASS_REQEST_UID);
			break;
		}
		default:
			log(LOG_ALL, "Wrong arguments.");
			return -1;
		}
	}
	return 0;
}

int CreateServerAccount(const char *Email_Server, char *password)
{
	log(LOG_FATAL, "Creating server account...");
	Connection *sqlCon = sManager->getServiceCon();
	if (SessionManager::readServerAccount(sqlCon, Email_Server)) {
		log(LOG_FATAL, "Server account already created.");
		return -1;
	}
	unsigned int id = User::storeUser(sqlCon, (char *)Email_Server, true);
	if (id <= 0) {
		log(LOG_FATAL, "Error(storeUser) creating server account.");
		return -1;
	}
	if (User::storePassword(sqlCon, id, password)) {
		log(LOG_FATAL, "Error(storePassword) creating server account.");
		return -1;
	}
	unsigned int server_id;
	if (!User::checkPassword(sqlCon, (char *)Email_Server, password, server_id)) {
		log(LOG_FATAL, "Error(checkPassword) creating server account.");
		return -1;
	}
	if (User::updateUserEmailAck(sqlCon, server_id)) {
		log(LOG_FATAL, "Error(updateUserEmailAck) creating server account.");
		return -1;
	}
	Translator *serverAcc = dynamic_cast<Translator *>(User::readUser(sqlCon, server_id));
	if (!serverAcc) {
		log(LOG_FATAL, "Error(readUser) creating server account.");
		return -1;
	}
	log(LOG_FATAL, "Done");
	return 0;
}

int ResetUserPassword(char *reset_uid)
{
	if (!reset_uid)
		return -1;
	log(LOG_NORMAL, "Reset password: UID=%s", reset_uid);
	unsigned int user_id = User::getConfirmResetPasswordID(sManager->getServiceCon(), reset_uid);
	if (user_id <= 0) {
		log(LOG_ERROR, "Reset password: user not found. (UID=%s)", reset_uid);
		return -1;
	}
	char password[MAX_PASSWORD+1];
	User::genPassword(password, MAX_PASSWORD);
	if (User::storePassword(sManager->getServiceCon(), user_id, password)) {
		log(LOG_ERROR, "Reset password: Unable to set password (id=%d).", user_id);
		return -1;
	}
	return 0;
}

void signal_maintenance(int signal)
{
	log(LOG_FATAL,"Maintenance exit signal: waiting for active calls to finish.");
	sManager->startMaintenanceExit();
}

void test()
{
	log(LOG_FATAL, "Running TEST");
	TwilioMessage *msg = new TwilioMessage();
	msg->phone = "+79096545914";
	msg->text = "Code 123";
	sManager->sendSMSMessage(msg);
}

int main(int argc, char *argv[])
{
	log_init();
	if (parse_cmdline(argc, argv))
		return -1;
	signal(SIGPIPE, SIG_IGN);				// socket operation may cause one
	signal(SIGALRM, SIG_IGN);				// curl may send one
	signal(SIGINT, signal_maintenance);		// Ctrl+C

	const char *OptionsFile = "TranslateServer.xml";
	log(LOG_VERBOSE, "Loading options...");
	Options* options = loadOptions(OptionsFile);
	if (!options) {
		log(LOG_FATAL, "Error reading options file %s.", OptionsFile);
	}

	log(LOG_VERBOSE, "Initializing database...");
	if (InitializeMySQL() < 0) {
		log(LOG_ERROR, "Unable to connect to database. Exiting.");
		return -1;
	}
	log(LOG_VERBOSE, "Initializing SSL...");
	SSL_CTX *ssl_ctx = InitializeSSL(options);
	if (!ssl_ctx) {
		log(LOG_FATAL, "Unable to in itialize OpenSSL. Exiting.");
		return -1;
	}
	try {
		XMLPlatformUtils::Initialize();
	} catch (const XMLException &ex) {
		log(LOG_FATAL, "Unable to initialize XercesXML parser. Exiting.");
		return -1;
	}

	int ThreadN = sysconf(_SC_NPROCESSORS_ONLN)*4;
	try {
		sManager = new SessionManager(ThreadN, pthread_self(), sqlDriver, *options);
	} catch (exception ex) {
		log(LOG_FATAL, "Exception: %s", ex.what());
		log(LOG_FATAL, "Unable to create SessionManager. Exiting.");
		return -1;
	}

	log(LOG_VERBOSE, "Loading %s...", XML_LANGS);
	string LangsXML = Langs::loadLangsFile(XML_LANGS, sManager->getServiceCon());
	if (LangsXML.length() == 0) {
		log(LOG_FATAL, "Error in %s file. Exiting.", XML_LANGS);
		return -1;
	}
	Langs::initCountries();
	sManager->setLangsXML(LangsXML);
	log(LOG_NORMAL, "Initialization finished.");

	if (strlen(ServerAccount_Password))
		return CreateServerAccount(options->Server_Email, ServerAccount_Password);
	if (strlen(ResetPassword_UID))
		return ResetUserPassword(ResetPassword_UID);

	pthread_t *threads = new pthread_t[THREAD_SERVICE_NUM + sManager->getThreadNum()];
	for (size_t i=0; i<THREAD_SERVICE_NUM + sManager->getThreadNum(); i++)
		pthread_create(&threads[i], 0, Thread_ProcessSessions, (void *)i);

//test();

//sManager->test_email();
//sManager->PayPal_Transfer("t1@m.ru", 100);

	log(LOG_NORMAL, "Processing connections from clients...");
	if (ProcessConnections(ssl_ctx, options)) {
		log(LOG_NORMAL, "Exiting.");
		sManager->startMaintenanceExit();
		return -1;
	}
	delete options;
	XMLPlatformUtils::Terminate();
	log_finit();
	return EXIT_SUCCESS;
}
