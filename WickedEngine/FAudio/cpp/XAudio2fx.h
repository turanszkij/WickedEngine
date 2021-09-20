#ifndef FACT_CPP_XAUDIO2FX_H
#define FACT_CPP_XAUDIO2FX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#ifndef XAUDIO2_VERSION
#define XAUDIO2_VERSION 7
#endif

#ifdef FAUDIOCPP_EXPORTS
#define FAUDIOCPP_API  uint32_t __stdcall
#else
#define FAUDIOCPP_API __declspec(dllimport) uint32_t __stdcall
#endif

#if XAUDIO2_VERSION >=8
FAUDIOCPP_API CreateAudioVolumeMeter(class IUnknown** ppApo);
FAUDIOCPP_API CreateAudioReverb(class IUnknown** ppApo);
#endif // XAUDIO2_VERSION >= 8



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // FACT_CPP_XAUDIO2_H
