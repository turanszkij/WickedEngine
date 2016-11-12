#pragma once
#include "CommonInclude.h"
#include "wiSpinLock.h"

class wiThreadSafeManager
{
protected:
	//mutex MUTEX;
	static mutex STATICMUTEX;
	wiSpinLock spinlock;
public:
	wiThreadSafeManager();
	~wiThreadSafeManager();

	void LOCK();
	bool TRY_LOCK();
	void UNLOCK();

	static void LOCK_STATIC();
	static void UNLOCK_STATIC();
};

