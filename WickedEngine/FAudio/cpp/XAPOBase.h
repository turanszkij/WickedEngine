#ifndef FACT_CPP_XAPOBASE_H
#define FACT_CPP_XAPOBASE_H

#include "XAPO.h"
#include <FAPOBase.h>

typedef FAPORegistrationProperties XAPO_REGISTRATION_PROPERTIES;

class CXAPOBase : public IXAPO 
{
protected:
	// for CXAPOParametersBase and other derived classes that wrap FAPO structs
	CXAPOBase(FAPOBase *base);

public:
	CXAPOBase(
		const XAPO_REGISTRATION_PROPERTIES* pRegistrationProperties, 
		BYTE* pParameterBlocks = NULL, 
		UINT32 uParameterBlockByteSize = 0, 
		BOOL fProducer = 0);
	virtual ~CXAPOBase();

	COM_METHOD(HRESULT) QueryInterface(REFIID riid, void** ppInterface);
	COM_METHOD(ULONG) AddRef();
	COM_METHOD(ULONG) Release();

	COM_METHOD(HRESULT) GetRegistrationProperties (XAPO_REGISTRATION_PROPERTIES** ppRegistrationProperties);
	COM_METHOD(HRESULT) IsInputFormatSupported (
		const WAVEFORMATEX* pOutputFormat, 
		const WAVEFORMATEX* pRequestedInputFormat, 
		WAVEFORMATEX** ppSupportedInputFormat);
	COM_METHOD(HRESULT) IsOutputFormatSupported (
		const WAVEFORMATEX* pInputFormat, 
		const WAVEFORMATEX* pRequestedOutputFormat, 
		WAVEFORMATEX** ppSupportedOutputFormat);
	COM_METHOD(HRESULT) Initialize(const void*pData, UINT32 DataByteSize);
	COM_METHOD(void) Reset();
	COM_METHOD(HRESULT) LockForProcess (
		UINT32 InputLockedParameterCount, 
		const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters, 
		UINT32 OutputLockedParameterCount, 
		const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pOutputLockedParameters);
	COM_METHOD(void) UnlockForProcess ();
	COM_METHOD(UINT32) CalcInputFrames(UINT32 OutputFrameCount);
	COM_METHOD(UINT32) CalcOutputFrames(UINT32 InputFrameCount);

protected:
	virtual HRESULT ValidateFormatDefault(WAVEFORMATEX* pFormat, BOOL fOverwrite);
	HRESULT ValidateFormatPair(
		const WAVEFORMATEX* pSupportedFormat,
		WAVEFORMATEX* pRequestedFormat,
		BOOL fOverwrite);
	void ProcessThru(
		void* pInputBuffer,
		FLOAT32* pOutputBuffer,
		UINT32 FrameCount,
		WORD InputChannelCount,
		WORD OutputChannelCount,
		BOOL MixWithOutput);

	const XAPO_REGISTRATION_PROPERTIES* GetRegistrationPropertiesInternal();
	BOOL IsLocked();

protected:
	FAPOBase *fapo_base;
	bool	 own_fapo_base;
};

class CXAPOParametersBase : public CXAPOBase, public IXAPOParameters 
{
protected:
	// for derived classes that wrap FAPO structs
	CXAPOParametersBase(FAPOBase *base);
public:
	CXAPOParametersBase(
		const XAPO_REGISTRATION_PROPERTIES* pRegistrationProperties, 
		BYTE* pParameterBlocks, 
		UINT32 uParameterBlockByteSize, 
		BOOL fProducer);
	virtual ~CXAPOParametersBase();

	COM_METHOD(HRESULT) QueryInterface(REFIID riid, void** ppInterface);
	COM_METHOD(ULONG) AddRef();
	COM_METHOD(ULONG) Release();

	COM_METHOD(void) SetParameters (const void* pParameters, UINT32 ParameterByteSize);
	COM_METHOD(void) GetParameters (void* pParameters, UINT32 ParameterByteSize);

	virtual void OnSetParameters(const void*, UINT32);

	BOOL ParametersChanged();
	BYTE* BeginProcess();
	void EndProcess();
};

#endif // FACT_CPP_XAPOBASE_H
