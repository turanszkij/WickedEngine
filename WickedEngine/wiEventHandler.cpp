#include "wiEventHandler.h"
#include "wiUnorderedMap.h"
#include "wiVector.h"

#include <list>
#include <mutex>

namespace wi::eventhandler
{
	struct EventManager
	{
		// Note: list is used because events can also add events from within and that mustn't cause invalidation like vector
		wi::unordered_map<int, std::list<std::function<void(uint64_t)>*>> subscribers;
		wi::unordered_map<int, std::list<std::function<void(uint64_t)>>> subscribers_once;
		std::mutex locker;
	};
	std::shared_ptr<EventManager> manager = std::make_shared<EventManager>();

	struct EventInternal
	{
		std::shared_ptr<EventManager> manager;
		int id = 0;
		std::function<void(uint64_t)> callback;

		~EventInternal()
		{
			std::scoped_lock lck(manager->locker);
			auto it = manager->subscribers.find(id);
			if (it != manager->subscribers.end())
			{
				it->second.remove(&callback);
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
		eventinternal->callback = callback;

		std::scoped_lock lck(manager->locker);
		manager->subscribers[id].push_back(&eventinternal->callback);

		return handle;
	}

	void Subscribe_Once(int id, std::function<void(uint64_t)> callback)
	{
		std::scoped_lock lck(manager->locker);
		manager->subscribers_once[id].push_back(callback);
	}

	void FireEvent(int id, uint64_t userdata)
	{
		thread_local wi::vector<std::function<void(uint64_t)>> current_callbacks;

		// Gather callbacks inside lock:
		//	This allows callbacks to call other events while not being locked
		{
			std::scoped_lock lck(manager->locker);

			// Callbacks that only live for once:
			{
				auto it = manager->subscribers_once.find(id);
				bool found = it != manager->subscribers_once.end();

				if (found)
				{
					auto& callbacks = it->second;
					for (auto& callback : callbacks)
					{
						current_callbacks.push_back(callback);
					}
					callbacks.clear();
				}
			}
			// Callbacks that live until deleted:
			{
				auto it = manager->subscribers.find(id);
				bool found = it != manager->subscribers.end();

				if (found)
				{
					auto& callbacks = it->second;
					for (auto& callback : callbacks)
					{
						current_callbacks.push_back(*callback);
					}
				}
			}
		}

		// Execution is outside locking:
		for (auto& callback : current_callbacks)
		{
			callback(userdata);
		}
		current_callbacks.clear();
	}
}
