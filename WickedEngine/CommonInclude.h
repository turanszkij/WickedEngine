#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

// NOTE:
// Do not include engine features in this file!

#include <SDKDDKVer.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <stack>
#include <deque>
#include <queue>
#include <list>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <wincodec.h>
#include <iomanip>
#include <float.h>
#include <functional>

#ifdef WINSTORE_SUPPORT
#include <Windows.UI.Core.h>
#endif

using namespace DirectX;
using namespace std;

#define ALIGN_16 void* operator new(size_t i){return _mm_malloc(i, 16);} void operator delete(void* p){_mm_free(p);}
#define SAFE_INIT(a) a = nullptr;
#define SAFE_RELEASE(a) if(a!=nullptr){a->Release();a=nullptr;}
#define SAFE_DELETE(a) if(a!=nullptr){delete a;a=nullptr;}
#define SAFE_DELETE_ARRAY(a) if(a!=nullptr){delete[]a;a=nullptr;}
#define GFX_STRUCT struct alignas(16)
#define GFX_CLASS class alignas(16)


#endif //WICKEDENGINE_COMMONINCLUDE_H
