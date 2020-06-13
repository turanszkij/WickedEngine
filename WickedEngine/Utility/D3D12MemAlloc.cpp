//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifdef _WIN32

#include "D3D12MemAlloc.h"

#include <mutex>
#include <atomic>
#include <algorithm>
#include <cstdlib>
#include <malloc.h> // for _aligned_malloc, _aligned_free

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Configuration Begin
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef D3D12MA_ASSERT
    #include <cassert>
    #define D3D12MA_ASSERT(cond) assert(cond)
#endif

// Assert that will be called very often, like inside data structures e.g. operator[].
// Making it non-empty can make program slow.
#ifndef D3D12MA_HEAVY_ASSERT
    #ifdef _DEBUG
        #define D3D12MA_HEAVY_ASSERT(expr)   //D3D12MA_ASSERT(expr)
    #else
        #define D3D12MA_HEAVY_ASSERT(expr)
    #endif
#endif

#ifndef D3D12MA_DEBUG_ALIGNMENT
    /*
    Minimum alignment of all allocations, in bytes.
    Set to more than 1 for debugging purposes only. Must be power of two.
    */
    #define D3D12MA_DEBUG_ALIGNMENT (1)
#endif

#ifndef D3D12MA_DEBUG_MARGIN
    // Minimum margin before and after every allocation, in bytes.
    // Set nonzero for debugging purposes only.
    #define D3D12MA_DEBUG_MARGIN (0)
#endif

#ifndef D3D12MA_DEBUG_GLOBAL_MUTEX
    /*
    Set this to 1 for debugging purposes only, to enable single mutex protecting all
    entry calls to the library. Can be useful for debugging multithreading issues.
    */
    #define D3D12MA_DEBUG_GLOBAL_MUTEX (0)
#endif

#ifndef D3D12MA_DEFAULT_BLOCK_SIZE
   /// Default size of a block allocated as single ID3D12Heap.
   #define D3D12MA_DEFAULT_BLOCK_SIZE (256ull * 1024 * 1024)
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Configuration End
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


