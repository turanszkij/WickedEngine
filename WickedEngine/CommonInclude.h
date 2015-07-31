#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

#include <SDKDDKVer.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DXGI1_2.h>
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
#define SAFE_RELEASE(a) if(a!=nullptr){a->Release();a=nullptr;}
#define SAFE_DELETE(a) if(a!=nullptr){delete a;a=nullptr;}
#define SAFE_DELETE_ARRAY(a) if(a!=nullptr){delete[]a;a=nullptr;}


#endif //WICKEDENGINE_COMMONINCLUDE_H
