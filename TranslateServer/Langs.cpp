/*
 * Langs.cpp
 *
 *  Created on: 08.05.2014
 *      Author: Raschupkin Roman
 */

#include "Langs.h"
#include "TranslateServer.h"
#include "Session.h"
#include "User.h"
#include <stdio.h>
vector<Lang *> Langs::AllLangs;
vector<LangGroup *> Langs::AllLangGroups;
int Langs::LangsVersion;
pthread_mutex_t Langs::alllangs_mutex = PTHREAD_MUTEX_INITIALIZER;
vector<string> Langs::AllCountries;

void Langs::print()
{
	for (int i=0; i<langs.size(); i++) {
		fprintf(stderr, langs[i].first.c_str());
		fprintf(stderr, "(%d),", langs[i].second);
	}
	fprintf(stderr, "\n");
}

void Langs::clear()
{
	lock();
	langs.clear();
	unlock();
}

void Langs::add(string lang, long price)
{
	if (!isLang(lang)) {
		log(LOG_VERBOSE, "Error Langs::add(): '%s' is not a language.", lang.c_str());
		return;
	}
	lock();
	for (size_t i=0; i<langs.size(); i++)
		if (!lang.compare(langs[i].first)) {
			unlock();
			return;
		}
	langs.push_back(make_pair(lang, price));
	unlock();
}

void Langs::remove(string lang)
{
	if (!isLang(lang))
		return;
	lock();
	for (size_t i=0; i<langs.size(); i++)
		if (!lang.compare(langs[i].first)) {
			langs.erase(langs.begin()+i);
		}
	unlock();
}

int Langs::get(string lang)
{
	lock();
	for (size_t i=0; i<langs.size(); i++)
		if (!lang.compare(langs[i].first)) {
			unlock();
			return i;
		}
	unlock();
	return -1;
}

string Langs::get(int p)
{
	string l = "";
	lock();
	if (p >= 0 && p < langs.size())
		l = langs[p].first;
	unlock();
	return l;
}

long Langs::getPrice(int p)
{
	long price = 0;
	lock();
	if (p >= 0 && p < langs.size())
		price = langs[p].second;
	unlock();
	return price;
}

string Langs::getBaseLang()
{
	string lang = LANG_DEFAULT;
	int price = 100000;
	for (int i=0; i<langs.size(); i++)
		if (langs[i].second < price)
			lang = langs[i].first;
	return lang;
}

long Langs::getPrice(string lang)
{
	return getPrice(get(lang));
}

bool Langs::isLang(string lang)
{
	AllLangsLock();
	for (size_t i=0; i<AllLangs.size(); i++)
		if (!lang.compare(AllLangs[i]->getCode())) {
			AllLangsUnlock();
			return true;
		}
	AllLangsUnlock();
	return false;
}

bool Langs::isCountry(string country)
{
	for (size_t i=0; i<AllCountries.size(); i++) {
//log(LOG_ERROR, "%d/%d:%s %s\t", i,AllCountries.size(), country.c_str(), AllCountries[i].c_str());
		if (!country.compare(AllCountries[i]))
			return true;
}
	return false;
}

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
using namespace xercesc;

