/*
 * ConditionVariable.h
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#ifndef CONDITIONVARIABLE_H_
#define CONDITIONVARIABLE_H_


#include "Mutex.h"
#include "MutexException.h"
#include "ConditionVariableException.h"
#include "TimedOutException.h"
#include "../date/TimeVal.h"

class ConditionVariable {
public:
	ConditionVariable(Mutex *conditionMutex=0)throw(ConditionVariableException, MutexException);
	virtual ~ConditionVariable();
	void lock() throw(MutexException){condMutex->lock();}
	bool trylock() throw(MutexException){return condMutex->trylock();}
	void unlock() throw(MutexException){condMutex->unlock();}
	/**
	 * wait() method causes the calling thread to be blocked (sleep) on the
	 * condition variable object until some other thread wakes up it later
	 * by calling signal() or broadcast().
	 *
	 * This method must be called after acquiring the lock by lock() function.
	 * typical usage is:
	 *
	 * 	1-	ConditionVariable cv;
	 * 	2-	cv.lock();
	 * 	3-	while(some condition is false){
	 * 	4-		cv.wait();
	 * 	5-	}
	 * 	6-	...
	 * 	7-	cv.unlock();
	 *
	 * Line 1 creates an object of condition variable.
	 * Line 2 acquires the lock before checking if "some condition is true/false"
	 * Line 4 causes the current thread to be blocked. Before blocking;
	 * 		 i) it implicitly releases the lock acquired at Line 2
	 * 		ii) if signal() or broadcast is called by another thread and this thread is
	 * 			to be waken up; again it implicitly acquires the lock that
	 * 			was released by calling wait(). and resumes from the line after wait().
	 * Line 7 explicitly releases the lock
	 */
	void wait()throw(ConditionVariableException);
	/**
	 * Exactly same as the wait() as in the calling thread blocks until some other thread
	 * calls signal() or broadcast() on the condition variable. However it differs from
	 * wait() as in after some amount of time designated by the time argument the thread
	 * automatically unblocks. again before returning and resuming from the first line
	 * following the wait(time) the lock is implicitly acquired. wait(TimeVal time) has
	 * the same semantics with wait().To distinguish between if the wait returns because
	 * of time out or signal/broadcast function calls; the method throws TimedOutException
	 * in case of timeout and returns normally in case of signal/broadcast. In both cases
	 * the lock is automatically acquired. So must be released later by the programmer.
	 * 	1-	ConditionVariable cv;
	 * 	2-	cv.lock();
	 * 	3-	while(some condition is false){
	 * 			try{
	 * 	4-			cv.wait(timeout_period);
	 * 			}catch (TimedOutException){
	 * 				...do stuff related to timeout
	 * 			}
	 * 	5-	}
	 * 	6-	...
	 * 	7-	cv.unlock();
	 *
	 * @argument period is in terms of milliseconds
	 */
	void wait(unsigned long int period)throw(ConditionVariableException,TimedOutException);
	/**
	 * signal() unblocks one thread among the threads that are already blocked
	 * --i.e. called wait() function-- on the condition variable object. The one
	 * that should be awakened is determined by the scheduling policy. To avoid
	 * infinite wait problem (see. http://www.cs.cf.ac.uk/Dave/C/node31.html) current
	 * thread must obtain the lock before calling signal(). Remember that the blocked
	 * thread will resume from the line just after the wait() call if and only if
	 * it implicitly acquires the lock. Hence, the signaling thread must also call unlock()
	 * in order to give resuming chance to the blocked thread.
	 * Typical usage is:
	 *
	 * 	1-	ConditionVariable cv;
	 * 	2-	cv.lock();
	 * 	3-	if(some condition has been met){
	 * 	5-		...
	 * 	6-		signal();
	 * 	7-		...
	 * 	8-	}
	 * 	9-	...
	 * 10-	cv.unlock();
	 *
	 * The signal() or broadcast() functions may be called by a thread whether or not it currently
	 * owns the mutex that threads calling wait() or wait(period) have associated with the
	 * condition variable during their waits; however, if predictable scheduling behavior is required,
	 * then that mutex shall be locked by the thread calling broadcast() or signal().
	 *
	 */
	void signal()throw(ConditionVariableException);
	/**
	 * Exactly same as signal() member-function except it unblocks all the threads that
	 * are currently waiting on the condition variable object.
	 */
	void broadcast()throw(ConditionVariableException);
private:
	Mutex *condMutex;
	bool internalAccessMutex;
	pthread_cond_t condVar;
};
#endif /* CONDITIONVARIABLE_H_ */
