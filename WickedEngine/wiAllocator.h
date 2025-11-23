#pragma once
#include "CommonInclude.h"
#include "wiVector.h"
#include "wiSpinLock.h"

#include "Utility/offsetAllocator.hpp"

#include <mutex>
#include <atomic>
#include <memory>
#include <cassert>
#include <algorithm>
#include <deque>

namespace wi::allocator
{
	// Allocation of consecutive bytes, but no freeing, instead the whole allocator can be reset
	struct LinearAllocator
	{
		uint8_t* data = nullptr;
		size_t capacity = 0;
		size_t offset = 0;

		constexpr void init(void* mem, size_t size)
		{
			data = (uint8_t*)mem;
			capacity = size;
			reset();
		}
		constexpr uint8_t* allocate(size_t size)
		{
			if (offset + size >= capacity)
				return nullptr;
			uint8_t* ptr = data + offset;
			offset += size;
			return ptr;
		}
		constexpr void free(size_t size)
		{
			size = std::min(size, offset);
			offset -= size;
		}
		constexpr void reset()
		{
			offset = 0;
		}
	};

	// Allocation and freeing of single elements of the same size
	template<typename T, size_t block_size = 256>
	struct BlockAllocator
	{
		struct Block
		{
			struct alignas(alignof(T)) RawStruct
			{
				uint8_t data[sizeof(T)];
			};
			wi::vector<RawStruct> mem;
		};
		wi::vector<Block> blocks;
		wi::vector<T*> free_list;

		template<typename... ARG>
		inline T* allocate(ARG&&... args)
		{
			if (free_list.empty())
			{
				free_list.reserve(block_size);
				Block& block = blocks.emplace_back();
				block.mem.resize(block_size);
				T* ptr = (T*)block.mem.data();
				for (size_t i = 0; i < block_size; ++i)
				{
					free_list.push_back(ptr + i);
				}
			}
			T* ptr = free_list.back();
			free_list.pop_back();
			return new (ptr) T(std::forward<ARG>(args)...);
		}
		inline void free(T* ptr)
		{
			ptr->~T();
			free_list.push_back(ptr);
		}

		inline bool is_empty() const
		{
			return (blocks.size() * block_size) == free_list.size();
		}
	};

	// Allocation and freeing of an arbitrary number of bytes, managed in pages of the same size
	//	- this is a wrapper around OffsetAllocator that adds thread safety and refcounting
	//	- also supports deferred release for suballocated GPU resources
	struct PageAllocator
	{
		uint32_t page_count = 0;
		uint32_t page_size = 0;
		struct AllocationInternal
		{
			std::atomic<int> refcount{ 0 };
			OffsetAllocator::Allocation allocation;
		};
		struct AllocatorInternal
		{
			std::mutex locker;
			OffsetAllocator::Allocator allocator;
			BlockAllocator<AllocationInternal> internal_blocks;
			bool deferred_release_enabled = false;
			uint64_t deferred_release_frame = 0;
			std::deque<std::pair<OffsetAllocator::Allocation, uint64_t>> deferred_release_queue;
		};
		std::shared_ptr<AllocatorInternal> allocator; // shared ptr is used to let any allocations extend the lifeftime of the allocator

		// Returns the total size that the allocator manages:
		constexpr uint64_t total_size_in_bytes() const { return uint64_t(page_count) * uint64_t(page_size); }

		// Calculates the page count that will accomodate an allocation size request
		constexpr uint32_t page_count_from_bytes(uint64_t sizeInBytes) const { return uint32_t(align((uint64_t)sizeInBytes, (uint64_t)page_size) / (uint64_t)page_size); }

		// Initializes the allocator, only after which it can be used
		//	total_size_in_bytes	:	the allocator will manage this number of bytes
		//	page_size			:	the allocation granularity in bytes, each allocation will be aligned to this
		//	deferred_release	:	if false, allocations are freed immediately (suitable for CPU only allocations), otherwise they are freed after a number of frames passed (which should be used for GPU allocations)
		void init(uint64_t total_size_in_bytes, uint32_t page_size = 64u * 1024u, bool deferred_release = false)
		{
			this->page_size = page_size;
			this->page_count = page_count_from_bytes(total_size_in_bytes);
			allocator = std::make_shared<AllocatorInternal>();
			allocator->allocator.init(page_count, std::min(page_count, OffsetAllocator::default_maxallocations));
			allocator->deferred_release_enabled = deferred_release;
			allocator->deferred_release_frame = 0;
			allocator->deferred_release_queue.clear();
		}
		// This needs to be called every frame if deferred release is enabled:
		void update_deferred_release(uint64_t framecount, uint32_t buffercount)
		{
			if (allocator == nullptr)
				return;
			std::scoped_lock lck(allocator->locker);
			allocator->deferred_release_frame = framecount;
			while (!allocator->deferred_release_queue.empty() && allocator->deferred_release_queue.front().second + buffercount < framecount)
			{
				allocator->allocator.free(allocator->deferred_release_queue.front().first);
				allocator->deferred_release_queue.pop_front();
			}
		}

