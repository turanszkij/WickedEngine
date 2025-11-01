#ifndef WI_ENTITY_COMPONENT_SYSTEM_H
#define WI_ENTITY_COMPONENT_SYSTEM_H

#include "wiArchive.h"
#include "wiJobSystem.h"
#include "wiUnorderedMap.h"
#include "wiUnorderedSet.h"
#include "wiVector.h"
#include "wiAllocator.h"

#include <cstdint>
#include <cassert>
#include <atomic>
#include <memory>
#include <string>

// Entity-Component System
namespace wi::ecs
{
	// The Entity is a global unique persistent identifier within the entity-component system
	//	It can be stored and used for the duration of the application
	//	The entity can be a different value on a different run of the application, if it was serialized
	//	It must be only serialized with the SerializeEntity() function if persistence is needed across different program runs,
	//		this will ensure that entities still match with their components correctly after serialization
	using Entity = uint64_t;
	inline static constexpr Entity INVALID_ENTITY = 0;
	// Runtime can create a new entity with this
	inline Entity CreateEntity()
	{
		static std::atomic<Entity> next{ INVALID_ENTITY + 1 };
		return next.fetch_add(1);
	}
	inline static constexpr size_t INVALID_INDEX = ~0ull;

	class ComponentLibrary;
	struct EntitySerializer
	{
		wi::jobsystem::context ctx; // allow components to spawn serialization subtasks
		wi::unordered_map<uint64_t, Entity> remap;
		bool allow_remap = true;
		uint64_t version = 0; // The ComponentLibrary serialization will modify this by the registered component's version number
		wi::unordered_set<std::string> resource_registration; // register for resource manager serialization
		ComponentLibrary* componentlibrary = nullptr;
		wi::unordered_map<std::string, uint64_t> library_versions;

		~EntitySerializer()
		{
			wi::jobsystem::Wait(ctx); // automatically wait for all subtasks after serialization
		}

		// Returns the library version of the currently serializing Component
		//	If not using ComponentLibrary, it returns version set by the user.
		uint64_t GetVersion() const
		{
			return version;
		}
		uint64_t GetVersion(const std::string& name) const
		{
			auto it = library_versions.find(name);
			if (it != library_versions.end())
			{
				return it->second;
			}
			return 0;
		}

