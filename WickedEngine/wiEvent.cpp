#include "wiEvent.h"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace wiEvent
{
	struct EventManager
	{
		std::unordered_map<int, std::vector<std::function<void(uint64_t)>>> subscribers;
		std::mutex locker;
	};
	std::shared_ptr<EventManager> manager = std::make_shared<EventManager>();

	struct EventInternal
	{
		std::shared_ptr<EventManager> manager;
		int id = 0;
		int index = -1;

		~EventInternal()
		{
			auto it = manager->subscribers.find(id);
			if (it != manager->subscribers.end() && it->second.size() > index)
			{
				it->second.erase(it->second.begin() + index);
			}
		}
	};

	Handle Subscribe(int id, std::function<void(uint64_t)> callback)
	{
		Handle handle;
		auto eventinternal = std::make_shared<EventInternal>();
		handle.internal_state = eventinternal;
		eventinternal->manager = manager;
		eventinternal->id = id;

		manager->locker.lock();
		eventinternal->index = (int)manager->subscribers[id].size();
		manager->subscribers[id].push_back(callback);
		manager->locker.unlock();

		return handle;
	}

	void FireEvent(int id, uint64_t userdata)
	{
		manager->locker.lock();
		auto it = manager->subscribers.find(id);
		if (it == manager->subscribers.end())
		{
			manager->locker.unlock();
			return;
		}
		auto& callbacks = it->second;
		for (auto& callback : callbacks)
		{
			callback(userdata);
		}
		manager->locker.unlock();
	}
}
