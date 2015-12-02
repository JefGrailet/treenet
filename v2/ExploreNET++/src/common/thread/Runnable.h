#ifndef RUNNABLE_H_
#define RUNNABLE_H_

class Thread;

class Runnable
{
public:
	friend class Thread;
	Runnable();
	virtual ~Runnable();
	virtual void run()=0;
protected:
	/**
	 * Implemented to be called only within the run() method of a Runnable object
	 * It returns 0 until the enclosing thread calls start() method
	 * after the start() method call it returns the true thread id
	 */
	unsigned long getEnclosingThreadID();
	unsigned long enclosingThreadID;
};

#endif /*RUNNABLE_H_*/
