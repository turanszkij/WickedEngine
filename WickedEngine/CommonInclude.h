#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

// NOTE:
// Do not include engine features in this file!


// Platform specific:
#include <SDKDDKVer.h>
#include <windows.h>
#ifdef WINSTORE_SUPPORT
#include <Windows.UI.Core.h>
#endif


// Platform agnostic:
#include <DirectXMath.h>
#include <DirectXCollision.h>
using namespace DirectX;

#define ALIGN_16 void* operator new(size_t i){return _mm_malloc(i, 16);} void operator delete(void* p){_mm_free(p);}
#define SAFE_INIT(a) (a) = nullptr;
#define SAFE_RELEASE(a) if((a)!=nullptr){(a)->Release();(a)=nullptr;}
#define SAFE_DELETE(a) if((a)!=nullptr){delete (a);(a)=nullptr;}
#define SAFE_DELETE_ARRAY(a) if((a)!=nullptr){delete[](a);(a)=nullptr;}
#define GFX_STRUCT struct alignas(16)
#define GFX_CLASS class alignas(16)

template <typename T>
inline void SwapPtr(T*& a, T*& b)
{
	T* swap = a;
	a = b;
	b = swap;
}


#endif //WICKEDENGINE_COMMONINCLUDE_H
