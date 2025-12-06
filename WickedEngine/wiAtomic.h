#pragma once

#include <atomic>

// MSVC does not have constexpr atomic load(), so for now
// we just test for that, in C++20 we could also use
// type traits to check
#if defined(_MSC_VER) && !defined(__clang__)
    #define WI_ATOMIC_CONSTEXPR inline
#else
    #define WI_ATOMIC_CONSTEXPR constexpr
#endif

namespace wi {


    // subclass of std::atomic that allows copying
    // note that copying *is not* atomic; this is a helper class that allows us
    // to copy classes/structs that contain atomic members where we can accept
    // that copying is not atomic
	template<typename T>
	struct copyable_atomic : public std::atomic<T> {
		copyable_atomic() noexcept = default;
		constexpr copyable_atomic(T v) noexcept : std::atomic<T>(v) {};
		copyable_atomic(const copyable_atomic& other) {
			std::atomic<T>::store(other.load());
		}
		copyable_atomic& operator=(const copyable_atomic& other) {
			std::atomic<T>::store(other.load());
			return *this;
		}
	};

	// simple wrapper that always defaults to memory_order_relaxed
	template<typename T>
	struct relaxed_atomic {
		std::atomic<T> val;

		relaxed_atomic() noexcept = default;
		constexpr relaxed_atomic(T v) noexcept : val(v) {};
		relaxed_atomic(const relaxed_atomic& other) : val(other.load(std::memory_order_relaxed)) {};
		relaxed_atomic& operator=(const relaxed_atomic& other) {
			val.store(other.val.load(std::memory_order_relaxed), std::memory_order_relaxed);
			return *this;
		}
		void store(T desired, std::memory_order order = std::memory_order_relaxed) {
			return val.store(desired, order);
		}
		T load(std::memory_order order = std::memory_order_relaxed) const {
			return val.load(order);
		}

		T fetch_xor(T arg, std::memory_order order = std::memory_order_relaxed) {
			return val.fetch_xor(arg, order);
		}
		T fetch_and(T arg, std::memory_order order = std::memory_order_relaxed) {
			return val.fetch_add(arg, order);
		}
		T fetch_or(T arg, std::memory_order order = std::memory_order_relaxed) {
			return val.fetch_or(arg, order);
		}

		operator T() const noexcept { return load(); }
		T operator=(T desired) noexcept { store(desired); return desired;}
		T operator&=(T arg) { return fetch_and(arg); }
		T operator|=(T arg) { return fetch_or(arg); }
		T operator^=(T arg) { return fetch_xor(arg); }
	};


}
