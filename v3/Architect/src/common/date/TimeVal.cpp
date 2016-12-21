#include <ctime>
#include <iostream>
using std::cout;
using std::endl;
#include <cassert>

#include "TimeVal.h"

const long int TimeVal::MICRO_SECONDS_LIMIT=1000000;
const long int TimeVal::HALF_A_SECOND=500000;

ostream & operator<<(ostream & out, const TimeVal & t){
	out<<t.time.tv_sec<<"sec : "<<t.time.tv_usec<<"micsec";
	return out;
}



TimeVal::TimeVal(long int seconds, long int microSeconds){
	setTime(seconds, microSeconds);

}
TimeVal::TimeVal(const struct timeval * t){
	setTime((long int)t->tv_sec, (long int)t->tv_usec);
}
TimeVal::TimeVal(const TimeVal &t){
	if(this!=&t){
		time.tv_sec=t.time.tv_sec;
		time.tv_usec=t.time.tv_usec;
	}
}
TimeVal & TimeVal::operator=(const TimeVal & t){
	if(this!=&t){
		time.tv_sec=t.time.tv_sec;
		time.tv_usec=t.time.tv_usec;
	}
	return *this;
}

TimeVal::~TimeVal()
{
}

void TimeVal::setTime(long int seconds, long int microSeconds){
	time.tv_sec=seconds;
	if(microSeconds>=TimeVal::MICRO_SECONDS_LIMIT){
		long overflow=microSeconds/TimeVal::MICRO_SECONDS_LIMIT;
		time.tv_sec+=overflow;
		time.tv_usec=microSeconds % TimeVal::MICRO_SECONDS_LIMIT;
	}else if(microSeconds<0){
		while(microSeconds<0){
			microSeconds+=TimeVal::MICRO_SECONDS_LIMIT;
			time.tv_sec--;
		}
		time.tv_usec=microSeconds;
	}else{
		time.tv_usec=microSeconds;
	}
	assert(time.tv_usec<TimeVal::MICRO_SECONDS_LIMIT);
	assert(time.tv_usec>=0);

}

long int TimeVal::getTimeMilliseconds()const{
	return time.tv_sec*1000 + time.tv_usec/1000;
}
int TimeVal::compare(const TimeVal &t)const{
	if(time.tv_sec>t.time.tv_sec){
		return 1;
	}else if(time.tv_sec<t.time.tv_sec){
		return -1;
	}else if(time.tv_usec>t.time.tv_usec){
		return 1;
	}else if(time.tv_usec<t.time.tv_usec){
		return -1;
	}else{
		return 0;
	}
}

