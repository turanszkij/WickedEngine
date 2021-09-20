#ifndef FACT_CPP_X3DAUDIO_H
#define FACT_CPP_X3DAUDIO_H

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
#define F3DAUDIOCPP_API  extern "C" DLLEXPORT void __cdecl
#else
#define F3DAUDIOCPP_API  extern "C" DLLIMPORT void __cdecl
#endif

#ifndef X3DAUDIO_VERSION
#define X3DAUDIO_VERSION 7
#endif

#include <F3DAudio.h>

typedef F3DAUDIO_HANDLE X3DAUDIO_HANDLE;
typedef F3DAUDIO_LISTENER X3DAUDIO_LISTENER;
typedef F3DAUDIO_EMITTER X3DAUDIO_EMITTER;
typedef F3DAUDIO_DSP_SETTINGS X3DAUDIO_DSP_SETTINGS;

F3DAUDIOCPP_API X3DAudioInitialize(
	uint32_t SpeakerChannelMask, 
	float SpeedOfSound, 
	X3DAUDIO_HANDLE Instance);

F3DAUDIOCPP_API X3DAudioCalculate(
	const X3DAUDIO_HANDLE Instance, 
	const X3DAUDIO_LISTENER* pListener, 
	const X3DAUDIO_EMITTER* pEmitter, 
	uint32_t Flags, 
	X3DAUDIO_DSP_SETTINGS* pDSPSettings);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // FACT_CPP_X3DAUDIO_H