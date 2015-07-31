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
};

