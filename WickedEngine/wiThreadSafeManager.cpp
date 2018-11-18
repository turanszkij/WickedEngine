#include "wiThreadSafeManager.h"

using namespace std;

mutex wiThreadSafeManager::STATICMUTEX;


void wiThreadSafeManager::LOCK()
{
	lock.lock();
}
bool wiThreadSafeManager::TRY_LOCK()
{
	return lock.try_lock();
}
void wiThreadSafeManager::UNLOCK()
{
	lock.unlock();
}

void wiThreadSafeManager::LOCK_STATIC()
{
	STATICMUTEX.lock();
}
void wiThreadSafeManager::UNLOCK_STATIC()
{
	STATICMUTEX.unlock();
}