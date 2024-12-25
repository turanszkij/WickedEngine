#pragma once

namespace wi::wayland {

template<typename T>
void safe_delete(T*& ptr, void(&deleter)(T*))
{
	if (ptr != nullptr)
	{
		deleter(ptr);
		ptr = nullptr;
	}
}

}
