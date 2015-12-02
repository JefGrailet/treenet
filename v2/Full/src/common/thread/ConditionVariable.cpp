/*
 * ConditionVariable.cpp
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#include <errno.h>
#include <ctime>
#include <iostream>
using std::cout;
using std::endl;

#include "ConditionVariable.h"

ConditionVariable::ConditionVariable(Mutex *conditionMutex)throw(ConditionVariableException, MutexException):
condMutex(conditionMutex),
internalAccessMutex(false)
{
	if(condMutex!=0){
		if(condMutex->getType()==Mutex::RECURSIVE_MUTEX){
			throw ConditionVariableException("Condition Variables are supposed to have access mutex object that is NOT of type Mutex::RECURSIVE_MUTEX");
		}
	}else{//create an internal mutex object
		condMutex=new Mutex(Mutex::ERROR_CHECKING_MUTEX);
		internalAccessMutex=true;
	}
	if(pthread_cond_init(&condVar,NULL)!=0){
		throw ConditionVariableException("Can NOT initialize condition variable object");
	}

}

ConditionVariable::~ConditionVariable() {
	pthread_cond_destroy(&condVar);
	if(internalAccessMutex){
		delete condMutex;
	}
}

void ConditionVariable::wait()throw(ConditionVariableException){
	int result=pthread_cond_wait(&condVar, &(condMutex->mutex));
	if(result!=0){
		if(result==EPERM){
			throw ConditionVariableException(string("Can NOT wait() on the condition variable because current thread does NOT own the mutex named")+condMutex->getName()+ string("(to solve problem call lock() method before calling wait())"));
		}else{
			throw ConditionVariableException(string("Can NOT wait() on the condition variable because either condition object or mutex object named ")+condMutex->getName()+ string("is invalid or different mutexes were supplied for concurrent pthread_cond_wait()"));
		}
	}
}

void ConditionVariable::wait(unsigned long int period)throw(ConditionVariableException,TimedOutException){
	struct timeval now;
	struct timespec abstimeout;
	/************START DONT ADD EXTRA CODE IN BETWEEN ***************/
	/**
	 * pthread_cond_timedwait(...) method takes absolute time so we need to take current time
	 * add the period to the current time to calculate abstimeout but these operations take
	 * some time as well so we may set a late absolute time if the system is heavily loaded
	 * which causes it to return with EINVAL
	 */
	gettimeofday(&now, NULL);
	abstimeout.tv_sec = now.tv_sec + period/1000 ;
	abstimeout.tv_nsec = now.tv_usec * 1000 + (period%1000)*1000000;//extra 200ms added

	int result=pthread_cond_timedwait(&condVar, &(condMutex->mutex), &abstimeout);
	/************END DONT ADD EXTRA CODE IN BETWEEN ***************/
	/*After some thread calls signal(), the thread called wait() will automatically acquire the lock*/
	if(result!=0){
		if(result==ETIMEDOUT){//the call returns because of timeout, again the calling thread automatically acquires the lock
			throw TimedOutException();
		}else if(result==EPERM){
			throw ConditionVariableException(string("Can NOT wait(period) on the condition variable because current thread does NOT own the mutex")+condMutex->getName()+ string("(Call lock() method before calling wait(time_period)"));
		}else{
			throw ConditionVariableException(string("Can NOT wait(period) on the condition variable because either condition object or mutex object")+condMutex->getName()+ string("is invalid or different mutexes were supplied for concurrent pthread_cond_timedwait()"));
		}
	}
}

void ConditionVariable::signal()throw(ConditionVariableException){
	if(pthread_cond_signal(&condVar)!=0){
		throw ConditionVariableException("Can NOT signal() on the condition variable");
	}
}
void ConditionVariable::broadcast()throw(ConditionVariableException){
	if(pthread_cond_broadcast(&condVar)!=0){
		throw ConditionVariableException("Can NOT broadcast() on the condition variable");
	}
}
