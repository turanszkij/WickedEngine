#include "xaudio2.h"
#include "XAPOBase.h"
#include "XAPOFX.h"

class XAPOFXWrapper : public CXAPOParametersBase
{
public:
	XAPOFXWrapper(void *object) :
		fapo_object(object),
		CXAPOParametersBase(reinterpret_cast<FAPOBase*>(object))
	{
	}

	COM_METHOD(void) Process(
		UINT32 InputProcessParameterCount,
		const XAPO_PROCESS_BUFFER_PARAMETERS *pInputProcessParameters,
		UINT32 OutputProcessParameterCount,
		XAPO_PROCESS_BUFFER_PARAMETERS *pOutputProcessParameters,
		BOOL IsEnabled
	) {
		reinterpret_cast<FAPO*>(fapo_object)->Process(
			fapo_object,
			InputProcessParameterCount,
			pInputProcessParameters,
			OutputProcessParameterCount,
			pOutputProcessParameters,
			IsEnabled
		);
	}

private:
	void *fapo_object;
};

void* CDECL XAPOFX_INTERNAL_Malloc(size_t size)
{
	return CoTaskMemAlloc(size);
}
void CDECL XAPOFX_INTERNAL_Free(void* ptr)
{
	CoTaskMemFree(ptr);
}
void* CDECL XAPOFX_INTERNAL_Realloc(void* ptr, size_t size)
{
	return CoTaskMemRealloc(ptr, size);
}

void* CreateFXInternal(
	REFCLSID clsid,
	const void *pInitData,
	uint32_t InitDataByteSize
) {
	FAPO *fapo_object = NULL;
	FAPOFX_CreateFXWithCustomAllocatorEXT(
		&clsid,
		&fapo_object,
		pInitData,
		InitDataByteSize,
		XAPOFX_INTERNAL_Malloc,
		XAPOFX_INTERNAL_Free,
		XAPOFX_INTERNAL_Realloc
	);
	return new XAPOFXWrapper(fapo_object);
}

#if XAUDIO2_VERSION >=8
FAPOFXCPP_API CreateFX(
	REFCLSID clsid,
	IUnknown **pEffect,
	const void *pInitData,
	UINT32 InitDataByteSize
) {
	*pEffect = reinterpret_cast<IUnknown*>(CreateFXInternal(
		clsid,
		pInitData,
		InitDataByteSize
	));
	return S_OK;
}
#else
FAPOFXCPP_API CreateFX(REFCLSID clsid, IUnknown **pEffect)
{
	*pEffect = reinterpret_cast<IUnknown*>(CreateFXInternal(
		clsid,
		NULL,
		0
	));
	return S_OK;
}
#endif // XAUDIO2_VERSION >=8
