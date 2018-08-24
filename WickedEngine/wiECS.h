#ifndef _ENTITY_COMPONENT_SYSTEM_H_
#define _ENTITY_COMPONENT_SYSTEM_H_

#include <cstdint>
#include <cassert>
#include <vector>
#include <unordered_map>

namespace wiECS
{
	typedef uint64_t Entity;

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
			indices.reserve(reservedCount);
		}

		// The ref is an indirection to the components. Components will be moved around to remain compacted, but ref will always be able to reference a component.
		struct ref
		{
			size_t value = ~0;
		};

		// Clear the whole container, invalidate all refs
		inline void Clear()
		{
			components.clear();
			entities.clear();
			lookup.clear();
			indices.clear();
			dead.clear();
		}

		// Check if an ref is referencing a component or not (ref is safe but uses indirection):
		inline bool IsValid(ref it) const
		{
			return it.value < indices.size() && indices[it.value] < components.size();
		}

		// Check if an ref is referencing a specific entity or not (ref is safe but uses indirection):
		inline bool IsValid(ref it, Entity entity) const
		{
			assert(entities.size() == components.size());
			return IsValid(it) && entities[indices[it.value]] == entity;
		}

		// Create a new component and retrieve ref (ref is safe but uses indirection):
		inline ref Create(Entity entity)
		{
			// Only one of this component type per entity is allowed!
			assert(!IsValid(Find(entity)));

			ref it;

			if (dead.empty())
			{
				// There are no dead elements, so we must have the same amount of components as indices:
				assert(components.size() == indices.size());

				// Any new component and index will just be pushed onto the end:
				it.value = components.size();
				indices.push_back(it.value);
			}
			else
			{
				// We have dead elements, which means components was popped, but indices only swapped, so there must be less components than indices:
				assert(components.size() < indices.size());

				// Essentially we pop the last dead ref and update its value to the new component (that will always be pushed to the end of components):
				it = dead.back();
				dead.pop_back();
				indices[it.value] = components.size();
			}

			// Entity count must always be the same as the number of coponents!
			assert(entities.size() == components.size());
			assert(lookup.size() == components.size());

			// New components are always pushed to the end:
			components.push_back(T());

			// Also push corresponding entity:
			entities.push_back(entity);

			// Update the entity lookup table:
			lookup[entity] = it;

			return it;
		}

		// Remove a component of a certain entity if it exists (referencing by Entity involves multiple indirection):
		inline void Remove(Entity entity)
		{
			ref it = Find(entity);
			if (IsValid(it))
			{
				Remove(it);
			}
		}

		// Remove a component referenced by a certain ref (ref is safe but uses indirection):
		inline void Remove(ref it)
		{
			// ref should be valid:
			assert(IsValid(it));

			// Directly index into components and entities array:
			const size_t index = indices[it.value];

			// Remove the corresponding entry from the lookup table:
			const Entity entity = entities[index];
			lookup.erase(entity);

			// Swap out the dead element with the last one, and shrink the container:
			components[index] = std::move(components.back()); // try to use move instead of copy
			components.pop_back();
			entities[index] = entities.back();
			entities.pop_back();

			// Because the last element of the container was moved to the position of the removed element, we update its ref accordingly:
			indices[components.size()] = index;

			//The current ref is marked as dead:
			indices[it.value] = ~0;
			dead.push_back(it);
		}

		// Swap two components' data that are referenced by refs (ref is safe but uses indirection):
		inline void Swap(ref a, ref b)
		{
			const size_t index_a = indices[a.value];
			const size_t index_b = indices[b.value];

			const size_t index_tmp = index_a;
			const Entity entity_tmp = entities[index_a];
			const T component_tmp = std::move(components[index_a]);

			indices[a.value] = index_b;
			entities[index_a] = entities[index_b];
			components[index_a] = std::move(components[index_b]);

			indices[b.value] = index_tmp;
			entities[index_b] = entity_tmp;
			components[index_b] = std::move(component_tmp);
		}

		// Find a specific component of a certain entity if exists (referencing by Entity involves multiple indirection):
		inline ref Find(Entity entity) const
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return it->second;
			}

			return ref();
		}

		// Retrieve a component specified by an ref (ref is safe but uses indirection):
		inline T& GetComponent(ref it)
		{
			assert(IsValid(it));
			return components[indices[it.value]];
		}
		// Retrieve an entity specified by an ref (ref is safe but uses indirection):
		inline Entity GetEntity(ref it) const
		{
			assert(IsValid(it));
			return entities[indices[it.value]];
		}

		// Retrieve the number of existing entries:
		inline size_t GetCount() const { return components.size(); }

		// Directly index a specific component without indirection:
		//	0 <= index < GetCount()
		inline T& GetComponent(size_t index) const { return components[index]; }

		// Directly index a specific component without indirection:
		//	0 <= index < GetCount()
		inline Entity GetEntity(size_t index) const { return entities[index]; }

		// Directly index a specific component without indirection:
		//	0 <= index < GetCount()
		inline T& operator[](size_t index) { return components[index]; }

	private:
		// This is a linear array of alive components:
		std::vector<T> components;
		// This is a linear array of entities corresponding to each alive component:
		std::vector<Entity> entities;
		// This is a lookup table for entities:
		std::unordered_map<Entity, ref> lookup;
		// This is the indirection of ref->component index:
		std::vector<size_t> indices;
		// The indices will also contain dead elements, and those are saved here:
		std::vector<ref> dead;
	};
}

#endif // _ENTITY_COMPONENT_SYSTEM_H_
