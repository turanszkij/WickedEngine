#pragma once
#include "wiSpinLock.h"

#include <cstdint>
#include <cassert>

namespace wi::allocator
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



	// Fixed size very simple thread safe ring buffer
	template <typename T, size_t capacity>
	class ThreadSafeRingBuffer
	{
	public:
		// Push an item to the end if there is free space
		//	Returns true if succesful
		//	Returns false if there is not enough space
		inline bool push_back(const T& item)
		{
			bool result = false;
			lock.lock();
			size_t next = (head + 1) % capacity;
			if (next != tail)
			{
				data[head] = item;
				head = next;
				result = true;
			}
			lock.unlock();
			return result;
		}

		// Get an item if there are any
		//	Returns true if succesful
		//	Returns false if there are no items
		inline bool pop_front(T& item)
		{
			bool result = false;
			lock.lock();
			if (tail != head)
			{
				item = data[tail];
				tail = (tail + 1) % capacity;
				result = true;
			}
			lock.unlock();
			return result;
		}

	private:
		T data[capacity];
		size_t head = 0;
		size_t tail = 0;
		wi::spinlock lock;
	};

}
