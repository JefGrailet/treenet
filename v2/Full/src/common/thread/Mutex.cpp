/*
 * Mutex.cpp
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */
#include "Mutex.h"
#include <errno.h>

const int Mutex::DEFAULT_MUTEX=1;
const int Mutex::NORMAL_FAST_MUTEX=2;
const int Mutex::ERROR_CHECKING_MUTEX=3;
const int Mutex::RECURSIVE_MUTEX=4;


Mutex::Mutex(int mutexType, string name)throw (MutexException):
type(mutexType),
mutexName(name)
{
	pthread_mutexattr_t attr;
	try{
		if(pthread_mutexattr_init(&attr)!=0){
			throw MutexException(string("Can NOT initialize mutex attribute object for the mutex named ")+mutexName);
		}
		switch(type){
		case Mutex::NORMAL_FAST_MUTEX:
			if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL)!=0){
				throw MutexException(string("Can NOT set mutex type to \"PTHREAD_MUTEX_NORMAL\" for the mutex named ")+mutexName);
			}
			break;
		case Mutex::ERROR_CHECKING_MUTEX:
			if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)!=0){
				throw MutexException(string("Can NOT set mutex type to \"PTHREAD_MUTEX_ERRORCHECK\" for the mutex named ")+mutexName);
			}
			break;
		case Mutex::RECURSIVE_MUTEX:
			if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)!=0){
				throw MutexException(string("Can NOT set mutex type to \"PTHREAD_MUTEX_RECURSIVE\" for the mutex named ")+mutexName);
			}
			break;
		case Mutex::DEFAULT_MUTEX:
		default:
			if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT)!=0){
				throw MutexException(string("Can NOT set mutex type to \"PTHREAD_MUTEX_DEFAULT\" for the mutex named ")+mutexName);
			}
			break;
		}
		if(pthread_mutex_init(&mutex, &attr)!=0){
			throw MutexException(string("Can NOT initialize the mutex object for the mutex named ")+mutexName);
		}
	}catch(MutexException &e){
		pthread_mutexattr_destroy(&attr);
		pthread_mutex_destroy(&mutex);
		throw;
	}

	pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex() {
	pthread_mutex_destroy(&mutex);
}

void Mutex::lock() throw(MutexException){
	int result=pthread_mutex_lock(&mutex);
	if(result!=0){//error
		if(result==EINVAL){
			throw MutexException(string("Can NOT lock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because it does not refer to an initialised mutex object"));
		}else if(result==EAGAIN){
			throw MutexException(string("Can NOT lock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because the maximum number of recursive locks has been exceeded"));
		}else if(result==EDEADLK){
			throw MutexException(string("Can NOT lock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because current thread already owns the mutex"));
		}else{
			throw MutexException(string("Can NOT lock the mutex object named ")+mutexName+string(" of type ")+getTypeString());
		}
	}
}
bool Mutex::trylock() throw(MutexException){
	bool acquired=false;
	int result=pthread_mutex_trylock(&mutex);
	if(result==0){//successful
		acquired=true;
	}else if(result==EBUSY){
		acquired=false;
	}else{//error occured
		if(result==EINVAL){
			throw MutexException(string("Can NOT try-lock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because it does not refer to an initialised mutex object"));
		}else if(result==EAGAIN){
			throw MutexException(string("Can NOT try-lock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because the maximum number of recursive locks has been exceeded"));
		}else{
			throw MutexException(string("Can NOT try-lock the mutex object named ")+mutexName+string(" of type ")+getTypeString());
		}
	}
	return acquired;
}
void Mutex::unlock() throw(MutexException){
	int result=pthread_mutex_unlock(&mutex);
	if(result!=0){//error
		if(result==EINVAL){
			throw MutexException(string("Can NOT unlock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because it does not refer to an initialised mutex object"));
		}else if(result==EAGAIN){
			throw MutexException(string("Can NOT unlock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because the maximum number of recursive locks has been exceeded"));
		}else if(result==EPERM){
			throw MutexException(string("Can NOT unlock the mutex object named ")+mutexName+string(" of type ")+getTypeString()+string(" because the current thread does not own the mutex"));
		}else{
			throw MutexException(string("Can NOT unlock the mutex object named ")+mutexName+string(" of type ")+getTypeString());
		}
	}
}

string Mutex::getTypeString()const{
	switch(type){
	case Mutex::NORMAL_FAST_MUTEX:
		return "NORMAL_FAST_MUTEX";
		break;
	case Mutex::ERROR_CHECKING_MUTEX:
		return "ERROR_CHECKING_MUTEX";
		break;
	case Mutex::RECURSIVE_MUTEX:
		return "RECURSIVE_MUTEX";
		break;
	case Mutex::DEFAULT_MUTEX:
		return "DEFAULT_MUTEX";
		break;
	default:
		return "UNKNOWN_MUTEX_TYPE";
	}
}
