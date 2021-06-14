#pragma once
#include <cstdint>
#include <memory>
#include <cassert>

namespace wiAllocators
{
	inline size_t Align(size_t uLocation, size_t uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			assert(0);
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}

	class LinearAllocator
	{
	public:

		~LinearAllocator()
		{
			_mm_free(buffer);
		}

		inline size_t get_capacity() const
		{
			return capacity;
		}

		inline void reserve(size_t newCapacity, size_t align = 1)
		{
			alignment = align;
			newCapacity = Align(newCapacity, alignment);
			capacity = newCapacity;

			_mm_free(buffer);
			buffer = (uint8_t*)_mm_malloc(capacity, alignment);
		}

		inline uint8_t* allocate(size_t size)
		{
			size = Align(size, alignment);
			if (offset + size <= capacity)
			{
				uint8_t* ret = &buffer[offset];
				offset += size;
				return ret;
			}
			return nullptr;
		}

		inline void free(size_t size)
		{
			size = Align(size, alignment);
			assert(offset >= size);
			offset -= size;
		}

		inline void reset()
		{
			offset = 0;
		}

		inline uint8_t* top()
		{
			return &buffer[offset];
		}

	private:
		uint8_t* buffer = nullptr;
		size_t capacity = 0;
		size_t offset = 0;
		size_t alignment = 1;
	};

}