		struct Allocation
		{
			std::shared_ptr<AllocatorInternal> allocator; // the allocator is retained so that allocation can deallocate itself
			AllocationInternal* internal_state = nullptr; // this is pointing within the allocator which is retained by shared_ptr
			uint64_t byte_offset = ~0ull;

			Allocation()
			{
				Reset();
			}
			Allocation(const Allocation& other)
			{
				Reset();
				allocator = other.allocator;
				internal_state = other.internal_state;
				byte_offset = other.byte_offset;
				if (internal_state != nullptr)
				{
					internal_state->refcount.fetch_add(1);
				}
			}
			Allocation(Allocation&& other) noexcept
			{
				Reset();
				allocator = std::move(other.allocator);
				internal_state = std::move(other.internal_state);
				byte_offset = other.byte_offset;
				other.allocator = nullptr;
				other.internal_state = nullptr;
				other.byte_offset = ~0ull;
			}
			~Allocation()
			{
				Reset();
			}
			Allocation& operator=(const Allocation& other)
			{
				Reset();
				allocator = other.allocator;
				internal_state = other.internal_state;
				byte_offset = other.byte_offset;
				if (internal_state != nullptr)
				{
					internal_state->refcount.fetch_add(1);
				}
				return *this;
			}
			Allocation& operator=(Allocation&& other) noexcept
			{
				Reset();
				allocator = std::move(other.allocator);
				internal_state = std::move(other.internal_state);
				byte_offset = other.byte_offset;
				other.allocator = nullptr;
				other.internal_state = nullptr;
				other.byte_offset = ~0ull;
				return *this;
			}
			void Reset()
			{
				if (IsValid() && (internal_state->refcount.fetch_sub(1) <= 1))
				{
					std::scoped_lock lck(allocator->locker);
					if (allocator->deferred_release_enabled)
					{
						// can only be reclaimed after buffering amount of frames passed, this is usually used for GPU resources:
						allocator->deferred_release_queue.push_back(std::make_pair(internal_state->allocation, allocator->deferred_release_frame));
					}
					else
					{
						// reclaimed immediately:
						allocator->allocator.free(internal_state->allocation);
					}
					allocator->internal_blocks.free(internal_state);
				}
				allocator = {};
				internal_state = nullptr;
				byte_offset = ~0ull;
			}

			constexpr bool IsValid() const { return internal_state != nullptr; }
		};

		// Allocates a reference counted allocation, viewing at least the requested amount of bytes
		//	To check if the allocation succeeded, call IsValid() on the returned object
		inline Allocation allocate(size_t sizeInBytes)
		{
			const uint32_t pages = page_count_from_bytes(sizeInBytes);
			std::scoped_lock lck(allocator->locker);
			OffsetAllocator::Allocation offsetallocation = allocator->allocator.allocate(pages);
			Allocation alloc;
			if (offsetallocation.offset != OffsetAllocator::Allocation::NO_SPACE)
			{
				alloc.allocator = allocator;
				alloc.internal_state = allocator->internal_blocks.allocate();
				alloc.internal_state->refcount.store(1);
				alloc.internal_state->allocation = offsetallocation;
				alloc.byte_offset = offsetallocation.offset * page_size;
			}
			return alloc;
		}

		// returns true if no pages are allocated
		inline bool is_empty() const
		{
			return allocator->allocator.storageReport().totalFreeSpace == page_count;
		}
	};




	// Interface for allocating pooled shared_ptr
	struct SharedBlockAllocator
	{
		virtual void init_refcount(void* ptr) = 0;
		virtual uint32_t get_refcount(void* ptr) = 0;
		virtual uint32_t inc_refcount(void* ptr) = 0;
		virtual uint32_t dec_refcount(void* ptr, bool destruct_on_zero = false) = 0;
		virtual uint32_t get_refcount_weak(void* ptr) = 0;
		virtual uint32_t inc_refcount_weak(void* ptr) = 0;
		virtual uint32_t dec_refcount_weak(void* ptr) = 0;
	};

