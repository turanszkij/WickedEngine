#pragma once
#include <cstdint>
#include <memory>

namespace wiAllocators
{

	class LinearAllocator
	{
	public:

		inline size_t get_capacity() const
		{
			return capacity;
		}

		inline void reserve(size_t newCapacity)
		{
			capacity = newCapacity;
			buffer = std::make_unique<uint8_t[]>(capacity);
		}

		inline uint8_t* allocate(size_t size)
		{
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
			assert(offset >= size);
			offset -= size;
		}

		inline void reset()
		{
			offset = 0;
		}

	private:
		std::unique_ptr<uint8_t[]> buffer;
		size_t capacity = 0;
		size_t offset = 0;
	};

}