		void RegisterResource(const std::string& resource_name)
		{
			if (resource_name.empty())
				return;
			resource_registration.insert(resource_name);
		}
	};
	// This is the safe way to serialize an entity
	inline void SerializeEntity(wi::Archive& archive, Entity& entity, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			// Entities are always serialized as uint64_t for back-compat
			uint64_t mem;
			archive >> mem;

			if (mem != INVALID_ENTITY && seri.allow_remap)
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
#if defined(__GNUC__) && !defined(__SCE__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif // __GNUC__ && !__SCE__ && ! __clang__

			archive << entity;

#if defined(__GNUC__) && !defined(__SCE__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif // __GNUC__ && !__SCE__ && !__clang__
		}
	}

	// This is an interface class to implement a ComponentManager,
	// inherit this class if you want to work with ComponentLibrary
	class ComponentManager_Interface
	{
	public:
		virtual ~ComponentManager_Interface() = default;
		virtual void Copy(const ComponentManager_Interface& other) = 0;
		virtual void Merge(ComponentManager_Interface& other) = 0;
		virtual void Clear() = 0;
		virtual void Serialize(wi::Archive& archive, EntitySerializer& seri) = 0;
		virtual void Component_Serialize(Entity entity, wi::Archive& archive, EntitySerializer& seri) = 0;
		virtual void Remove(Entity entity) = 0;
		virtual void Remove_KeepSorted(Entity entity) = 0;
		virtual void MoveItem(size_t index_from, size_t index_to) = 0;
		virtual bool Contains(Entity entity) const = 0;
		virtual size_t GetIndex(Entity entity) const = 0;
		virtual size_t GetCount() const = 0;
		virtual Entity GetEntity(size_t index) const = 0;
		virtual const wi::vector<Entity>& GetEntityArray() const = 0;
	};

	// The ComponentManager is a container that stores components and matches them with entities
	//	Note: final keyword is used to indicate this is a final implementation.
	//	This allows function inlining and avoid calls, improves performance considerably
	template<typename Component>
	class ComponentManager final : public ComponentManager_Interface
	{
	public:

		// reservedCount : how much components can be held initially before growing the container
		ComponentManager(size_t reservedCount = 0)
		{
			Reserve(reservedCount);
		}

		inline void Reserve(size_t count)
		{
			components.reserve(count);
			entities.reserve(count);
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
			components.reserve(GetCount() + other.GetCount());
			entities.reserve(GetCount() + other.GetCount());
			for (size_t i = 0; i < other.GetCount(); ++i)
			{
				Entity entity = other.entities[i];
				assert(!Contains(entity));
				entities.push_back(entity);
				lookup.insert(entity, components.size());
				components.push_back(other.components[i]);
			}
		}

		// Merge in an other component manager of the same type to this.
		//	The other component manager MUST NOT contain any of the same entities!
		//	The other component manager is not retained after this operation!
		inline void Merge(ComponentManager<Component>& other)
		{
			components.reserve(GetCount() + other.GetCount());
			entities.reserve(GetCount() + other.GetCount());

			for (size_t i = 0; i < other.GetCount(); ++i)
			{
				Entity entity = other.entities[i];
				assert(!Contains(entity));
				entities.push_back(entity);
				lookup.insert(entity, components.size());
				components.push_back(std::move(other.components[i]));
			}

			other.Clear();
		}

		inline void Copy(const ComponentManager_Interface& other)
		{
			Copy((ComponentManager<Component>&)other);
		}

		inline void Merge(ComponentManager_Interface& other)
		{
			Merge((ComponentManager<Component>&)other);
		}

		// Read/Write everything to an archive depending on the archive state
		inline void Serialize(wi::Archive& archive, EntitySerializer& seri)
		{
			if (archive.IsReadMode())
			{
				const size_t prev_count = components.size();

				size_t count;
				archive >> count;

				components.resize(prev_count + count);
				for (size_t i = 0; i < count; ++i)
				{
					components[prev_count + i].Serialize(archive, seri);
				}

				entities.resize(prev_count + count);
				for (size_t i = 0; i < count; ++i)
				{
					// assign value to make GCC happy
					Entity entity = INVALID_ENTITY;
					SerializeEntity(archive, entity, seri);
					entities[prev_count + i] = entity;
					lookup.insert(entity, prev_count + i);
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

		//Read one single component onto an archive, make sure entity are serialized first
		inline void Component_Serialize(Entity entity, wi::Archive& archive, EntitySerializer& seri)
		{
			if(archive.IsReadMode())
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = this->Create(entity);
					component.Serialize(archive, seri);
				}
			}
			else
			{
				auto component = this->GetComponent(entity);
				if (component != nullptr)
				{
					archive << true;
					component->Serialize(archive, seri);
				}
				else
				{
					archive << false;
				}
			}
		}

		// Create a new component and retrieve a reference to it
		inline Component& Create(Entity entity)
		{
			// INVALID_ENTITY is not allowed!
			assert(entity != INVALID_ENTITY);

			// Only one of this component type per entity is allowed!
			assert(!Contains(entity));

			// Entity count must always be the same as the number of components!
			assert(entities.size() == components.size());

			// Update the entity lookup table:
			lookup.insert(entity, components.size());

			// New components are always pushed to the end:
			components.emplace_back();

			// Also push corresponding entity:
			entities.push_back(entity);

			return components.back();
		}

		// Remove a component of a certain entity if it exists
		inline void Remove(Entity entity)
		{
			const size_t index = GetIndex(entity);
			if (index != INVALID_INDEX)
			{
				if (index < components.size() - 1)
				{
					// Swap out the dead element with the last one:
					components[index] = std::move(components.back()); // try to use move instead of copy
					entities[index] = entities.back();

					// Update the lookup table:
					lookup.insert(entities[index], index);
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
			const size_t index = GetIndex(entity);
			if (index != INVALID_INDEX)
			{
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
						lookup.insert(entities[i - 1], i - 1);
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
				lookup.insert(entities[i], i);
			}

			// Saved entity-component moved to the required position:
			components[index_to] = std::move(component);
			entities[index_to] = entity;
			lookup.insert(entity, index_to);
		}

		// Check if a component exists for a given entity or not
		inline bool Contains(Entity entity) const
		{
			return GetIndex(entity) != INVALID_INDEX;
		}

		// Retrieve a [read/write] component specified by an entity (if it exists, otherwise nullptr)
		inline Component* GetComponent(Entity entity)
		{
			const size_t index = GetIndex(entity);
			if (index == INVALID_INDEX)
				return nullptr;
			return &components[index];
		}

		// Retrieve a [read only] component specified by an entity (if it exists, otherwise nullptr)
		inline const Component* GetComponent(Entity entity) const
		{
			const size_t index = GetIndex(entity);
			if (index == INVALID_INDEX)
				return nullptr;
			return &components[index];
		}

		// Retrieve component index by entity handle (if doesn't exist, returns INVALID_INDEX value)
		inline size_t GetIndex(Entity entity) const
		{
			return lookup.get(entity);
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

		// Returns the tightly packed [read only] entity array
		inline const wi::vector<Entity>& GetEntityArray() const { return entities; }

		// Returns the tightly packed [read only] component array
		inline const wi::vector<Component>& GetComponentArray() const { return components; }

	private:
		// This is a linear array of alive components
		wi::vector<Component> components;
		// This is a linear array of entities corresponding to each alive component
		wi::vector<Entity> entities;

//#define LOOKUP_STRAIGHT
//#define LOOKUP_SPARSE
#define LOOKUP_BUCKET_HASH
//#define LOOKUP_HASH

		// This is a lookup table for entity -> index resolving
		struct LookupTable
		{

#ifdef LOOKUP_STRAIGHT
			// Straight lookup with memory wasting implementation:
			//	Matches Entity to index with storing every possible Entity in memory up to the max stored entity index
			//	Fastest lookup but wastes memory if a large range of Entity indices were stored
			struct Item
			{
				size_t index = INVALID_INDEX;
			};
			wi::vector<Item> table;

			inline void clear()
			{
				table.clear();
			}
			inline void erase(Entity entity)
			{
				table[entity].index = INVALID_INDEX;
			}
			inline void insert(Entity entity, size_t index)
			{
				if (table.size() <= entity)
					table.resize(entity + 1);
				table[entity].index = index;
			}
			inline size_t get(Entity entity) const
			{
				if (table.size() <= entity)
					return INVALID_INDEX;
				return table[entity].index;
			}
#endif // LOOKUP_NAIVE

#ifdef LOOKUP_SPARSE
			// Straight lookup a sparse memory manager:
			// Similar to straight as it stores a direct mapping of entity -> index, but unused ranges of 64 entities will not waste memory
			// The lookup is slowed by the extra block data indirection
			struct BlockData
			{
				struct Item
				{
					size_t index = INVALID_INDEX;
				};
				Item items[64];
			};
			struct Block
			{
				uint64_t status = 0; // one bit per item in block, if it's 0 then block can be freed and reused
				BlockData* block_data = nullptr;
			};
			wi::vector<Block> blocks;
			wi::allocator::BlockAllocator<BlockData, 8> block_allocator; // block allocator manages memory per 8 blocks here (sizeof(BlockData) * 8 = 4 KB pages)

			inline void clear()
			{
				blocks.clear();
				block_allocator = {};
			}
			inline void erase(Entity entity)
			{
				const uint64_t block_index = entity >> 6ull; // entity / 64
				if (blocks.size() > block_index)
				{
					Block& block = blocks[block_index];
					if (block.status)
					{
						const uint64_t item_index = entity & 63ull; // entity % 64
						block.block_data->items[item_index].index = INVALID_INDEX;
						block.status &= ~(1ull << item_index);
						if (block.status == 0)
						{
							// Free the block data for reuse:
							block_allocator.free(block.block_data);
							block.block_data = nullptr;
						}
					}
				}
			}
			inline void insert(Entity entity, size_t index)
			{
				if (index == INVALID_INDEX)
					return;
				const uint64_t block_index = entity >> 6ull; // entity / 64
				if (blocks.size() <= block_index)
				{
					// Allocate new block:
					blocks.resize(block_index + 1);
				}
				Block& block = blocks[block_index];
				if (block.block_data == nullptr)
				{
					block.block_data = block_allocator.allocate();
				}
				const uint64_t item_index = entity & 63ull; // entity % 64
				block.status |= 1ull << item_index;
				block.block_data->items[item_index].index = index;
			}
			inline size_t get(Entity entity) const
			{
				const uint64_t block_index = entity >> 6ull; // entity / 64
				if (blocks.size() > block_index)
				{
					const Block& block = blocks[block_index];
					if (block.status)
					{
						const uint64_t item_index = entity & 63ull; // entity % 64
						return block.block_data->items[item_index].index;
					}
				}
				return INVALID_INDEX;
			}
#endif // LOOKUP_SPARSE

#ifdef LOOKUP_BUCKET_HASH
			// Implementation with hash table:
			// Compared to standard hashing, this one hashes per 64-item bucket, resulting in less hashed entries
			struct Block
			{
				uint64_t status = 0; // one bit per item in block, if it's 0 then block can be freed and reused
				struct Item
				{
					size_t index = INVALID_INDEX;
				};
				Item items[64];
			};
			wi::unordered_map<uint64_t, Block> table;

			inline void clear()
			{
				table.clear();
			}
			inline void erase(Entity entity)
			{
				const uint64_t block_index = entity >> 6ull; // entity / 64
				auto it = table.find(block_index);
				if (it == table.end())
					return;
				Block& block = it->second;
				const uint64_t item_index = entity & 63ull; // entity % 64
				block.items[item_index].index = INVALID_INDEX;
				block.status &= ~(1ull << item_index);
				if (block.status == 0)
				{
					// Whole block can be freed:
					table.erase(block_index);
				}
			}
			inline void insert(Entity entity, size_t index)
			{
				const uint64_t block_index = entity >> 6ull; // entity / 64
				const uint64_t item_index = entity & 63ull; // entity % 64
				Block& block = table[block_index];
				block.items[item_index].index = index;
				block.status |= 1ull << item_index;
			}
			inline size_t get(Entity entity) const
			{
				const uint64_t block_index = entity >> 6ull; // entity / 64
				auto it = table.find(block_index);
				if (it == table.end())
					return INVALID_INDEX;
				const uint64_t item_index = entity & 63ull; // entity % 64
				return it->second.items[item_index].index;
			}
#endif // LOOKUP_BUCKET_HASH

#ifdef LOOKUP_HASH
			// Implementation with hash table:
			// The standard hashing method, performance depends on hashing, hash collisions
			wi::unordered_map<Entity, size_t> table;

			inline void clear()
			{
				table.clear();
			}
			inline void erase(Entity entity)
			{
				table.erase(entity);
			}
			inline void insert(Entity entity, size_t index)
			{
				table[entity] = index;
			}
			inline size_t get(Entity entity) const
			{
				if (table.empty())
					return INVALID_INDEX;
				auto it = table.find(entity);
				if (it == table.end())
					return INVALID_INDEX;
				return it->second;
			}
#endif // LOOKUP_HASH

		} lookup;

		// Disallow this to be copied by mistake
		ComponentManager(const ComponentManager&) = delete;
	};

	// This is the class to store all component managers,
	// this is useful for bulk operation of all attached components within an entity
	class ComponentLibrary
	{
	public:
		struct LibraryEntry
		{
			std::unique_ptr<ComponentManager_Interface> component_manager;
			uint64_t version = 0;
		};
		wi::unordered_map<std::string, LibraryEntry> entries;

		// Create an instance of ComponentManager of a certain data type
		//	The name must be unique, it will be used in serialization
		//	version is optional, it will be propagated to ComponentManager::Serialize() inside the EntitySerializer parameter
		template<typename T>
		inline ComponentManager<T>& Register(const std::string& name, uint64_t version = 0)
		{
			entries[name].component_manager = std::make_unique<ComponentManager<T>>();
			entries[name].version = version;
			return static_cast<ComponentManager<T>&>(*entries[name].component_manager);
		}

		template<typename T>
		inline ComponentManager<T>* Get(const std::string& name)
		{
			auto it = entries.find(name);
			if (it == entries.end())
				return nullptr;
			return static_cast<ComponentManager<T>*>(it->second.component_manager.get());
		}

		template<typename T>
		inline const ComponentManager<T>* Get(const std::string& name) const
		{
			auto it = entries.find(name);
			if (it == entries.end())
				return nullptr;
			return static_cast<const ComponentManager<T>*>(it->second.component_manager.get());
		}

		inline uint64_t GetVersion(std::string name) const
		{
			auto it = entries.find(name);
			if (it == entries.end())
				return 0;
			return it->second.version;
		}

		// Serialize all registered component managers
		inline void Serialize(wi::Archive& archive, EntitySerializer& seri)
		{
			seri.componentlibrary = this;
			if(archive.IsReadMode())
			{
				bool has_next = false;
				size_t begin = archive.GetPos();

				// First pass, gather component type versions and jump over all data:
				//	This is so that we can look up other component versions within component serialization if needed
				do
				{
					archive >> has_next;
					if (has_next)
					{
						std::string name;
						archive >> name;
						uint64_t jump_pos = 0;
						archive >> jump_pos;
						auto it = entries.find(name);
						if (it != entries.end())
						{
							archive >> seri.version;
							seri.library_versions[name] = seri.version;
						}
						archive.Jump(jump_pos);
					}
				} while (has_next);

				// Jump back to beginning of component library data
				archive.Jump(begin);

				// Second pass, read all component data:
				//	At this point, all existing component type versions are available
				do
				{
					archive >> has_next;
					if(has_next)
					{
						std::string name;
						archive >> name;
						uint64_t jump_pos = 0;
						archive >> jump_pos;
						auto it = entries.find(name);
						if(it != entries.end())
						{
							archive >> seri.version;
							it->second.component_manager->Serialize(archive, seri);
						}
						else
						{
							// component manager of this name was not registered, skip serialization by jumping over the data
							archive.Jump(jump_pos);
						}
					}
				}
				while(has_next);
			}
			else
			{
				// Save all component type versions:
				for (auto& it : entries)
				{
					seri.library_versions[it.first] = it.second.version;
				}
				// Serialize all component data, at this point component type version lookup is also complete
				for(auto& it : entries)
				{
					archive << true;
					archive << it.first; // name
					size_t offset = archive.WriteUnknownJumpPosition(); // we will be able to jump from here...
					archive << it.second.version;
					seri.version = it.second.version;
					it.second.component_manager->Serialize(archive, seri);
					archive.PatchUnknownJumpPosition(offset); // ...to here, if this component manager was not registered
				}
				archive << false;
			}
		}

		// Serialize all components for one entity
		inline void Entity_Serialize(Entity entity, wi::Archive& archive, EntitySerializer& seri)
		{
			seri.componentlibrary = this;
			if(archive.IsReadMode())
			{
				bool has_next = false;
				size_t begin = archive.GetPos();

				// First pass, gather component type versions and jump over all data:
				//	This is so that we can look up other component versions within component serialization if needed
				do
				{
					archive >> has_next;
					if (has_next)
					{
						std::string name;
						archive >> name;
						uint64_t jump_pos = 0;
						archive >> jump_pos;
						auto it = entries.find(name);
						if (it != entries.end())
						{
							archive >> seri.version;
							seri.library_versions[name] = seri.version;
						}
						archive.Jump(jump_pos);
					}
				} while (has_next);

				// Jump back to beginning of component library data
				archive.Jump(begin);

				// Second pass: read entities
				do
				{
					archive >> has_next;
					if(has_next)
					{
						std::string name;
						archive >> name;
						uint64_t jump_size = 0;
						archive >> jump_size;
						auto it = entries.find(name);
						if (it != entries.end())
						{
							archive >> seri.version;
							it->second.component_manager->Component_Serialize(entity, archive, seri);
						}
						else
						{
							// component manager of this name was not registered, skip serialization by jumping over the data
							archive.Jump(jump_size);
						}
					}
				}
				while(has_next);
			}
			else
			{
				// Save all component type versions:
				for (auto& it : entries)
				{
					seri.library_versions[it.first] = it.second.version;
				}
				// Serialize:
				for(auto& it : entries)
				{
					archive << true;
					archive << it.first; // name
					size_t offset = archive.WriteUnknownJumpPosition(); // we will be able to jump from here...
					archive << it.second.version;
					seri.version = it.second.version;
					it.second.component_manager->Component_Serialize(entity, archive, seri);
					archive.PatchUnknownJumpPosition(offset); // ...to here, if this component manager was not registered
				}
				archive << false;
			}
		}
	};
}

#endif // WI_ENTITY_COMPONENT_SYSTEM_H
