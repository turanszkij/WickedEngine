#pragma once
#include "CommonInclude.h"

#include <memory>
#include <functional>

namespace wi::eventhandler
{
	inline constexpr int EVENT_THREAD_SAFE_POINT = -1;
	inline constexpr int EVENT_RELOAD_SHADERS = -2;
	inline constexpr int EVENT_SET_VSYNC = -3;

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
		FireEvent(EVENT_SET_VSYNC, enabled ? 1ull : 0ull);
	}
}
