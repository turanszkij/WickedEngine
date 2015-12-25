#include "wiThreadSafeManager.h"

mutex wiThreadSafeManager::STATICMUTEX;

wiThreadSafeManager::wiThreadSafeManager()
{
}


wiThreadSafeManager::~wiThreadSafeManager()
{
}


void wiThreadSafeManager::LOCK()
{
	MUTEX.lock();
}
void wiThreadSafeManager::UNLOCK()
{
	MUTEX.unlock();
}

void wiThreadSafeManager::LOCK_STATIC()
{
	STATICMUTEX.lock();
}
void wiThreadSafeManager::UNLOCK_STATIC()
{
	STATICMUTEX.unlock();
}