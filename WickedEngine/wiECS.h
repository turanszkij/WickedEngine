#ifndef _ENTITY_COMPONENT_SYSTEM_H_
#define _ENTITY_COMPONENT_SYSTEM_H_

#include "wiArchive.h"

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

	template<typename Component>
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
		}

		// Perform deep copy of all the contents of "other" into this
		inline void Copy(const ComponentManager<Component>& other)
		{
			Clear();
			components = other.components;
			entities = other.entities;
			lookup = other.lookup;
		}

		inline void Serialize(wiArchive& archive)
		{
			if (archive.IsReadMode())
			{
				size_t count;
				archive >> count;

				components.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					components[i].Serialize(archive);
				}

				entities.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					archive >> entities[i];
				}

				lookup.reserve(count);
				for (size_t i = 0; i < count; ++i)
				{
					Entity entity;
					size_t index;
					archive >> entity;
					archive >> index;
					lookup[entity] = index;
				}
			}
			else
			{
				archive << components.size();
				for (Component& component : components)
				{
					component.Serialize(archive);
				}
				for (Entity entity : entities)
				{
					archive << entity;
				}
				for (auto it : lookup)
				{
					archive << it.first;
					archive << it.second;
				}
			}
		}

		// Create a new component and retrieve a reference to it:
		inline Component& Create(Entity entity)
		{
			// Only one of this component type per entity is allowed!
			assert(lookup.find(entity) == lookup.end());

			// Entity count must always be the same as the number of coponents!
			assert(entities.size() == components.size());
			assert(lookup.size() == components.size());

			// Update the entity lookup table:
			lookup[entity] = components.size();

			// New components are always pushed to the end:
			components.push_back(Component());

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
				const Entity entity = entities[index];

				if (index < components.size() - 1)
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

		// Remove a component of a certain entity if it exists while keeping the current ordering:
		inline void Remove_KeepSorted(Entity entity)
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				// Directly index into components and entities array:
				const size_t index = it->second;
				const Entity entity = entities[index];

				if (index < components.size() - 1)
				{
					// Move every component left by one that is after this element:
					for (size_t i = index + 1; i < components.size(); ++i)
					{
						components[i - 1] = std::move(components[i]);
					}
					// Move every entity left by one that is after this element and update lut:
					for (size_t i = index + 1; i < entities.size(); ++i)
					{
						entities[i - 1] = entities[i];
						lookup[entities[i - 1]] = i - 1;
					}
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
				const Component component_tmp = std::move(components[index_a]);

				entities[index_a] = entities[index_b];
				components[index_a] = std::move(components[index_b]);

				entities[index_b] = entity_tmp;
				components[index_b] = std::move(component_tmp);

				lookup[a] = index_b;
				lookup[b] = index_a;
			}

		}

		// Place the last added entity-component to the specified index position while keeping the ordering intact:
		inline void MoveLastTo(size_t index)
		{
			// No operation if less than two components, or the target index is already the last index:
			if (GetCount() < 2 || index == GetCount() - 1)
			{
				return;
			}

			// Save the last component and entity:
			Component component = std::move(components.back());
			Entity entity = entities.back();

			// Each component gets moved to the right by one after the index:
			for (size_t i = components.size() - 1; i > index; --i)
			{
				components[i] = std::move(components[i - 1]);
			}
			// Each entity gets moved to the right by one after the index and lut is updated:
			for (size_t i = entities.size() - 1; i > index; --i)
			{
				entities[i] = entities[i - 1];
				lookup[entities[i]] = i;
			}

			// Saved entity-component moved to the required position:
			components[index] = std::move(component);
			entities[index] = entity;
			lookup[entity] = index;
		}

		// Check if a component exists for a given entity or not:
		inline bool Contains(Entity entity) const
		{
			return lookup.find(entity) != lookup.end();
		}

		// Retrieve a [read/write] component specified by an entity (if it exists, otherwise nullptr):
		inline Component* GetComponent(Entity entity)
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return &components[it->second];
			}
			return nullptr;
		}

		// Retrieve a [read only] component specified by an entity (if it exists, otherwise nullptr):
		inline const Component* GetComponent(Entity entity) const
		{
			const auto it = lookup.find(entity);
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

		// Directly index a specific [read/write] component without indirection:
		//	0 <= index < GetCount()
		inline Component& operator[](size_t index) { return components[index]; }

		// Directly index a specific [read only] component without indirection:
		//	0 <= index < GetCount()
		inline const Component& operator[](size_t index) const { return components[index]; }

	private:
		// This is a linear array of alive components:
		std::vector<Component> components;
		// This is a linear array of entities corresponding to each alive component:
		std::vector<Entity> entities;
		// This is a lookup table for entities:
		std::unordered_map<Entity, size_t> lookup;

		// Disallow this to be copied by mistake:
		ComponentManager(const ComponentManager&) = delete;
	};
}

#endif // _ENTITY_COMPONENT_SYSTEM_H_
