#ifndef WI_VECTOR_REPLACEMENT
#define WI_VECTOR_REPLACEMENT
// This file is used to allow replacement of std::vector

#ifndef WI_VECTOR_TYPE
#define WI_VECTOR_TYPE 0
#endif // WI_VECTOR_TYPE

#if WI_VECTOR_TYPE == 1
#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <initializer_list>
#include <new>
#else
#include <vector>
#endif // WI_VECTOR_TYPE

namespace wi
{
	template<typename T, typename A = std::allocator<T>>
#if WI_VECTOR_TYPE == 0
	using vector = std::vector<T, A>;
#elif WI_VECTOR_TYPE == 1
	class vector
	{
	public:
		using value_type = T;

		inline vector(size_t size = 0)
		{
			resize(size);
		}
		inline vector(const vector<T>& other)
		{
			copy_from(other);
		}
		inline vector(vector<T>&& other) noexcept
		{
			move_from(std::move(other));
		}
		inline vector(std::initializer_list<T> init)
		{
			reserve(init.size());
			for (auto& x : init)
			{
				push_back(x);
			}
		}
		inline ~vector()
		{
			clear(); // need to destroy objects, not just deallocate!
			if (m_data != nullptr)
			{
				m_allocator.deallocate(m_data, m_capacity);
			}
		}
		inline vector<T>& operator=(const vector<T>& other)
		{
			copy_from(other);
			return *this;
		}
		inline vector<T>& operator=(vector<T>&& other)
		{
			move_from(std::move(other));
			return *this;
		}
		constexpr T& operator[](size_t index) noexcept
		{
			assert(m_size > 0 && index < m_size);
			return m_data[index];
		}
		constexpr const T& operator[](size_t index) const noexcept
		{
			assert(m_size > 0 && index < m_size);
			return m_data[index];
		}

		inline void reserve(size_t amount)
		{
			if (amount > m_capacity)
			{
				size_t old_capacity = m_capacity;
				T* old_data = data_allocate(amount);
				if (old_data != nullptr)
				{
					m_allocator.deallocate(old_data, old_capacity);
				}
			}
		}
		inline void resize(size_t amount)
		{
			if (amount > m_size)
			{
				// expand
				reserve(amount);
				// Construct elements with placement new:
				for (size_t i = m_size; i < amount; ++i)
				{
					new (m_data + i) T();
				}
				m_size = amount;
			}
			else if (amount < m_size)
			{
				// shrink
				while (m_size > amount)
				{
					pop_back();
				}
			}
		}
		inline void shrink_to_fit()
		{
			if (m_capacity > m_size)
			{
				size_t old_capacity = m_capacity;
				T* old_data = data_allocate(m_size);
				if (old_data != nullptr)
				{
					m_allocator.deallocate(old_data, old_capacity);
				}
			}
		}

		template<typename... ARG>
		inline T& emplace_back(ARG&&... args)
		{
			size_t old_capacity = m_capacity;
			T* old_data = nullptr;
			const size_t required_capacity = m_size + 1;
			if (required_capacity > m_capacity)
			{
				old_data = data_allocate(required_capacity * 2); // *2: faster grow rate
			}

			// Allocation is separate from construction, this will call object constructor:
			T* ptr = new (m_data + m_size) T(std::forward<ARG>(args)...);
			m_size++;

			// Iterator invalidation can only happen after the new item was constructed,
			//	because the call stack might still contain references to the old data
			if (old_data != nullptr)
			{
				m_allocator.deallocate(old_data, old_capacity);
			}
			return *ptr;
		}
		inline void push_back(const T& item)
		{
			emplace_back(item);
		}
		inline void push_back(T&& item)
		{
			emplace_back(std::move(item));
		}

		constexpr void clear() noexcept
		{
			for (size_t i = 0; i < m_size; ++i)
			{
				m_data[i].~T();
			}
			m_size = 0;
		}
		constexpr void pop_back() noexcept
		{
			assert(m_size > 0);
			m_data[--m_size].~T();
		}
		constexpr void erase(T* pos) noexcept
		{
			assert(pos >= begin());
			assert(pos < end());
			while (pos != end() - 1)
			{
				*pos = std::move(*(pos + 1));
				pos++;
			}
			pop_back();
		}

		constexpr T& back() noexcept
		{
			assert(m_size > 0);
			return m_data[m_size - 1];
		}
		constexpr const T& back() const noexcept
		{
			assert(m_size > 0);
			return m_data[m_size - 1];
		}
		constexpr T& front() noexcept
		{
			assert(m_size > 0);
			return m_data[0];
		}
		constexpr const T& front() const noexcept
		{
			assert(m_size > 0);
			return m_data[0];
		}
		constexpr T* data() noexcept
		{
			return m_data;
		}
		constexpr const T* data() const noexcept
		{
			return m_data;
		}
		constexpr T& at(size_t index) noexcept
		{
			assert(m_size > 0 && index < m_size);
			return m_data[index];
		}
		constexpr const T& at(size_t index) const noexcept
		{
			assert(m_size > 0 && index < m_size);
			return m_data[index];
		}
		constexpr size_t size() const noexcept
		{
			return m_size;
		}
		constexpr bool empty() const noexcept
		{
			return m_size == 0;
		}
		constexpr T* begin() noexcept
		{
			return m_data;
		}
		constexpr const T* begin() const noexcept
		{
			return m_data;
		}
		constexpr T* end() noexcept
		{
			return m_data + m_size;
		}
		constexpr const T* end() const noexcept
		{
			return m_data + m_size;
		}

	private:
		// returns old data ptr if allocation happened. It needs to be freed.
		//	This function must not free it, as iterator invalidation can't happen yet.
		//	References to the old data could be still on current call stack.
		inline T* data_allocate(size_t amount)
		{
			if (amount > m_size) // allocation must always fit all existing items
			{
				m_capacity = amount;
				// We don't use new[] because we don't want to construct every item in the capacity
				//	Instead, placement new is used when a new object is created in the container
				//	Placement new is also used to realloc existing items when reservation grows
				T* allocation = m_allocator.allocate(m_capacity);
				for (size_t i = 0; i < m_size; ++i)
				{
					new (allocation + i) T(std::move(m_data[i]));
				}
				std::swap(m_data, allocation);
				return allocation;
			}
			return nullptr;
		}
		inline void copy_from(const vector<T>& other)
		{
			resize(other.size());
			for (size_t i = 0; i < m_size; ++i)
			{
				m_data[i] = other.m_data[i];
			}
		}
		inline void move_from(vector<T>&& other)
		{
			clear();
			if (m_data != nullptr)
			{
				m_allocator.deallocate(m_data, m_capacity);
			}
			m_capacity = other.m_capacity;
			m_size = other.m_size;
			m_data = other.m_data;
			other.m_data = nullptr;
			other.m_size = 0;
			other.m_capacity = 0;
		}

		T* m_data = nullptr;
		size_t m_size = 0;
		size_t m_capacity = 0;
		A m_allocator;
	};
#endif // WI_VECTOR_TYPE
}

#endif // WI_VECTOR_REPLACEMENT
