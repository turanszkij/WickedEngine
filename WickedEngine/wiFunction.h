#pragma once
// A minimal replacement for std::function that doesn't use allocation, optimized for performance

#include <type_traits>
#include <utility>
#include <cstring>

namespace wi
{
	template <typename Sig, unsigned BufferSize = 32>
	class function;

	template <typename R, typename... Args, unsigned BufferSize>
	class function<R(Args...), BufferSize>
	{
		struct Storage
		{
			unsigned char bytes[BufferSize];
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
			destroy();
			if (other.ptr)
			{
				other.ptr->copy_to(buffer);
				ptr = reinterpret_cast<callable_base*>(&buffer.bytes);
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
			destroy();
			if (other.ptr)
			{
				other.ptr->move_to(buffer);
				ptr = reinterpret_cast<callable_base*>(&buffer.bytes);
				other.ptr = nullptr;
			}
			return *this;
		}

		// Call
		R operator()(Args... args) const
		{
			return ptr->call(std::forward<Args>(args)...);
		}

		constexpr bool operator==(void* a) const { return ptr == a; }
		constexpr bool operator!=(void* a) const { return ptr != a; }
	};

}
