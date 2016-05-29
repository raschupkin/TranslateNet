/*
 * protocol.h
 *
 *  Created on: 27.04.2014
 *      Author: Raschupkin Roman
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define PACKET_BUF_SIZE	1048576

typedef struct _Header {
	int32_t	len;
} __attribute__((__packed__)) Header;

#define ERROR_NOERROR					0
#define ERROR_OTHER						1
#define ERROR_MAINTENANCE				2
#define ERROR_VERSION					3
#define ERROR_LOAD						4
#define ERROR_FORMAT					5
#define ERROR_NO_USER					6
#define ERROR_USER_OFFLINE				7
#define ERROR_USER_ALREADY_EXIST		8
#define ERROR_NAME_EXIST				9
#define ERROR_ALREADY_LOGIN				10
#define ERROR_LOGIN_REQUIRED			11
#define	ERROR_WRONG_PASSWORD			12
#define	ERROR_WRONG_SMSCODE				13
#define ERROR_NO_USERDATA				14
#define ERROR_NO_PHONE					15
#define ERROR_PHONE_CHANGED				16
#define ERROR_NO_LANG					17
#define ERROR_PHONE_AWAITING			18
#define ERROR_TEMP_BLOCKED				19
#define ERROR_UNKOWN_CALL				20
#define ERROR_CALL_EXIST				21
#define ERROR_CALL_STATE				22
#define ERROR_BALANCE					23
#define ERROR_PHONECALL_ERROR			24
#define ERROR_PEER_DISCON				25
#define ERROR_RATING_ERROR				26
#define ERROR_PAYPAL_TRANSFER_ACTIVE	27
#define ERROR_PURCHASE_SIGNATURE		28
#define ERROR_ANOTHER_LOGIN				29
#define ERROR_ANOTHER_PHONE				30
#define ERROR_UNKNOWN_COUNTRY			31

#define SESSION_OS_IOS				"ios"
#define SESSION_OS_ANDROID			"android"
#define SESSION_OS_TIZEN			"tizen"

#define PHONE_STATUS_NONE		0
#define PHONE_STATUS_CONFIRMED	1
#define PHONE_STATUS_AWAIT		2

/* empty fields in xml are not allowed
 * Packet types:
S  C
 -> error(int code, command, [int langs_version, message, int id])
 -> error(int code, command="phonecall_request|phonecall_confirm", int client)
 -> error(int code, command="register_phone, resend_sms", int sms_sent_num, int sms_block_days, int sms_max_num)
 <- login(email, password, os, int version)
 <- register_user(email, bool is_translator)
 -> await_login_confirm
 <- reset_password(email)
 <- delete_user
// <- change_password(old_password)
 <- set_country(country)
 <- register_phone(phone, [bool user_input])
 -> await_phone_confirm
 <- resend_sms
 <- confirm_register_phone(sms_code)
 <- set_busy(bool busy)
 <- user_data(name, translate(lang, int price), [paypal_email])
 <- get_user_data
 -> user_data(bool is_translator, email, name, int phone_status, phone, await_phone, int balance, translate(lang, int price),
 	 	 	 	 	 [int rating{0-100}, int rating_num, paypal_email], sms_sent,
 	 	 	 	 	 options(int fee_market, int fee_app, int call_time_free_sec, int call_min_balance_sec, call_min_time_rating_sec, int sms_max, int sms_max_days, int active_tsearch))
 <- billing(int money, data, signature)
 <- paypal_transfer(int money)
 <- get_languages
 -> languages(langs_with_medium_prices_xml);
 <- request_translator_list(list_lang)
 -> translator_list(translator(bool delete, bool busy, int id, [name, translate(lang, int price), client_lang,
 	 	 	 	 	 int price, country, int rating{0-100}, int rating_num),
					bool await, bool confirmed, bool rejected, bool error, phone], translators)
 <- request_client_list
 -> client_list(client(int id, [name, client_lang, translate_lang, country, int balance, int price, bool confirmed, bool rejected, bool error], bool delete))
 <- stop_translator_list
 <- phonecall_request(int translator, translate_lang)
 -> phonecall_request(int client, name, translate_lang, client_lang, int price, country, int balance)
 <- phonecall_confirm(int client, bool accept)
 -> phonecall_confirm(int translator, phone, bool accept)
 -> phonecall_timeout(int client)
 -> phonecall_timeout(int translator)
 // peer_phone is required only if active=1, if active=0 peer_phone is not used
 // can signal busy because of any phonecall
 <- phonecall_status([peer_phone], bool active, int time)
 -> phonecall_status(int peer, int error, int price, int balance, bool active, int time, translate_lang, client_lang,
 	 	 	 	 translator_name, int cost)
 -> phonecall_status(bool active=0, int time=0)
 <- get_last_phonecall_status
 <- get_call_history
 -> call_history(call(int translator, name, lang, int price, int cost, int start, int length, client_country, translator_country))
 <- get_mark_history
 -> mark_history(translator(int id, name, int rating, int rating_num, bool deleted))
 -> mark_request(int translator, name, int date)
 <- mark_rating(int translator, int mark)
 <- get_statistic
 -> statistic(int clients, int translators, language_stat(string lang, int translators), int calls_hour, int users_hour)
 <- get_translator_statistic
 -> translator_statistic(int all_rating_num, int higher_rating_num, int money_sum, int call_num, int call_time_sum)
*/

#endif /* PROTOCOL_H_ */
