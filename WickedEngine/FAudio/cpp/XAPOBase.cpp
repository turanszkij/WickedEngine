#include "XAPOBase.h"
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
//
// CXAPOBase
//

void* CDECL XAPOBase_INTERNAL_Malloc(size_t size)
{
	return CoTaskMemAlloc(size);
}
void CDECL XAPOBase_INTERNAL_Free(void* ptr)
{
	CoTaskMemFree(ptr);
}
void* CDECL XAPOBase_INTERNAL_Realloc(void* ptr, size_t size)
{
	return CoTaskMemRealloc(ptr, size);
}

CXAPOBase::CXAPOBase(FAPOBase *base) 
	: fapo_base(base), 
	  own_fapo_base(false) 
{
}

CXAPOBase::CXAPOBase(const XAPO_REGISTRATION_PROPERTIES* pRegistrationProperties,
	BYTE* pParameterBlocks,
	UINT32 uParameterBlockByteSize,
	BOOL fProducer) 
	: fapo_base(new FAPOBase()),
	own_fapo_base(true) 
{
	CreateFAPOBaseWithCustomAllocatorEXT(
		fapo_base,
		pRegistrationProperties,
		pParameterBlocks,
		uParameterBlockByteSize,
		fProducer,
		XAPOBase_INTERNAL_Malloc,
		XAPOBase_INTERNAL_Free,
		XAPOBase_INTERNAL_Realloc
	);
}

CXAPOBase::~CXAPOBase() 
{
	if (own_fapo_base) 
	{
		delete fapo_base;
	} 
	else if (fapo_base->Destructor) 
	{
		fapo_base->Destructor(fapo_base);
	}
}

HRESULT CXAPOBase::QueryInterface(REFIID riid, void** ppInterface) 
{
	if (guid_equals(riid, IID_IXAPO))
	{
		*ppInterface = static_cast<IXAPO *>(this);
	} 
	else if (guid_equals(riid, IID_IUnknown))
	{
		*ppInterface = static_cast<IUnknown *>(this);
	} 
	else 
	{
		*ppInterface = NULL;
		return E_NOINTERFACE;
	}

	reinterpret_cast<IUnknown *>(*ppInterface)->AddRef();

	return S_OK;
}

ULONG CXAPOBase::AddRef() 
{
	return FAPOBase_AddRef(fapo_base);
}

ULONG CXAPOBase::Release() 
{
	ULONG refcount = FAPOBase_Release(fapo_base);
	if (refcount == 0) 
	{
		delete this;
	}

	return refcount;
}

HRESULT CXAPOBase::GetRegistrationProperties(XAPO_REGISTRATION_PROPERTIES** ppRegistrationProperties) 
{
	assert(fapo_base->base.GetRegistrationProperties != NULL);
	return fapo_base->base.GetRegistrationProperties(fapo_base, ppRegistrationProperties);
}

HRESULT CXAPOBase::IsInputFormatSupported(
	const WAVEFORMATEX* pOutputFormat,
	const WAVEFORMATEX* pRequestedInputFormat,
	WAVEFORMATEX** ppSupportedInputFormat
) {
	assert(fapo_base->base.IsInputFormatSupported != NULL);
	return fapo_base->base.IsInputFormatSupported(
		fapo_base, 
		pOutputFormat, 
		pRequestedInputFormat, 
		ppSupportedInputFormat);
}

HRESULT CXAPOBase::IsOutputFormatSupported(
	const WAVEFORMATEX* pInputFormat,
	const WAVEFORMATEX* pRequestedOutputFormat,
	WAVEFORMATEX** ppSupportedOutputFormat
) {
	assert(fapo_base->base.IsOutputFormatSupported != NULL);
	return fapo_base->base.IsOutputFormatSupported(
		fapo_base, 
		pInputFormat, 
		pRequestedOutputFormat, 
		ppSupportedOutputFormat);
}

HRESULT CXAPOBase::Initialize(const void*pData, UINT32 DataByteSize) 
{
	assert(fapo_base->base.Initialize != NULL);
	return fapo_base->base.Initialize(fapo_base, pData, DataByteSize);
}

void CXAPOBase::Reset() 
{
	assert(fapo_base->base.Reset != NULL);
	fapo_base->base.Reset(fapo_base);
}

HRESULT CXAPOBase::LockForProcess(
	UINT32 InputLockedParameterCount,
	const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters,
	UINT32 OutputLockedParameterCount,
	const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pOutputLockedParameters
) {
	assert(fapo_base->base.LockForProcess != NULL);
	return fapo_base->base.LockForProcess(
		fapo_base, 
		InputLockedParameterCount, 
		pInputLockedParameters, 
		OutputLockedParameterCount, 
		pOutputLockedParameters);
}

