#pragma once
#include <memory>
#include <cstdint>
#include <cassert>
#include <vector>

namespace wiAllocators
{
	// Simple and efficient allocator that reserves a linear memory buffer and can:
	//	- allocate bottom-up until there is space
	//	- free from the last allocation top-down, for temporary allocations
	//	- reset all allocations at once
	class LinearAllocator
	{
	public:

		constexpr size_t get_capacity() const
		{
			return capacity;
		}

		inline void reserve(size_t newCapacity)
		{
			capacity = newCapacity;

			std::free(buffer);
			buffer = (uint8_t*)std::malloc(capacity);
		}

		constexpr uint8_t* allocate(size_t size)
		{
			if (offset + size <= capacity)
			{
				uint8_t* ret = &buffer[offset];
				offset += size;
				return ret;
			}
			return nullptr;
		}

		constexpr void free(size_t size)
		{
			assert(offset >= size);
			offset -= size;
		}

		constexpr void reset()
		{
			offset = 0;
		}

		constexpr uint8_t* top()
		{
			return buffer + offset;
		}

	private:
		uint8_t* buffer = nullptr;
		size_t capacity = 0;
		size_t offset = 0;
		size_t alignment = 1;
	};

}
