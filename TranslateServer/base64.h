/*
 * base64.h
 *
 *  Created on: 30.05.2015
 *      Author: roman
 */

#ifndef BASE64_H_
#define BASE64_H_

#include <string>
::std::string base64_encode(const ::std::string &bindata);
::std::string base64_decode(const ::std::string &ascdata);

#endif /* BASE64_H_ */
