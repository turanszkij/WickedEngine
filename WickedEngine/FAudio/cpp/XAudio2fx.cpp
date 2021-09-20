#include "XAudio2fx.h"
#include "XAPOBase.h"

#include <FAudioFX.h>


class XAudio2VolumeMeter : public CXAPOParametersBase 
{
public:
	XAudio2VolumeMeter(void *object) : fapo_object(object),
									   CXAPOParametersBase(reinterpret_cast<FAPOBase *>(object)) 
	{
	}

	COM_METHOD(void) Process(
		UINT32 InputProcessParameterCount,
		const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters,
		UINT32 OutputProcessParameterCount,
		XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters,
		BOOL IsEnabled
	) {
		reinterpret_cast<FAPO *>(fapo_object)->Process(
			fapo_object,
			InputProcessParameterCount,
			pInputProcessParameters,
			OutputProcessParameterCount,
			pOutputProcessParameters,
			IsEnabled);
	}

private:
	void *fapo_object;
};

class XAudio2Reverb : public CXAPOParametersBase 
{
public:
	XAudio2Reverb(void *object) : fapo_object(object),
								  CXAPOParametersBase(reinterpret_cast<FAPOBase *>(object)) 
	{
	}

	COM_METHOD(void) Process(
		UINT32 InputProcessParameterCount,
		const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters,
		UINT32 OutputProcessParameterCount,
		XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters,
		BOOL IsEnabled
	) {
		reinterpret_cast<FAPO *>(fapo_object)->Process(
			fapo_object,
			InputProcessParameterCount,
			pInputProcessParameters,
			OutputProcessParameterCount,
			pOutputProcessParameters,
			IsEnabled);
	}

private:
	void *fapo_object;
};



///////////////////////////////////////////////////////////////////////////////
//
// Create functions
//

void* CDECL XAudio2FX_INTERNAL_Malloc(size_t size)
{
	return CoTaskMemAlloc(size);
}
void CDECL XAudio2FX_INTERNAL_Free(void* ptr)
{
	CoTaskMemFree(ptr);
}
void* CDECL XAudio2FX_INTERNAL_Realloc(void* ptr, size_t size)
{
	return CoTaskMemRealloc(ptr, size);
}

void *CreateAudioVolumeMeterInternal() 
{
	FAPO *fapo_object = NULL;
	FAudioCreateVolumeMeterWithCustomAllocatorEXT(
		&fapo_object,
		0,
		XAudio2FX_INTERNAL_Malloc,
		XAudio2FX_INTERNAL_Free,
		XAudio2FX_INTERNAL_Realloc
	);
	return new XAudio2VolumeMeter(fapo_object);
}

void *CreateAudioReverbInternal() 
{
	FAPO *fapo_object = NULL;
	FAudioCreateReverbWithCustomAllocatorEXT(
		&fapo_object,
		0,
		XAudio2FX_INTERNAL_Malloc,
		XAudio2FX_INTERNAL_Free,
		XAudio2FX_INTERNAL_Realloc
	);
	return new XAudio2Reverb(fapo_object);
}

#if XAUDIO2_VERSION >=8

extern "C" FAUDIOCPP_API CreateAudioVolumeMeter(class IUnknown** ppApo)
{
	*ppApo = reinterpret_cast<IUnknown *> (CreateAudioVolumeMeterInternal());
	return S_OK;
}

extern "C" FAUDIOCPP_API CreateAudioReverb(class IUnknown** ppApo)
{
	*ppApo = reinterpret_cast<IUnknown *> (CreateAudioReverbInternal());
	return S_OK;
}

#endif // XAUDIO2_VERSION >=8
