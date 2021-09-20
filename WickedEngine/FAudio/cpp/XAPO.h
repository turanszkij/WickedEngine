#ifndef FACT_CPP_XAPO_H
#define FACT_CPP_XAPO_H

#include "com_utils.h"
#include <FAPO.h>

typedef FAPORegistrationProperties XAPO_REGISTRATION_PROPERTIES;
typedef FAPOLockForProcessBufferParameters XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS;
typedef FAPOProcessBufferParameters XAPO_PROCESS_BUFFER_PARAMETERS;
typedef FAudioWaveFormatEx WAVEFORMATEX;

class IXAPO : public IUnknown 
{
public:
	COM_METHOD(HRESULT) GetRegistrationProperties (XAPO_REGISTRATION_PROPERTIES** ppRegistrationProperties) = 0;
	COM_METHOD(HRESULT) IsInputFormatSupported(
		const WAVEFORMATEX* pOutputFormat, 
		const WAVEFORMATEX* pRequestedInputFormat, 
		WAVEFORMATEX** ppSupportedInputFormat) = 0;
	COM_METHOD(HRESULT) IsOutputFormatSupported(
		const WAVEFORMATEX* pInputFormat, 
		const WAVEFORMATEX* pRequestedOutputFormat, 
		WAVEFORMATEX** ppSupportedOutputFormat) = 0;
	COM_METHOD(HRESULT) Initialize(const void* pData, UINT32 DataByteSize) = 0;
	COM_METHOD(void) Reset() = 0;
	COM_METHOD(HRESULT) LockForProcess(
		UINT32 InputLockedParameterCount, 
		const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters, 
		UINT32 OutputLockedParameterCount, 
		const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pOutputLockedParameters) = 0;
	COM_METHOD(void) UnlockForProcess() = 0;
	COM_METHOD(void) Process(
		UINT32 InputProcessParameterCount, 
		const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters, 
		UINT32 OutputProcessParameterCount, 
		XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters, 
		BOOL IsEnabled) = 0;
	COM_METHOD(UINT32) CalcInputFrames(UINT32 OutputFrameCount) = 0;
	COM_METHOD(UINT32) CalcOutputFrames(UINT32 InputFrameCount) = 0;
};

class IXAPOParameters : public IUnknown 
{
public:
	COM_METHOD(void) SetParameters(const void* pParameters, UINT32 ParameterByteSize) = 0;
	COM_METHOD(void) GetParameters(void* pParameters, UINT32 ParameterByteSize) = 0;
};

#endif // FACT_CPP_XAPO_H