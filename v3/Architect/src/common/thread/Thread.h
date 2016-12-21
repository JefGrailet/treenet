#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <iostream>
using std::ostream;
#include <sstream>
using std::stringstream;

#include "ThreadException.h"
#include "../date/TimeVal.h"

#include "Runnable.h"

class Thread
{
public:
	friend ostream &operator<<(ostream &out, const Thread &thread)
	{
		out << "Thread-" << thread.threadId;
		return out;
	}
	static const Thread currentThread();
	/**
	 * Sleeps for the given amount of time if time.isPositive() returns true otherwise returns immediately.
	 */
	static void invokeSleep(const TimeVal &time);
	Thread(Runnable *runnable);
	virtual ~Thread();
	void join() throw(ThreadException);
	void start() throw(ThreadException);
	Runnable *getRunnable() { return runnable; }
	unsigned long getThreadID() const { return (unsigned long) threadId; }
private:
	Thread(pthread_t threadId);
	static void *concurrentEntry(void*);
	Runnable *runnable;
	bool alive;
	pthread_t threadId;
};

#endif /*THREAD_H_*/