namespace D3D12MA
{

////////////////////////////////////////////////////////////////////////////////
// Private globals - CPU memory allocation

static void* DefaultAllocate(size_t Size, size_t Alignment, void* /*pUserData*/)
{
    return _aligned_malloc(Size, Alignment);
}
static void DefaultFree(void* pMemory, void* /*pUserData*/)
{
    return _aligned_free(pMemory);
}

static void* Malloc(const ALLOCATION_CALLBACKS& allocs, size_t size, size_t alignment)
{
    return (*allocs.pAllocate)(size, alignment, allocs.pUserData);
}
static void Free(const ALLOCATION_CALLBACKS& allocs, void* memory)
{
    (*allocs.pFree)(memory, allocs.pUserData);
}

template<typename T>
static T* Allocate(const ALLOCATION_CALLBACKS& allocs)
{
    return (T*)Malloc(allocs, sizeof(T), __alignof(T));
}
template<typename T>
static T* AllocateArray(const ALLOCATION_CALLBACKS& allocs, size_t count)
{
    return (T*)Malloc(allocs, sizeof(T) * count, __alignof(T));
}

#define D3D12MA_NEW(allocs, type) new(D3D12MA::Allocate<type>(allocs))(type)
#define D3D12MA_NEW_ARRAY(allocs, type, count) new(D3D12MA::AllocateArray<type>((allocs), (count)))(type)

template<typename T>
static void D3D12MA_DELETE(const ALLOCATION_CALLBACKS& allocs, T* memory)
{
    if(memory)
    {
        memory->~T();
        Free(allocs, memory);
    }
}
template<typename T>
static void D3D12MA_DELETE_ARRAY(const ALLOCATION_CALLBACKS& allocs, T* memory, size_t count)
{
    if(memory)
    {
        for(size_t i = count; i--; )
        {
            memory[i].~T();
        }
        Free(allocs, memory);
    }
}

static void SetupAllocationCallbacks(ALLOCATION_CALLBACKS& outAllocs, const ALLOCATOR_DESC& allocatorDesc)
{
    if(allocatorDesc.pAllocationCallbacks)
    {
        outAllocs = *allocatorDesc.pAllocationCallbacks;
        D3D12MA_ASSERT(outAllocs.pAllocate != NULL && outAllocs.pFree != NULL);
    }
    else
    {
        outAllocs.pAllocate = &DefaultAllocate;
        outAllocs.pFree = &DefaultFree;
        outAllocs.pUserData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Private globals - basic facilities

#define D3D12MA_VALIDATE(cond) do { if(!(cond)) { \
        D3D12MA_ASSERT(0 && "Validation failed: " #cond); \
        return false; \
    } } while(false)

template<typename T>
static inline T D3D12MA_MIN(const T& a, const T& b)
{
    return a <= b ? a : b;
}
template<typename T>
static inline T D3D12MA_MAX(const T& a, const T& b)
{
    return a <= b ? b : a;
}

template<typename T>
static inline void D3D12MA_SWAP(T& a, T& b)
{
    T tmp = a; a = b; b = tmp;
}

#ifndef D3D12MA_MUTEX
    class Mutex
    {
    public:
        void Lock() { m_Mutex.lock(); }
        void Unlock() { m_Mutex.unlock(); }
    private:
        std::mutex m_Mutex;
    };
    #define D3D12MA_MUTEX Mutex
#endif

#if !defined(_WIN32) || !defined(WINVER) || WINVER < 0x0600
    #error Required at least WinAPI version supporting: client = Windows Vista, server = Windows Server 2008.
#endif

#ifndef D3D12MA_RW_MUTEX
    class RWMutex
    {
    public:
        RWMutex() { InitializeSRWLock(&m_Lock); }
        void LockRead() { AcquireSRWLockShared(&m_Lock); }
        void UnlockRead() { ReleaseSRWLockShared(&m_Lock); }
        void LockWrite() { AcquireSRWLockExclusive(&m_Lock); }
        void UnlockWrite() { ReleaseSRWLockExclusive(&m_Lock); }
    private:
        SRWLOCK m_Lock;
    };
    #define D3D12MA_RW_MUTEX RWMutex
#endif

/*
If providing your own implementation, you need to implement a subset of std::atomic.
*/
#ifndef D3D12MA_ATOMIC_UINT32
    #define D3D12MA_ATOMIC_UINT32 std::atomic<UINT>
#endif

// Aligns given value up to nearest multiply of align value. For example: AlignUp(11, 8) = 16.
// Use types like UINT, uint64_t as T.
template <typename T>
static inline T AlignUp(T val, T align)
{
	return (val + align - 1) / align * align;
}
// Aligns given value down to nearest multiply of align value. For example: AlignUp(11, 8) = 8.
// Use types like UINT, uint64_t as T.
template <typename T>
static inline T AlignDown(T val, T align)
{
    return val / align * align;
}

// Division with mathematical rounding to nearest number.
template <typename T>
static inline T RoundDiv(T x, T y)
{
	return (x + (y / (T)2)) / y;
}

/*
Returns true if given number is a power of two.
T must be unsigned integer number or signed integer but always nonnegative.
For 0 returns true.
*/
template <typename T>
inline bool IsPow2(T x)
{
    return (x & (x-1)) == 0;
}

// Returns smallest power of 2 greater or equal to v.
static inline UINT NextPow2(UINT v)
{
	v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
static inline uint64_t NextPow2(uint64_t v)
{
	v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

// Returns largest power of 2 less or equal to v.
static inline UINT PrevPow2(UINT v)
{
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v = v ^ (v >> 1);
    return v;
}
static inline uint64_t PrevPow2(uint64_t v)
{
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v = v ^ (v >> 1);
    return v;
}

static inline bool StrIsEmpty(const char* pStr)
{
    return pStr == NULL || *pStr == '\0';
}

// Helper RAII class to lock a mutex in constructor and unlock it in destructor (at the end of scope).
struct MutexLock
{
public:
    MutexLock(D3D12MA_MUTEX& mutex, bool useMutex = true) :
        m_pMutex(useMutex ? &mutex : NULL)
    {
        if(m_pMutex)
        {
            m_pMutex->Lock();
        }
    }
    ~MutexLock()
    {
        if(m_pMutex)
        {
            m_pMutex->Unlock();
        }
    }
private:
    D3D12MA_MUTEX* m_pMutex;

    D3D12MA_CLASS_NO_COPY(MutexLock)
};

// Helper RAII class to lock a RW mutex in constructor and unlock it in destructor (at the end of scope), for reading.
struct MutexLockRead
{
public:
    MutexLockRead(D3D12MA_RW_MUTEX& mutex, bool useMutex) :
        m_pMutex(useMutex ? &mutex : NULL)
    {
        if(m_pMutex)
        {
            m_pMutex->LockRead();
        }
    }
    ~MutexLockRead()
    {
        if(m_pMutex)
        {
            m_pMutex->UnlockRead();
        }
    }
private:
    D3D12MA_RW_MUTEX* m_pMutex;

    D3D12MA_CLASS_NO_COPY(MutexLockRead)
};

// Helper RAII class to lock a RW mutex in constructor and unlock it in destructor (at the end of scope), for writing.
struct MutexLockWrite
{
public:
    MutexLockWrite(D3D12MA_RW_MUTEX& mutex, bool useMutex) :
        m_pMutex(useMutex ? &mutex : NULL)
    {
        if(m_pMutex)
        {
            m_pMutex->LockWrite();
        }
    }
    ~MutexLockWrite()
    {
        if(m_pMutex)
        {
            m_pMutex->UnlockWrite();
        }
    }
private:
    D3D12MA_RW_MUTEX* m_pMutex;

    D3D12MA_CLASS_NO_COPY(MutexLockWrite)
};

#if D3D12MA_DEBUG_GLOBAL_MUTEX
    static D3D12MA_MUTEX g_DebugGlobalMutex;
    #define D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK MutexLock debugGlobalMutexLock(g_DebugGlobalMutex, true);
#else
    #define D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
#endif

// Minimum size of a free suballocation to register it in the free suballocation collection.
static const UINT64 MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER = 16;

/*
Performs binary search and returns iterator to first element that is greater or
equal to `key`, according to comparison `cmp`.

Cmp should return true if first argument is less than second argument.

Returned value is the found element, if present in the collection or place where
new element with value (key) should be inserted.
*/
template <typename CmpLess, typename IterT, typename KeyT>
static IterT BinaryFindFirstNotLess(IterT beg, IterT end, const KeyT &key, const CmpLess& cmp)
{
    size_t down = 0, up = (end - beg);
    while(down < up)
    {
        const size_t mid = (down + up) / 2;
        if(cmp(*(beg+mid), key))
        {
            down = mid + 1;
        }
        else
        {
            up = mid;
        }
    }
    return beg + down;
}

/*
Performs binary search and returns iterator to an element that is equal to `key`,
according to comparison `cmp`.

Cmp should return true if first argument is less than second argument.

Returned value is the found element, if present in the collection or end if not
found.
*/
template<typename CmpLess, typename IterT, typename KeyT>
IterT BinaryFindSorted(const IterT& beg, const IterT& end, const KeyT& value, const CmpLess& cmp)
{
    IterT it = BinaryFindFirstNotLess<CmpLess, IterT, KeyT>(beg, end, value, cmp);
    if(it == end ||
        (!cmp(*it, value) && !cmp(value, *it)))
    {
        return it;
    }
    return end;
}

struct PointerLess
{
    bool operator()(const void* lhs, const void* rhs) const
    {
        return lhs < rhs;
    }
};

static UINT HeapTypeToIndex(D3D12_HEAP_TYPE type)
{
    switch(type)
    {
    case D3D12_HEAP_TYPE_DEFAULT:  return 0;
    case D3D12_HEAP_TYPE_UPLOAD:   return 1;
    case D3D12_HEAP_TYPE_READBACK: return 2;
    default: D3D12MA_ASSERT(0); return UINT_MAX;
    }
}

static const WCHAR* const HeapTypeNames[] = {
    L"DEFAULT",
    L"UPLOAD",
    L"READBACK",
};

// Stat helper functions

static void AddStatInfo(StatInfo& dst, const StatInfo& src)
{
    dst.BlockCount += src.BlockCount;
    dst.AllocationCount += src.AllocationCount;
    dst.UnusedRangeCount += src.UnusedRangeCount;
    dst.UsedBytes += src.UsedBytes;
    dst.UnusedBytes += src.UnusedBytes;
    dst.AllocationSizeMin = D3D12MA_MIN(dst.AllocationSizeMin, src.AllocationSizeMin);
    dst.AllocationSizeMax = D3D12MA_MAX(dst.AllocationSizeMax, src.AllocationSizeMax);
    dst.UnusedRangeSizeMin = D3D12MA_MIN(dst.UnusedRangeSizeMin, src.UnusedRangeSizeMin);
    dst.UnusedRangeSizeMax = D3D12MA_MAX(dst.UnusedRangeSizeMax, src.UnusedRangeSizeMax);
}

static void PostProcessStatInfo(StatInfo& statInfo)
{
    statInfo.AllocationSizeAvg = statInfo.AllocationCount ?
        statInfo.UsedBytes / statInfo.AllocationCount : 0;
    statInfo.UnusedRangeSizeAvg = statInfo.UnusedRangeCount ?
        statInfo.UnusedBytes / statInfo.UnusedRangeCount : 0;
}

static UINT64 HeapFlagsToAlignment(D3D12_HEAP_FLAGS flags)
{
    /*
    Documentation of D3D12_HEAP_DESC structure says:

    - D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT   defined as 64KB.
    - D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT   defined as 4MB. An
    application must decide whether the heap will contain multi-sample
    anti-aliasing (MSAA), in which case, the application must choose [this flag].

    https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_heap_desc
    */

    const D3D12_HEAP_FLAGS denyAllTexturesFlags =
        D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
    const bool canContainAnyTextures =
        (flags & denyAllTexturesFlags) != denyAllTexturesFlags;
    return canContainAnyTextures ?
        D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
}

////////////////////////////////////////////////////////////////////////////////
// Private class Vector

/*
Dynamically resizing continuous array. Class with interface similar to std::vector.
T must be POD because constructors and destructors are not called and memcpy is
used for these objects.
*/
template<typename T>
class Vector
{
public:
    typedef T value_type;

    // allocationCallbacks externally owned, must outlive this object.
    Vector(const ALLOCATION_CALLBACKS& allocationCallbacks) :
        m_AllocationCallbacks(allocationCallbacks),
        m_pArray(NULL),
        m_Count(0),
        m_Capacity(0)
    {
    }

    Vector(size_t count, const ALLOCATION_CALLBACKS& allocationCallbacks) :
        m_AllocationCallbacks(allocationCallbacks),
        m_pArray(count ? AllocateArray<T>(allocationCallbacks, count) : NULL),
        m_Count(count),
        m_Capacity(count)
    {
    }

    Vector(const Vector<T>& src) :
        m_AllocationCallbacks(src.m_AllocationCallbacks),
        m_pArray(src.m_Count ? AllocateArray<T>(src.m_AllocationCallbacks, src.m_Count) : NULL),
        m_Count(src.m_Count),
        m_Capacity(src.m_Count)
    {
        if(m_Count > 0)
        {
            memcpy(m_pArray, src.m_pArray, m_Count * sizeof(T));
        }
    }

    ~Vector()
    {
        Free(m_AllocationCallbacks, m_pArray);
    }

    Vector& operator=(const Vector<T>& rhs)
    {
        if(&rhs != this)
        {
            resize(rhs.m_Count);
            if(m_Count != 0)
            {
                memcpy(m_pArray, rhs.m_pArray, m_Count * sizeof(T));
            }
        }
        return *this;
    }

    bool empty() const { return m_Count == 0; }
    size_t size() const { return m_Count; }
    T* data() { return m_pArray; }
    const T* data() const { return m_pArray; }

    T& operator[](size_t index)
    {
        D3D12MA_HEAVY_ASSERT(index < m_Count);
        return m_pArray[index];
    }
    const T& operator[](size_t index) const
    {
        D3D12MA_HEAVY_ASSERT(index < m_Count);
        return m_pArray[index];
    }

    T& front()
    {
        D3D12MA_HEAVY_ASSERT(m_Count > 0);
        return m_pArray[0];
    }
    const T& front() const
    {
        D3D12MA_HEAVY_ASSERT(m_Count > 0);
        return m_pArray[0];
    }
    T& back()
    {
        D3D12MA_HEAVY_ASSERT(m_Count > 0);
        return m_pArray[m_Count - 1];
    }
    const T& back() const
    {
        D3D12MA_HEAVY_ASSERT(m_Count > 0);
        return m_pArray[m_Count - 1];
    }

    void reserve(size_t newCapacity, bool freeMemory = false)
    {
        newCapacity = D3D12MA_MAX(newCapacity, m_Count);

        if((newCapacity < m_Capacity) && !freeMemory)
        {
            newCapacity = m_Capacity;
        }

        if(newCapacity != m_Capacity)
        {
            T* const newArray = newCapacity ? AllocateArray<T>(m_AllocationCallbacks, newCapacity) : NULL;
            if(m_Count != 0)
            {
                memcpy(newArray, m_pArray, m_Count * sizeof(T));
            }
            Free(m_AllocationCallbacks, m_pArray);
            m_Capacity = newCapacity;
            m_pArray = newArray;
        }
    }

    void resize(size_t newCount, bool freeMemory = false)
    {
        size_t newCapacity = m_Capacity;
        if(newCount > m_Capacity)
        {
            newCapacity = D3D12MA_MAX(newCount, D3D12MA_MAX(m_Capacity * 3 / 2, (size_t)8));
        }
        else if(freeMemory)
        {
            newCapacity = newCount;
        }

        if(newCapacity != m_Capacity)
        {
            T* const newArray = newCapacity ? AllocateArray<T>(m_AllocationCallbacks, newCapacity) : NULL;
            const size_t elementsToCopy = D3D12MA_MIN(m_Count, newCount);
            if(elementsToCopy != 0)
            {
                memcpy(newArray, m_pArray, elementsToCopy * sizeof(T));
            }
            Free(m_AllocationCallbacks, m_pArray);
            m_Capacity = newCapacity;
            m_pArray = newArray;
        }

        m_Count = newCount;
    }

    void clear(bool freeMemory = false)
    {
        resize(0, freeMemory);
    }

    void insert(size_t index, const T& src)
    {
        D3D12MA_HEAVY_ASSERT(index <= m_Count);
        const size_t oldCount = size();
        resize(oldCount + 1);
        if(index < oldCount)
        {
            memmove(m_pArray + (index + 1), m_pArray + index, (oldCount - index) * sizeof(T));
        }
        m_pArray[index] = src;
    }

    void remove(size_t index)
    {
        D3D12MA_HEAVY_ASSERT(index < m_Count);
        const size_t oldCount = size();
        if(index < oldCount - 1)
        {
            memmove(m_pArray + index, m_pArray + (index + 1), (oldCount - index - 1) * sizeof(T));
        }
        resize(oldCount - 1);
    }

    void push_back(const T& src)
    {
        const size_t newIndex = size();
        resize(newIndex + 1);
        m_pArray[newIndex] = src;
    }

    void pop_back()
    {
        D3D12MA_HEAVY_ASSERT(m_Count > 0);
        resize(size() - 1);
    }

    void push_front(const T& src)
    {
        insert(0, src);
    }

    void pop_front()
    {
        D3D12MA_HEAVY_ASSERT(m_Count > 0);
        remove(0);
    }

    typedef T* iterator;

    iterator begin() { return m_pArray; }
    iterator end() { return m_pArray + m_Count; }

    template<typename CmpLess>
    size_t InsertSorted(const T& value, const CmpLess& cmp)
    {
        const size_t indexToInsert = BinaryFindFirstNotLess<CmpLess, iterator, T>(
            m_pArray,
            m_pArray + m_Count,
            value,
            cmp) - m_pArray;
        insert(indexToInsert, value);
        return indexToInsert;
    }

    template<typename CmpLess>
    bool RemoveSorted(const T& value, const CmpLess& cmp)
    {
        const iterator it = BinaryFindFirstNotLess(
            m_pArray,
            m_pArray + m_Count,
            value,
            cmp);
        if((it != end()) && !cmp(*it, value) && !cmp(value, *it))
        {
            size_t indexToRemove = it - begin();
            remove(indexToRemove);
            return true;
        }
        return false;
    }

private:
    const ALLOCATION_CALLBACKS& m_AllocationCallbacks;
    T* m_pArray;
    size_t m_Count;
    size_t m_Capacity;
};

////////////////////////////////////////////////////////////////////////////////
// Private class StringBuilder

class StringBuilder
{
public:
    StringBuilder(const ALLOCATION_CALLBACKS& allocationCallbacks) : m_Data(allocationCallbacks) { }

    size_t GetLength() const { return m_Data.size(); }
    LPCWSTR GetData() const { return m_Data.data(); }

    void Add(WCHAR ch) { m_Data.push_back(ch); }
    void Add(LPCWSTR str);
    void AddNewLine() { Add(L'\n'); }
    void AddNumber(UINT num);
    void AddNumber(UINT64 num);

private:
    Vector<WCHAR> m_Data;
};

void StringBuilder::Add(LPCWSTR str)
{
    const size_t len = wcslen(str);
    if (len > 0)
    {
        const size_t oldCount = m_Data.size();
        m_Data.resize(oldCount + len);
        memcpy(m_Data.data() + oldCount, str, len * sizeof(WCHAR));
    }
}

void StringBuilder::AddNumber(UINT num)
{
    WCHAR buf[11];
    buf[10] = L'\0';
    WCHAR *p = &buf[10];
    do
    {
        *--p = L'0' + (num % 10);
        num /= 10;
    }
    while (num);
    Add(p);
}

void StringBuilder::AddNumber(UINT64 num)
{
    WCHAR buf[21];
    buf[20] = L'\0';
    WCHAR *p = &buf[20];
    do
    {
        *--p = L'0' + (num % 10);
        num /= 10;
    }
    while (num);
    Add(p);
}

////////////////////////////////////////////////////////////////////////////////
// Private class JsonWriter
class JsonWriter
{
public:
    JsonWriter(const ALLOCATION_CALLBACKS& allocationCallbacks, StringBuilder& stringBuilder);
    ~JsonWriter();

    void BeginObject(bool singleLine = false);
    void EndObject();

    void BeginArray(bool singleLine = false);
    void EndArray();

    void WriteString(LPCWSTR pStr);
    void BeginString(LPCWSTR pStr = NULL);
    void ContinueString(LPCWSTR pStr);
    void ContinueString(UINT num);
    void ContinueString(UINT64 num);
    void AddAllocationToObject(const Allocation& alloc);
    // void ContinueString_Pointer(const void* ptr);
    void EndString(LPCWSTR pStr = NULL);

    void WriteNumber(UINT num);
    void WriteNumber(UINT64 num);
    void WriteBool(bool b);
    void WriteNull();

private:
    static const WCHAR* const INDENT;

    enum CollectionType
    {
        COLLECTION_TYPE_OBJECT,
        COLLECTION_TYPE_ARRAY,
    };
    struct StackItem
    {
        CollectionType type;
        UINT valueCount;
        bool singleLineMode;
    };

    StringBuilder& m_SB;
    Vector<StackItem> m_Stack;
    bool m_InsideString;

    void BeginValue(bool isString);
    void WriteIndent(bool oneLess = false);
};

const WCHAR* const JsonWriter::INDENT = L"  ";

JsonWriter::JsonWriter(const ALLOCATION_CALLBACKS& allocationCallbacks, StringBuilder& stringBuilder) :
    m_SB(stringBuilder),
    m_Stack(allocationCallbacks),
    m_InsideString(false)
{
}

JsonWriter::~JsonWriter()
{
    D3D12MA_ASSERT(!m_InsideString);
    D3D12MA_ASSERT(m_Stack.empty());
}

void JsonWriter::BeginObject(bool singleLine)
{
    D3D12MA_ASSERT(!m_InsideString);

    BeginValue(false);
    m_SB.Add(L'{');

    StackItem stackItem;
    stackItem.type = COLLECTION_TYPE_OBJECT;
    stackItem.valueCount = 0;
    stackItem.singleLineMode = singleLine;
    m_Stack.push_back(stackItem);
}

void JsonWriter::EndObject()
{
    D3D12MA_ASSERT(!m_InsideString);
    D3D12MA_ASSERT(!m_Stack.empty() && m_Stack.back().type == COLLECTION_TYPE_OBJECT);
    D3D12MA_ASSERT(m_Stack.back().valueCount % 2 == 0);

    WriteIndent(true);
    m_SB.Add(L'}');

    m_Stack.pop_back();
}

void JsonWriter::BeginArray(bool singleLine)
{
    D3D12MA_ASSERT(!m_InsideString);

    BeginValue(false);
    m_SB.Add(L'[');

    StackItem stackItem;
    stackItem.type = COLLECTION_TYPE_ARRAY;
    stackItem.valueCount = 0;
    stackItem.singleLineMode = singleLine;
    m_Stack.push_back(stackItem);
}

void JsonWriter::EndArray()
{
    D3D12MA_ASSERT(!m_InsideString);
    D3D12MA_ASSERT(!m_Stack.empty() && m_Stack.back().type == COLLECTION_TYPE_ARRAY);

    WriteIndent(true);
    m_SB.Add(L']');

    m_Stack.pop_back();
}

void JsonWriter::WriteString(LPCWSTR pStr)
{
    BeginString(pStr);
    EndString();
}

void JsonWriter::BeginString(LPCWSTR pStr)
{
    D3D12MA_ASSERT(!m_InsideString);

    BeginValue(true);
    m_InsideString = true;
    m_SB.Add(L'"');
    if (pStr != NULL)
    {
        ContinueString(pStr);
    }
}

void JsonWriter::ContinueString(LPCWSTR pStr)
{
    D3D12MA_ASSERT(m_InsideString);
    D3D12MA_ASSERT(pStr);

    for (const WCHAR *p = pStr; *p; ++p)
    {
        // the strings we encode are assumed to be in UTF-16LE format, the native
        // windows wide character unicode format. In this encoding unicode code
        // points U+0000 to U+D7FF and U+E000 to U+FFFF are encoded in two bytes,
        // and everything else takes more than two bytes. We will reject any
        // multi wchar character encodings for simplicity.
        UINT val = (UINT)*p;
        D3D12MA_ASSERT(((val <= 0xD7FF) || (0xE000 <= val && val <= 0xFFFF)) &&
            "Character not currently supported.");
        switch (*p)
        {
        case L'"':  m_SB.Add(L'\\'); m_SB.Add(L'"');  break;
        case L'\\': m_SB.Add(L'\\'); m_SB.Add(L'\\'); break;
        case L'/':  m_SB.Add(L'\\'); m_SB.Add(L'/');  break;
        case L'\b': m_SB.Add(L'\\'); m_SB.Add(L'b');  break;
        case L'\f': m_SB.Add(L'\\'); m_SB.Add(L'f');  break;
        case L'\n': m_SB.Add(L'\\'); m_SB.Add(L'n');  break;
        case L'\r': m_SB.Add(L'\\'); m_SB.Add(L'r');  break;
        case L'\t': m_SB.Add(L'\\'); m_SB.Add(L't');  break;
        default:
            // conservatively use encoding \uXXXX for any unicode character
            // requiring more than one byte.
            if (32 <= val && val < 256)
                m_SB.Add(*p);
            else
            {
                m_SB.Add(L'\\');
                m_SB.Add(L'u');
                for (UINT i = 0; i < 4; ++i)
                {
                    UINT hexDigit = (val & 0xF000) >> 12;
                    val <<= 4;
                    if (hexDigit < 10)
                        m_SB.Add(L'0' + hexDigit);
                    else
                        m_SB.Add(L'A' + hexDigit);
                }
            }
            break;
        }
    }
}

void JsonWriter::ContinueString(UINT num)
{
    D3D12MA_ASSERT(m_InsideString);
    m_SB.AddNumber(num);
}

void JsonWriter::ContinueString(UINT64 num)
{
    D3D12MA_ASSERT(m_InsideString);
    m_SB.AddNumber(num);
}

void JsonWriter::EndString(LPCWSTR pStr)
{
    D3D12MA_ASSERT(m_InsideString);

    if (pStr)
        ContinueString(pStr);
    m_SB.Add(L'"');
    m_InsideString = false;
}

void JsonWriter::WriteNumber(UINT num)
{
    D3D12MA_ASSERT(!m_InsideString);
    BeginValue(false);
    m_SB.AddNumber(num);
}

void JsonWriter::WriteNumber(UINT64 num)
{
    D3D12MA_ASSERT(!m_InsideString);
    BeginValue(false);
    m_SB.AddNumber(num);
}

void JsonWriter::WriteBool(bool b)
{
    D3D12MA_ASSERT(!m_InsideString);
    BeginValue(false);
    if (b)
        m_SB.Add(L"true");
    else
        m_SB.Add(L"false");
}

void JsonWriter::WriteNull()
{
    D3D12MA_ASSERT(!m_InsideString);
    BeginValue(false);
    m_SB.Add(L"null");
}

void JsonWriter::BeginValue(bool isString)
{
    if (!m_Stack.empty())
    {
        StackItem& currItem = m_Stack.back();
        if (currItem.type == COLLECTION_TYPE_OBJECT && currItem.valueCount % 2 == 0)
        {
            D3D12MA_ASSERT(isString);
        }

        if (currItem.type == COLLECTION_TYPE_OBJECT && currItem.valueCount % 2 == 1)
        {
            m_SB.Add(L':'); m_SB.Add(L' ');
        }
        else if (currItem.valueCount > 0)
        {
            m_SB.Add(L','); m_SB.Add(L' ');
            WriteIndent();
        }
        else
        {
            WriteIndent();
        }
        ++currItem.valueCount;
    }
}

void JsonWriter::WriteIndent(bool oneLess)
{
    if (!m_Stack.empty() && !m_Stack.back().singleLineMode)
    {
        m_SB.AddNewLine();

        size_t count = m_Stack.size();
        if (count > 0 && oneLess)
        {
            --count;
        }
        for (size_t i = 0; i < count; ++i)
        {
            m_SB.Add(INDENT);
        }
    }
}

void JsonWriter::AddAllocationToObject(const Allocation& alloc)
{
    WriteString(L"Type");
    switch (alloc.m_ResourceDimension) {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        WriteString(L"UNKNOWN");
        break;
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        WriteString(L"BUFFER");
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        WriteString(L"TEXTURE1D");
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        WriteString(L"TEXTURE2D");
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        WriteString(L"TEXTURE3D");
        break;
    default: D3D12MA_ASSERT(0); break;
    }
    WriteString(L"Size");
    WriteNumber(alloc.GetSize());
    LPCWSTR name = alloc.GetName();
    if(name != NULL)
    {
        WriteString(L"Name");
        WriteString(name);
    }
    if(alloc.m_ResourceFlags)
    {
        WriteString(L"Flags");
        WriteNumber((UINT)alloc.m_ResourceFlags);
    }
    if(alloc.m_TextureLayout)
    {
        WriteString(L"Layout");
        WriteNumber((UINT)alloc.m_TextureLayout);
    }
    if(alloc.m_CreationFrameIndex)
    {
        WriteString(L"CreationFrameIndex");
        WriteNumber(alloc.m_CreationFrameIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Private class PoolAllocator

/*
Allocator for objects of type T using a list of arrays (pools) to speed up
allocation. Number of elements that can be allocated is not bounded because
allocator can create multiple blocks.
T should be POD because constructor and destructor is not called in Alloc or
Free.
*/
template<typename T>
class PoolAllocator
{
    D3D12MA_CLASS_NO_COPY(PoolAllocator)
public:
    // allocationCallbacks externally owned, must outlive this object.
    PoolAllocator(const ALLOCATION_CALLBACKS& allocationCallbacks, UINT firstBlockCapacity);
    ~PoolAllocator() { Clear(); }
    void Clear();
    T* Alloc();
    void Free(T* ptr);

private:
    union Item
    {
        UINT NextFreeIndex; // UINT32_MAX means end of list.
        T Value;
    };

    struct ItemBlock
    {
        Item* pItems;
        UINT Capacity;
        UINT FirstFreeIndex;
    };

    const ALLOCATION_CALLBACKS& m_AllocationCallbacks;
    const UINT m_FirstBlockCapacity;
    Vector<ItemBlock> m_ItemBlocks;

    ItemBlock& CreateNewBlock();
};

template<typename T>
PoolAllocator<T>::PoolAllocator(const ALLOCATION_CALLBACKS& allocationCallbacks, UINT firstBlockCapacity) :
    m_AllocationCallbacks(allocationCallbacks),
    m_FirstBlockCapacity(firstBlockCapacity),
    m_ItemBlocks(allocationCallbacks)
{
    D3D12MA_ASSERT(m_FirstBlockCapacity > 1);
}

template<typename T>
void PoolAllocator<T>::Clear()
{
    for(size_t i = m_ItemBlocks.size(); i--; )
    {
        D3D12MA_DELETE_ARRAY(m_AllocationCallbacks, m_ItemBlocks[i].pItems, m_ItemBlocks[i].Capacity);
    }
    m_ItemBlocks.clear(true);
}

template<typename T>
T* PoolAllocator<T>::Alloc()
{
    for(size_t i = m_ItemBlocks.size(); i--; )
    {
        ItemBlock& block = m_ItemBlocks[i];
        // This block has some free items: Use first one.
        if(block.FirstFreeIndex != UINT32_MAX)
        {
            Item* const pItem = &block.pItems[block.FirstFreeIndex];
            block.FirstFreeIndex = pItem->NextFreeIndex;
            return &pItem->Value;
        }
    }

    // No block has free item: Create new one and use it.
    ItemBlock& newBlock = CreateNewBlock();
    Item* const pItem = &newBlock.pItems[0];
    newBlock.FirstFreeIndex = pItem->NextFreeIndex;
    return &pItem->Value;
}

template<typename T>
void PoolAllocator<T>::Free(T* ptr)
{
    // Search all memory blocks to find ptr.
    for(size_t i = m_ItemBlocks.size(); i--; )
    {
        ItemBlock& block = m_ItemBlocks[i];

        Item* pItemPtr;
        memcpy(&pItemPtr, &ptr, sizeof(pItemPtr));

        // Check if pItemPtr is in address range of this block.
        if((pItemPtr >= block.pItems) && (pItemPtr < block.pItems + block.Capacity))
        {
            const UINT index = static_cast<UINT>(pItemPtr - block.pItems);
            pItemPtr->NextFreeIndex = block.FirstFreeIndex;
            block.FirstFreeIndex = index;
            return;
        }
    }
    D3D12MA_ASSERT(0 && "Pointer doesn't belong to this memory pool.");
}

template<typename T>
typename PoolAllocator<T>::ItemBlock& PoolAllocator<T>::CreateNewBlock()
{
    const UINT newBlockCapacity = m_ItemBlocks.empty() ?
        m_FirstBlockCapacity : m_ItemBlocks.back().Capacity * 3 / 2;

    const ItemBlock newBlock = {
        D3D12MA_NEW_ARRAY(m_AllocationCallbacks, Item, newBlockCapacity),
        newBlockCapacity,
        0 };

    m_ItemBlocks.push_back(newBlock);

    // Setup singly-linked list of all free items in this block.
    for(UINT i = 0; i < newBlockCapacity - 1; ++i)
    {
        newBlock.pItems[i].NextFreeIndex = i + 1;
    }
    newBlock.pItems[newBlockCapacity - 1].NextFreeIndex = UINT32_MAX;
    return m_ItemBlocks.back();
}

////////////////////////////////////////////////////////////////////////////////
// Private class List

/*
Doubly linked list, with elements allocated out of PoolAllocator.
Has custom interface, as well as STL-style interface, including iterator and
const_iterator.
*/
template<typename T>
class List
{
    D3D12MA_CLASS_NO_COPY(List)
public:
    struct Item
    {
        Item* pPrev;
        Item* pNext;
        T Value;
    };

    // allocationCallbacks externally owned, must outlive this object.
    List(const ALLOCATION_CALLBACKS& allocationCallbacks);
    ~List();
    void Clear();

    size_t GetCount() const { return m_Count; }
    bool IsEmpty() const { return m_Count == 0; }

    Item* Front() { return m_pFront; }
    const Item* Front() const { return m_pFront; }
    Item* Back() { return m_pBack; }
    const Item* Back() const { return m_pBack; }

    Item* PushBack();
    Item* PushFront();
    Item* PushBack(const T& value);
    Item* PushFront(const T& value);
    void PopBack();
    void PopFront();

    // Item can be null - it means PushBack.
    Item* InsertBefore(Item* pItem);
    // Item can be null - it means PushFront.
    Item* InsertAfter(Item* pItem);

    Item* InsertBefore(Item* pItem, const T& value);
    Item* InsertAfter(Item* pItem, const T& value);

    void Remove(Item* pItem);

    class iterator
    {
    public:
        iterator() :
            m_pList(NULL),
            m_pItem(NULL)
        {
        }

        T& operator*() const
        {
            D3D12MA_HEAVY_ASSERT(m_pItem != NULL);
            return m_pItem->Value;
        }
        T* operator->() const
        {
            D3D12MA_HEAVY_ASSERT(m_pItem != NULL);
            return &m_pItem->Value;
        }

        iterator& operator++()
        {
            D3D12MA_HEAVY_ASSERT(m_pItem != NULL);
            m_pItem = m_pItem->pNext;
            return *this;
        }
        iterator& operator--()
        {
            if(m_pItem != NULL)
            {
                m_pItem = m_pItem->pPrev;
            }
            else
            {
                D3D12MA_HEAVY_ASSERT(!m_pList->IsEmpty());
                m_pItem = m_pList->Back();
            }
            return *this;
        }

        iterator operator++(int)
        {
            iterator result = *this;
            ++*this;
            return result;
        }
        iterator operator--(int)
        {
            iterator result = *this;
            --*this;
            return result;
        }

        bool operator==(const iterator& rhs) const
        {
            D3D12MA_HEAVY_ASSERT(m_pList == rhs.m_pList);
            return m_pItem == rhs.m_pItem;
        }
        bool operator!=(const iterator& rhs) const
        {
            D3D12MA_HEAVY_ASSERT(m_pList == rhs.m_pList);
            return m_pItem != rhs.m_pItem;
        }

    private:
        List<T>* m_pList;
        List<T>::Item* m_pItem;

        iterator(List<T>* pList, List<T>::Item* pItem) :
            m_pList(pList),
            m_pItem(pItem)
        {
        }

        friend class List<T>;
    };

    class const_iterator
    {
    public:
        const_iterator() :
            m_pList(NULL),
            m_pItem(NULL)
        {
        }

        const_iterator(const iterator& src) :
            m_pList(src.m_pList),
            m_pItem(src.m_pItem)
        {
        }

        const T& operator*() const
        {
            D3D12MA_HEAVY_ASSERT(m_pItem != NULL);
            return m_pItem->Value;
        }
        const T* operator->() const
        {
            D3D12MA_HEAVY_ASSERT(m_pItem != NULL);
            return &m_pItem->Value;
        }

        const_iterator& operator++()
        {
            D3D12MA_HEAVY_ASSERT(m_pItem != NULL);
            m_pItem = m_pItem->pNext;
            return *this;
        }
        const_iterator& operator--()
        {
            if(m_pItem != NULL)
            {
                m_pItem = m_pItem->pPrev;
            }
            else
            {
                D3D12MA_HEAVY_ASSERT(!m_pList->IsEmpty());
                m_pItem = m_pList->Back();
            }
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator result = *this;
            ++*this;
            return result;
        }
        const_iterator operator--(int)
        {
            const_iterator result = *this;
            --*this;
            return result;
        }

        bool operator==(const const_iterator& rhs) const
        {
            D3D12MA_HEAVY_ASSERT(m_pList == rhs.m_pList);
            return m_pItem == rhs.m_pItem;
        }
        bool operator!=(const const_iterator& rhs) const
        {
            D3D12MA_HEAVY_ASSERT(m_pList == rhs.m_pList);
            return m_pItem != rhs.m_pItem;
        }

    private:
        const_iterator(const List<T>* pList, const List<T>::Item* pItem) :
            m_pList(pList),
            m_pItem(pItem)
        {
        }

        const List<T>* m_pList;
        const List<T>::Item* m_pItem;

        friend class List<T>;
    };

    bool empty() const { return IsEmpty(); }
    size_t size() const { return GetCount(); }

    iterator begin() { return iterator(this, Front()); }
    iterator end() { return iterator(this, NULL); }

    const_iterator cbegin() const { return const_iterator(this, Front()); }
    const_iterator cend() const { return const_iterator(this, NULL); }

    void clear() { Clear(); }
    void push_back(const T& value) { PushBack(value); }
    void erase(iterator it) { Remove(it.m_pItem); }
    iterator insert(iterator it, const T& value) { return iterator(this, InsertBefore(it.m_pItem, value)); }

private:
    const ALLOCATION_CALLBACKS& m_AllocationCallbacks;
    PoolAllocator<Item> m_ItemAllocator;
    Item* m_pFront;
    Item* m_pBack;
    size_t m_Count;
};

template<typename T>
List<T>::List(const ALLOCATION_CALLBACKS& allocationCallbacks) :
    m_AllocationCallbacks(allocationCallbacks),
    m_ItemAllocator(allocationCallbacks, 128),
    m_pFront(NULL),
    m_pBack(NULL),
    m_Count(0)
{
}

template<typename T>
List<T>::~List()
{
    // Intentionally not calling Clear, because that would be unnecessary
    // computations to return all items to m_ItemAllocator as free.
}

template<typename T>
void List<T>::Clear()
{
    if(!IsEmpty())
    {
        Item* pItem = m_pBack;
        while(pItem != NULL)
        {
            Item* const pPrevItem = pItem->pPrev;
            m_ItemAllocator.Free(pItem);
            pItem = pPrevItem;
        }
        m_pFront = NULL;
        m_pBack = NULL;
        m_Count = 0;
    }
}

template<typename T>
typename List<T>::Item* List<T>::PushBack()
{
    Item* const pNewItem = m_ItemAllocator.Alloc();
    pNewItem->pNext = NULL;
    if(IsEmpty())
    {
        pNewItem->pPrev = NULL;
        m_pFront = pNewItem;
        m_pBack = pNewItem;
        m_Count = 1;
    }
    else
    {
        pNewItem->pPrev = m_pBack;
        m_pBack->pNext = pNewItem;
        m_pBack = pNewItem;
        ++m_Count;
    }
    return pNewItem;
}

template<typename T>
typename List<T>::Item* List<T>::PushFront()
{
    Item* const pNewItem = m_ItemAllocator.Alloc();
    pNewItem->pPrev = NULL;
    if(IsEmpty())
    {
        pNewItem->pNext = NULL;
        m_pFront = pNewItem;
        m_pBack = pNewItem;
        m_Count = 1;
    }
    else
    {
        pNewItem->pNext = m_pFront;
        m_pFront->pPrev = pNewItem;
        m_pFront = pNewItem;
        ++m_Count;
    }
    return pNewItem;
}

template<typename T>
typename List<T>::Item* List<T>::PushBack(const T& value)
{
    Item* const pNewItem = PushBack();
    pNewItem->Value = value;
    return pNewItem;
}

template<typename T>
typename List<T>::Item* List<T>::PushFront(const T& value)
{
    Item* const pNewItem = PushFront();
    pNewItem->Value = value;
    return pNewItem;
}

template<typename T>
void List<T>::PopBack()
{
    D3D12MA_HEAVY_ASSERT(m_Count > 0);
    Item* const pBackItem = m_pBack;
    Item* const pPrevItem = pBackItem->pPrev;
    if(pPrevItem != NULL)
    {
        pPrevItem->pNext = NULL;
    }
    m_pBack = pPrevItem;
    m_ItemAllocator.Free(pBackItem);
    --m_Count;
}

template<typename T>
void List<T>::PopFront()
{
    D3D12MA_HEAVY_ASSERT(m_Count > 0);
    Item* const pFrontItem = m_pFront;
    Item* const pNextItem = pFrontItem->pNext;
    if(pNextItem != NULL)
    {
        pNextItem->pPrev = NULL;
    }
    m_pFront = pNextItem;
    m_ItemAllocator.Free(pFrontItem);
    --m_Count;
}

template<typename T>
void List<T>::Remove(Item* pItem)
{
    D3D12MA_HEAVY_ASSERT(pItem != NULL);
    D3D12MA_HEAVY_ASSERT(m_Count > 0);

    if(pItem->pPrev != NULL)
    {
        pItem->pPrev->pNext = pItem->pNext;
    }
    else
    {
        D3D12MA_HEAVY_ASSERT(m_pFront == pItem);
        m_pFront = pItem->pNext;
    }

    if(pItem->pNext != NULL)
    {
        pItem->pNext->pPrev = pItem->pPrev;
    }
    else
    {
        D3D12MA_HEAVY_ASSERT(m_pBack == pItem);
        m_pBack = pItem->pPrev;
    }

    m_ItemAllocator.Free(pItem);
    --m_Count;
}

template<typename T>
typename List<T>::Item* List<T>::InsertBefore(Item* pItem)
{
    if(pItem != NULL)
    {
        Item* const prevItem = pItem->pPrev;
        Item* const newItem = m_ItemAllocator.Alloc();
        newItem->pPrev = prevItem;
        newItem->pNext = pItem;
        pItem->pPrev = newItem;
        if(prevItem != NULL)
        {
            prevItem->pNext = newItem;
        }
        else
        {
            D3D12MA_HEAVY_ASSERT(m_pFront == pItem);
            m_pFront = newItem;
        }
        ++m_Count;
        return newItem;
    }
    else
    {
        return PushBack();
    }
}

template<typename T>
typename List<T>::Item* List<T>::InsertAfter(Item* pItem)
{
    if(pItem != NULL)
    {
        Item* const nextItem = pItem->pNext;
        Item* const newItem = m_ItemAllocator.Alloc();
        newItem->pNext = nextItem;
        newItem->pPrev = pItem;
        pItem->pNext = newItem;
        if(nextItem != NULL)
        {
            nextItem->pPrev = newItem;
        }
        else
        {
            D3D12MA_HEAVY_ASSERT(m_pBack == pItem);
            m_pBack = newItem;
        }
        ++m_Count;
        return newItem;
    }
    else
        return PushFront();
}

template<typename T>
typename List<T>::Item* List<T>::InsertBefore(List<T>::Item* pItem, const T& value)
{
    Item* const newItem = InsertBefore(pItem);
    newItem->Value = value;
    return newItem;
}

template<typename T>
typename List<T>::Item* List<T>::InsertAfter(List<T>::Item* pItem, const T& value)
{
    Item* const newItem = InsertAfter(pItem);
    newItem->Value = value;
    return newItem;
}

////////////////////////////////////////////////////////////////////////////////
// Private class BlockMetadata and derived classes - declarations

enum SuballocationType
{
    SUBALLOCATION_TYPE_FREE = 0,
    SUBALLOCATION_TYPE_ALLOCATION = 1,
};

/*
Represents a region of NormalBlock that is either assigned and returned as
allocated memory block or free.
*/
struct Suballocation
{
    UINT64 offset;
    UINT64 size;
    Allocation* allocation;
    SuballocationType type;
};

// Comparator for offsets.
struct SuballocationOffsetLess
{
    bool operator()(const Suballocation& lhs, const Suballocation& rhs) const
    {
        return lhs.offset < rhs.offset;
    }
};
struct SuballocationOffsetGreater
{
    bool operator()(const Suballocation& lhs, const Suballocation& rhs) const
    {
        return lhs.offset > rhs.offset;
    }
};

typedef List<Suballocation> SuballocationList;

struct SuballocationItemSizeLess
{
    bool operator()(const SuballocationList::iterator lhs, const SuballocationList::iterator rhs) const
    {
        return lhs->size < rhs->size;
    }
    bool operator()(const SuballocationList::iterator lhs, UINT64 rhsSize) const
    {
        return lhs->size < rhsSize;
    }
};

/*
Parameters of planned allocation inside a NormalBlock.
*/
struct AllocationRequest
{
    UINT64 offset;
    UINT64 sumFreeSize; // Sum size of free items that overlap with proposed allocation.
    UINT64 sumItemSize; // Sum size of items to make lost that overlap with proposed allocation.
    SuballocationList::iterator item;
};

/*
Data structure used for bookkeeping of allocations and unused ranges of memory
in a single ID3D12Heap memory block.
*/
class BlockMetadata
{
public:
    BlockMetadata(const ALLOCATION_CALLBACKS* allocationCallbacks);
    virtual ~BlockMetadata() { }
    virtual void Init(UINT64 size) { m_Size = size; }

    // Validates all data structures inside this object. If not valid, returns false.
    virtual bool Validate() const = 0;
    UINT64 GetSize() const { return m_Size; }
    virtual size_t GetAllocationCount() const = 0;
    virtual UINT64 GetSumFreeSize() const = 0;
    virtual UINT64 GetUnusedRangeSizeMax() const = 0;
    // Returns true if this block is empty - contains only single free suballocation.
    virtual bool IsEmpty() const = 0;

    // Tries to find a place for suballocation with given parameters inside this block.
    // If succeeded, fills pAllocationRequest and returns true.
    // If failed, returns false.
    virtual bool CreateAllocationRequest(
        UINT64 allocSize,
        UINT64 allocAlignment,
        AllocationRequest* pAllocationRequest) = 0;

    // Makes actual allocation based on request. Request must already be checked and valid.
    virtual void Alloc(
        const AllocationRequest& request,
        UINT64 allocSize,
        Allocation* Allocation) = 0;

    // Frees suballocation assigned to given memory region.
    virtual void Free(const Allocation* allocation) = 0;
    virtual void FreeAtOffset(UINT64 offset) = 0;

    virtual void CalcAllocationStatInfo(StatInfo& outInfo) const = 0;
    virtual void WriteAllocationInfoToJson(JsonWriter& json) const = 0;

protected:
    const ALLOCATION_CALLBACKS* GetAllocs() const { return m_pAllocationCallbacks; }

private:
    UINT64 m_Size;
    const ALLOCATION_CALLBACKS* m_pAllocationCallbacks;

    D3D12MA_CLASS_NO_COPY(BlockMetadata);
};

class BlockMetadata_Generic : public BlockMetadata
{
public:
    BlockMetadata_Generic(const ALLOCATION_CALLBACKS* allocationCallbacks);
    virtual ~BlockMetadata_Generic();
    virtual void Init(UINT64 size);

    virtual bool Validate() const;
    virtual size_t GetAllocationCount() const { return m_Suballocations.size() - m_FreeCount; }
    virtual UINT64 GetSumFreeSize() const { return m_SumFreeSize; }
    virtual UINT64 GetUnusedRangeSizeMax() const;
    virtual bool IsEmpty() const;

    virtual bool CreateAllocationRequest(
        UINT64 allocSize,
        UINT64 allocAlignment,
        AllocationRequest* pAllocationRequest);

    virtual void Alloc(
        const AllocationRequest& request,
        UINT64 allocSize,
        Allocation* hAllocation);

    virtual void Free(const Allocation* allocation);
    virtual void FreeAtOffset(UINT64 offset);

    virtual void CalcAllocationStatInfo(StatInfo& outInfo) const;
    virtual void WriteAllocationInfoToJson(JsonWriter& json) const;

private:
    UINT m_FreeCount;
    UINT64 m_SumFreeSize;
    SuballocationList m_Suballocations;
    // Suballocations that are free and have size greater than certain threshold.
    // Sorted by size, ascending.
    Vector<SuballocationList::iterator> m_FreeSuballocationsBySize;

    bool ValidateFreeSuballocationList() const;

    // Checks if requested suballocation with given parameters can be placed in given pFreeSuballocItem.
    // If yes, fills pOffset and returns true. If no, returns false.
    bool CheckAllocation(
        UINT64 allocSize,
        UINT64 allocAlignment,
        SuballocationList::const_iterator suballocItem,
        UINT64* pOffset,
        UINT64* pSumFreeSize,
        UINT64* pSumItemSize) const;
    // Given free suballocation, it merges it with following one, which must also be free.
    void MergeFreeWithNext(SuballocationList::iterator item);
    // Releases given suballocation, making it free.
    // Merges it with adjacent free suballocations if applicable.
    // Returns iterator to new free suballocation at this place.
    SuballocationList::iterator FreeSuballocation(SuballocationList::iterator suballocItem);
    // Given free suballocation, it inserts it into sorted list of
    // m_FreeSuballocationsBySize if it's suitable.
    void RegisterFreeSuballocation(SuballocationList::iterator item);
    // Given free suballocation, it removes it from sorted list of
    // m_FreeSuballocationsBySize if it's suitable.
    void UnregisterFreeSuballocation(SuballocationList::iterator item);

    D3D12MA_CLASS_NO_COPY(BlockMetadata_Generic)
};

////////////////////////////////////////////////////////////////////////////////
// Private class MemoryBlock definition

/*
Represents a single block of device memory (heap).
Base class for inheritance.
Thread-safety: This class must be externally synchronized.
*/
class MemoryBlock
{
public:
    MemoryBlock(
        AllocatorPimpl* allocator,
        D3D12_HEAP_TYPE heapType,
        D3D12_HEAP_FLAGS heapFlags,
        UINT64 size,
        UINT id);
    virtual ~MemoryBlock();
    // Creates the ID3D12Heap.

    D3D12_HEAP_TYPE GetHeapType() const { return m_HeapType; }
    D3D12_HEAP_FLAGS GetHeapFlags() const { return m_HeapFlags; }
    UINT64 GetSize() const { return m_Size; }
    UINT GetId() const { return m_Id; }
    ID3D12Heap* GetHeap() const { return m_Heap; }

protected:
    AllocatorPimpl* const m_Allocator;
    const D3D12_HEAP_TYPE m_HeapType;
    const D3D12_HEAP_FLAGS m_HeapFlags;
    const UINT64 m_Size;
    const UINT m_Id;

    HRESULT Init();

private:
    ID3D12Heap* m_Heap = NULL;

    D3D12MA_CLASS_NO_COPY(MemoryBlock)
};

////////////////////////////////////////////////////////////////////////////////
// Private class NormalBlock definition

/*
Represents a single block of device memory (heap) with all the data about its
regions (aka suballocations, Allocation), assigned and free.
Thread-safety: This class must be externally synchronized.
*/
class NormalBlock : public MemoryBlock
{
public:
    BlockMetadata* m_pMetadata;

    NormalBlock(
        AllocatorPimpl* allocator,
        BlockVector* blockVector,
        D3D12_HEAP_TYPE heapType,
        D3D12_HEAP_FLAGS heapFlags,
        UINT64 size,
        UINT id);
    virtual ~NormalBlock();
    HRESULT Init();

    BlockVector* GetBlockVector() const { return m_BlockVector; }

    // Validates all data structures inside this object. If not valid, returns false.
    bool Validate() const;

private:
    BlockVector* m_BlockVector;

    D3D12MA_CLASS_NO_COPY(NormalBlock)
};

////////////////////////////////////////////////////////////////////////////////
// Private class BlockVector definition

/*
Sequence of NormalBlock. Represents memory blocks allocated for a specific
heap type and possibly resource type (if only Tier 1 is supported).

Synchronized internally with a mutex.
*/
class BlockVector
{
    D3D12MA_CLASS_NO_COPY(BlockVector)
public:
    BlockVector(
        AllocatorPimpl* hAllocator,
        D3D12_HEAP_TYPE heapType,
        D3D12_HEAP_FLAGS heapFlags,
        UINT64 preferredBlockSize,
        size_t minBlockCount,
        size_t maxBlockCount,
        bool explicitBlockSize);
    ~BlockVector();

    HRESULT CreateMinBlocks();

    UINT GetHeapType() const { return m_HeapType; }
    UINT64 GetPreferredBlockSize() const { return m_PreferredBlockSize; }

    bool IsEmpty() const { return m_Blocks.empty(); }

    HRESULT Allocate(
        UINT64 size,
        UINT64 alignment,
        const ALLOCATION_DESC& createInfo,
        size_t allocationCount,
        Allocation** pAllocations);

    void Free(
        Allocation* hAllocation);

    void AddStats(Stats& outpStats);

    void WriteBlockInfoToJson(JsonWriter& json);

private:
    AllocatorPimpl* const m_hAllocator;
    const D3D12_HEAP_TYPE m_HeapType;
    const D3D12_HEAP_FLAGS m_HeapFlags;
    const UINT64 m_PreferredBlockSize;
    const size_t m_MinBlockCount;
    const size_t m_MaxBlockCount;
    const bool m_ExplicitBlockSize;
    /* There can be at most one allocation that is completely empty - a
    hysteresis to avoid pessimistic case of alternating creation and destruction
    of a VkDeviceMemory. */
    bool m_HasEmptyBlock;
    D3D12MA_RW_MUTEX m_Mutex;
    // Incrementally sorted by sumFreeSize, ascending.
    Vector<NormalBlock*> m_Blocks;
    UINT m_NextBlockId;

    UINT64 CalcMaxBlockSize() const;

    // Finds and removes given block from vector.
    void Remove(NormalBlock* pBlock);

    // Performs single step in sorting m_Blocks. They may not be fully sorted
    // after this call.
    void IncrementallySortBlocks();

    HRESULT AllocatePage(
        UINT64 size,
        UINT64 alignment,
        const ALLOCATION_DESC& createInfo,
        Allocation** pAllocation);

    HRESULT AllocateFromBlock(
        NormalBlock* pBlock,
        UINT64 size,
        UINT64 alignment,
        ALLOCATION_FLAGS allocFlags,
        Allocation** pAllocation);

    HRESULT CreateBlock(UINT64 blockSize, size_t* pNewBlockIndex);
};

////////////////////////////////////////////////////////////////////////////////
// Private class AllocatorPimpl definition

static const UINT DEFAULT_POOL_MAX_COUNT = 9;

class AllocatorPimpl
{
public:
    AllocatorPimpl(const ALLOCATION_CALLBACKS& allocationCallbacks, const ALLOCATOR_DESC& desc);
    HRESULT Init();
    ~AllocatorPimpl();

    ID3D12Device* GetDevice() const { return m_Device; }
    // Shortcut for "Allocation Callbacks", because this function is called so often.
    const ALLOCATION_CALLBACKS& GetAllocs() const { return m_AllocationCallbacks; }
    const D3D12_FEATURE_DATA_D3D12_OPTIONS& GetD3D12Options() const { return m_D3D12Options; }
    bool SupportsResourceHeapTier2() const { return m_D3D12Options.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2; }
    bool UseMutex() const { return m_UseMutex; }

    HRESULT CreateResource(
        const ALLOCATION_DESC* pAllocDesc,
        const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE *pOptimizedClearValue,
        Allocation** ppAllocation,
        REFIID riidResource,
        void** ppvResource);

    HRESULT AllocateMemory(
        const ALLOCATION_DESC* pAllocDesc,
        D3D12_HEAP_FLAGS heapFlags,
        const D3D12_RESOURCE_ALLOCATION_INFO* pAllocInfo,
        Allocation** ppAllocation);

    // Unregisters allocation from the collection of dedicated allocations.
    // Allocation object must be deleted externally afterwards.
    void FreeCommittedMemory(Allocation* allocation);
    // Unregisters allocation from the collection of placed allocations.
    // Allocation object must be deleted externally afterwards.
    void FreePlacedMemory(Allocation* allocation);
    // Unregisters allocation from the collection of dedicated allocations and destroys associated heap.
    // Allocation object must be deleted externally afterwards.
    void FreeHeapMemory(Allocation* allocation);

    void SetCurrentFrameIndex(UINT frameIndex);

    UINT GetCurrentFrameIndex() const { return m_CurrentFrameIndex.load(); }

    void CalculateStats(Stats& outStats);

    void BuildStatsString(WCHAR** ppStatsString, BOOL DetailedMap);

    void FreeStatsString(WCHAR* pStatsString);

private:
    friend class Allocator;

    /*
    Heuristics that decides whether a resource should better be placed in its own,
    dedicated allocation (committed resource rather than placed resource).
    */
    static bool PrefersCommittedAllocation(const D3D12_RESOURCE_DESC& resourceDesc);

    const bool m_UseMutex;
    const bool m_AlwaysCommitted;
    ID3D12Device* m_Device;
    UINT64 m_PreferredBlockSize;
    ALLOCATION_CALLBACKS m_AllocationCallbacks;
    D3D12MA_ATOMIC_UINT32 m_CurrentFrameIndex;

    D3D12_FEATURE_DATA_D3D12_OPTIONS m_D3D12Options;

    typedef Vector<Allocation*> AllocationVectorType;
    AllocationVectorType* m_pCommittedAllocations[HEAP_TYPE_COUNT];
    D3D12MA_RW_MUTEX m_CommittedAllocationsMutex[HEAP_TYPE_COUNT];

    // Default pools.
    BlockVector* m_BlockVectors[DEFAULT_POOL_MAX_COUNT];

    // Allocates and registers new committed resource with implicit heap, as dedicated allocation.
    // Creates and returns Allocation object.
    HRESULT AllocateCommittedResource(
        const ALLOCATION_DESC* pAllocDesc,
        const D3D12_RESOURCE_DESC* pResourceDesc,
        const D3D12_RESOURCE_ALLOCATION_INFO& resAllocInfo,
        D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE *pOptimizedClearValue,
        Allocation** ppAllocation,
        REFIID riidResource,
        void** ppvResource);

    // Allocates and registers new heap without any resources placed in it, as dedicated allocation.
    // Creates and returns Allocation object.
    HRESULT AllocateHeap(
        const ALLOCATION_DESC* pAllocDesc,
        D3D12_HEAP_FLAGS heapFlags,
        const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo,
        Allocation** ppAllocation);

    /*
    If SupportsResourceHeapTier2():
        0: D3D12_HEAP_TYPE_DEFAULT
        1: D3D12_HEAP_TYPE_UPLOAD
        2: D3D12_HEAP_TYPE_READBACK
    else:
        0: D3D12_HEAP_TYPE_DEFAULT + buffer
        1: D3D12_HEAP_TYPE_DEFAULT + texture
        2: D3D12_HEAP_TYPE_DEFAULT + texture RT or DS
        3: D3D12_HEAP_TYPE_UPLOAD + buffer
        4: D3D12_HEAP_TYPE_UPLOAD + texture
        5: D3D12_HEAP_TYPE_UPLOAD + texture RT or DS
        6: D3D12_HEAP_TYPE_READBACK + buffer
        7: D3D12_HEAP_TYPE_READBACK + texture
        8: D3D12_HEAP_TYPE_READBACK + texture RT or DS
    */
    UINT CalcDefaultPoolCount() const;
    UINT CalcDefaultPoolIndex(const ALLOCATION_DESC& allocDesc, const D3D12_RESOURCE_DESC& resourceDesc) const;
    // This one returns UINT32_MAX if nonstandard heap flags are used and index cannot be calculcated.
    UINT CalcDefaultPoolIndex(const ALLOCATION_DESC& allocDesc, const D3D12_HEAP_FLAGS heapFlags) const;
    void CalcDefaultPoolParams(D3D12_HEAP_TYPE& outHeapType, D3D12_HEAP_FLAGS& outHeapFlags, UINT index) const;

    // Registers Allocation object in m_pCommittedAllocations.
    void RegisterCommittedAllocation(Allocation* alloc, D3D12_HEAP_TYPE heapType);
    // Unregisters Allocation object from m_pCommittedAllocations.
    void UnregisterCommittedAllocation(Allocation* alloc, D3D12_HEAP_TYPE heapType);
};

////////////////////////////////////////////////////////////////////////////////
// Private class BlockMetadata implementation

BlockMetadata::BlockMetadata(const ALLOCATION_CALLBACKS* allocationCallbacks) :
    m_Size(0),
    m_pAllocationCallbacks(allocationCallbacks)
{
    D3D12MA_ASSERT(allocationCallbacks);
}

////////////////////////////////////////////////////////////////////////////////
// Private class BlockMetadata_Generic implementation

BlockMetadata_Generic::BlockMetadata_Generic(const ALLOCATION_CALLBACKS* allocationCallbacks) :
    BlockMetadata(allocationCallbacks),
    m_FreeCount(0),
    m_SumFreeSize(0),
    m_Suballocations(*allocationCallbacks),
    m_FreeSuballocationsBySize(*allocationCallbacks)
{
    D3D12MA_ASSERT(allocationCallbacks);
}

BlockMetadata_Generic::~BlockMetadata_Generic()
{
}

void BlockMetadata_Generic::Init(UINT64 size)
{
    BlockMetadata::Init(size);

    m_FreeCount = 1;
    m_SumFreeSize = size;

    Suballocation suballoc = {};
    suballoc.offset = 0;
    suballoc.size = size;
    suballoc.type = SUBALLOCATION_TYPE_FREE;
    suballoc.allocation = NULL;

    D3D12MA_ASSERT(size > MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER);
    m_Suballocations.push_back(suballoc);
    SuballocationList::iterator suballocItem = m_Suballocations.end();
    --suballocItem;
    m_FreeSuballocationsBySize.push_back(suballocItem);
}

bool BlockMetadata_Generic::Validate() const
{
    D3D12MA_VALIDATE(!m_Suballocations.empty());

    // Expected offset of new suballocation as calculated from previous ones.
    UINT64 calculatedOffset = 0;
    // Expected number of free suballocations as calculated from traversing their list.
    UINT calculatedFreeCount = 0;
    // Expected sum size of free suballocations as calculated from traversing their list.
    UINT64 calculatedSumFreeSize = 0;
    // Expected number of free suballocations that should be registered in
    // m_FreeSuballocationsBySize calculated from traversing their list.
    size_t freeSuballocationsToRegister = 0;
    // True if previous visited suballocation was free.
    bool prevFree = false;

    for(SuballocationList::const_iterator suballocItem = m_Suballocations.cbegin();
        suballocItem != m_Suballocations.cend();
        ++suballocItem)
    {
        const Suballocation& subAlloc = *suballocItem;

        // Actual offset of this suballocation doesn't match expected one.
        D3D12MA_VALIDATE(subAlloc.offset == calculatedOffset);

        const bool currFree = (subAlloc.type == SUBALLOCATION_TYPE_FREE);
        // Two adjacent free suballocations are invalid. They should be merged.
        D3D12MA_VALIDATE(!prevFree || !currFree);

        D3D12MA_VALIDATE(currFree == (subAlloc.allocation == NULL));

        if(currFree)
        {
            calculatedSumFreeSize += subAlloc.size;
            ++calculatedFreeCount;
            if(subAlloc.size >= MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER)
            {
                ++freeSuballocationsToRegister;
            }

            // Margin required between allocations - every free space must be at least that large.
            D3D12MA_VALIDATE(subAlloc.size >= D3D12MA_DEBUG_MARGIN);
        }
        else
        {
            D3D12MA_VALIDATE(subAlloc.allocation->GetOffset() == subAlloc.offset);
            D3D12MA_VALIDATE(subAlloc.allocation->GetSize() == subAlloc.size);

            // Margin required between allocations - previous allocation must be free.
            D3D12MA_VALIDATE(D3D12MA_DEBUG_MARGIN == 0 || prevFree);
        }

        calculatedOffset += subAlloc.size;
        prevFree = currFree;
    }

    // Number of free suballocations registered in m_FreeSuballocationsBySize doesn't
    // match expected one.
    D3D12MA_VALIDATE(m_FreeSuballocationsBySize.size() == freeSuballocationsToRegister);

    UINT64 lastSize = 0;
    for(size_t i = 0; i < m_FreeSuballocationsBySize.size(); ++i)
    {
        SuballocationList::iterator suballocItem = m_FreeSuballocationsBySize[i];

        // Only free suballocations can be registered in m_FreeSuballocationsBySize.
        D3D12MA_VALIDATE(suballocItem->type == SUBALLOCATION_TYPE_FREE);
        // They must be sorted by size ascending.
        D3D12MA_VALIDATE(suballocItem->size >= lastSize);

        lastSize = suballocItem->size;
    }

    // Check if totals match calculacted values.
    D3D12MA_VALIDATE(ValidateFreeSuballocationList());
    D3D12MA_VALIDATE(calculatedOffset == GetSize());
    D3D12MA_VALIDATE(calculatedSumFreeSize == m_SumFreeSize);
    D3D12MA_VALIDATE(calculatedFreeCount == m_FreeCount);

    return true;
}

UINT64 BlockMetadata_Generic::GetUnusedRangeSizeMax() const
{
    if(!m_FreeSuballocationsBySize.empty())
    {
        return m_FreeSuballocationsBySize.back()->size;
    }
    else
    {
        return 0;
    }
}

bool BlockMetadata_Generic::IsEmpty() const
{
    return (m_Suballocations.size() == 1) && (m_FreeCount == 1);
}

bool BlockMetadata_Generic::CreateAllocationRequest(
    UINT64 allocSize,
    UINT64 allocAlignment,
    AllocationRequest* pAllocationRequest)
{
    D3D12MA_ASSERT(allocSize > 0);
    D3D12MA_ASSERT(pAllocationRequest != NULL);
    D3D12MA_HEAVY_ASSERT(Validate());

    // There is not enough total free space in this block to fullfill the request: Early return.
    if(m_SumFreeSize < allocSize + 2 * D3D12MA_DEBUG_MARGIN)
    {
        return false;
    }

    // New algorithm, efficiently searching freeSuballocationsBySize.
    const size_t freeSuballocCount = m_FreeSuballocationsBySize.size();
    if(freeSuballocCount > 0)
    {
        // Find first free suballocation with size not less than allocSize + 2 * D3D12MA_DEBUG_MARGIN.
        SuballocationList::iterator* const it = BinaryFindFirstNotLess(
            m_FreeSuballocationsBySize.data(),
            m_FreeSuballocationsBySize.data() + freeSuballocCount,
            allocSize + 2 * D3D12MA_DEBUG_MARGIN,
            SuballocationItemSizeLess());
        size_t index = it - m_FreeSuballocationsBySize.data();
        for(; index < freeSuballocCount; ++index)
        {
            if(CheckAllocation(
                allocSize,
                allocAlignment,
                m_FreeSuballocationsBySize[index],
                &pAllocationRequest->offset,
                &pAllocationRequest->sumFreeSize,
                &pAllocationRequest->sumItemSize))
            {
                pAllocationRequest->item = m_FreeSuballocationsBySize[index];
                return true;
            }
        }
    }

    return false;
}

void BlockMetadata_Generic::Alloc(
    const AllocationRequest& request,
    UINT64 allocSize,
    Allocation* allocation)
{
    D3D12MA_ASSERT(request.item != m_Suballocations.end());
    Suballocation& suballoc = *request.item;
    // Given suballocation is a free block.
    D3D12MA_ASSERT(suballoc.type == SUBALLOCATION_TYPE_FREE);
    // Given offset is inside this suballocation.
    D3D12MA_ASSERT(request.offset >= suballoc.offset);
    const UINT64 paddingBegin = request.offset - suballoc.offset;
    D3D12MA_ASSERT(suballoc.size >= paddingBegin + allocSize);
    const UINT64 paddingEnd = suballoc.size - paddingBegin - allocSize;

    // Unregister this free suballocation from m_FreeSuballocationsBySize and update
    // it to become used.
    UnregisterFreeSuballocation(request.item);

    suballoc.offset = request.offset;
    suballoc.size = allocSize;
    suballoc.type = SUBALLOCATION_TYPE_ALLOCATION;
    suballoc.allocation = allocation;

    // If there are any free bytes remaining at the end, insert new free suballocation after current one.
    if(paddingEnd)
    {
        Suballocation paddingSuballoc = {};
        paddingSuballoc.offset = request.offset + allocSize;
        paddingSuballoc.size = paddingEnd;
        paddingSuballoc.type = SUBALLOCATION_TYPE_FREE;
        SuballocationList::iterator next = request.item;
        ++next;
        const SuballocationList::iterator paddingEndItem =
            m_Suballocations.insert(next, paddingSuballoc);
        RegisterFreeSuballocation(paddingEndItem);
    }

    // If there are any free bytes remaining at the beginning, insert new free suballocation before current one.
    if(paddingBegin)
    {
        Suballocation paddingSuballoc = {};
        paddingSuballoc.offset = request.offset - paddingBegin;
        paddingSuballoc.size = paddingBegin;
        paddingSuballoc.type = SUBALLOCATION_TYPE_FREE;
        const SuballocationList::iterator paddingBeginItem =
            m_Suballocations.insert(request.item, paddingSuballoc);
        RegisterFreeSuballocation(paddingBeginItem);
    }

    // Update totals.
    m_FreeCount = m_FreeCount - 1;
    if(paddingBegin > 0)
    {
        ++m_FreeCount;
    }
    if(paddingEnd > 0)
    {
        ++m_FreeCount;
    }
    m_SumFreeSize -= allocSize;
}

void BlockMetadata_Generic::Free(const Allocation* allocation)
{
    for(SuballocationList::iterator suballocItem = m_Suballocations.begin();
        suballocItem != m_Suballocations.end();
        ++suballocItem)
    {
        Suballocation& suballoc = *suballocItem;
        if(suballoc.allocation == allocation)
        {
            FreeSuballocation(suballocItem);
            D3D12MA_HEAVY_ASSERT(Validate());
            return;
        }
    }
    D3D12MA_ASSERT(0 && "Not found!");
}

void BlockMetadata_Generic::FreeAtOffset(UINT64 offset)
{
    for(SuballocationList::iterator suballocItem = m_Suballocations.begin();
        suballocItem != m_Suballocations.end();
        ++suballocItem)
    {
        Suballocation& suballoc = *suballocItem;
        if(suballoc.offset == offset)
        {
            FreeSuballocation(suballocItem);
            return;
        }
    }
    D3D12MA_ASSERT(0 && "Not found!");
}

bool BlockMetadata_Generic::ValidateFreeSuballocationList() const
{
    UINT64 lastSize = 0;
    for(size_t i = 0, count = m_FreeSuballocationsBySize.size(); i < count; ++i)
    {
        const SuballocationList::iterator it = m_FreeSuballocationsBySize[i];

        D3D12MA_VALIDATE(it->type == SUBALLOCATION_TYPE_FREE);
        D3D12MA_VALIDATE(it->size >= MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER);
        D3D12MA_VALIDATE(it->size >= lastSize);
        lastSize = it->size;
    }
    return true;
}

bool BlockMetadata_Generic::CheckAllocation(
    UINT64 allocSize,
    UINT64 allocAlignment,
    SuballocationList::const_iterator suballocItem,
    UINT64* pOffset,
    UINT64* pSumFreeSize,
    UINT64* pSumItemSize) const
{
    D3D12MA_ASSERT(allocSize > 0);
    D3D12MA_ASSERT(suballocItem != m_Suballocations.cend());
    D3D12MA_ASSERT(pOffset != NULL);

    *pSumFreeSize = 0;
    *pSumItemSize = 0;

    const Suballocation& suballoc = *suballocItem;
    D3D12MA_ASSERT(suballoc.type == SUBALLOCATION_TYPE_FREE);

    *pSumFreeSize = suballoc.size;

    // Size of this suballocation is too small for this request: Early return.
    if(suballoc.size < allocSize)
    {
        return false;
    }

    // Start from offset equal to beginning of this suballocation.
    *pOffset = suballoc.offset;

    // Apply D3D12MA_DEBUG_MARGIN at the beginning.
    if(D3D12MA_DEBUG_MARGIN > 0)
    {
        *pOffset += D3D12MA_DEBUG_MARGIN;
    }

    // Apply alignment.
    *pOffset = AlignUp(*pOffset, allocAlignment);

    // Calculate padding at the beginning based on current offset.
    const UINT64 paddingBegin = *pOffset - suballoc.offset;

    // Calculate required margin at the end.
    const UINT64 requiredEndMargin = D3D12MA_DEBUG_MARGIN;

    // Fail if requested size plus margin before and after is bigger than size of this suballocation.
    if(paddingBegin + allocSize + requiredEndMargin > suballoc.size)
    {
        return false;
    }

    // All tests passed: Success. pOffset is already filled.
    return true;
}

void BlockMetadata_Generic::MergeFreeWithNext(SuballocationList::iterator item)
{
    D3D12MA_ASSERT(item != m_Suballocations.end());
    D3D12MA_ASSERT(item->type == SUBALLOCATION_TYPE_FREE);

    SuballocationList::iterator nextItem = item;
    ++nextItem;
    D3D12MA_ASSERT(nextItem != m_Suballocations.end());
    D3D12MA_ASSERT(nextItem->type == SUBALLOCATION_TYPE_FREE);

    item->size += nextItem->size;
    --m_FreeCount;
    m_Suballocations.erase(nextItem);
}

SuballocationList::iterator BlockMetadata_Generic::FreeSuballocation(SuballocationList::iterator suballocItem)
{
    // Change this suballocation to be marked as free.
    Suballocation& suballoc = *suballocItem;
    suballoc.type = SUBALLOCATION_TYPE_FREE;
    suballoc.allocation = NULL;

    // Update totals.
    ++m_FreeCount;
    m_SumFreeSize += suballoc.size;

    // Merge with previous and/or next suballocation if it's also free.
    bool mergeWithNext = false;
    bool mergeWithPrev = false;

    SuballocationList::iterator nextItem = suballocItem;
    ++nextItem;
    if((nextItem != m_Suballocations.end()) && (nextItem->type == SUBALLOCATION_TYPE_FREE))
    {
        mergeWithNext = true;
    }

    SuballocationList::iterator prevItem = suballocItem;
    if(suballocItem != m_Suballocations.begin())
    {
        --prevItem;
        if(prevItem->type == SUBALLOCATION_TYPE_FREE)
        {
            mergeWithPrev = true;
        }
    }

    if(mergeWithNext)
    {
        UnregisterFreeSuballocation(nextItem);
        MergeFreeWithNext(suballocItem);
    }

    if(mergeWithPrev)
    {
        UnregisterFreeSuballocation(prevItem);
        MergeFreeWithNext(prevItem);
        RegisterFreeSuballocation(prevItem);
        return prevItem;
    }
    else
    {
        RegisterFreeSuballocation(suballocItem);
        return suballocItem;
    }
}

void BlockMetadata_Generic::RegisterFreeSuballocation(SuballocationList::iterator item)
{
    D3D12MA_ASSERT(item->type == SUBALLOCATION_TYPE_FREE);
    D3D12MA_ASSERT(item->size > 0);

    // You may want to enable this validation at the beginning or at the end of
    // this function, depending on what do you want to check.
    D3D12MA_HEAVY_ASSERT(ValidateFreeSuballocationList());

    if(item->size >= MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER)
    {
        if(m_FreeSuballocationsBySize.empty())
        {
            m_FreeSuballocationsBySize.push_back(item);
        }
        else
        {
            m_FreeSuballocationsBySize.InsertSorted(item, SuballocationItemSizeLess());
        }
    }

    //D3D12MA_HEAVY_ASSERT(ValidateFreeSuballocationList());
}


void BlockMetadata_Generic::UnregisterFreeSuballocation(SuballocationList::iterator item)
{
    D3D12MA_ASSERT(item->type == SUBALLOCATION_TYPE_FREE);
    D3D12MA_ASSERT(item->size > 0);

    // You may want to enable this validation at the beginning or at the end of
    // this function, depending on what do you want to check.
    D3D12MA_HEAVY_ASSERT(ValidateFreeSuballocationList());

    if(item->size >= MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER)
    {
        SuballocationList::iterator* const it = BinaryFindFirstNotLess(
            m_FreeSuballocationsBySize.data(),
            m_FreeSuballocationsBySize.data() + m_FreeSuballocationsBySize.size(),
            item,
            SuballocationItemSizeLess());
        for(size_t index = it - m_FreeSuballocationsBySize.data();
            index < m_FreeSuballocationsBySize.size();
            ++index)
        {
            if(m_FreeSuballocationsBySize[index] == item)
            {
                m_FreeSuballocationsBySize.remove(index);
                return;
            }
            D3D12MA_ASSERT((m_FreeSuballocationsBySize[index]->size == item->size) && "Not found.");
        }
        D3D12MA_ASSERT(0 && "Not found.");
    }

    //D3D12MA_HEAVY_ASSERT(ValidateFreeSuballocationList());
}

void BlockMetadata_Generic::CalcAllocationStatInfo(StatInfo& outInfo) const
{
    outInfo.BlockCount = 1;

    const UINT rangeCount = (UINT)m_Suballocations.size();
    outInfo.AllocationCount = rangeCount - m_FreeCount;
    outInfo.UnusedRangeCount = m_FreeCount;

    outInfo.UsedBytes = GetSize() - m_SumFreeSize;
    outInfo.UnusedBytes = m_SumFreeSize;

    outInfo.AllocationSizeMin = UINT64_MAX;
    outInfo.AllocationSizeMax = 0;
    outInfo.UnusedRangeSizeMin = UINT64_MAX;
    outInfo.UnusedRangeSizeMax = 0;

    for(SuballocationList::const_iterator suballocItem = m_Suballocations.cbegin();
        suballocItem != m_Suballocations.cend();
        ++suballocItem)
    {
        const Suballocation& suballoc = *suballocItem;
        if(suballoc.type == SUBALLOCATION_TYPE_FREE)
        {
            outInfo.UnusedRangeSizeMin = D3D12MA_MIN(suballoc.size, outInfo.UnusedRangeSizeMin);
            outInfo.UnusedRangeSizeMax = D3D12MA_MAX(suballoc.size, outInfo.UnusedRangeSizeMax);
        }
        else
        {
            outInfo.AllocationSizeMin = D3D12MA_MIN(suballoc.size, outInfo.AllocationSizeMin);
            outInfo.AllocationSizeMax = D3D12MA_MAX(suballoc.size, outInfo.AllocationSizeMax);
        }
    }
}

void BlockMetadata_Generic::WriteAllocationInfoToJson(JsonWriter& json) const
{
    json.BeginObject();
    json.WriteString(L"TotalBytes");
    json.WriteNumber(GetSize());
    json.WriteString(L"UnusuedBytes");
    json.WriteNumber(GetSumFreeSize());
    json.WriteString(L"Allocations");
    json.WriteNumber(GetAllocationCount());
    json.WriteString(L"UnusedRanges");
    json.WriteNumber(m_FreeCount);
    json.WriteString(L"Suballocations");
    json.BeginArray();
    for(SuballocationList::const_iterator suballocItem = m_Suballocations.cbegin();
        suballocItem != m_Suballocations.cend();
        ++suballocItem)
    {
        const Suballocation& suballoc = *suballocItem;
        json.BeginObject(true);
        json.WriteString(L"Offset");
        json.WriteNumber(suballoc.offset);
        if(suballoc.type == SUBALLOCATION_TYPE_FREE)
        {
            json.WriteString(L"Type");
            json.WriteString(L"FREE");
            json.WriteString(L"Size");
            json.WriteNumber(suballoc.size);
        }
        else
        {
            const Allocation* const alloc = suballoc.allocation;
            D3D12MA_ASSERT(alloc);
            json.AddAllocationToObject(*alloc);
        }
        json.EndObject();
    }
    json.EndArray();
    json.EndObject();
}

////////////////////////////////////////////////////////////////////////////////
// Private class NormalBlock implementation

NormalBlock::NormalBlock(
    AllocatorPimpl* allocator,
    BlockVector* blockVector,
    D3D12_HEAP_TYPE heapType,
    D3D12_HEAP_FLAGS heapFlags,
    UINT64 size,
    UINT id) :
    MemoryBlock(allocator, heapType, heapFlags, size, id),
    m_BlockVector(blockVector)
{
}

NormalBlock::~NormalBlock()
{
    // THIS IS THE MOST IMPORTANT ASSERT IN THE ENTIRE LIBRARY!
    // Hitting it means you have some memory leak - unreleased Allocation objects.
    D3D12MA_ASSERT(m_pMetadata->IsEmpty() && "Some allocations were not freed before destruction of this memory block!");

    D3D12MA_DELETE(m_Allocator->GetAllocs(), m_pMetadata);
    m_pMetadata = NULL;
}

HRESULT NormalBlock::Init()
{
    HRESULT hr = MemoryBlock::Init();
    if(FAILED(hr))
    {
        return hr;
    }
    
    m_pMetadata = D3D12MA_NEW(m_Allocator->GetAllocs(), BlockMetadata_Generic)(&m_Allocator->GetAllocs());
    m_pMetadata->Init(m_Size);

    return hr;
}

bool NormalBlock::Validate() const
{
    D3D12MA_VALIDATE(GetHeap() &&
        m_pMetadata &&
        m_pMetadata->GetSize() != 0 &&
        m_pMetadata->GetSize() == GetSize());
    return m_pMetadata->Validate();
}

////////////////////////////////////////////////////////////////////////////////
// Private class MemoryBlock definition

MemoryBlock::MemoryBlock(
    AllocatorPimpl* allocator,
    D3D12_HEAP_TYPE heapType,
    D3D12_HEAP_FLAGS heapFlags,
    UINT64 size,
    UINT id) :
    m_Allocator(allocator),
    m_HeapType(heapType),
    m_HeapFlags(heapFlags),
    m_Size(size),
    m_Id(id)
{
}

MemoryBlock::~MemoryBlock()
{
    if(m_Heap)
    {
        m_Heap->Release();
    }
}

HRESULT MemoryBlock::Init()
{
    D3D12MA_ASSERT(m_Heap == NULL && m_Size > 0);

    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.SizeInBytes = m_Size;
    heapDesc.Properties.Type = m_HeapType;
    heapDesc.Alignment = HeapFlagsToAlignment(m_HeapFlags);
    heapDesc.Flags = m_HeapFlags;

    return m_Allocator->GetDevice()->CreateHeap(&heapDesc, __uuidof(*m_Heap), (void**)&m_Heap);
}

////////////////////////////////////////////////////////////////////////////////
// Private class BlockVector implementation

BlockVector::BlockVector(
    AllocatorPimpl* hAllocator,
    D3D12_HEAP_TYPE heapType,
    D3D12_HEAP_FLAGS heapFlags,
    UINT64 preferredBlockSize,
    size_t minBlockCount,
    size_t maxBlockCount,
    bool explicitBlockSize) :
    m_hAllocator(hAllocator),
    m_HeapType(heapType),
    m_HeapFlags(heapFlags),
    m_PreferredBlockSize(preferredBlockSize),
    m_MinBlockCount(minBlockCount),
    m_MaxBlockCount(maxBlockCount),
    m_ExplicitBlockSize(explicitBlockSize),
    m_HasEmptyBlock(false),
    m_Blocks(hAllocator->GetAllocs()),
    m_NextBlockId(0)
{
}

BlockVector::~BlockVector()
{
    for(size_t i = m_Blocks.size(); i--; )
    {
        D3D12MA_DELETE(m_hAllocator->GetAllocs(), m_Blocks[i]);
    }
}

HRESULT BlockVector::CreateMinBlocks()
{
    for(size_t i = 0; i < m_MinBlockCount; ++i)
    {
        HRESULT hr = CreateBlock(m_PreferredBlockSize, NULL);
        if(FAILED(hr))
        {
            return hr;
        }
    }
    return S_OK;
}

HRESULT BlockVector::Allocate(
    UINT64 size,
    UINT64 alignment,
    const ALLOCATION_DESC& createInfo,
    size_t allocationCount,
    Allocation** pAllocations)
{
    size_t allocIndex;
    HRESULT hr = S_OK;

    {
        MutexLockWrite lock(m_Mutex, m_hAllocator->UseMutex());
        for(allocIndex = 0; allocIndex < allocationCount; ++allocIndex)
        {
            hr = AllocatePage(
                size,
                alignment,
                createInfo,
                pAllocations + allocIndex);
            if(FAILED(hr))
            {
                break;
            }
        }
    }

    if(FAILED(hr))
    {
        // Free all already created allocations.
        while(allocIndex--)
        {
            Free(pAllocations[allocIndex]);
        }
        memset(pAllocations, 0, sizeof(Allocation*) * allocationCount);
    }

    return hr;
}

HRESULT BlockVector::AllocatePage(
    UINT64 size,
    UINT64 alignment,
    const ALLOCATION_DESC& createInfo,
    Allocation** pAllocation)
{
    // Early reject: requested allocation size is larger that maximum block size for this block vector.
    if(size + 2 * D3D12MA_DEBUG_MARGIN > m_PreferredBlockSize)
    {
        return E_OUTOFMEMORY;
    }

    const bool canCreateNewBlock =
        ((createInfo.Flags & ALLOCATION_FLAG_NEVER_ALLOCATE) == 0) &&
        (m_Blocks.size() < m_MaxBlockCount);

    if(canCreateNewBlock)
    {
        // 1. Search existing allocations. Try to allocate without making other allocations lost.
        ALLOCATION_FLAGS allocFlagsCopy = createInfo.Flags;

        {
            {
                // Forward order in m_Blocks - prefer blocks with smallest amount of free space.
                for(size_t blockIndex = 0; blockIndex < m_Blocks.size(); ++blockIndex )
                {
                    NormalBlock* const pCurrBlock = m_Blocks[blockIndex];
                    D3D12MA_ASSERT(pCurrBlock);
                    HRESULT hr = AllocateFromBlock(
                        pCurrBlock,
                        size,
                        alignment,
                        allocFlagsCopy,
                        pAllocation);
                    if(SUCCEEDED(hr))
                    {
                        return hr;
                    }
                }
            }
        }

        // 2. Try to create new block.
        if(canCreateNewBlock)
        {
            // Calculate optimal size for new block.
            UINT64 newBlockSize = m_PreferredBlockSize;
            UINT newBlockSizeShift = 0;
            const UINT NEW_BLOCK_SIZE_SHIFT_MAX = 3;

            if(!m_ExplicitBlockSize)
            {
                // Allocate 1/8, 1/4, 1/2 as first blocks.
                const UINT64 maxExistingBlockSize = CalcMaxBlockSize();
                for(UINT i = 0; i < NEW_BLOCK_SIZE_SHIFT_MAX; ++i)
                {
                    const UINT64 smallerNewBlockSize = newBlockSize / 2;
                    if(smallerNewBlockSize > maxExistingBlockSize && smallerNewBlockSize >= size * 2)
                    {
                        newBlockSize = smallerNewBlockSize;
                        ++newBlockSizeShift;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            size_t newBlockIndex = 0;
            HRESULT hr = CreateBlock(newBlockSize, &newBlockIndex);
            // Allocation of this size failed? Try 1/2, 1/4, 1/8 of m_PreferredBlockSize.
            if(!m_ExplicitBlockSize)
            {
                while(FAILED(hr) && newBlockSizeShift < NEW_BLOCK_SIZE_SHIFT_MAX)
                {
                    const UINT64 smallerNewBlockSize = newBlockSize / 2;
                    if(smallerNewBlockSize >= size)
                    {
                        newBlockSize = smallerNewBlockSize;
                        ++newBlockSizeShift;
                        hr = CreateBlock(newBlockSize, &newBlockIndex);
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if(SUCCEEDED(hr))
            {
                NormalBlock* const pBlock = m_Blocks[newBlockIndex];
                D3D12MA_ASSERT(pBlock->m_pMetadata->GetSize() >= size);

                hr = AllocateFromBlock(
                    pBlock,
                    size,
                    alignment,
                    allocFlagsCopy,
                    pAllocation);
                if(SUCCEEDED(hr))
                {
                    return hr;
                }
                else
                {
                    // Allocation from new block failed, possibly due to D3D12MA_DEBUG_MARGIN or alignment.
                    return E_OUTOFMEMORY;
                }
            }
        }
    }

    return E_OUTOFMEMORY;
}

void BlockVector::Free(Allocation* hAllocation)
{
    NormalBlock* pBlockToDelete = NULL;

    // Scope for lock.
    {
        MutexLockWrite lock(m_Mutex, m_hAllocator->UseMutex());

        NormalBlock* pBlock = hAllocation->m_Placed.block;

        pBlock->m_pMetadata->Free(hAllocation);
        D3D12MA_HEAVY_ASSERT(pBlock->Validate());

        // pBlock became empty after this deallocation.
        if(pBlock->m_pMetadata->IsEmpty())
        {
            // Already has empty Allocation. We don't want to have two, so delete this one.
            if(m_HasEmptyBlock && m_Blocks.size() > m_MinBlockCount)
            {
                pBlockToDelete = pBlock;
                Remove(pBlock);
            }
            // We now have first empty block.
            else
            {
                m_HasEmptyBlock = true;
            }
        }
        // pBlock didn't become empty, but we have another empty block - find and free that one.
        // (This is optional, heuristics.)
        else if(m_HasEmptyBlock)
        {
            NormalBlock* pLastBlock = m_Blocks.back();
            if(pLastBlock->m_pMetadata->IsEmpty() && m_Blocks.size() > m_MinBlockCount)
            {
                pBlockToDelete = pLastBlock;
                m_Blocks.pop_back();
                m_HasEmptyBlock = false;
            }
        }

        IncrementallySortBlocks();
    }

    // Destruction of a free Allocation. Deferred until this point, outside of mutex
    // lock, for performance reason.
    if(pBlockToDelete != NULL)
    {
        D3D12MA_DELETE(m_hAllocator->GetAllocs(), pBlockToDelete);
    }
}

UINT64 BlockVector::CalcMaxBlockSize() const
{
    UINT64 result = 0;
    for(size_t i = m_Blocks.size(); i--; )
    {
        result = D3D12MA_MAX(result, m_Blocks[i]->m_pMetadata->GetSize());
        if(result >= m_PreferredBlockSize)
        {
            break;
        }
    }
    return result;
}

void BlockVector::Remove(NormalBlock* pBlock)
{
    for(UINT blockIndex = 0; blockIndex < m_Blocks.size(); ++blockIndex)
    {
        if(m_Blocks[blockIndex] == pBlock)
        {
            m_Blocks.remove(blockIndex);
            return;
        }
    }
    D3D12MA_ASSERT(0);
}

void BlockVector::IncrementallySortBlocks()
{
    // Bubble sort only until first swap.
    for(size_t i = 1; i < m_Blocks.size(); ++i)
    {
        if(m_Blocks[i - 1]->m_pMetadata->GetSumFreeSize() > m_Blocks[i]->m_pMetadata->GetSumFreeSize())
        {
            D3D12MA_SWAP(m_Blocks[i - 1], m_Blocks[i]);
            return;
        }
    }
}

HRESULT BlockVector::AllocateFromBlock(
    NormalBlock* pBlock,
    UINT64 size,
    UINT64 alignment,
    ALLOCATION_FLAGS allocFlags,
    Allocation** pAllocation)
{
    AllocationRequest currRequest = {};
    if(pBlock->m_pMetadata->CreateAllocationRequest(
        size,
        alignment,
        &currRequest))
    {
        // We no longer have an empty Allocation.
        if(pBlock->m_pMetadata->IsEmpty())
        {
            m_HasEmptyBlock = false;
        }

        *pAllocation = D3D12MA_NEW(m_hAllocator->GetAllocs(), Allocation)();
        pBlock->m_pMetadata->Alloc(currRequest, size, *pAllocation);
        (*pAllocation)->InitPlaced(
            m_hAllocator,
            size,
            currRequest.offset,
            alignment,
            pBlock);
        D3D12MA_HEAVY_ASSERT(pBlock->Validate());
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT BlockVector::CreateBlock(UINT64 blockSize, size_t* pNewBlockIndex)
{
    NormalBlock* const pBlock = D3D12MA_NEW(m_hAllocator->GetAllocs(), NormalBlock)(
        m_hAllocator,
        this,
        m_HeapType,
        m_HeapFlags,
        blockSize,
        m_NextBlockId++);
    HRESULT hr = pBlock->Init();
    if(FAILED(hr))
    {
        D3D12MA_DELETE(m_hAllocator->GetAllocs(), pBlock);
        return hr;
    }

    m_Blocks.push_back(pBlock);
    if(pNewBlockIndex != NULL)
    {
        *pNewBlockIndex = m_Blocks.size() - 1;
    }

    return hr;
}

void BlockVector::AddStats(Stats& outStats)
{
    const UINT heapTypeIndex = HeapTypeToIndex(m_HeapType);
    StatInfo* const pStatInfo = &outStats.HeapType[heapTypeIndex];

    MutexLockRead lock(m_Mutex, m_hAllocator->UseMutex());

    for(size_t i = 0; i < m_Blocks.size(); ++i)
    {
        const NormalBlock* const pBlock = m_Blocks[i];
        D3D12MA_ASSERT(pBlock);
        D3D12MA_HEAVY_ASSERT(pBlock->Validate());
        StatInfo blockStatInfo;
        pBlock->m_pMetadata->CalcAllocationStatInfo(blockStatInfo);
        AddStatInfo(outStats.Total, blockStatInfo);
        AddStatInfo(*pStatInfo, blockStatInfo);
    }
}

void BlockVector::WriteBlockInfoToJson(JsonWriter& json)
{
    MutexLockRead lock(m_Mutex, m_hAllocator->UseMutex());

    json.BeginObject();

    for (size_t i = 0, count = m_Blocks.size(); i < count; ++i)
    {
        const NormalBlock* const pBlock = m_Blocks[i];
        D3D12MA_ASSERT(pBlock);
        D3D12MA_HEAVY_ASSERT(pBlock->Validate());
        json.BeginString();
        json.ContinueString(pBlock->GetId());
        json.EndString();

        pBlock->m_pMetadata->WriteAllocationInfoToJson(json);
    }

    json.EndObject();
}

////////////////////////////////////////////////////////////////////////////////
// Private class AllocatorPimpl implementation

AllocatorPimpl::AllocatorPimpl(const ALLOCATION_CALLBACKS& allocationCallbacks, const ALLOCATOR_DESC& desc) :
    m_UseMutex((desc.Flags & ALLOCATOR_FLAG_SINGLETHREADED) == 0),
    m_AlwaysCommitted((desc.Flags & ALLOCATOR_FLAG_ALWAYS_COMMITTED) != 0),
    m_Device(desc.pDevice),
    m_PreferredBlockSize(desc.PreferredBlockSize != 0 ? desc.PreferredBlockSize : D3D12MA_DEFAULT_BLOCK_SIZE),
    m_AllocationCallbacks(allocationCallbacks),
    m_CurrentFrameIndex(0)
    // Below this line don't use allocationCallbacks but m_AllocationCallbacks!!!
{
    // desc.pAllocationCallbacks intentionally ignored here, preprocessed by CreateAllocator.
    ZeroMemory(&m_D3D12Options, sizeof(m_D3D12Options));

    ZeroMemory(m_pCommittedAllocations, sizeof(m_pCommittedAllocations));
    ZeroMemory(m_BlockVectors, sizeof(m_BlockVectors));

    for(UINT heapTypeIndex = 0; heapTypeIndex < HEAP_TYPE_COUNT; ++heapTypeIndex)
    {
        m_pCommittedAllocations[heapTypeIndex] = D3D12MA_NEW(GetAllocs(), AllocationVectorType)(GetAllocs());
    }
}

HRESULT AllocatorPimpl::Init()
{
    HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_D3D12Options, sizeof(m_D3D12Options));
    if(FAILED(hr))
    {
        return hr;
    }

    const UINT defaultPoolCount = CalcDefaultPoolCount();
    for(UINT i = 0; i < defaultPoolCount; ++i)
    {
        D3D12_HEAP_TYPE heapType;
        D3D12_HEAP_FLAGS heapFlags;
        CalcDefaultPoolParams(heapType, heapFlags, i);

        m_BlockVectors[i] = D3D12MA_NEW(GetAllocs(), BlockVector)(
            this, // hAllocator
            heapType, // heapType
            heapFlags, // heapFlags
            m_PreferredBlockSize,
            0, // minBlockCount
            SIZE_MAX, // maxBlockCount
            false); // explicitBlockSize
        // No need to call m_pBlockVectors[i]->CreateMinBlocks here, becase minBlockCount is 0.
    }

    return S_OK;
}

AllocatorPimpl::~AllocatorPimpl()
{
    for(UINT i = DEFAULT_POOL_MAX_COUNT; i--; )
    {
        D3D12MA_DELETE(GetAllocs(), m_BlockVectors[i]);
    }

    for(UINT i = HEAP_TYPE_COUNT; i--; )
    {
        if(m_pCommittedAllocations[i] && !m_pCommittedAllocations[i]->empty())
        {
            D3D12MA_ASSERT(0 && "Unfreed committed allocations found!");
        }

        D3D12MA_DELETE(GetAllocs(), m_pCommittedAllocations[i]);
    }
}

HRESULT AllocatorPimpl::CreateResource(
    const ALLOCATION_DESC* pAllocDesc,
    const D3D12_RESOURCE_DESC* pResourceDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE *pOptimizedClearValue,
    Allocation** ppAllocation,
    REFIID riidResource,
    void** ppvResource)
{
    if(pAllocDesc->HeapType != D3D12_HEAP_TYPE_DEFAULT &&
        pAllocDesc->HeapType != D3D12_HEAP_TYPE_UPLOAD &&
        pAllocDesc->HeapType != D3D12_HEAP_TYPE_READBACK)
    {
        return E_INVALIDARG;
    }

    ALLOCATION_DESC finalAllocDesc = *pAllocDesc;

    if(ppvResource)
    {
        *ppvResource = NULL;
    }

    D3D12_RESOURCE_ALLOCATION_INFO resAllocInfo = m_Device->GetResourceAllocationInfo(0, 1, pResourceDesc);
    resAllocInfo.Alignment = D3D12MA_MAX<UINT64>(resAllocInfo.Alignment, D3D12MA_DEBUG_ALIGNMENT);
    D3D12MA_ASSERT(IsPow2(resAllocInfo.Alignment));
    D3D12MA_ASSERT(resAllocInfo.SizeInBytes > 0);

    const UINT defaultPoolIndex = CalcDefaultPoolIndex(*pAllocDesc, *pResourceDesc);
    BlockVector* blockVector = m_BlockVectors[defaultPoolIndex];
    D3D12MA_ASSERT(blockVector);

    const UINT64 preferredBlockSize = blockVector->GetPreferredBlockSize();
    bool preferCommittedMemory =
        m_AlwaysCommitted ||
        PrefersCommittedAllocation(*pResourceDesc) ||
        // Heuristics: Allocate committed memory if requested size if greater than half of preferred block size.
        resAllocInfo.SizeInBytes > preferredBlockSize / 2;
    if(preferCommittedMemory &&
        (finalAllocDesc.Flags & ALLOCATION_FLAG_NEVER_ALLOCATE) == 0)
    {
        finalAllocDesc.Flags |= ALLOCATION_FLAG_COMMITTED;
    }

    if((finalAllocDesc.Flags & ALLOCATION_FLAG_COMMITTED) != 0)
    {
        return AllocateCommittedResource(
            &finalAllocDesc,
            pResourceDesc,
            resAllocInfo,
            InitialResourceState,
            pOptimizedClearValue,
            ppAllocation,
            riidResource,
            ppvResource);
    }
    else
    {
        HRESULT hr = blockVector->Allocate(
            resAllocInfo.SizeInBytes,
            resAllocInfo.Alignment,
            finalAllocDesc,
            1,
            (Allocation**)ppAllocation);
        if(SUCCEEDED(hr))
        {
            ID3D12Resource* res = NULL;
            hr = m_Device->CreatePlacedResource(
                (*ppAllocation)->m_Placed.block->GetHeap(),
                (*ppAllocation)->GetOffset(),
                pResourceDesc,
                InitialResourceState,
                pOptimizedClearValue,
                riidResource,
                (void**)&res);
            if(SUCCEEDED(hr))
            {
                (*ppAllocation)->SetResource(res, pResourceDesc);
                if(ppvResource != NULL)
                {
                    res->AddRef();
                    *ppvResource = res;
                }
                return hr;
            }
            else
            {
                (*ppAllocation)->Release();
                *ppAllocation = NULL;
                return hr;
            }
        }

        return AllocateCommittedResource(
            &finalAllocDesc,
            pResourceDesc,
            resAllocInfo,
            InitialResourceState,
            pOptimizedClearValue,
            ppAllocation,
            riidResource,
            ppvResource);
    }
}

HRESULT AllocatorPimpl::AllocateMemory(
    const ALLOCATION_DESC* pAllocDesc,
    D3D12_HEAP_FLAGS heapFlags,
    const D3D12_RESOURCE_ALLOCATION_INFO* pAllocInfo,
    Allocation** ppAllocation)
{
    if(pAllocDesc->HeapType != D3D12_HEAP_TYPE_DEFAULT &&
        pAllocDesc->HeapType != D3D12_HEAP_TYPE_UPLOAD &&
        pAllocDesc->HeapType != D3D12_HEAP_TYPE_READBACK)
    {
        return E_INVALIDARG;
    }

    ALLOCATION_DESC finalAllocDesc = *pAllocDesc;

    const UINT defaultPoolIndex = CalcDefaultPoolIndex(*pAllocDesc, heapFlags);
    if(defaultPoolIndex == UINT32_MAX)
    {
        return E_INVALIDARG;
    }
    BlockVector* blockVector = m_BlockVectors[defaultPoolIndex];
    D3D12MA_ASSERT(blockVector);

    const UINT64 preferredBlockSize = blockVector->GetPreferredBlockSize();
    bool preferCommittedMemory =
        m_AlwaysCommitted ||
        // Heuristics: Allocate committed memory if requested size if greater than half of preferred block size.
        pAllocInfo->SizeInBytes > preferredBlockSize / 2;
    if(preferCommittedMemory &&
        (finalAllocDesc.Flags & ALLOCATION_FLAG_NEVER_ALLOCATE) == 0)
    {
        finalAllocDesc.Flags |= ALLOCATION_FLAG_COMMITTED;
    }

    if((finalAllocDesc.Flags & ALLOCATION_FLAG_COMMITTED) != 0)
    {
        return AllocateHeap(&finalAllocDesc, heapFlags, *pAllocInfo, ppAllocation);
    }
    else
    {
        HRESULT hr = blockVector->Allocate(
            pAllocInfo->SizeInBytes,
            pAllocInfo->Alignment,
            finalAllocDesc,
            1,
            (Allocation**)ppAllocation);
        if(SUCCEEDED(hr))
        {
            return hr;
        }

        return AllocateHeap(&finalAllocDesc, heapFlags, *pAllocInfo, ppAllocation);
    }
}

bool AllocatorPimpl::PrefersCommittedAllocation(const D3D12_RESOURCE_DESC& resourceDesc)
{
    // Intentional. It may change in the future.
    return false;
}

HRESULT AllocatorPimpl::AllocateCommittedResource(
    const ALLOCATION_DESC* pAllocDesc,
    const D3D12_RESOURCE_DESC* pResourceDesc,
    const D3D12_RESOURCE_ALLOCATION_INFO& resAllocInfo,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE *pOptimizedClearValue,
    Allocation** ppAllocation,
    REFIID riidResource,
    void** ppvResource)
{
    if((pAllocDesc->Flags & ALLOCATION_FLAG_NEVER_ALLOCATE) != 0)
    {
        return E_OUTOFMEMORY;
    }

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = pAllocDesc->HeapType;
    ID3D12Resource* res = NULL;
    HRESULT hr = m_Device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, pResourceDesc, InitialResourceState,
        pOptimizedClearValue, riidResource, (void**)&res);
    if(SUCCEEDED(hr))
    {
        Allocation* alloc = D3D12MA_NEW(m_AllocationCallbacks, Allocation)();
        alloc->InitCommitted(this, resAllocInfo.SizeInBytes, pAllocDesc->HeapType);
        alloc->SetResource(res, pResourceDesc);

        *ppAllocation = alloc;
        if(ppvResource != NULL)
        {
            res->AddRef();
            *ppvResource = res;
        }

        RegisterCommittedAllocation(*ppAllocation, pAllocDesc->HeapType);
    }
    return hr;
}

HRESULT AllocatorPimpl::AllocateHeap(
    const ALLOCATION_DESC* pAllocDesc,
    D3D12_HEAP_FLAGS heapFlags,
    const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo,
    Allocation** ppAllocation)
{
    *ppAllocation = nullptr;

    if((pAllocDesc->Flags & ALLOCATION_FLAG_NEVER_ALLOCATE) != 0)
    {
        return E_OUTOFMEMORY;
    }

    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.SizeInBytes = allocInfo.SizeInBytes;
    heapDesc.Properties.Type = pAllocDesc->HeapType;
    heapDesc.Alignment = allocInfo.Alignment;
    heapDesc.Flags = heapFlags;

    ID3D12Heap* heap = nullptr;
    HRESULT hr = m_Device->CreateHeap(&heapDesc, __uuidof(*heap), (void**)&heap);
    if(SUCCEEDED(hr))
    {
        (*ppAllocation) = D3D12MA_NEW(m_AllocationCallbacks, Allocation)();
        (*ppAllocation)->InitHeap(this, allocInfo.SizeInBytes, pAllocDesc->HeapType, heap);
        RegisterCommittedAllocation(*ppAllocation, pAllocDesc->HeapType);
    }
    return hr;
}

UINT AllocatorPimpl::CalcDefaultPoolCount() const
{
    if(SupportsResourceHeapTier2())
    {
        return 3;
    }
    else
    {
        return 9;
    }
}

UINT AllocatorPimpl::CalcDefaultPoolIndex(const ALLOCATION_DESC& allocDesc, const D3D12_RESOURCE_DESC& resourceDesc) const
{
    UINT poolIndex = UINT_MAX;
    switch(allocDesc.HeapType)
    {
    case D3D12_HEAP_TYPE_DEFAULT:  poolIndex = 0; break;
    case D3D12_HEAP_TYPE_UPLOAD:   poolIndex = 1; break;
    case D3D12_HEAP_TYPE_READBACK: poolIndex = 2; break;
    default: D3D12MA_ASSERT(0);
    }

    if(!SupportsResourceHeapTier2())
    {
        poolIndex *= 3;
        if(resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            ++poolIndex;
            const bool isRenderTargetOrDepthStencil =
                (resourceDesc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0;
            if(isRenderTargetOrDepthStencil)
            {
                ++poolIndex;
            }
        }
    }

    return poolIndex;
}

UINT AllocatorPimpl::CalcDefaultPoolIndex(const ALLOCATION_DESC& allocDesc, const D3D12_HEAP_FLAGS heapFlags) const
{
    UINT poolIndex = UINT_MAX;
    switch(allocDesc.HeapType)
    {
    case D3D12_HEAP_TYPE_DEFAULT:  poolIndex = 0; break;
    case D3D12_HEAP_TYPE_UPLOAD:   poolIndex = 1; break;
    case D3D12_HEAP_TYPE_READBACK: poolIndex = 2; break;
    default: D3D12MA_ASSERT(0);
    }

    if(!SupportsResourceHeapTier2())
    {
        poolIndex *= 3;

        const bool allowBuffers = (heapFlags & D3D12_HEAP_FLAG_DENY_BUFFERS) == 0;
        const bool allowRtDsTextures = (heapFlags & D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES) == 0;
        const bool allowNonRtDsTextures = (heapFlags & D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES) == 0;

        const uint8_t allowedGroupCount = (allowBuffers ? 1 : 0) + (allowRtDsTextures ? 1 : 0) + (allowNonRtDsTextures ? 1 : 0);
        if(allowedGroupCount != 1)
        {
            return UINT32_MAX;
        }

        if(!allowBuffers)
        {
            ++poolIndex;
            if(allowRtDsTextures)
            {
                ++poolIndex;
            }
        }
    }

    return poolIndex;
}

void AllocatorPimpl::CalcDefaultPoolParams(D3D12_HEAP_TYPE& outHeapType, D3D12_HEAP_FLAGS& outHeapFlags, UINT index) const
{
    outHeapType = D3D12_HEAP_TYPE_DEFAULT;
    outHeapFlags = D3D12_HEAP_FLAG_NONE;

    if(!SupportsResourceHeapTier2())
    {
        switch(index % 3)
        {
        case 0:
            outHeapFlags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
            break;
        case 1:
            outHeapFlags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
            break;
        case 2:
            outHeapFlags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
            break;
        }

        index /= 3;
    }

    switch(index)
    {
    case 0:
        outHeapType = D3D12_HEAP_TYPE_DEFAULT;
        break;
    case 1:
        outHeapType = D3D12_HEAP_TYPE_UPLOAD;
        break;
    case 2:
        outHeapType = D3D12_HEAP_TYPE_READBACK;
        break;
    default:
        D3D12MA_ASSERT(0);
    }
}

void AllocatorPimpl::RegisterCommittedAllocation(Allocation* alloc, D3D12_HEAP_TYPE heapType)
{
    const UINT heapTypeIndex = HeapTypeToIndex(heapType);

    MutexLockWrite lock(m_CommittedAllocationsMutex[heapTypeIndex], m_UseMutex);
    AllocationVectorType* const committedAllocations = m_pCommittedAllocations[heapTypeIndex];
    D3D12MA_ASSERT(committedAllocations);
    committedAllocations->InsertSorted(alloc, PointerLess());
}

void AllocatorPimpl::UnregisterCommittedAllocation(Allocation* alloc, D3D12_HEAP_TYPE heapType)
{
    const UINT heapTypeIndex = HeapTypeToIndex(heapType);
    
    MutexLockWrite lock(m_CommittedAllocationsMutex[heapTypeIndex], m_UseMutex);
    AllocationVectorType* const committedAllocations = m_pCommittedAllocations[heapTypeIndex];
    D3D12MA_ASSERT(committedAllocations);
    bool success = committedAllocations->RemoveSorted(alloc, PointerLess());
    D3D12MA_ASSERT(success);
}

void AllocatorPimpl::FreeCommittedMemory(Allocation* allocation)
{
    D3D12MA_ASSERT(allocation && allocation->m_Type == Allocation::TYPE_COMMITTED);
    UnregisterCommittedAllocation(allocation, allocation->m_Committed.heapType);
}

void AllocatorPimpl::FreePlacedMemory(Allocation* allocation)
{
    D3D12MA_ASSERT(allocation && allocation->m_Type == Allocation::TYPE_PLACED);

    NormalBlock* const block = allocation->m_Placed.block;
    D3D12MA_ASSERT(block);
    BlockVector* const blockVector = block->GetBlockVector();
    D3D12MA_ASSERT(blockVector);
    blockVector->Free(allocation);
}

void AllocatorPimpl::FreeHeapMemory(Allocation* allocation)
{
    D3D12MA_ASSERT(allocation && allocation->m_Type == Allocation::TYPE_HEAP);
    UnregisterCommittedAllocation(allocation, allocation->m_Heap.heapType);
    allocation->m_Heap.heap->Release();
}

void AllocatorPimpl::SetCurrentFrameIndex(UINT frameIndex)
{
    m_CurrentFrameIndex.store(frameIndex);
}

void AllocatorPimpl::CalculateStats(Stats& outStats)
{
    // Init stats
    memset(&outStats, 0, sizeof(outStats));
    outStats.Total.AllocationSizeMin = UINT64_MAX;
    outStats.Total.UnusedRangeSizeMin = UINT64_MAX;
    for(size_t i = 0; i < HEAP_TYPE_COUNT; i++)
    {
        outStats.HeapType[i].AllocationSizeMin = UINT64_MAX;
        outStats.HeapType[i].UnusedRangeSizeMin = UINT64_MAX;
    }

    // Process deafult pools.
    for(size_t i = 0; i < HEAP_TYPE_COUNT; ++i)
    {
        BlockVector* const pBlockVector = m_BlockVectors[i];
        D3D12MA_ASSERT(pBlockVector);
        pBlockVector->AddStats(outStats);
    }

    // Process committed allocations.
    for(size_t i = 0; i < HEAP_TYPE_COUNT; ++i)
    {
        StatInfo& heapStatInfo = outStats.HeapType[i];
        MutexLockRead lock(m_CommittedAllocationsMutex[i], m_UseMutex);
        const AllocationVectorType* const allocationVector = m_pCommittedAllocations[i];
        D3D12MA_ASSERT(allocationVector);
        for(size_t j = 0, count = allocationVector->size(); j < count; ++j)
        {
            UINT64 size = (*allocationVector)[j]->GetSize();
            StatInfo statInfo = {};
            statInfo.BlockCount = 1;
            statInfo.AllocationCount = 1;
            statInfo.UnusedRangeCount = 0;
            statInfo.UsedBytes = size;
            statInfo.UnusedBytes = 0;
            statInfo.AllocationSizeMin = size;
            statInfo.AllocationSizeMax = size;
            statInfo.UnusedRangeSizeMin = 0;
            statInfo.UnusedRangeSizeMax = 0;
            AddStatInfo(outStats.Total, statInfo);
            AddStatInfo(heapStatInfo, statInfo);
        }
    }

    // Post process
    PostProcessStatInfo(outStats.Total);
    for(size_t i = 0; i < HEAP_TYPE_COUNT; ++i)
        PostProcessStatInfo(outStats.HeapType[i]);
}

static void AddStatInfoToJson(JsonWriter& json, const StatInfo& statInfo)
{
    json.BeginObject();
    json.WriteString(L"Blocks");
    json.WriteNumber(statInfo.BlockCount);
    json.WriteString(L"Allocations");
    json.WriteNumber(statInfo.AllocationCount);
    json.WriteString(L"UnusedRanges");
    json.WriteNumber(statInfo.UnusedRangeCount);
    json.WriteString(L"UsedBytes");
    json.WriteNumber(statInfo.UsedBytes);
    json.WriteString(L"UnusedBytes");
    json.WriteNumber(statInfo.UnusedBytes);

    json.WriteString(L"AllocationSize");
    json.BeginObject(true);
    json.WriteString(L"Min");
    json.WriteNumber(statInfo.AllocationSizeMin);
    json.WriteString(L"Avg");
    json.WriteNumber(statInfo.AllocationSizeAvg);
    json.WriteString(L"Max");
    json.WriteNumber(statInfo.AllocationSizeMax);
    json.EndObject();

    json.WriteString(L"UnusedRangeSize");
    json.BeginObject(true);
    json.WriteString(L"Min");
    json.WriteNumber(statInfo.UnusedRangeSizeMin);
    json.WriteString(L"Avg");
    json.WriteNumber(statInfo.UnusedRangeSizeAvg);
    json.WriteString(L"Max");
    json.WriteNumber(statInfo.UnusedRangeSizeMax);
    json.EndObject();

    json.EndObject();
}

void AllocatorPimpl::BuildStatsString(WCHAR** ppStatsString, BOOL DetailedMap)
{
    StringBuilder sb(GetAllocs());
    {
        JsonWriter json(GetAllocs(), sb);

        Stats stats;
        CalculateStats(stats);

        json.BeginObject();
        json.WriteString(L"Total");
        AddStatInfoToJson(json, stats.Total);
        for (size_t heapType = 0; heapType < HEAP_TYPE_COUNT; ++heapType)
        {
            json.WriteString(HeapTypeNames[heapType]);
            AddStatInfoToJson(json, stats.HeapType[heapType]);
        }

        if (DetailedMap)
        {
            json.WriteString(L"DetailedMap");
            json.BeginObject();

            json.WriteString(L"DefaultPools");
            json.BeginObject();

            D3D12MA_ASSERT(SupportsResourceHeapTier2());
            if (SupportsResourceHeapTier2())
            {
                for (size_t heapType = 0; heapType < HEAP_TYPE_COUNT; ++heapType)
                {
                    json.WriteString(HeapTypeNames[heapType]);
                    json.BeginObject();

                    json.WriteString(L"Blocks");

                    BlockVector* blockVector = m_BlockVectors[heapType];
                    D3D12MA_ASSERT(blockVector);
                    blockVector->WriteBlockInfoToJson(json);

                    json.EndObject(); // heap name
                }
            }
            else
            {
                for (size_t heapType = 0; heapType < HEAP_TYPE_COUNT; ++heapType)
                {
                    for (size_t heapSubType = 0; heapSubType < 3; ++heapSubType)
                    {
                        static const WCHAR* const heapSubTypeName[] = {
                            L" + buffer",
                            L" + texture",
                            L" + texture RT or DS",
                        };
                        json.BeginString();
                        json.ContinueString(HeapTypeNames[heapType]);
                        json.ContinueString(heapSubTypeName[heapSubType]);
                        json.EndString();
                        json.BeginObject();

                        json.WriteString(L"Blocks");

                        BlockVector* blockVector = m_BlockVectors[heapType * 3 + heapSubType];
                        D3D12MA_ASSERT(blockVector);
                        blockVector->WriteBlockInfoToJson(json);

                        json.EndObject(); // heap name
                    }
                }
            }

            json.EndObject(); // DefaultPools

            json.WriteString(L"CommittedAllocations");
            json.BeginObject();

            for (size_t heapType = 0; heapType < HEAP_TYPE_COUNT; ++heapType)
            {
                json.WriteString(HeapTypeNames[heapType]);
                MutexLockRead lock(m_CommittedAllocationsMutex[heapType], m_UseMutex);

                json.BeginArray();
                const AllocationVectorType* const allocationVector = m_pCommittedAllocations[heapType];
                D3D12MA_ASSERT(allocationVector);
                for (size_t i = 0, count = allocationVector->size(); i < count; ++i)
                {
                    Allocation* alloc = (*allocationVector)[i];
                    D3D12MA_ASSERT(alloc);

                    json.BeginObject(true);
                    json.AddAllocationToObject(*alloc);
                    json.EndObject();
                }
                json.EndArray();
            }

            json.EndObject(); // CommittedAllocations

            json.EndObject(); // DetailedMap
        }
        json.EndObject();
    }

    const size_t length = sb.GetLength();
    WCHAR* result = AllocateArray<WCHAR>(GetAllocs(), length + 1);
    memcpy(result, sb.GetData(), length * sizeof(WCHAR));
    result[length] = L'\0';
    *ppStatsString = result;
}

void AllocatorPimpl::FreeStatsString(WCHAR* pStatsString)
{
    D3D12MA_ASSERT(pStatsString);
    Free(GetAllocs(), pStatsString);
}

////////////////////////////////////////////////////////////////////////////////
// Public class Allocation implementation

void Allocation::Release()
{
    if(this == NULL)
    {
        return;
    }

    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK

    if(m_Resource)
    {
        m_Resource->Release();
    }

    switch(m_Type)
    {
    case TYPE_COMMITTED:
        m_Allocator->FreeCommittedMemory(this);
        break;
    case TYPE_PLACED:
        m_Allocator->FreePlacedMemory(this);
        break;
    case TYPE_HEAP:
        m_Allocator->FreeHeapMemory(this);
        break;
    }

    FreeName();

    D3D12MA_DELETE(m_Allocator->GetAllocs(), this);
}

UINT64 Allocation::GetOffset() const
{
    switch(m_Type)
    {
    case TYPE_COMMITTED:
    case TYPE_HEAP:
        return 0;
    case TYPE_PLACED:
        return m_Placed.offset;
    default:
        D3D12MA_ASSERT(0);
        return 0;
    }
}

ID3D12Heap* Allocation::GetHeap() const
{
    switch(m_Type)
    {
    case TYPE_COMMITTED:
        return NULL;
    case TYPE_PLACED:
        return m_Placed.block->GetHeap();
    case TYPE_HEAP:
        return m_Heap.heap;
    default:
        D3D12MA_ASSERT(0);
        return 0;
    }
}

void Allocation::SetName(LPCWSTR Name)
{
    FreeName();

    if(Name)
    {
        const size_t nameCharCount = wcslen(Name) + 1;
        m_Name = D3D12MA_NEW_ARRAY(m_Allocator->GetAllocs(), WCHAR, nameCharCount);
        memcpy(m_Name, Name, nameCharCount * sizeof(WCHAR));
    }
}

Allocation::Allocation()
{
    // Must be empty because Allocation objects will be allocated out of PoolAllocator
    // and may not call constructor and destructor at the right time.
    // Use Init* methods instead.
}

Allocation::~Allocation()
{
    // Must be empty because Allocation objects will be allocated out of PoolAllocator
    // and may not call constructor and destructor at the right time.
    // Use Release method instead.
}

void Allocation::InitCommitted(AllocatorPimpl* allocator, UINT64 size, D3D12_HEAP_TYPE heapType)
{
    D3D12MA_ASSERT(allocator);
    m_Allocator = allocator;
    m_Type = TYPE_COMMITTED;
    m_Size = size;
    m_Resource = NULL;
    m_Name = NULL;
    m_Committed.heapType = heapType;
    m_CreationFrameIndex = allocator->GetCurrentFrameIndex();
    m_ResourceDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    m_TextureLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
}

void Allocation::InitPlaced(AllocatorPimpl* allocator, UINT64 size, UINT64 offset, UINT64 alignment, NormalBlock* block)
{
    D3D12MA_ASSERT(allocator);
    m_Allocator = allocator;
    m_Type = TYPE_PLACED;
    m_Size = size;
    m_Resource = NULL;
    m_Name = NULL;
    m_Placed.offset = offset;
    m_Placed.block = block;
    m_CreationFrameIndex = allocator->GetCurrentFrameIndex();
    m_ResourceDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    m_TextureLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
}

void Allocation::InitHeap(AllocatorPimpl* allocator, UINT64 size, D3D12_HEAP_TYPE heapType, ID3D12Heap* heap)
{
    D3D12MA_ASSERT(allocator);
    m_Allocator = allocator;
    m_Type = TYPE_HEAP;
    m_Size = size;
    m_Resource = NULL;
    m_Name = NULL;
    m_Heap.heapType = heapType;
    m_Heap.heap = heap;
    m_CreationFrameIndex = allocator->GetCurrentFrameIndex();
    m_ResourceDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    m_TextureLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
}

void Allocation::SetResource(ID3D12Resource* resource, const D3D12_RESOURCE_DESC* pResourceDesc)
{
    D3D12MA_ASSERT(m_Resource == NULL);
    D3D12MA_ASSERT(pResourceDesc);
    m_Resource = resource;
    m_ResourceDimension = pResourceDesc->Dimension;
    m_ResourceFlags = pResourceDesc->Flags;
    m_TextureLayout = pResourceDesc->Layout;
}

void Allocation::FreeName()
{
    if(m_Name)
    {
        const size_t nameCharCount = wcslen(m_Name) + 1;
        D3D12MA_DELETE_ARRAY(m_Allocator->GetAllocs(), m_Name, nameCharCount);
        m_Name = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Public class Allocator implementation

Allocator::Allocator(const ALLOCATION_CALLBACKS& allocationCallbacks, const ALLOCATOR_DESC& desc) :
    m_Pimpl(D3D12MA_NEW(allocationCallbacks, AllocatorPimpl)(allocationCallbacks, desc))
{
}

Allocator::~Allocator()
{
    D3D12MA_DELETE(m_Pimpl->GetAllocs(), m_Pimpl);
}

void Allocator::Release()
{
    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK

    // Copy is needed because otherwise we would call destructor and invalidate the structure with callbacks before using it to free memory.
    const ALLOCATION_CALLBACKS allocationCallbacksCopy = m_Pimpl->GetAllocs();
    D3D12MA_DELETE(allocationCallbacksCopy, this);
}



const D3D12_FEATURE_DATA_D3D12_OPTIONS& Allocator::GetD3D12Options() const
{
    return m_Pimpl->GetD3D12Options();
}

HRESULT Allocator::CreateResource(
    const ALLOCATION_DESC* pAllocDesc,
    const D3D12_RESOURCE_DESC* pResourceDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE *pOptimizedClearValue,
    Allocation** ppAllocation,
    REFIID riidResource,
    void** ppvResource)
{
    if(!pAllocDesc || !pResourceDesc || !ppAllocation || riidResource == IID_NULL)
    {
        D3D12MA_ASSERT(0 && "Invalid arguments passed to Allocator::CreateResource.");
        return E_INVALIDARG;
    }
    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
    return m_Pimpl->CreateResource(pAllocDesc, pResourceDesc, InitialResourceState, pOptimizedClearValue, ppAllocation, riidResource, ppvResource);
}

HRESULT Allocator::AllocateMemory(
    const ALLOCATION_DESC* pAllocDesc,
    D3D12_HEAP_FLAGS heapFlags,
    const D3D12_RESOURCE_ALLOCATION_INFO* pAllocInfo,
    Allocation** ppAllocation)
{
    if(!pAllocDesc ||
        !pAllocInfo ||
        !ppAllocation ||
        !(pAllocInfo->Alignment == 0 ||
            pAllocInfo->Alignment == D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT ||
            pAllocInfo->Alignment == D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT) ||
        pAllocInfo->SizeInBytes == 0 ||
        pAllocInfo->SizeInBytes % (64ull * 1024) != 0 ||
        (heapFlags & ~(D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES)) != 0)
    {
        D3D12MA_ASSERT(0 && "Invalid arguments passed to Allocator::AllocateMemory.");
        return E_INVALIDARG;
    }
    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
    return m_Pimpl->AllocateMemory(pAllocDesc, heapFlags, pAllocInfo, ppAllocation);
}

void Allocator::SetCurrentFrameIndex(UINT frameIndex)
{
    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
    m_Pimpl->SetCurrentFrameIndex(frameIndex);
}

void Allocator::CalculateStats(Stats* pStats)
{
    D3D12MA_ASSERT(pStats);
    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
    m_Pimpl->CalculateStats(*pStats);
}

void Allocator::BuildStatsString(WCHAR** ppStatsString, BOOL DetailedMap)
{
    D3D12MA_ASSERT(ppStatsString);
    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
    m_Pimpl->BuildStatsString(ppStatsString, DetailedMap);
}

void Allocator::FreeStatsString(WCHAR* pStatsString)
{
    if (pStatsString != NULL)
    {
        D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK
        m_Pimpl->FreeStatsString(pStatsString);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Public global functions

HRESULT CreateAllocator(const ALLOCATOR_DESC* pDesc, Allocator** ppAllocator)
{
    if(!pDesc || !ppAllocator || !pDesc->pDevice ||
        !(pDesc->PreferredBlockSize == 0 || (pDesc->PreferredBlockSize >= 16 && pDesc->PreferredBlockSize < 0x10000000000ull)))
    {
        D3D12MA_ASSERT(0 && "Invalid arguments passed to CreateAllocator.");
        return E_INVALIDARG;
    }

    D3D12MA_DEBUG_GLOBAL_MUTEX_LOCK

    ALLOCATION_CALLBACKS allocationCallbacks;
    SetupAllocationCallbacks(allocationCallbacks, *pDesc);

    *ppAllocator = D3D12MA_NEW(allocationCallbacks, Allocator)(allocationCallbacks, *pDesc);
    HRESULT hr = (*ppAllocator)->m_Pimpl->Init();
    if(FAILED(hr))
    {
        D3D12MA_DELETE(allocationCallbacks, *ppAllocator);
        *ppAllocator = NULL;
    }
    return hr;
}

} // namespace D3D12MA

#endif // _WIN32
