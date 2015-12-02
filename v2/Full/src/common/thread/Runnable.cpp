#include "Runnable.h"

Runnable::Runnable():
enclosingThreadID(0)
{
}

Runnable::~Runnable()
{
}

unsigned long Runnable::getEnclosingThreadID(){
	return enclosingThreadID;
}