// langs_lock must be locked;
int Langs::loadLangsXML(DOMDocument *doc, DOMNode *node, Connection *sqlCon, LangGroup *group,
		vector<Lang *> *_AllLangs, vector<LangGroup *> *_AllLangGroups)
{
	if (!node)
		return -1;
	do {
		char *nodeName = XMLString::transcode(node->getNodeName());
		char *nodeVal = XMLString::transcode(node->getNodeName());
		if (!strcasecmp(nodeName, "languages")) {
			if (node->getNodeType() == DOMNode::TEXT_NODE)
				continue;
			return loadLangsXML(doc, node->getFirstChild(), sqlCon, 0, _AllLangs, _AllLangGroups);
		} else if (!strcasecmp(nodeName, "lang")) {
			string code, name, native, country, iso3;
			DOMNode *lnode = node->getFirstChild();
			while (lnode) {
				char *lnodeName = XMLString::transcode(lnode->getNodeName());
				if (lnodeName && lnode->getFirstChild()) {
					if (lnode->getNodeType() == DOMNode::TEXT_NODE) {
						XMLString::release(&lnodeName);
						continue;
					}
					char *lnodeValue = XMLString::transcode(lnode->getFirstChild()->getNodeValue());
					if (lnodeValue) {
						if (!strcasecmp(lnodeName, "code"))
							code = lnodeValue;
						else if (!strcasecmp(lnodeName, "name"))
							name = lnodeValue;
						else if (!strcasecmp(lnodeName, "native"))
							native = lnodeValue;
						else if (!strcasecmp(lnodeName, "country"))
							country = lnodeValue;
						else if (!strcasecmp(lnodeName, "iso3"))
							iso3 = lnodeValue;
						else {
							log(LOG_ERROR, "Unknown tag %s.", lnodeName);
							return -1;
						}
						XMLString::release(&lnodeValue);
					}
				}
				XMLString::release(&lnodeName);
				lnode = lnode->getNextSibling();
			}

//fprintf(stderr,"lang: %s %s %s\n",code.c_str(),name.c_str(),native.c_str());
			if (!code.length() || !name.length() || !native.length()) {
				log(LOG_ERROR, "Missing tag in element: code='%s', name='%s'.", code.c_str(), name.c_str());
				return -1;
			}
			_AllLangs->push_back(new Lang(code, name, native, country, group));

// counting AveragePrice for language
			DOMElement *priceNode = doc->createElement(XMLString::transcode("avg_price"));
			int avg_price = User::CalcAveragePrice(sqlCon, code);
			DOMText *priceTextNode = doc->createTextNode(XMLString::transcode((std::to_string(avg_price)).c_str()));
			priceNode->appendChild(priceTextNode);
			node->appendChild(priceNode);

		} else if (!strcasecmp(nodeName, "group")) {
			XMLCh *attrName = XMLString::transcode("name");
			DOMNode *attr = node->getAttributes()->getNamedItem(attrName);
			if (!attr) {
				log(LOG_ERROR, "No 'name' attribute in group description.");
				return -1;
			}
			char *nodeValue = XMLString::transcode(attr->getNodeValue());
			if (nodeValue) {
				string groupName = nodeValue;
				LangGroup *g = new LangGroup(groupName.c_str(), group);
				_AllLangGroups->push_back(g);
				XMLString::release(&nodeValue);
				loadLangsXML(doc, node->getFirstChild(), sqlCon, g, _AllLangs, _AllLangGroups);
			}
			XMLString::release(&attrName);
		}  else if (!strcasecmp(nodeName, "version")) {
			DOMNode *vnode = node->getFirstChild();
			if (!vnode)
				continue;
			if (vnode->getNodeType() == DOMNode::TEXT_NODE) {
				char *vnodeValue = XMLString::transcode(vnode->getNodeValue());
				if (vnodeValue) {
					int version;
					if (sscanf(vnodeValue, "%d", &version) > 0)
						LangsVersion = version;
					XMLString::release(&vnodeValue);
				}
			}
		} else
			;//return -1;
		XMLString::release(&nodeName);
	} while (node = node->getNextSibling());
	return 0;
}

void Langs::FreeAllLangs()
{
	for (int i=0; i<AllLangs.size(); i++)
		delete AllLangs[i];
	for (int i=0; i<AllLangGroups.size(); i++)
		delete AllLangGroups[i];
}

// langs_lock must be locked;
string Langs::loadLangsFile(const char *xml, Connection *sqlCon)
{
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
    DOMLSParser *parser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
    DOMDocument *doc = parser->parseURI(xml);
	if (!doc) {
//	    delete parser;
		return "";
	}
    DOMNode *rootNode = doc->getDocumentElement();
    if (!rootNode) {
    	delete doc;
//        delete parser;
    	return "";
    }

    vector<Lang *> _AllLangs;
    vector<LangGroup *> _AllLangGroups;
    if (loadLangsXML(doc, rootNode, sqlCon, 0, &_AllLangs, &_AllLangGroups)) {
    	delete doc;
//        delete parser;
    	return "";
    }
    FreeAllLangs();
    AllLangs = _AllLangs;
    AllLangGroups = _AllLangGroups;
    DOMLSSerializer *serializer = impl->createLSSerializer();
    string LangsWithPrice = XMLString::transcode(serializer->writeToString(rootNode));
    delete doc;
    delete serializer;
//    delete parser;
    //parser->release();
    //delete impl;
    return LangsWithPrice;
}

int Langs::initCountries()
{
	AllCountries.push_back("ru");
	AllCountries.push_back(COUNTRY_UNKNOWN);
	return 0;
}

Langs::Langs()
{
	pthread_mutex_init(&mutex, 0);
}

Langs::~Langs()
{
	pthread_mutex_destroy(&mutex);
}
