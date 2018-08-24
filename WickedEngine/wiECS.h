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

		// The iterator is an indirection to the components. Components will be moved around to remain compacted, but iterator will always be able to reference a component.
		struct iterator
		{
			size_t value = ~0;

			inline iterator operator=(iterator other) { value = other.value; return *this; }
			inline bool operator==(iterator other) const { return value == other.value; }
			inline bool operator!=(iterator other) const { return value != other.value; }
			inline iterator& operator++() { value++; return *this; }
			inline iterator operator++(int v) { iterator temp = *this; ++*this; return temp; }
			inline iterator& operator--() { value--; return *this; }
			inline iterator operator--(int v) { iterator temp = *this; --*this; return temp; }
		};

		// Clear the whole container, invalidate all iterators
		inline void Clear()
		{
			components.clear();
			entities.clear();
			lookup.clear();
			indices.clear();
			dead.clear();
		}

		// Get the beginning of the iteration sequence (iterator is safe but uses indirection):
		inline iterator Begin() const
		{
			iterator it;
			it.value = 0;
			return it;
		}

		// Get the end of the iteration sequence (iterator is safe but uses indirection):
		inline iterator End() const
		{
			iterator it;
			it.value = indices.size();
			return it;
		}

		// Check if an iterator is referencing a component or not (iterator is safe but uses indirection):
		inline bool IsValid(iterator it) const
		{
			return it.value < indices.size() && indices[it.value] < components.size();
		}

		// Check if an iterator is referencing a specific entity or not (iterator is safe but uses indirection):
		inline bool IsValid(iterator it, Entity entity) const
		{
			assert(entities.size() == components.size());
			return IsValid(it) && entities[indices[it.value]] == entity;
		}

		// Create a new component and retrieve iterator (iterator is safe but uses indirection):
		inline iterator Create(Entity entity)
		{
			// Only one of this component type per entity is allowed!
			assert(!IsValid(Find(entity)));

			iterator it;

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

				// Essentially we pop the last dead iterator and update its value to the new component (that will always be pushed to the end of components):
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
			iterator it = Find(entity);
			if (IsValid(it))
			{
				Remove(it);
			}
		}

		// Remove a component referenced by a certain iterator (iterator is safe but uses indirection):
		inline void Remove(iterator it)
		{
			// Iterator should be valid:
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

			// Because the last element of the container was moved to the position of the removed element, we update its iterator accordingly:
			indices[components.size()] = index;

			//The current iterator is marked as dead:
			indices[it.value] = ~0;
			dead.push_back(it);
		}

		// Swap two components' data that are referenced by iterators (iterator is safe but uses indirection):
		inline void Swap(iterator a, iterator b)
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
		inline iterator Find(Entity entity) const
		{
			auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return it->second;
			}

			return End();
		}

		// Retrieve a component specified by an iterator (iterator is safe but uses indirection):
		inline T& GetComponent(iterator it)
		{
			assert(IsValid(it));
			return components[indices[it.value]];
		}
		// Retrieve an entity specified by an iterator (iterator is safe but uses indirection):
		inline Entity GetEntity(iterator it) const
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
		std::unordered_map<Entity, iterator> lookup;
		// This is the indirection of iterator->component index:
		std::vector<size_t> indices;
		// The indices will also contain dead elements, and those are saved here:
		std::vector<iterator> dead;
	};
}

#endif // _ENTITY_COMPONENT_SYSTEM_H_
