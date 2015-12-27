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
bool wiThreadSafeManager::TRY_LOCK()
{
	return MUTEX.try_lock();
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