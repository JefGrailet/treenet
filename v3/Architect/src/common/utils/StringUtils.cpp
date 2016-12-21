/*
 * StringUtils.cpp
 *
 *  Created on: Jul 22, 2008
 *      Author: root
 */

#include <cstdlib>
#include <sstream>


#include "StringUtils.h"
#include <cassert>


void StringUtils::trimString(string &source){
	const string whiteSpaceChars=" \t\n\r";
	//remove heading space
	size_t pos=source.find_first_not_of(whiteSpaceChars);
	if(pos!=string::npos){
		source.erase(0, pos);
	}

	pos=source.find_last_not_of(whiteSpaceChars);
	if(pos!=string::npos){
		source.erase(pos+1);
	}
}
vector<string> * StringUtils::splitString(const string &source, const string &pattern){
	vector<string> *v=0;
	string::size_type searchAllign=0;
	string::size_type loc=string::npos;
	do{
		loc=source.find(pattern,searchAllign);
		if(loc!=string::npos){
			if(v==0){
				v=new vector<string>();
				if(loc==0){//avoid adding an  empty string to the vector if source immediately starts with pattern
					searchAllign=loc+pattern.size();
					continue;
				}
			}
			(*v).push_back(source.substr(searchAllign,loc-searchAllign));
			searchAllign=loc+pattern.size();

		}
	}while(searchAllign<source.size() && loc!=string::npos);

	if(searchAllign<source.size()){
		(*v).push_back(source.substr(searchAllign));
	}
	return v;
}


int StringUtils::string2int(const string &str){
	std::stringstream out;
	out<<str;
	int tmp=0;
	out>>tmp;
	return tmp;
}
unsigned int StringUtils::string2Uint(const string &str){
	std::stringstream out;
	out<<str;
	unsigned int tmp=0;
	out>>tmp;
	return tmp;
}
long StringUtils::string2long(const string &str){
	std::stringstream out;
	out<<str;
	long tmp=0;
	out>>tmp;
	return tmp;
}
unsigned long StringUtils::string2Ulong(const string &str){
	std::stringstream out;
	out<<str;
	unsigned long tmp=0;
	out>>tmp;
	return tmp;
}
long long StringUtils::string2longlong(const string &str){
	std::stringstream out;
	out<<str;
	long long tmp=0;
	out>>tmp;
	return tmp;
}
unsigned long long StringUtils::string2Ulonglong(const string &str){
	std::stringstream out;
	out<<str;
	unsigned long long tmp=0;
	out>>tmp;
	return tmp;
}
char StringUtils::string2char(const string &str){
	return (char)string2Uint(str);
}
double StringUtils::string2double(const string &str){
	std::stringstream out;
	out<<str;
	double tmp=0.0;
	out>>tmp;
	return tmp;
}
unsigned char StringUtils::string2Uchar(const string &str){
	return (unsigned char)string2Uint(str);
}

string StringUtils::int2string(int numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}
string StringUtils::Uint2string(unsigned int numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}
string StringUtils::long2string(long numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}
string StringUtils::Ulong2string(unsigned long numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}
string StringUtils::longlong2string(long long numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}
string StringUtils::Ulonglong2string(unsigned long long numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}
string StringUtils::char2string(char numeric){
	std::stringstream out;
	out<<(unsigned int)numeric;
	return out.str();
}
string StringUtils::Uchar2string(unsigned char numeric){
	std::stringstream out;
	out<<(unsigned int)numeric;
	return out.str();
}

string StringUtils::double2string(double numeric){
	std::stringstream out;
	out<<numeric;
	return out.str();
}


void StringUtils::test(){
	char c=65;
	string cStr="65";
	unsigned char uc=99;
	string ucStr="99";
	int i=-4555;
	string iStr="-4555";
	unsigned int ui=4555;
	string uiStr="4555";
	long l=-6666;
	string lStr="-6666";
	unsigned long ul=6666;
	string ulStr="6666";

	unsigned char xuc=StringUtils::string2Uchar(ucStr);
	cout<<"xuc="<<(int)xuc<<endl;
	cout<<"uc="<<(int)uc<<endl;
	if(uc==xuc){
		cout<<"MATCH"<<endl;
	}else{
		cout<<"NO MATCH"<<endl;
	}


	assert(c==StringUtils::string2char(cStr));
	assert(uc==StringUtils::string2Uchar(ucStr));
	assert(i==StringUtils::string2int(iStr));
	assert(ui==StringUtils::string2Uint(uiStr));
	assert(l==StringUtils::string2long(lStr));
	assert(ul==StringUtils::string2Ulong(ulStr));

	assert(cStr==StringUtils::char2string(c));
	assert(ucStr==StringUtils::Uchar2string(uc));
	assert(iStr==StringUtils::int2string(i));
	assert(uiStr==StringUtils::Uint2string(ui));
	assert(lStr==StringUtils::long2string(l));
	assert(ulStr==StringUtils::Ulong2string(ul));


}


auto_ptr<string> StringUtils::toBinary(unsigned long number){
	int bitSize=sizeof(unsigned long)*8;
	unsigned long tmp=1;
	tmp=tmp<<(bitSize-1);
	std::stringstream out;
	while(tmp>0){
		out<<((number&tmp)>0?"1":"0");
		tmp=tmp>>1;
	}
	auto_ptr<string> str(new string(out.str()));
	return str;
}

StringUtils::StringUtils() {
	// TODO Auto-generated constructor stub

}

StringUtils::~StringUtils() {
	// TODO Auto-generated destructor stub
}