	// The per-type block allocators can be indexed with bottom 8 bits of the shared_ptr's handle:
	inline SharedBlockAllocator* block_allocators[256] = {};
	inline std::atomic<uint8_t> next_allocator_id{ 0 };
	inline uint8_t register_shared_block_allocator(SharedBlockAllocator* allocator)
	{
		uint8_t id = next_allocator_id.fetch_add(1);
		assert(id < arraysize(block_allocators));
		block_allocators[id] = allocator;
		return id;
	}
	inline uint8_t get_shared_block_allocator_count() { return next_allocator_id.load(); }

	// Shared ptr using a block allocation strategy, refcounted, thread-safe, reduced size using single uint64_t handle
	//	This makes it easy to swap-out std::shared_ptr, but not feature complete, only has minimal feature set
	//	Use this if you require many object of the same type, their memory allocation will be pooled
	//	If you require just a single object, it will be better to use std::shared_ptr instead
	template<typename T>
	struct shared_ptr
	{
		uint64_t handle = 0;

		constexpr bool IsValid() const { return handle != 0; }

		constexpr T* get_ptr() const { return (T*)(handle & (~0ull << 8ull)); }
		constexpr SharedBlockAllocator* get_allocator() const { return block_allocators[handle & 0xFF]; }

		constexpr T* operator->() const { return get_ptr(); }
		constexpr operator T* () const { return get_ptr(); }
		constexpr T* get() const { return get_ptr(); }

		template<typename U>
		operator shared_ptr<U>& () const { return *(shared_ptr<U>*)this; }

		shared_ptr() = default;
		shared_ptr(const shared_ptr& other) { copy(other); }
		shared_ptr(shared_ptr&& other) noexcept { move(other); }
		~shared_ptr() noexcept { reset(); }
		shared_ptr& operator=(const shared_ptr& other) { copy(other); return *this; }
		shared_ptr& operator=(shared_ptr&& other) noexcept { move(other); return *this; }

		void reset() noexcept
		{
			if (IsValid())
			{
				get_allocator()->dec_refcount(get_ptr());
			}
			handle = 0;
		}
		void copy(const shared_ptr& other)
		{
			reset();
			handle = other.handle;
			if (IsValid())
			{
				get_allocator()->inc_refcount(get_ptr());
			}
		}
		void move(shared_ptr& other) noexcept
		{
			if (this == &other)
				return;
			reset();
			handle = other.handle;
			other.handle = 0;
		}
		uint32_t use_count() const { return IsValid() ? get_allocator()->get_refcount(get_ptr()) : 0; }
	};

	// Similar to std::weak_ptr but works with the shared block allocator, and reduced feature set
	template<typename T>
	struct weak_ptr
	{
		uint64_t handle = 0;

		constexpr bool IsValid() const { return handle != 0; }

		constexpr T* get_ptr() const { return (T*)(handle & (~0ull << 8ull)); }
		constexpr SharedBlockAllocator* get_allocator() const { return block_allocators[handle & 0xFF]; }

		template<typename U>
		operator weak_ptr<U>& () const { return *(weak_ptr<U>*)this; }

		weak_ptr() = default;
		weak_ptr(const weak_ptr& other) { copy(other); }
		weak_ptr(weak_ptr&& other) noexcept { move(other); }
		~weak_ptr() noexcept { reset(); }
		weak_ptr& operator=(const weak_ptr& other) { copy(other); return *this; }
		weak_ptr& operator=(weak_ptr&& other) noexcept { move(other); return *this; }

		weak_ptr(const shared_ptr<T>& other)
		{
			reset();
			handle = other.handle;
			if (IsValid())
			{
				get_allocator()->inc_refcount_weak(get_ptr());
			}
		}

		shared_ptr<T> lock()
		{
			if (!IsValid())
				return {};

			SharedBlockAllocator* alloc = get_allocator();
			T* ptr = get_ptr();

			uint32_t old_strong = alloc->inc_refcount(ptr);
			if (old_strong == 0)
			{
				alloc->dec_refcount(ptr, false); // undo refcount
				return {};
			}

			shared_ptr<T> ret;
			ret.handle = handle;
			return ret; // Already incremented refcount
		}

		void reset() noexcept
		{
			if (IsValid())
			{
				get_allocator()->dec_refcount_weak(get_ptr());
			}
			handle = 0;
		}
		void copy(const weak_ptr& other)
		{
			reset();
			handle = other.handle;
			if (IsValid())
			{
				get_allocator()->inc_refcount_weak(get_ptr());
			}
		}
		void move(weak_ptr& other) noexcept
		{
			if (this == &other)
				return;
			reset();
			handle = other.handle;
			other.handle = 0;
		}
		uint32_t use_count() const { return get_allocator()->get_refcount(get_ptr()); }
		bool expired() const noexcept
		{
			return !IsValid() || use_count() == 0;
		}
	};

