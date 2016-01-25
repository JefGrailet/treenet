/*
 * StringUtils.h
 *
 *  Created on: Jul 22, 2008
 *      Author: root
 */

#ifndef STRINGUTILS_H_
#define STRINGUTILS_H_

#include <memory>
using std::auto_ptr;

#include <string>
using std::string;

#include <vector>
using std::vector;
#include <sstream>
using std::stringstream;
#include <iostream>
using std::cout;
using std::endl;

class StringUtils {
public:
	/**
	 * Java's String.split method, returns a vector of strings. The ownership of the vector
	 * passes to the caller. (i.e. caller must call delete after it has done with it)
	 * To split by a string source by the substring "the" just call
	 * StringUtils::splitString(source, "the");
	 */
	static vector<string> * splitString(const string &source, const string &pattern);
	static void trimString(string &source);
	static void test();
	// Some compilers gave error, that is why I have abondoned generics in implementation of those utiliy functions.
	static int string2int(const string &str);
	static unsigned int string2Uint(const string &str);
	static long string2long(const string &str);
	static unsigned long string2Ulong(const string &str);
	static long long string2longlong(const string &str);
	static unsigned long long string2Ulonglong(const string &str);
	static char string2char(const string &str);
	static unsigned char string2Uchar(const string &str);
	static double string2double(const string &str);

	static string int2string(int numeric);
	static string Uint2string(unsigned int numeric);
	static string long2string(long numeric);
	static string Ulong2string(unsigned long numeric);
	static string char2string(char numeric);
	static string Uchar2string(unsigned char numeric);
	static string longlong2string(long long numeric);
	static string Ulonglong2string(unsigned long long numeric);
	static string double2string(double numeric);


	static auto_ptr<string> toBinary(unsigned long number);
	StringUtils();
	virtual ~StringUtils();
};

#endif /* STRINGUTILS_H_ */
