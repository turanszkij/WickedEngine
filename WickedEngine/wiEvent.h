#pragma once
#include "CommonInclude.h"

#include <functional>

static const int SYSTEM_EVENT_RELOAD_SHADERS = -1;
static const int SYSTEM_EVENT_CHANGE_RESOLUTION = -2;
static const int SYSTEM_EVENT_CHANGE_DPI = -3;

namespace wiEvent
{
	void Subscribe(int id, std::function<void(uint64_t)> callback);
	void FireEvent(int id, uint64_t userdata);
}
