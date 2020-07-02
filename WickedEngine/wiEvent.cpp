#include "wiEvent.h"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace wiEvent
{
	std::unordered_map<int, std::vector<std::function<void(uint64_t)>>> subscribers;
	std::mutex locker;

	void Subscribe(int id, std::function<void(uint64_t)> callback)
	{
		locker.lock();
		subscribers[id].push_back(callback);
		locker.unlock();
	}

	void FireEvent(int id, uint64_t userdata)
	{
		locker.lock();
		auto it = subscribers.find(id);
		if (it == subscribers.end())
		{
			locker.unlock();
			return;
		}
		auto& callbacks = it->second;
		for (auto& callback : callbacks)
		{
			callback(userdata);
		}
		locker.unlock();
	}
}
