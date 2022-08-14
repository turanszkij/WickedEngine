#ifndef WI_ENTITY_COMPONENT_SYSTEM_H
#define WI_ENTITY_COMPONENT_SYSTEM_H

#include "wiArchive.h"
#include "wiJobSystem.h"
#include "wiUnorderedMap.h"
#include "wiUnorderedSet.h"
#include "wiVector.h"

#include <cstdint>
#include <cassert>
#include <atomic>
#include <memory>

// Entity-Component System
namespace wi::ecs
{
	// The Entity is a global unique persistent identifier within the entity-component system
	//	It can be stored and used for the duration of the application
	//	The entity can be a different value on a different run of the application, if it was serialized
	//	It must be only serialized with the SerializeEntity() function. It will ensure that entities still match with their components correctly after serialization
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
		wi::jobsystem::context ctx; // allow components to spawn serialization subtasks
		wi::unordered_map<uint64_t, Entity> remap;
		bool allow_remap = true;

		~EntitySerializer()
		{
			wi::jobsystem::Wait(ctx); // automatically wait for all subtasks after serialization
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
			archive << entity;
		}
	}

	// The ComponentManager is a container that stores components and matches them with entities
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
		inline void Serialize(wi::Archive& archive, EntitySerializer& seri)
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

		// Retrieve component index by entity handle (if not exists, returns ~0ull value)
		inline size_t GetIndex(Entity entity) const 
		{
			const auto it = lookup.find(entity);
			if (it != lookup.end())
			{
				return it->second;
			}
			return ~0ull;
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
		// This is a lookup table for entities
		wi::unordered_map<Entity, size_t> lookup;

		// Disallow this to be copied by mistake
		ComponentManager(const ComponentManager&) = delete;
	};

	// The ComponentManager_Generic is a container interface for ContainerManager, abstracts underlying ComponentManager
	//  
	// How to initialize:
	// wi::ecs::ComponentManager_Generic names = wi::ecs::ComponentManager_MakeGeneric(NameComponent);
	class ComponentManager_Generic
	{
	public:
		std::shared_ptr<void> componentManager;
		ComponentManager_Generic(
			std::shared_ptr<void> _componentManager,
			std::function<void(std::shared_ptr<void>)> _impl_Clear,
			std::function<void(std::shared_ptr<void>,std::shared_ptr<void>)> _impl_Copy,
			std::function<void(std::shared_ptr<void>,std::shared_ptr<void>)> _impl_Merge,
			std::function<void(std::shared_ptr<void>,wi::Archive&,EntitySerializer&)> _impl_Serialize,
			std::function<void(std::shared_ptr<void>,Entity,wi::Archive&,EntitySerializer&)> _impl_Entity_Serialize,
			std::function<void(std::shared_ptr<void>,Entity)> _impl_Create,
			std::function<void(std::shared_ptr<void>,Entity)> _impl_Remove,
			std::function<void(std::shared_ptr<void>,Entity)> _impl_Remove_KeepSorted,
			std::function<void(std::shared_ptr<void>,size_t,size_t)> _impl_MoveItem,
			std::function<bool(std::shared_ptr<void>,Entity)> _impl_Contains,
			std::function<size_t(std::shared_ptr<void>,Entity)> _impl_GetIndex,
			std::function<size_t(std::shared_ptr<void>)> _impl_GetCount,
			std::function<size_t(std::shared_ptr<void>,size_t)> _impl_GetEntity,
			std::function<std::shared_ptr<wi::vector<Entity>>(std::shared_ptr<void>)> _impl_GetEntityArray
		){
			componentManager = _componentManager;
			impl_Clear = _impl_Clear;
			impl_Copy = _impl_Copy;
			impl_Merge = _impl_Merge;
			impl_Serialize = _impl_Serialize;
			impl_Entity_Serialize = _impl_Entity_Serialize;
			impl_Create = _impl_Create;
			impl_Remove = _impl_Remove;
			impl_Remove_KeepSorted = _impl_Remove_KeepSorted;
			impl_MoveItem = _impl_MoveItem;
			impl_Contains = _impl_Contains;
			impl_GetIndex = _impl_GetIndex;
			impl_GetCount = _impl_GetCount;
			impl_GetEntity = _impl_GetEntity;
			impl_GetEntityArray = _impl_GetEntityArray;
		}
		inline void Clear(){ impl_Clear(componentManager); }
		inline void Copy(std::shared_ptr<void> other){ impl_Copy(componentManager, other); }
		inline void Merge(std::shared_ptr<void> other){ impl_Copy(componentManager, other); }
		inline void Serialize(wi::Archive& archive, EntitySerializer& seri){ impl_Serialize(componentManager, archive, seri); }
		inline void Entity_Serialize(Entity entity, wi::Archive& archive, EntitySerializer& seri){ impl_Entity_Serialize(componentManager, entity, archive, seri); }
		inline void Create(Entity entity){ impl_Create(componentManager, entity); }
		inline void Remove(Entity entity){ impl_Remove(componentManager, entity); }
		inline void Remove_KeepSorted(Entity entity){ impl_Remove_KeepSorted(componentManager, entity); }
		inline void MoveItem(size_t index_from, size_t index_to){ impl_MoveItem(componentManager, index_from, index_to); }
		inline bool Contains(Entity entity) const{ return impl_Contains(componentManager, entity); }
		inline size_t GetIndex(Entity entity) const{ return impl_GetIndex(componentManager, entity); }
		inline size_t GetCount() const{ return impl_GetCount(componentManager); }
		inline const wi::vector<Entity>& GetEntityArray() const{ return *impl_GetEntityArray(componentManager).get(); }
	private:
		std::function<void(std::shared_ptr<void>)> impl_Clear;
		std::function<void(std::shared_ptr<void>,std::shared_ptr<void>)> impl_Copy;
		std::function<void(std::shared_ptr<void>,std::shared_ptr<void>)> impl_Merge;
		std::function<void(std::shared_ptr<void>,wi::Archive&,EntitySerializer&)> impl_Serialize;
		std::function<void(std::shared_ptr<void>,Entity,wi::Archive&,EntitySerializer&)> impl_Entity_Serialize;
		std::function<void(std::shared_ptr<void>,Entity)> impl_Create;
		std::function<void(std::shared_ptr<void>,Entity)> impl_Remove;
		std::function<void(std::shared_ptr<void>,Entity)> impl_Remove_KeepSorted;
		std::function<void(std::shared_ptr<void>,size_t,size_t)> impl_MoveItem;
		std::function<bool(std::shared_ptr<void>,Entity)> impl_Contains;
		std::function<size_t(std::shared_ptr<void>,Entity)> impl_GetIndex;
		std::function<size_t(std::shared_ptr<void>)> impl_GetCount;
		std::function<size_t(std::shared_ptr<void>,size_t)> impl_GetEntity;
		std::function<std::shared_ptr<wi::vector<Entity>>(std::shared_ptr<void>)> impl_GetEntityArray;
	};

	// This is a macro to quicky convert ComponentManager into generic container
	#define ComponentManager_MakeGeneric(ComponentType,ComponentManagerVariable) ComponentManager_Generic( \
		ComponentManagerVariable, \
		[=](std::shared_ptr<void> compMgr_ptr){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			compMgr->Clear(); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, std::shared_ptr<void> other_ptr){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			auto other = static_cast< ComponentManager< ComponentType > *>(other_ptr.get()); \
			compMgr->Copy(*other); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, std::shared_ptr<void> other_ptr){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			auto other = static_cast< ComponentManager< ComponentType > *>(other_ptr.get()); \
			compMgr->Merge(*other); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, wi::Archive& archive, EntitySerializer& seri){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			compMgr->Serialize(archive, seri); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, Entity entity, wi::Archive& archive, EntitySerializer& seri){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			if(archive.IsReadMode()) { \
				auto component = compMgr->Create(entity); \
				component.Serialize(archive, seri); \
			} else { \
				auto component = compMgr->GetComponent(entity); \
				if (component != nullptr) { \
					component->Serialize(archive, seri); \
				} \
			} \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, Entity entity){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			compMgr->Create(entity); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, Entity entity){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			compMgr->Remove(entity); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, Entity entity){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			compMgr->Remove_KeepSorted(entity); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, size_t index_from, size_t index_to){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			compMgr->MoveItem(index_from, index_to); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, Entity entity){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			return compMgr->Contains(entity); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, Entity entity){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			return compMgr->GetIndex(entity); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			return compMgr->GetCount(); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr, size_t index){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			return compMgr->GetEntity(index); \
		}, \
		[=](std::shared_ptr<void> compMgr_ptr){ \
			auto compMgr = static_cast< ComponentManager< ComponentType > *>(compMgr_ptr.get()); \
			return std::make_shared<wi::vector<Entity>>(compMgr->GetEntityArray()); \
		} \
	)

	// This is macro to quickly create an initializer
	#define ComponentManager_Generic_CreateInitializer( ComponentType ) [=](){ \
		auto generic = std::make_shared<ComponentManager_Generic>(wi::ecs::ComponentManager_MakeGeneric( ComponentType , std::make_shared<ComponentManager>())); \
		return generic; \
	}

	// This is a container that stores componentManager initializers, required for ComponentManagerContainer
	struct ComponentManagerListInitializer
	{
		wi::unordered_map<std::string, std::shared_ptr<std::function<std::shared_ptr<ComponentManager_Generic>()>>> initializers;
		// Register your own ComponentManager initializer here
		// How to use:
		// RegsiterInitializer("names",ComponentManager_Generic_CreateInitializer(NameComponent))
		void RegisterInitializer(std::string name, std::function<std::shared_ptr<ComponentManager_Generic>()> initialize_func){
			initializers[name] = std::make_shared<std::function<std::shared_ptr<ComponentManager_Generic>()>>(initialize_func);
		}
		// This will be called on the ComponentManagerContainer to build and initialize the list
		void Initialize(wi::unordered_map<std::string, std::shared_ptr<ComponentManager_Generic>>& componentList){
			for(auto& init_kval : initializers){
				componentList[init_kval.first] = (*init_kval.second.get())();
			}
		}
	};

	// This is a container that stores ComponentManagers, for use with scene data manipulation, initializing, and serializing whole data in group
	struct ComponentManagerContainer
	{
		wi::unordered_map<std::string, std::shared_ptr<ComponentManager_Generic>> componentList;
		//Do your own custom initializer
		ComponentManagerContainer();
		//Initialize immediately
		ComponentManagerContainer(std::shared_ptr<ComponentManagerListInitializer> initializer){
			initializer->Initialize(componentList);
		}
		//Initialize at a late stage, if you wish it to be
		void Initialize(std::shared_ptr<ComponentManagerListInitializer> initializer){
			initializer->Initialize(componentList);
		}
		// Set the custom initializer
		// Get one ComponentManager_Generic by name
		ComponentManager_Generic& GetComponentManager_Generic(std::string name){
			return *componentList[name].get();
		}
		// Get one ComponentManager by name
		std::shared_ptr<void> GetComponentManager(std::string name){
			return componentList[name]->componentManager;
		}
		
		// Bulk functions for handling all contained ComponentManagers
		// Clear all data on all ContainerManagers
		void Clear(){
			for(auto& componentManager_kval : componentList){
				componentManager_kval.second->Clear();
			}
		}
		// Copy data from one ComponentManagerContainer to another
		void Copy(ComponentManagerContainer& other){
			for(auto& componentManager_kval : componentList){
				componentManager_kval.second->Copy(GetComponentManager(componentManager_kval.first));
			}
		}
		// Merge data from one ComponentManagerContainer to another
		void Merge(ComponentManagerContainer& other){
			for(auto& componentManager_kval : componentList){
				componentManager_kval.second->Merge(GetComponentManager(componentManager_kval.first));
			}
		}
		// Serialize all components to an archive
		void Serialize(wi::Archive& archive, EntitySerializer& seri){
			for(auto& componentManager_kval : componentList){
				if(archive.IsReadMode()){
					std::string component_id;
					archive >> component_id;
					if(component_id == componentManager_kval.first){
						componentManager_kval.second->Serialize(archive, seri);
					}
				}else{
					archive << componentManager_kval.first;
					componentManager_kval.second->Serialize(archive, seri);
				}
			}
		}
		void Entity_Serialize(Entity entity, wi::Archive& archive,EntitySerializer& seri){
			for(auto& componentManager_kval : componentList){
				bool component_exists;
				if(archive.IsReadMode()){
					archive >> component_exists;
					if(component_exists){
						componentManager_kval.second->Entity_Serialize(entity, archive, seri);
					}
				}else{
					component_exists = componentManager_kval.second->Contains(entity);
					archive << component_exists;
					if(component_exists){
						componentManager_kval.second->Entity_Serialize(entity, archive, seri);
					}
				}
			}
		}
		// Remove an entity from all ComponentManager
		void Remove(Entity entity){
			for(auto& componentManager_kval : componentList){
				componentManager_kval.second->Remove(entity);
			}
		}
		// Remove an entity from all ComponentManager while keeping the ordering on all ComponentManager
		void Remove_KeepSorted(Entity entity){
			for(auto& componentManager_kval : componentList){
				componentManager_kval.second->Remove_KeepSorted(entity);
			}
		}
		// Check from all ComponentManager if an entity exists
		bool Contains(Entity entity){
			for(auto& componentManager_kval : componentList){
				if(componentManager_kval.second->Contains(entity)){
					return true;
				}
			}
			return false;
		}
		std::shared_ptr<wi::unordered_set<wi::ecs::Entity>> GetEntityArray(){
			auto entities = std::make_shared<wi::unordered_set<wi::ecs::Entity>>();
			for(auto& componentManager_kval : componentList){
				entities->insert(componentManager_kval.second->GetEntityArray().begin(), componentManager_kval.second->GetEntityArray().end());
			}
			return entities;
		}
		const std::shared_ptr<wi::unordered_set<wi::ecs::Entity>> GetEntityArray() const{
			auto entities = std::make_shared<wi::unordered_set<wi::ecs::Entity>>();
			for(auto& componentManager_kval : componentList){
				entities->insert(componentManager_kval.second->GetEntityArray().begin(), componentManager_kval.second->GetEntityArray().end());
			}
			return entities;
		}
	};
}

#endif // WI_ENTITY_COMPONENT_SYSTEM_H
