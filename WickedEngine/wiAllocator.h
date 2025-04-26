#pragma once
#include "CommonInclude.h"
#include "wiVector.h"

#include "Utility/offsetAllocator.hpp"

#include <mutex>
#include <atomic>
#include <memory>
#include <cassert>
#include <algorithm>

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

	// Allocation and freeing of single blocks of the same size
	template<typename T, size_t block_size = 256>
	struct BlockAllocator
	{
		struct Block
		{
			wi::vector<uint8_t> mem;
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
				block.mem.resize(sizeof(T) * block_size);
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
	};

	// Allocation and freeing of a number of bytes, managed in pages of the same size, with refcounting and auto freeing on destruction and thread safety
	struct PageAllocator
	{
		uint32_t page_count = 0;
		uint32_t page_size = 0;
		struct AllocationInternal
		{
			std::atomic<uint32_t> refcount{ 0 };
			OffsetAllocator::Allocation allocation;
		};
		struct AllocatorInternal
		{
			std::mutex locker;
			OffsetAllocator::Allocator allocator;
			BlockAllocator<AllocationInternal> internal_blocks;
		};
		std::shared_ptr<AllocatorInternal> allocator;

		void init(uint64_t total_size_in_bytes, uint32_t page_size = 64 * 1024)
		{
			this->page_count = uint32_t(align(total_size_in_bytes, (uint64_t)page_size) / (uint64_t)page_size);
			this->page_size = page_size;
			allocator = std::make_shared<AllocatorInternal>();
			allocator->allocator.init(page_count);
		}

		constexpr uint64_t GetTotalSizeInBytes() const { return uint64_t(page_count) * uint64_t(page_size); }

		struct Allocation
		{
			std::shared_ptr<AllocatorInternal> allocator;
			AllocationInternal* internal_state = nullptr;
			uint64_t byte_offset = ~0ull;

			Allocation() = default;
			Allocation(const Allocation& other)
			{
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
				allocator = std::move(other.allocator);
				internal_state = other.internal_state;
				byte_offset = other.byte_offset;
				other.allocator = nullptr;
				other.internal_state = nullptr;
				other.byte_offset = ~0ull;
			}
			~Allocation()
			{
				if (IsValid())
				{
					if (internal_state->refcount.fetch_sub(1) == 1)
					{
						std::scoped_lock lck(allocator->locker);
						allocator->allocator.free(internal_state->allocation);
						allocator->internal_blocks.free(internal_state);
					}
				}
			}
			void operator=(const Allocation& other)
			{
				allocator = other.allocator;
				internal_state = other.internal_state;
				byte_offset = other.byte_offset;
				if (internal_state != nullptr)
				{
					internal_state->refcount.fetch_add(1);
				}
			}
			void operator=(Allocation&& other) noexcept
			{
				allocator = std::move(other.allocator);
				internal_state = other.internal_state;
				byte_offset = other.byte_offset;
				other.allocator = nullptr;
				other.internal_state = nullptr;
				other.byte_offset = ~0ull;
			}

			constexpr bool IsValid() const { return byte_offset != ~0ull; }
		};

		Allocation allocate(size_t sizeInBytes)
		{
			const uint32_t pages = uint32_t(align((uint64_t)sizeInBytes, (uint64_t)page_size) / (uint64_t)page_size);
			std::scoped_lock lck(allocator->locker);
			OffsetAllocator::Allocation offsetallocation = allocator->allocator.allocate(pages);
			Allocation alloc;
			if (offsetallocation.offset != OffsetAllocator::Allocation::NO_SPACE)
			{
				alloc.allocator = allocator;
				alloc.internal_state = allocator->internal_blocks.allocate();
				alloc.internal_state->refcount.fetch_add(1);
				alloc.internal_state->allocation = offsetallocation;
				alloc.byte_offset = offsetallocation.offset * page_size;
			}
			return alloc;
		}
	};
}
