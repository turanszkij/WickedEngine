#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

// This is a helper include file pasted into all engine headers try to keep it minimal!
// Do not include engine features in this file!

#include <cstdint>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
using namespace DirectX;
using namespace DirectX::PackedVector;

#define arraysize(a) (sizeof(a) / sizeof(a[0]))
#define ALIGN_16 void* operator new(size_t i){return _mm_malloc(i, 16);} void operator delete(void* p){_mm_free(p);}
#define SAFE_RELEASE(a) if((a)!=nullptr){(a)->Release();(a)=nullptr;}
#define SAFE_DELETE(a) {delete (a);(a)=nullptr;}
#define SAFE_DELETE_ARRAY(a) {delete[](a);(a)=nullptr;}
#define GFX_STRUCT struct alignas(16)

static const XMFLOAT4X4 IDENTITYMATRIX = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

typedef uint64_t wiCPUHandle;
static const wiCPUHandle WI_NULL_HANDLE = 0;

#endif //WICKEDENGINE_COMMONINCLUDE_H
