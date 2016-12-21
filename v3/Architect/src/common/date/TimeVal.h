#ifndef TIMEVAL_H_
#define TIMEVAL_H_
#include <sys/time.h>
#include <memory>
using std::auto_ptr;
#include <string>
using std::string;
#include <iostream>
using std::ostream;
/**
 * Represents  time in terms of seconds and microseconds. Defines a number of arithmetic
 * methods and operators to be used with TimeVal. It represents time in two fields;seconds and microseconds.
 * The microseconds field is always positive. That is, when you initialize TimeVal object to "5 sec -400 usec"
 * it will automatically be set to "4 sec 600 usec" or when you set it to "0 sec -200 usec" it will re-set to
 * "-1 sec 800 usec". A timeValue is undefined when the sec part is negative, but still you can apply
 * arithmetic operators and they will be computed correctly. Such as (-1 sec 200 usec) + (5 sec 900 usec)
 * will result in (5 sec 100 usec)
 */
class TimeVal
{
public:
	static const long int MICRO_SECONDS_LIMIT;
	static const long int HALF_A_SECOND;
	friend ostream & operator<<(ostream & out, const TimeVal & time);
	static void test();
	static auto_ptr<TimeVal> getCurrentSystemTime();
	TimeVal(const struct timeval * t);
	TimeVal(const TimeVal &t);
	TimeVal(long int seconds=0, long int microSeconds=0);
	void setTime(long int seconds=0, long int microSeconds=0);
	TimeVal & operator=(const TimeVal & t);
	void operator+=(const TimeVal &t);
	void operator-=(const TimeVal &t);
	void operator/=( float denominator);
	void operator*=( float factor);
	TimeVal operator-(const TimeVal &t)const;
	TimeVal operator+(const TimeVal &t)const;
	TimeVal operator/( float denominator)const;
	TimeVal operator*( float factor)const;
	bool operator==(const TimeVal &t)const{return this->compare(t)==0;}
	bool operator!=(const TimeVal &t)const{return this->compare(t)!=0;}
	bool operator<(const TimeVal &t)const{return this->compare(t)==-1;}
	bool operator>(const TimeVal &t)const{return this->compare(t)==1;}
	bool operator<=(const TimeVal &t)const{return this->compare(t)!=1;}
	bool operator>=(const TimeVal &t)const{return this->compare(t)!=-1;}

	virtual ~TimeVal();
	long int getTimeMilliseconds()const;
	long int getSecondsPart()const{return time.tv_sec;}
	long int getMicroSecondsPart()const{return time.tv_usec;}
	void setMicroSecondsPart(long microSeconds);
	auto_ptr<string> getHumanReadableTime()const;
	bool isUndefined()const{return time.tv_sec<0;}
	bool isZero()const{return time.tv_sec==0 && time.tv_usec==0;}
	bool isPositive()const{return (!isUndefined() && !isZero());}
	void resetToZero(){time.tv_sec=0;time.tv_usec=0;}
	int compare(const TimeVal &t)const;
	struct timeval * getStructure() {return &time;}

private:

	struct timeval time;
};

#endif /*TIMEVAL_H_*/
