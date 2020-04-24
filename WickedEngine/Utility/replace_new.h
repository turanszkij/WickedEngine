#pragma once
#include <new>
#include <cstdlib>
#include <atomic>

std::atomic<uint32_t> number_of_allocs{ 0 };

void* operator new(std::size_t size) {
	number_of_allocs.fetch_add(1);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size) {
	number_of_allocs.fetch_add(1);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size, const std::nothrow_t&) throw() {
	number_of_allocs.fetch_add(1);
	return malloc(size);
}
void* operator new(std::size_t size, const std::nothrow_t&) throw() {
	number_of_allocs.fetch_add(1);
	return malloc(size);
}
void operator delete(void* ptr) throw() { free(ptr); }
void operator delete (void* ptr, const std::nothrow_t&) throw() { free(ptr); }
void operator delete[](void* ptr) throw() { free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) throw() { free(ptr); }
