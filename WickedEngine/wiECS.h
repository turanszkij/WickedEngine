#ifndef WI_ENTITY_COMPONENT_SYSTEM_H
#define WI_ENTITY_COMPONENT_SYSTEM_H

#include "wiArchive.h"
#include "wiJobSystem.h"

#include <cstdint>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace wiECS
{
	using Entity = uint32_t;
	static const Entity INVALID_ENTITY = 0;
	// Runtime can create a new entity with this
	inline Entity CreateEntity()
	{
		static std::atomic<Entity> next{ INVALID_ENTITY + 1 };
		return next.fetch_add(1);
	}

	struct EntitySerializer
	{
		wiJobSystem::context ctx; // allow components to spawn serialization subtasks
		std::unordered_map<uint64_t, Entity> remap;
		bool allow_remap = true;

		~EntitySerializer()
		{
			wiJobSystem::Wait(ctx); // automatically wait for all subtasks after serialization
		}
	};
	// This is the safe way to serialize an entity
	inline void SerializeEntity(wiArchive& archive, Entity& entity, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			// Entities are always serialized as uint64_t for back-compat
			uint64_t mem;
			archive >> mem;

			if (seri.allow_remap)
			{
				auto it = seri.remap.find(mem);
				if (it == seri.remap.end())
				{
					entity = CreateEntity();
					seri.remap[mem] = entity;
				}
				else
				{
					entity = it->second;
				}
			}
			else
			{
				entity = (Entity)mem;
			}
		}
		else
		{
			archive << entity;
		}
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

		// Merge in an other component manager of the same type to this. 
		//	The other component manager MUST NOT contain any of the same entities!
		//	The other component manager is not retained after this operation!
		inline void Merge(ComponentManager<Component>& other)
		{
			components.reserve(GetCount() + other.GetCount());
			entities.reserve(GetCount() + other.GetCount());
			lookup.reserve(GetCount() + other.GetCount());

			for (size_t i = 0; i < other.GetCount(); ++i)
			{
				Entity entity = other.entities[i];
				assert(!Contains(entity));
				entities.push_back(entity);
				lookup[entity] = components.size();
				components.push_back(std::move(other.components[i]));
			}

			other.Clear();
		}

		// Read/Write everything to an archive depending on the archive state
		inline void Serialize(wiArchive& archive, EntitySerializer& seri)
		{
			if (archive.IsReadMode())
			{
				Clear(); // If we deserialize, we start from empty

				size_t count;
				archive >> count;

				components.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					components[i].Serialize(archive, seri);
				}

				entities.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					Entity entity;
					SerializeEntity(archive, entity, seri);
					entities[i] = entity;
					lookup[entity] = i;
				}
			}
			else
			{
				archive << components.size();
				for (Component& component : components)
				{
					component.Serialize(archive, seri);
				}
				for (Entity entity : entities)
				{
					SerializeEntity(archive, entity, seri);
				}
			}
		}

		// Create a new component and retrieve a reference to it
		inline Component& Create(Entity entity)
		{
			// INVALID_ENTITY is not allowed!
			assert(entity != INVALID_ENTITY);

			// Only one of this component type per entity is allowed!
			assert(lookup.find(entity) == lookup.end());

			// Entity count must always be the same as the number of coponents!
			assert(entities.size() == components.size());
			assert(lookup.size() == components.size());

			// Update the entity lookup table:
			lookup[entity] = components.size();

			// New components are always pushed to the end:
			components.emplace_back();

			// Also push corresponding entity:
			entities.push_back(entity);

			return components.back();
		}

		// Remove a component of a certain entity if it exists
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

		// Remove a component of a certain entity if it exists while keeping the current ordering
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

		// Place an entity-component to the specified index position while keeping the ordering intact
		inline void MoveItem(size_t index_from, size_t index_to)
		{
			assert(index_from < GetCount());
			assert(index_to < GetCount());
			if (index_from == index_to)
			{
				return;
			}

			// Save the moved component and entity:
			Component component = std::move(components[index_from]);
			Entity entity = entities[index_from];

			// Every other entity-component that's in the way gets moved by one and lut is kept updated:
			const int direction = index_from < index_to ? 1 : -1;
			for (size_t i = index_from; i != index_to; i += direction)
			{
				const size_t next = i + direction;
				components[i] = std::move(components[next]);
				entities[i] = entities[next];
				lookup[entities[i]] = i;
			}

			// Saved entity-component moved to the required position:
			components[index_to] = std::move(component);
			entities[index_to] = entity;
			lookup[entity] = index_to;
		}

		// Check if a component exists for a given entity or not
		inline bool Contains(Entity entity) const
		{
			return lookup.find(entity) != lookup.end();
		}

		// Retrieve a [read/write] component specified by an entity (if it exists, otherwise nullptr)
		inline Component* GetComponent(Entity entity)
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return &components[it->second];
			}
			return nullptr;
		}

		// Retrieve a [read only] component specified by an entity (if it exists, otherwise nullptr)
		inline const Component* GetComponent(Entity entity) const
		{
			const auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return &components[it->second];
			}
			return nullptr;
		}

		// Retrieve component index by entity handle (if not exists, returns ~0 value)
		inline size_t GetIndex(Entity entity) const 
		{
			const auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return it->second;
			}
			return ~0;
		}

		// Retrieve the number of existing entries
		inline size_t GetCount() const { return components.size(); }

		// Directly index a specific component without indirection
		//	0 <= index < GetCount()
		inline Entity GetEntity(size_t index) const { return entities[index]; }

		// Directly index a specific [read/write] component without indirection
		//	0 <= index < GetCount()
		inline Component& operator[](size_t index) { return components[index]; }

		// Directly index a specific [read only] component without indirection
		//	0 <= index < GetCount()
		inline const Component& operator[](size_t index) const { return components[index]; }

	private:
		// This is a linear array of alive components
		std::vector<Component> components;
		// This is a linear array of entities corresponding to each alive component
		std::vector<Entity> entities;
		// This is a lookup table for entities
		std::unordered_map<Entity, size_t> lookup;

		// Disallow this to be copied by mistake
		ComponentManager(const ComponentManager&) = delete;
	};
}

#endif // WI_ENTITY_COMPONENT_SYSTEM_H
