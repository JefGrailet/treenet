#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
using std::cout;
using std::endl;



#include "Thread.h"


const Thread Thread::currentThread(){
	const Thread self(pthread_self ());
	return self;
}

/**
 * If the main thread terminates before the thread on which invokeSleep
 * is executed, the thread will immediately exits from sleep and terminates,
 * unless the main thread is joined to the thread on which invokeSleep is
 * executed via join() method
 */
void Thread::invokeSleep(const TimeVal &time){
	if(time.isPositive()){
		if(time.getSecondsPart()>0){
			sleep((unsigned int)time.getSecondsPart());
		}
		if(time.getMicroSecondsPart()>0){
			//usleep can only accepts arguments up to and equal 999999
			usleep(time.getMicroSecondsPart());
		}
	}
}

Thread::Thread(Runnable *runnableObj):
runnable(runnableObj),
alive(false),
threadId(runnableObj->enclosingThreadID)
{
	if(runnableObj==0){
		this->threadId = 0;
	}else{
		this->threadId = runnableObj->enclosingThreadID;
	}
}
Thread::Thread(pthread_t id):
runnable(0),
alive(false),
threadId(id)
{

}

Thread::~Thread()
{
    if(runnable != NULL)
        delete runnable;
}

void Thread::join() throw(ThreadException){
	int rc=pthread_join(threadId, 0);
    if (rc!=0){
    	if(rc==EINVAL){
    		throw ThreadException("The Thread object that the current thread wants to join is NOT joinable");
    	}else if(rc==ESRCH){
    		throw ThreadException("The Thread object that the current thread wants to join can NOT be found");
    	}else if(rc==EDEADLK){
    		throw ThreadException("Can NOT successfully join because either there is a deadlock or current thread wants to join itself");
    	}else{
    		throw ThreadException("Can NOT successfully join");
    	}
	}

}
void Thread::start() throw(ThreadException){
	/*Define attributes to make thread joinable explicitly*/
	int result = -1;

	pthread_attr_t attr;
	result=pthread_attr_init(&attr);
	if(result!=0){
		if(result==ENOMEM){
			throw ThreadException("Can NOT initialize pthread_attr_t object because of insufficient memory exists to initialise the thread attributes object");
		}else{
			throw ThreadException("Can NOT create the thread because of problems in initializing pthread_attr_t object");
		}
	}
	result=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if(result!=0){
		if(result==EINVAL){
			throw ThreadException("Can NOT set to pthread_attr_setdetachstate object because the value of detach state was not valid");
		}else{
			throw ThreadException("Can NOT create the thread because of problems in setting detach state");
		}
	}


	//returns 0 if a thread has created successfully
	result = pthread_create(&threadId, &attr, Thread::concurrentEntry, this->runnable);
	if(result!=0){
		if(result==EAGAIN){
			throw ThreadException("The system lacks necessary resources to create a thread or PTHREAD_THREADS_MAX is exceeded");
		}else if(result==EINVAL){
			throw ThreadException("The attribute provided to the pthread_create() function is NOT valid");
		}else if(result==EPERM){
			throw ThreadException("The caller does not have appropriate permission to set the required scheduling parameters or scheduling policy");
		}else{
			throw ThreadException("Can NOT create the thread");
		}
	}else{
		alive=true;
		runnable->enclosingThreadID = (unsigned long)threadId;
	}
	pthread_attr_destroy(&attr);
}
void * Thread::concurrentEntry(void *runnableObj){
	if(runnableObj!=0){
		Runnable *runnable=(Runnable *)(runnableObj);
		runnable->run();
	}
	pthread_exit(0);
	return 0; //to make compiler happy
}
