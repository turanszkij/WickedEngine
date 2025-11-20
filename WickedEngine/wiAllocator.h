#pragma once
#include "CommonInclude.h"
#include "wiVector.h"

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




	// Allocation of single elements of implementation-defined size with block allocation strategy, refcounted, thread-safe
	struct InternalStateAllocator
	{
		struct Entry
		{
			void* ptr = nullptr;
			volatile long* refcount = nullptr;
			size_t block_id = 0;
		};
		wi::vector<Entry> free_list;
		std::mutex locker;

		struct Allocation
		{
			InternalStateAllocator* allocator = nullptr;
			Entry entry;

			operator void* () const { return entry.ptr; }
			void* get() const { return entry.ptr; }

			constexpr bool IsValid() const { return entry.ptr != nullptr; }

			Allocation() = default;
			Allocation(const Allocation& other) { copy(other); }
			Allocation(Allocation&& other) { move(other); }
			virtual ~Allocation() { destroy(); }
			Allocation& operator=(const Allocation& other)
			{
				copy(other);
				return *this;
			}
			Allocation& operator=(Allocation&& other) noexcept
			{
				move(other);
				return *this;
			}

			void destroy()
			{
				if (allocator != nullptr && entry.ptr != nullptr && entry.refcount != nullptr)
				{
					int prev = AtomicAdd(entry.refcount, -1);
					if (prev == 1)
					{
						allocator->reclaim(entry);
					}
				}
				allocator = nullptr;
				entry = {};
			}
			void copy(const Allocation& other)
			{
				destroy();
				allocator = other.allocator;
				entry = other.entry;
				if (entry.refcount != nullptr)
				{
					AtomicAdd(entry.refcount, 1);
				}
			}
			void move(Allocation& other)
			{
				destroy();
				allocator = other.allocator;
				entry = other.entry;
				other.allocator = nullptr;
				other.entry = {};
			}
		};

		virtual void reclaim(const Entry& entry) = 0;
	};

	template<typename T>
	struct InternalStateAllocatorImpl final : public InternalStateAllocator
	{
		static constexpr size_t block_size = 256;
		struct Block
		{
			struct alignas(alignof(T)) RawStruct
			{
				uint8_t data[sizeof(T)];
			};
			wi::vector<RawStruct> mem;
			wi::vector<long> refcounts;
		};
		wi::vector<Block> blocks;

		struct AllocationImpl final : public InternalStateAllocator::Allocation
		{
			T* operator->() { return (T*)entry.ptr; }
			operator T*() { return (T*)entry.ptr; }
			T* get() { return (T*)entry.ptr; }
			operator InternalStateAllocator::Allocation& () const { return *this; }
			static AllocationImpl& from(InternalStateAllocator::Allocation& other) { return *(AllocationImpl*)&other; }
			static const AllocationImpl& from(const InternalStateAllocator::Allocation& other) { return *(const AllocationImpl*)&other; }
		};

		template<typename... ARG>
		inline AllocationImpl allocate(ARG&&... args)
		{
			locker.lock();
			if (free_list.empty())
			{
				free_list.reserve(block_size);
				const size_t block_id = blocks.size();
				Block& block = blocks.emplace_back();
				block.mem.resize(block_size);
				block.refcounts.resize(block_size);
				T* ptr = (T*)block.mem.data();
				volatile long* refcount = (volatile long*)block.refcounts.data();
				for (size_t i = 0; i < block_size; ++i)
				{
					Entry entry;
					entry.ptr = ptr + i;
					entry.refcount = refcount + i;
					entry.block_id = block_id;
					free_list.push_back(entry);
				}
			}
			Entry entry = free_list.back();
			free_list.pop_back();
			locker.unlock();

			// construct can happen outside of lock
			AllocationImpl allocation;
			allocation.allocator = this;
			allocation.entry = entry;
			new (allocation.entry.ptr) T(std::forward<ARG>(args)...);
			*allocation.entry.refcount = 1;
			return allocation;
		}

		void reclaim(const Entry& entry) override
		{
			((T*)entry.ptr)->~T();
			std::scoped_lock lck(locker);
			free_list.push_back(entry);
		}
	};

	template<typename T>
	inline auto make_internal()
	{
		static InternalStateAllocatorImpl<T> allocator;
		return allocator.allocate();
	}
	template<typename T>
	inline auto upcast_internal(const InternalStateAllocator::Allocation& allocation)
	{
		return InternalStateAllocatorImpl<T>::AllocationImpl::from(allocation);
	}
}