void TimeVal::operator-=(const TimeVal &t){
	this->setTime((long int)(this->time.tv_sec-t.time.tv_sec), (long int)(this->time.tv_usec-t.time.tv_usec));
}
TimeVal TimeVal::operator-( const TimeVal &t)const{
	TimeVal tmp;
	//2.5-4.6=-2.-1 (OK)undefined
	//4.6-2.5=2.1 (OK)
	//2.7-4.6=-2.1 (OK)undefined
	//4.4-4.6=0.-2 (OK)undefined
	//4.4-2.5=2.-1(must be 1.9)
	tmp.setTime((long int)(this->time.tv_sec-t.time.tv_sec), (long int)(this->time.tv_usec-t.time.tv_usec));
	return tmp;
}
void TimeVal::operator+=(const TimeVal &t){
	this->setTime((long int)(this->time.tv_sec+t.time.tv_sec), (long int)(this->time.tv_usec+t.time.tv_usec));
}
TimeVal TimeVal::operator +(const TimeVal &t)const{
	TimeVal temp;
	temp.setTime((long int)(this->time.tv_sec+t.time.tv_sec), (long int)(this->time.tv_usec+t.time.tv_usec));
	return temp;

}
void TimeVal::operator/=( float denominator){
	long int sec=(long)((this->time).tv_sec / denominator);//whole part is in terms of seconds
	long int usec=(long)(((this->time).tv_sec / denominator-(double)sec)*(TimeVal::MICRO_SECONDS_LIMIT));
	usec+=(long)((this->time).tv_usec / denominator);
	if(usec>TimeVal::MICRO_SECONDS_LIMIT){
		sec++;
		usec-=TimeVal::MICRO_SECONDS_LIMIT;
	}
	(this->time).tv_sec=sec;
	(this->time).tv_usec=usec;
}
TimeVal TimeVal::operator/( float denominator)const{
	long int sec=(long)((this->time).tv_sec / denominator);//whole part is in terms of seconds
	long int usec=(long)(((this->time).tv_sec / denominator-(double)sec)*(TimeVal::MICRO_SECONDS_LIMIT));
	usec+=(long)((this->time).tv_usec / denominator);
	if(usec>TimeVal::MICRO_SECONDS_LIMIT){
		sec++;
		usec-=TimeVal::MICRO_SECONDS_LIMIT;
	}
	TimeVal tmp(sec,usec);
	return tmp;

}
void TimeVal::operator*=( float factor){

	long int sec_sec=(long)((this->time).tv_sec *factor);
	long int usec_sec=(long)((double)TimeVal::MICRO_SECONDS_LIMIT*(((this->time).tv_sec*factor)-sec_sec));
	long int sec_usec=(long)(((this->time).tv_usec *factor)/TimeVal::MICRO_SECONDS_LIMIT);
	long int usec_usec=(long)((this->time).tv_usec *factor)%TimeVal::MICRO_SECONDS_LIMIT;
	(this->time).tv_sec=sec_sec+sec_usec;
	(this->time).tv_usec=usec_sec+usec_usec;
	if((this->time).tv_usec>=TimeVal::MICRO_SECONDS_LIMIT){
		(this->time).tv_sec+=1;
		(this->time).tv_usec-=TimeVal::MICRO_SECONDS_LIMIT;
	}
}
TimeVal TimeVal::operator*( float factor)const{
	long int sec_sec=(long)((this->time).tv_sec *factor);
	long int usec_sec=(long)((double)TimeVal::MICRO_SECONDS_LIMIT*(((this->time).tv_sec*factor)-sec_sec));
	long int sec_usec=(long)(((this->time).tv_usec *factor)/TimeVal::MICRO_SECONDS_LIMIT);
	long int usec_usec=(long)((this->time).tv_usec *factor)%TimeVal::MICRO_SECONDS_LIMIT;
	sec_sec+=sec_usec;
	usec_sec+=usec_usec;
	if(usec_sec>=TimeVal::MICRO_SECONDS_LIMIT){
		sec_sec+=1;
		usec_sec-=TimeVal::MICRO_SECONDS_LIMIT;
	}
	TimeVal tmp(sec_sec, usec_sec);
	return tmp;

}
auto_ptr<TimeVal> TimeVal::getCurrentSystemTime(){
	struct timeval t;
	gettimeofday(&t,0);
	auto_ptr<TimeVal> sysTime(new TimeVal(&t));
	return sysTime;
}
auto_ptr<string> TimeVal::getHumanReadableTime()const{
	static char buff[24];
	struct tm* ptm;
	ptm = localtime (&(time.tv_sec));
	int usedBytes = strftime(buff, 24, "%Y-%m-%d, %H:%M:%S",ptm);
	auto_ptr<string> tmp(new string(buff, usedBytes));
	return tmp;
}

void TimeVal::test(){

	cout<<"TEST :TimeVal class STARTED"<<endl;

	TimeVal t0(4,400000);
	TimeVal t1(4,400000);
	TimeVal t2(2,800000);
	TimeVal t3(2,100000);

	assert(t3.compare(t1)==-1);
	assert(t3.compare(t2)==-1);
	assert(t2.compare(t3)==1);
	assert(t1.compare(t2)==1);
	assert(t0.compare(t1)==0);

	TimeVal t4=t1+t2;
	assert(TimeVal(7,200000)==t4);
	TimeVal t5=t1+t3;
	assert(TimeVal(6,500000)==t5);

	TimeVal t6=t2-t3;
	assert(TimeVal(0,700000)==t6);
	assert(t6.isPositive());
	assert(!t6.isZero());
	TimeVal t7=t1-t2;
	assert(TimeVal(1,600000)==t7);
	TimeVal t8=t3-t1;
	assert(TimeVal(-2,-300000)==t8);
	assert(t8.isUndefined());
	assert(t3.compare(t8)==1);
	TimeVal t9=t3-t2;
	assert(TimeVal(0,-700000)==t9);
	assert(t9.isUndefined());
	assert(!t9.isZero());
	assert(!t9.isPositive());

	t3-=t2;
	assert(TimeVal(0,-700000)==t3);
	assert(t3.isUndefined());
	assert(!t3.isZero());
	assert(!t3.isPositive());

	t1+=t3;
	assert(TimeVal(3,700000)==t1);

	cout<<"TEST :TimeVal class FINISHED"<<endl;



}