void CXAPOBase::UnlockForProcess() 
{
	assert(fapo_base->base.UnlockForProcess != NULL);
	fapo_base->base.UnlockForProcess(fapo_base);
}

UINT32 CXAPOBase::CalcInputFrames(UINT32 OutputFrameCount) 
{
	assert(fapo_base->base.CalcInputFrames != NULL);
	return fapo_base->base.CalcInputFrames(fapo_base, OutputFrameCount);
}

UINT32 CXAPOBase::CalcOutputFrames(UINT32 InputFrameCount) 
{
	assert(fapo_base->base.CalcOutputFrames != NULL);
	return fapo_base->base.CalcOutputFrames(fapo_base, InputFrameCount);
}

// protected functions
HRESULT CXAPOBase::ValidateFormatDefault(WAVEFORMATEX* pFormat, BOOL fOverwrite) 
{
	return FAPOBase_ValidateFormatDefault(fapo_base, pFormat, fOverwrite);
}

HRESULT CXAPOBase::ValidateFormatPair(
	const WAVEFORMATEX* pSupportedFormat,
	WAVEFORMATEX* pRequestedFormat,
	BOOL fOverwrite
) {
	return FAPOBase_ValidateFormatPair(
		fapo_base, 
		pSupportedFormat, 
		pRequestedFormat, 
		fOverwrite);
}

void CXAPOBase::ProcessThru(
	void* pInputBuffer,
	FLOAT32* pOutputBuffer,
	UINT32 FrameCount,
	WORD InputChannelCount,
	WORD OutputChannelCount,
	BOOL MixWithOutput) 
{
	FAPOBase_ProcessThru(
		fapo_base, 
		pInputBuffer, 
		pOutputBuffer, 
		FrameCount, 
		InputChannelCount, 
		OutputChannelCount, 
		MixWithOutput);
}

BOOL CXAPOBase::IsLocked() 
{
	return fapo_base->m_fIsLocked;
}

///////////////////////////////////////////////////////////////////////////////
//
// CXAPOParametersBase
//

CXAPOParametersBase::CXAPOParametersBase(FAPOBase *base)
	: CXAPOBase(base) 
{
}

CXAPOParametersBase::CXAPOParametersBase(
	const XAPO_REGISTRATION_PROPERTIES* pRegistrationProperties,
	BYTE* pParameterBlocks,
	UINT32 uParameterBlockByteSize,
	BOOL fProducer) 
	: CXAPOBase(pRegistrationProperties, pParameterBlocks,
		uParameterBlockByteSize, fProducer)
{
}

CXAPOParametersBase::~CXAPOParametersBase() 
{
}

HRESULT CXAPOParametersBase::QueryInterface(REFIID riid, void** ppInterface) 
{
	if (guid_equals(riid, IID_IXAPOParameters))
	{
		*ppInterface = static_cast<IXAPOParameters *>(this);
		CXAPOBase::AddRef();
		return S_OK;
	} 
	else 
	{
		return CXAPOBase::QueryInterface(riid, ppInterface);
	}
}

ULONG CXAPOParametersBase::AddRef() 
{
	return CXAPOBase::AddRef();
}

ULONG CXAPOParametersBase::Release() 
{
	return CXAPOBase::Release();
}

void CXAPOParametersBase::SetParameters(const void* pParameters, UINT32 ParameterByteSize) 
{
	assert(fapo_base->base.SetParameters);
	fapo_base->base.SetParameters(fapo_base, pParameters, ParameterByteSize);
}

void CXAPOParametersBase::GetParameters(void* pParameters, UINT32 ParameterByteSize) {
	assert(fapo_base->base.GetParameters);
	fapo_base->base.GetParameters(fapo_base, pParameters, ParameterByteSize);
}

void CXAPOParametersBase::OnSetParameters(const void* pParameters, UINT32 ParameterByteSize) 
{
	assert(fapo_base->OnSetParameters);
	fapo_base->OnSetParameters(fapo_base, pParameters, ParameterByteSize);
}

BOOL CXAPOParametersBase::ParametersChanged() 
{
	return FAPOBase_ParametersChanged(fapo_base);
}

BYTE* CXAPOParametersBase::BeginProcess() 
{
	return FAPOBase_BeginProcess(fapo_base);
}

void CXAPOParametersBase::EndProcess() 
{
	FAPOBase_EndProcess(fapo_base);
}
