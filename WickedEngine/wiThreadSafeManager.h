#pragma once
#include "CommonInclude.h"
#include "wiSpinLock.h"

#include <mutex>

class wiThreadSafeManager
{
protected:
	//mutex MUTEX;
	static std::mutex STATICMUTEX;
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

