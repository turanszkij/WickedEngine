#ifndef FACT_CPP_FAUDIO_COM_H
#define FACT_CPP_FAUDIO_COM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <FAudio.h>

// CoTaskMem functionality, needed for XAudio2 compat
#if defined _WIN32 || defined __CYGWIN__
  #define DLLIMPORT __declspec(dllimport)
#else
  #if __GNUC__ >= 4
    #define DLLIMPORT __attribute__((visibility ("default")))
  #else
    #define DLLIMPORT
  #endif
#endif
extern "C" DLLIMPORT void * __stdcall CoTaskMemAlloc(size_t cb);
extern "C" DLLIMPORT void __stdcall CoTaskMemFree(void* ptr);
extern "C" DLLIMPORT void * __stdcall CoTaskMemRealloc(void* ptr, size_t cb);

#define CDECL __cdecl

// common windows types
#ifndef FAUDIO_USE_STD_TYPES

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t HRESULT;
typedef uint32_t UINT32;
typedef long LONG;
typedef unsigned long ULONG;
typedef wchar_t WHCAR;
typedef const WHCAR *LPCWSTR;
typedef int BOOL;
typedef unsigned long DWORD;
typedef void *LPVOID;
typedef float FLOAT32;

#endif

// HRESULT stuff
#define S_OK 0
#define S_FALSE 1
#define E_NOTIMPL 80004001
#define E_NOINTERFACE 80004002
#define E_OUTOFMEMORY 0x8007000E
#define CLASS_E_NOAGGREGATION 0x80040110
#define CLASS_E_CLASSNOTAVAILABLE 0x80040111

// GUID, IID stuff
typedef FAudioGUID GUID;
typedef GUID IID;
#define REFIID const IID &
#define REFCLSID const IID &
bool guid_equals(REFIID a, REFIID b);

extern const IID IID_IUnknown;
extern const IID IID_IClassFactory;
extern const IID IID_IXAudio2;

extern const IID IID_IXAPO;
extern const IID IID_IXAPOParameters;

extern const IID CLSID_XAudio2_0;
extern const IID CLSID_XAudio2_1;
extern const IID CLSID_XAudio2_2;
extern const IID CLSID_XAudio2_3;
extern const IID CLSID_XAudio2_4;
extern const IID CLSID_XAudio2_5;
extern const IID CLSID_XAudio2_6;
extern const IID CLSID_XAudio2_7;
extern const IID *CLSID_XAudio2[];

extern const IID CLSID_AudioVolumeMeter_0;
extern const IID CLSID_AudioVolumeMeter_1;
extern const IID CLSID_AudioVolumeMeter_2;
extern const IID CLSID_AudioVolumeMeter_3;
extern const IID CLSID_AudioVolumeMeter_4;
extern const IID CLSID_AudioVolumeMeter_5;
extern const IID CLSID_AudioVolumeMeter_6;
extern const IID CLSID_AudioVolumeMeter_7;
extern const IID *CLSID_AudioVolumeMeter[];

extern const IID CLSID_AudioReverb_0;
extern const IID CLSID_AudioReverb_1;
extern const IID CLSID_AudioReverb_2;
extern const IID CLSID_AudioReverb_3;
extern const IID CLSID_AudioReverb_4;
extern const IID CLSID_AudioReverb_5;
extern const IID CLSID_AudioReverb_6;
extern const IID CLSID_AudioReverb_7;
extern const IID *CLSID_AudioReverb[];

extern const IID IID_IXACT3Engine3_0;
extern const IID IID_IXACT3Engine3_1;
extern const IID IID_IXACT3Engine3_2;
extern const IID IID_IXACT3Engine3_3;
extern const IID IID_IXACT3Engine3_4;
extern const IID IID_IXACT3Engine3_5;
extern const IID IID_IXACT3Engine3_6;
extern const IID IID_IXACT3Engine3_7;

#define JOIN_IID_VERSION(x, y) JOIN_IID_VERSION2(x, y)
#define JOIN_IID_VERSION2(x, y) x ## y
#define IID_IXACT3Engine JOIN_IID_VERSION(IID_IXACT3Engine3_, XACT3_VERSION)

extern const IID CLSID_XACTEngine3_0;
extern const IID CLSID_XACTEngine3_1;
extern const IID CLSID_XACTEngine3_2;
extern const IID CLSID_XACTEngine3_3;
extern const IID CLSID_XACTEngine3_4;
extern const IID CLSID_XACTEngine3_5;
extern const IID CLSID_XACTEngine3_6;
extern const IID CLSID_XACTEngine3_7;
extern const IID *CLSID_XACTEngine[];

// quality of life macro's
#define COM_METHOD(rtype)		virtual rtype __stdcall 

// common interfaces
class IUnknown {
public:
	COM_METHOD(HRESULT) QueryInterface(REFIID riid, void** ppvInterface) = 0;
	COM_METHOD(ULONG) AddRef() = 0;
	COM_METHOD(ULONG) Release() = 0;
};

class IClassFactory : public IUnknown {
public:
	COM_METHOD(HRESULT) CreateInstance(
		IUnknown *pUnkOuter,
		REFIID riid,
		void **ppvObject) = 0;

	COM_METHOD(HRESULT) LockServer(BOOL fLock) = 0;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // FACT_CPP_FAUDIO_COM_H
