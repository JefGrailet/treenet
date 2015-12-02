/*
 * Mutex.h
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#ifndef MUTEX_H_
#define MUTEX_H_
#include <pthread.h>
#include <string>
using std::string;
#include <iostream>
using std::ostream;

#include "MutexException.h"

class Mutex {
	friend class ConditionVariable;
public:
	static const int DEFAULT_MUTEX;
	static const int NORMAL_FAST_MUTEX;
	static const int ERROR_CHECKING_MUTEX;
	static const int RECURSIVE_MUTEX;
	friend ostream & operator<<(ostream &out, const Mutex &mut){
		out<<"Mutex object \""<<mut.mutexName<<"\" of type "<<mut.getTypeString();
		return out;
	}
	/**
	 * Initializes a mutex object. The @mutexType parameter tells the type
	 * of the mutex. If the parameter is not given the object is set to
	 * DEFAULT_MUTEX. To learn more about the behaviours of different types
	 * please seet the page :
	 * http://www.opengroup.org/onlinepubs/007908775/xsh/pthread_mutexattr_settype.html
	 *
	 * Mutexes of type RECURSIVE_MUTEX must NOT be used with Condition Variables.
	 */
	Mutex(int mutexType, string mutexName="ANONYMOUS") throw (MutexException);
	virtual ~Mutex();
	void lock() throw(MutexException);
	bool trylock() throw(MutexException);
	void unlock() throw(MutexException);
	int getType()const{return type;}
	string getName(){return mutexName;}
private:
	string getTypeString()const;
	pthread_mutex_t mutex;
	int type;
	string mutexName;
};
#endif /* MUTEX_H_ */
