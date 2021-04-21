#pragma once
#include "CommonInclude.h"

#include <memory>
#include <functional>

static const int SYSTEM_EVENT_THREAD_SAFE_POINT = -1;
static const int SYSTEM_EVENT_RELOAD_SHADERS = -2;
static const int SYSTEM_EVENT_SET_VSYNC = -3;

namespace wiEvent
{
	struct Handle
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
	};

	Handle Subscribe(int id, std::function<void(uint64_t)> callback);
	void Subscribe_Once(int id, std::function<void(uint64_t)> callback);
	void FireEvent(int id, uint64_t userdata);


	// helper event wrappers can be placed below:
	inline void SetVSync(bool enabled)
	{
		wiEvent::FireEvent(SYSTEM_EVENT_SET_VSYNC, enabled ? 1ull : 0ull);
	}
}

