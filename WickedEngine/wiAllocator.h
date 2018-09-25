#ifndef _WI_ALLOCATORS_H_
#define _WI_ALLOCATORS_H_

#include <cstdint>

namespace wiAllocators
{

	class LinearAllocator
	{
	public:
		~LinearAllocator()
		{
			delete[] buffer;
		}

		inline void reserve(size_t newCapacity)
		{
			if (capacity > 0)
			{
				delete[] buffer;
			}
			capacity = newCapacity;
			buffer = new uint8_t[capacity];
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
		uint8_t* buffer = nullptr;
		size_t capacity = 0;
		size_t offset = 0;
	};

}

#endif // _WI_ALLOCATORS_H_
