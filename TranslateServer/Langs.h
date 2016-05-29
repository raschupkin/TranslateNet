/*
 * lang.h
 *
 *  Created on: 04.05.2014
 *      Author: Raschupkin Roman
 */

#ifndef LANG_H_
#define LANG_H_
#include <vector>
#include <string>
#include <xercesc/dom/DOM.hpp>
#include <cppconn/connection.h>
using namespace std;

#define MAX_LANG			2+1	// ISO 639-2/B
#define MAX_COUNTRY			2+1	// ISO-3166-1(Alpha-3)
#define COUNTRY_UNKNOWN		"--"
#define LANG_DEFAULT		"en"

class LangGroup {
public:
	LangGroup(string _name, LangGroup *parent): name(_name), group(parent)	{}
	string getName()		{	return name;					}
	LangGroup *getGroup()	{	return group;					}
private:
	string name;
	LangGroup *group;
};

class Lang {
public:
	Lang(string _code, string _name, string _native, string _country, LangGroup *parent)
		: code(_code), name(_name), group(parent), native(_native), country(_country)	{}
	string getName()		{	return name;					}
	string getCode()		{	return code;					}
	LangGroup *getGroup()	{	return group;					}
private:
	string code;
	string name;
	string country;
	string native;
	LangGroup *group;
};

class Langs {
public:
	void print();
	Langs();
	~Langs();
	void lock()					{	pthread_mutex_lock(&mutex);		}
	void unlock()				{	pthread_mutex_unlock(&mutex);	}
	void add(string lang, long price);
	void remove(string lang);
	size_t num()				{	return langs.size();			}
	string get(int p);
	int get(string lang);
	long getPrice(int p);
	long getPrice(string lang);
	string getBaseLang();
	void clear();
	static bool isLang(string lang);
	static bool isCountry(string country);

	static int getLangsVersion()	{	return LangsVersion;			}
	static int loadLangsXML(xercesc::DOMDocument *doc, xercesc::DOMNode *node, sql::Connection *sqlCon, LangGroup *group,
			vector<Lang *> *_AllLangs, vector<LangGroup *> *_AllLangGroups);
	static string loadLangsFile(const char *xml, sql::Connection *sqlCon);
	static void FreeAllLangs();
	static void AllLangsLock()				{	pthread_mutex_lock(&Langs::alllangs_mutex);			}
	static void AllLangsUnlock()			{	pthread_mutex_unlock(&Langs::alllangs_mutex);		}
	static int initCountries();
private:
	static pthread_mutex_t alllangs_mutex;
	static vector<Lang *> AllLangs;						// AllLangsLock/AllLangsUnlock
	static vector<LangGroup *> AllLangGroups;			// AllLangsLock/AllLangsUnlock
	static vector<string> AllCountries;
	static int LangsVersion;
	vector<pair<string,long>> langs;
	pthread_mutex_t mutex;
};

#endif /* LANG_H_ */
