#ifndef FAUDIO_CPP_XAPOFX_H
#define FAUDIO_CPP_XAPOFX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined _WIN32 || defined __CYGWIN__
  #define DLLIMPORT __declspec(dllimport)
  #define DLLEXPORT __declspec(dllexport)
#else
  #if __GNUC__ >= 4
    #define DLLIMPORT __attribute__((visibility ("default")))
    #define DLLEXPORT __attribute__((visibility ("default")))
  #else
    #define DLLIMPORT
    #define DLLEXPORT
  #endif
#endif

#ifdef FAUDIOCPP_EXPORTS
#define FAPOFXCPP_API extern "C" DLLEXPORT HRESULT __cdecl
#else
#define FAPOFXCPP_API extern "C" DLLIMPORT HRESULT __cdecl
#endif

#ifndef XAPOFX_VERSION
#define XAPOFX_VERSION 5
#endif

#include <FAPOFX.h>

#if XAUDIO2_VERSION >=8
FAPOFXCPP_API CreateFX(
	REFCLSID clsid,
	IUnknown **pEffect,
	const void *pInitData,
	UINT32 InitDataByteSize
);
#else
FAPOFXCPP_API CreateFX(REFCLSID clsid, IUnknown **pEffect);
#endif // XAUDIO2_VERSION >=8

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FAUDIO_CPP_XAPOFX_H */