	// Implementation of a thread-safe refcounted block allocator
	template<typename T, size_t block_size = 256>
	struct SharedBlockAllocatorImpl final : public SharedBlockAllocator
	{
		const uint8_t allocator_id = register_shared_block_allocator(this);

		struct alignas(std::max(size_t(256), alignof(T))) RawStruct // 256 alignment is used at least because I use bottom 8 bits of pointer as allocator id
		{
			uint8_t data[sizeof(T)];
			std::atomic<uint32_t> refcount;
			std::atomic<uint32_t> refcount_weak;
		};
		static_assert(offsetof(RawStruct, data) == 0); // we assume that data is located at 0 when casting ptr to T*, this avoids having to do a function call that would return T* like the refcounts

		struct Block
		{
			std::unique_ptr<RawStruct[]> mem;
		};
		wi::vector<Block> blocks;
		wi::vector<RawStruct*> free_list;
		//std::mutex locker;
		wi::SpinLock locker;

		template<typename... ARG>
		inline shared_ptr<T> allocate(ARG&&... args)
		{
			locker.lock();
			if (free_list.empty())
			{
				Block& block = blocks.emplace_back();
				block.mem.reset(new RawStruct[block_size]);
				RawStruct* ptr = block.mem.get();
				free_list.reserve(block_size);
				for (size_t i = 0; i < block_size; ++i)
				{
					free_list.push_back(ptr + i);
				}
			}
			RawStruct* ptr = free_list.back();
			assert((uint64_t)ptr == ((uint64_t)ptr & (~0ull << 8ull))); // The pointer lower 8 bits must be 0, it will be used as allocator index
			free_list.pop_back();
			locker.unlock();

			// Construction can be outside of lock, this structure wasn't shared yet:
			new (ptr) T(std::forward<ARG>(args)...);
			init_refcount(ptr);
			shared_ptr<T> allocation;
			allocation.handle = uint64_t(ptr) | uint64_t(allocator_id);
			return allocation;
		}

		void reclaim(void* ptr)
		{
			std::scoped_lock lck(locker);
			free_list.push_back((RawStruct*)ptr);
		}

		void init_refcount(void* ptr) override
		{
			static_cast<RawStruct*>(ptr)->refcount.store(1, std::memory_order_relaxed);
			static_cast<RawStruct*>(ptr)->refcount_weak.store(1, std::memory_order_relaxed);
		}
		uint32_t get_refcount(void* ptr) override
		{
			return static_cast<RawStruct*>(ptr)->refcount.load(std::memory_order_acquire);
		}
		uint32_t inc_refcount(void* ptr) override
		{
			return static_cast<RawStruct*>(ptr)->refcount.fetch_add(1, std::memory_order_relaxed);
		}
		uint32_t dec_refcount(void* ptr, bool destruct_on_zero) override
		{
			uint32_t old = static_cast<RawStruct*>(ptr)->refcount.fetch_sub(1, std::memory_order_acq_rel);
			if (old == 1)
			{
				if (destruct_on_zero) static_cast<T*>(ptr)->~T();
				dec_refcount_weak(ptr);
			}
			return old;
		}
		uint32_t get_refcount_weak(void* ptr) override
		{
			return static_cast<RawStruct*>(ptr)->refcount_weak.load(std::memory_order_acquire);
		}
		uint32_t inc_refcount_weak(void* ptr) override
		{
			return static_cast<RawStruct*>(ptr)->refcount_weak.fetch_add(1, std::memory_order_relaxed);
		}
		uint32_t dec_refcount_weak(void* ptr) override
		{
			uint32_t old = static_cast<RawStruct*>(ptr)->refcount_weak.fetch_sub(1, std::memory_order_acq_rel);
			if (old == 1)
			{
				reclaim(ptr);
			}
			return old;
		}
	};

	// The allocators are global intentionally, this avoids runtime construction, guard check
	template<typename T, size_t block_size = 256>
	inline static SharedBlockAllocatorImpl<T, block_size>* shared_block_allocator = new SharedBlockAllocatorImpl<T, block_size>; // only destroyed after program exit, never earlier

	// Create a new shared pooled object:
	template<typename T, size_t block_size = 256, typename... ARG>
	inline shared_ptr<T> make_shared(ARG&&... args)
	{
		return shared_block_allocator<T, block_size>->allocate(std::forward<ARG>(args)...);
	}

}
