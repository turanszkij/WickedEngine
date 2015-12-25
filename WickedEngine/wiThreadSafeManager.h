#pragma once
#include "CommonInclude.h"

class wiThreadSafeManager
{
protected:
	mutex MUTEX;
	static mutex STATICMUTEX;
public:
	wiThreadSafeManager();
	~wiThreadSafeManager();

	void LOCK();
	void UNLOCK();

	static void LOCK_STATIC();
	static void UNLOCK_STATIC();
};

