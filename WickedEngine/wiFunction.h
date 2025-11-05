#pragma once
// A minimal replacement for std::function that doesn't use allocation, optimized for performance
// There are two versions:
//	wi::function<>			: simple function wrapper without allocation
//	wi::unsafe_function<>	: simple function wrapper without allocation that doesn't support destruction of objects

#include <type_traits>
#include <utility>
#include <cstring>

namespace wi
{
	template <typename Sig, std::size_t BufferSize>
	class function;

	template <typename R, typename... Args, std::size_t BufferSize>
	class function<R(Args...), BufferSize> {
		struct Storage
		{
			uint8_t bytes[BufferSize];
		};

		struct callable_base
		{
			virtual R call(Args... args) = 0;
			virtual void copy_to(Storage& dest) const = 0;
			virtual void move_to(Storage& dest) noexcept = 0;
			virtual ~callable_base() = default;
		};

		template <typename F>
		struct callable_impl final : callable_base
		{
			F functor;

			template <typename FF>
			callable_impl(FF&& f) : functor(std::forward<FF>(f)) {}

			R call(Args... args) override
			{
				return functor(std::forward<Args>(args)...);
			}

			void copy_to(Storage& dest) const override
			{
				new (&dest.bytes) callable_impl(functor);
			}

			void move_to(Storage& dest) noexcept override
			{
				new (&dest.bytes) callable_impl(std::move(*this));
			}
		};

		void destroy()
		{
			if (ptr)
			{
				ptr->~callable_base();
				ptr = nullptr;
			}
		}

		Storage buffer;
		callable_base* ptr = nullptr;

	public:
		function() = default;

		// Create from lambda
		template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, function> && sizeof(callable_impl<std::decay_t<F>>) <= BufferSize>>
		function(F&& f)
		{
			using FD = std::decay_t<F>;
			static_assert(sizeof(callable_impl<FD>) <= BufferSize, "Callable too large for buffer");
			ptr = new (buffer.bytes) callable_impl<FD>(std::forward<F>(f));
		}

		// Copy
		function(const function& other)
		{
			if (other.ptr)
			{
				other.ptr->copy_to(buffer);
				ptr = reinterpret_cast<callable_base*>(&buffer.bytes);
			}
		}

		function& operator=(const function& other)
		{
			if (this != &other)
			{
				destroy();
				if (other.ptr)
				{
					other.ptr->copy_to(buffer);
					ptr = reinterpret_cast<callable_base*>(&buffer.bytes);
				}
			}
			return *this;
		}

		// Move
		function(function&& other) noexcept
		{
			if (other.ptr)
			{
				other.ptr->move_to(buffer);
				ptr = reinterpret_cast<callable_base*>(&buffer.bytes);
				other.ptr = nullptr;
			}
		}

		function& operator=(function&& other) noexcept
		{
			if (this != &other)
			{
				destroy();
				if (other.ptr)
				{
					other.ptr->move_to(buffer);
					ptr = reinterpret_cast<callable_base*>(&buffer.bytes);
					other.ptr = nullptr;
				}
			}
			return *this;
		}

		// Call
		R operator()(Args... args) const
		{
			return ptr->call(std::forward<Args>(args)...);
		}
	};



	// Reduced function to fit exactly into the space of buffer size
	//	The ptr cannot be validated, so destruction is not supported
	//	Use with caution!
	template <typename Sig, std::size_t BufferSize>
	class unsafe_function;

	template <typename R, typename... Args, std::size_t BufferSize>
	class unsafe_function<R(Args...), BufferSize> {
		struct Storage
		{
			uint8_t bytes[BufferSize];
		};

		struct callable_base
		{
			virtual R call(Args... args) = 0;
			virtual void copy_to(Storage& dest) const = 0;
			virtual void move_to(Storage& dest) noexcept = 0;
			virtual ~callable_base() = default;
		};

		template <typename F>
		struct callable_impl final : callable_base
		{
			F functor;

			template <typename FF>
			callable_impl(FF&& f) : functor(std::forward<FF>(f)) {}

			R call(Args... args) override
			{
				return functor(std::forward<Args>(args)...);
			}

			void copy_to(Storage& dest) const override
			{
				new (&dest.bytes) callable_impl(functor);
			}

			void move_to(Storage& dest) noexcept override
			{
				new (&dest.bytes) callable_impl(std::move(*this));
			}
		};

		constexpr callable_base* ptr() const { return (callable_base*)&buffer.bytes; }

		Storage buffer;

	public:
		unsafe_function() = default;

		// Create from lambda
		template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, unsafe_function> && sizeof(callable_impl<std::decay_t<F>>) <= BufferSize>>
		unsafe_function(F&& f)
		{
			using FD = std::decay_t<F>;
			static_assert(sizeof(callable_impl<FD>) <= BufferSize, "Callable too large for buffer");
			new (buffer.bytes) callable_impl<FD>(std::forward<F>(f));
		}

		// Copy
		unsafe_function(const unsafe_function& other)
		{
			other.ptr()->copy_to(buffer);
		}

		unsafe_function& operator=(const unsafe_function& other)
		{
			other.ptr()->copy_to(buffer);
			return *this;
		}

		// Move
		unsafe_function(unsafe_function&& other) noexcept
		{
			other.ptr()->move_to(buffer);
		}

		unsafe_function& operator=(unsafe_function&& other) noexcept
		{
			other.ptr()->move_to(buffer);
			return *this;
		}

		// Call
		R operator()(Args... args) const
		{
			return ptr()->call(std::forward<Args>(args)...);
		}
	};

}
