#ifndef _ENTITY_COMPONENT_SYSTEM_H_
#define _ENTITY_COMPONENT_SYSTEM_H_

#include <cstdint>
#include <cassert>
#include <vector>
#include <unordered_map>

namespace wiECS
{
	typedef uint64_t Entity;
	static const Entity INVALID_ENTITY = 0;
	inline Entity CreateEntity()
	{
		static Entity id = 0;
		return ++id;
	}

	template<typename T>
	class ComponentManager
	{
	public:

		// reservedCount : how much components can be held initially before growing the container
		ComponentManager(size_t reservedCount = 0)
		{
			components.reserve(reservedCount);
			entities.reserve(reservedCount);
			lookup.reserve(reservedCount);
		}

		// Clear the whole container
		inline void Clear()
		{
			components.clear();
			entities.clear();
			lookup.clear();
			dead.clear();
		}

		// Create a new component and retrieve a reference to it:
		inline T& Create(Entity entity)
		{
			// Only one of this component type per entity is allowed!
			assert(lookup.find(entity) == lookup.end());

			// Entity count must always be the same as the number of coponents!
			assert(entities.size() == components.size());
			assert(lookup.size() == components.size());

			// Update the entity lookup table:
			lookup[entity] = components.size();

			// New components are always pushed to the end:
			components.push_back(T());

			// Also push corresponding entity:
			entities.push_back(entity);

			return components.back();
		}

		// Remove a component of a certain entity if it exists:
		inline void Remove(Entity entity)
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				// Directly index into components and entities array:
				const size_t index = it->second;

				// Remove the corresponding entry from the lookup table:
				const Entity entity = entities[index];

				if (components.size() > 1)
				{
					// Swap out the dead element with the last one:
					components[index] = std::move(components.back()); // try to use move instead of copy
					entities[index] = entities.back();

					// Update the lookup table:
					lookup[entities[index]] = index;
				}

				// Shrink the container:
				components.pop_back();
				entities.pop_back();
				lookup.erase(entity);
			}
		}

		// Swap two components' data:
		inline void Swap(Entity a, Entity b)
		{
			auto it_a = lookup.find(a);
			auto it_b = lookup.find(b);

			if (it_a != lookup.end() && it_b != lookup.end())
			{
				const size_t index_a = it_a->second;
				const size_t index_b = it_b->second;

				const Entity entity_tmp = entities[index_a];
				const T component_tmp = std::move(components[index_a]);

				entities[index_a] = entities[index_b];
				components[index_a] = std::move(components[index_b]);

				entities[index_b] = entity_tmp;
				components[index_b] = std::move(component_tmp);

				lookup[a] = index_b;
				lookup[b] = index_a;
			}

		}

		// Retrieve a component specified by an entity (if it exists, otherwise nullptr):
		inline T* GetComponent(Entity entity)
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return &components[it->second];
			}
			return nullptr;
		}

		// Retrieve the number of existing entries:
		inline size_t GetCount() const { return components.size(); }

		// Directly index a specific component without indirection:
		//	0 <= index < GetCount()
		inline Entity GetEntity(size_t index) const { return entities[index]; }

		// Directly index a specific component without indirection:
		//	0 <= index < GetCount()
		inline T& operator[](size_t index) { return components[index]; }

		// Directly index a specific component without indirection:
		//	0 <= index < GetCount()
		inline const T& operator[](size_t index) const { return components[index]; }

	private:
		// This is a linear array of alive components:
		std::vector<T> components;
		// This is a linear array of entities corresponding to each alive component:
		std::vector<Entity> entities;
		// This is a lookup table for entities:
		std::unordered_map<Entity, size_t> lookup;
	};
}

#endif // _ENTITY_COMPONENT_SYSTEM_H_
