#include "com_utils.h"
#include <string.h> /* memcmp */

/* GUIDs */
const IID IID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 00, 00, 00, 00, 00, 00, 0x46}};
const IID IID_IClassFactory = {0x00000001, 0x0000, 0x0000, {0xC0, 00, 00, 00, 00, 00, 00, 0x46}};

const IID IID_IXAudio2 = {0x8bcf1f58, 0x9fe7, 0x4583, {0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb}};
const IID IID_IXAPO = {0xA90BC001, 0xE897, 0xE897, {0x55, 0xE4, 0x9E, 0x47, 0x00, 0x00, 0x00, 0x00}};
const IID IID_IXAPOParameters = {0xA90BC001, 0xE897, 0xE897, {0x55, 0xE4, 0x9E, 0x47, 0x00, 0x00, 0x00, 0x01}};

const IID CLSID_XAudio2_0 = {0xfac23f48, 0x31f5, 0x45a8, {0xb4, 0x9b, 0x52, 0x25, 0xd6, 0x14, 0x01, 0xaa}};
const IID CLSID_XAudio2_1 = {0xe21a7345, 0xeb21, 0x468e, {0xbe, 0x50, 0x80, 0x4d, 0xb9, 0x7c, 0xf7, 0x08}};
const IID CLSID_XAudio2_2 = {0xb802058a, 0x464a, 0x42db, {0xbc, 0x10, 0xb6, 0x50, 0xd6, 0xf2, 0x58, 0x6a}};
const IID CLSID_XAudio2_3 = {0x4c5e637a, 0x16c7, 0x4de3, {0x9c, 0x46, 0x5e, 0xd2, 0x21, 0x81, 0x96, 0x2d}};
const IID CLSID_XAudio2_4 = {0x03219e78, 0x5bc3, 0x44d1, {0xb9, 0x2e, 0xf6, 0x3d, 0x89, 0xcc, 0x65, 0x26}};
const IID CLSID_XAudio2_5 = {0x4c9b6dde, 0x6809, 0x46e6, {0xa2, 0x78, 0x9b, 0x6a, 0x97, 0x58, 0x86, 0x70}};
const IID CLSID_XAudio2_6 = {0x3eda9b49, 0x2085, 0x498b, {0x9b, 0xb2, 0x39, 0xa6, 0x77, 0x84, 0x93, 0xde}};
const IID CLSID_XAudio2_7 = {0x5a508685, 0xa254, 0x4fba, {0x9b, 0x82, 0x9a, 0x24, 0xb0, 0x03, 0x06, 0xaf}};

const IID *CLSID_XAudio2[] = 
{
	&CLSID_XAudio2_0, &CLSID_XAudio2_1,
	&CLSID_XAudio2_2, &CLSID_XAudio2_3,
	&CLSID_XAudio2_4, &CLSID_XAudio2_5,
	&CLSID_XAudio2_6, &CLSID_XAudio2_7
};

const IID CLSID_AudioVolumeMeter_0 = {0xC0C56F46, 0x29B1, 0x44E9, {0x99, 0x39, 0xA3, 0x2C, 0xE8, 0x68, 0x67, 0xE2}};
const IID CLSID_AudioVolumeMeter_1 = {0xc1e3f122, 0xa2ea, 0x442c, {0x85, 0x4f, 0x20, 0xd9, 0x8f, 0x83, 0x57, 0xa1}};
const IID CLSID_AudioVolumeMeter_2 = {0xf5ca7b34, 0x8055, 0x42c0, {0xb8, 0x36, 0x21, 0x61, 0x29, 0xeb, 0x7e, 0x30}};
const IID CLSID_AudioVolumeMeter_3 = {0xe180344b, 0xac83, 0x4483, {0x95, 0x9e, 0x18, 0xa5, 0xc5, 0x6a, 0x5e, 0x19}};
const IID CLSID_AudioVolumeMeter_4 = {0xc7338b95, 0x52b8, 0x4542, {0xaa, 0x79, 0x42, 0xeb, 0x01, 0x6c, 0x8c, 0x1c}};
const IID CLSID_AudioVolumeMeter_5 = {0x2139e6da, 0xc341, 0x4774, {0x9a, 0xc3, 0xb4, 0xe0, 0x26, 0x34, 0x7f, 0x64}};
const IID CLSID_AudioVolumeMeter_6 = {0xe48c5a3f, 0x93ef, 0x43bb, {0xa0, 0x92, 0x2c, 0x7c, 0xeb, 0x94, 0x6f, 0x27}};
const IID CLSID_AudioVolumeMeter_7 = {0xcac1105f, 0x619b, 0x4d04, {0x83, 0x1a, 0x44, 0xe1, 0xcb, 0xf1, 0x2d, 0x57}};

const IID *CLSID_AudioVolumeMeter[] = 
{
	&CLSID_AudioVolumeMeter_0, &CLSID_AudioVolumeMeter_1,
	&CLSID_AudioVolumeMeter_2, &CLSID_AudioVolumeMeter_3,
	&CLSID_AudioVolumeMeter_4, &CLSID_AudioVolumeMeter_5,
	&CLSID_AudioVolumeMeter_6, &CLSID_AudioVolumeMeter_7,
};

const IID CLSID_AudioReverb_0 = {0x6F6EA3A9, 0x2CF5, 0x41CF, {0x91, 0xC1, 0x21, 0x70, 0xB1, 0x54, 0x00, 0x63}};
const IID CLSID_AudioReverb_1 = {0xf4769300, 0xb949, 0x4df9, {0xb3, 0x33, 0x00, 0xd3, 0x39, 0x32, 0xe9, 0xa6}};
const IID CLSID_AudioReverb_2 = {0x629cf0de, 0x3ecc, 0x41e7, {0x99, 0x26, 0xf7, 0xe4, 0x3e, 0xeb, 0xec, 0x51}};
const IID CLSID_AudioReverb_3 = {0x9cab402c, 0x1d37, 0x44b4, {0x88, 0x6d, 0xfa, 0x4f, 0x36, 0x17, 0x0a, 0x4c}};
const IID CLSID_AudioReverb_4 = {0x8bb7778b, 0x645b, 0x4475, {0x9a, 0x73, 0x1d, 0xe3, 0x17, 0x0b, 0xd3, 0xaf}};
const IID CLSID_AudioReverb_5 = {0xd06df0d0, 0x8518, 0x441e, {0x82, 0x2f, 0x54, 0x51, 0xd5, 0xc5, 0x95, 0xb8}};
const IID CLSID_AudioReverb_6 = {0xcecec95a, 0xd894, 0x491a, {0xbe, 0xe3, 0x5e, 0x10, 0x6f, 0xb5, 0x9f, 0x2d}};
const IID CLSID_AudioReverb_7 = {0x6a93130e, 0x1d53, 0x41d1, {0xa9, 0xcf, 0xe7, 0x58, 0x80, 0x0b, 0xb1, 0x79}};

const IID *CLSID_AudioReverb[] = {
	&CLSID_AudioReverb_0, &CLSID_AudioReverb_1,
	&CLSID_AudioReverb_2, &CLSID_AudioReverb_3,
	&CLSID_AudioReverb_4, &CLSID_AudioReverb_5,
	&CLSID_AudioReverb_6, &CLSID_AudioReverb_7,
};

bool guid_equals(REFIID a, REFIID b)
{
	return memcmp(&a, &b, sizeof(IID)) == 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Class Factory: fun COM stuff
//

typedef void *(*FACTORY_FUNC)(void);
void *CreateXAudio2Internal(void);
void *CreateAudioVolumeMeterInternal(void);
void *CreateAudioReverbInternal(void);

template <REFIID fact_iid, FACTORY_FUNC fact_creator> class ClassFactory : public IClassFactory
{
public:
	COM_METHOD(HRESULT) QueryInterface(REFIID riid, void **ppvInterface)
	{
		if (guid_equals(riid, IID_IUnknown) || guid_equals(riid, IID_IClassFactory))
		{
			*ppvInterface = static_cast<IClassFactory *>(this);
		}
		else
		{
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}

		reinterpret_cast<IUnknown *>(*ppvInterface)->AddRef();

		return S_OK;
	}

	COM_METHOD(ULONG) AddRef()
	{
		// FIXME: atomics
		return ++refcount;
	}

	COM_METHOD(ULONG) Release()
	{
		// FIXME: atomics
		long rc = --refcount;

		if (rc == 0)
		{
			delete this;
		}

		return rc;
	}

	COM_METHOD(HRESULT) CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
	{
		if (pUnkOuter != NULL)
		{
			return CLASS_E_NOAGGREGATION;
		}

		void *obj = NULL;

		if (guid_equals(riid, fact_iid))
		{
			obj = fact_creator();
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}

		if (obj == NULL)
		{
			return E_OUTOFMEMORY;
		}

		return reinterpret_cast<IUnknown *>(obj)->QueryInterface(riid, ppvObject);
	}

	COM_METHOD(HRESULT) LockServer(BOOL fLock) { return E_NOTIMPL; }

private:
	long refcount;
};

///////////////////////////////////////////////////////////////////////////////
//
// COM DLL interface functions
//

static void *DllHandle = NULL;

BOOL __stdcall DllMain(void *hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	if (dwReason == 1 /*DLL_PROCESS_ATTACH*/)
	{
		DllHandle = hinstDLL;
	}
	return 1;
}

#if XAUDIO2_VERSION <= 7

extern "C" HRESULT __stdcall DllCanUnloadNow() 
{ 
	return S_FALSE; 
}

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	IClassFactory *factory = NULL;

	if (guid_equals(rclsid, *CLSID_XAudio2[XAUDIO2_VERSION]))
	{
		factory = new ClassFactory<IID_IXAudio2, CreateXAudio2Internal>();
	}
	else if (guid_equals(rclsid, *CLSID_AudioVolumeMeter[XAUDIO2_VERSION]))
	{
		factory = new ClassFactory<IID_IUnknown, CreateAudioVolumeMeterInternal>();
	}
	else if (guid_equals(rclsid, *CLSID_AudioReverb[XAUDIO2_VERSION]))
	{
		factory = new ClassFactory<IID_IUnknown, CreateAudioReverbInternal>();
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	if (!factory)
	{
		return E_OUTOFMEMORY;
	}

	return factory->QueryInterface(riid, ppv);
}

extern "C" HRESULT register_faudio_dll(void *, REFIID);
extern "C" HRESULT unregister_faudio_dll(void *, REFIID);

extern "C" HRESULT __stdcall DllRegisterServer(void)
{
#if XAUDIO2_VERSION <= 7
	/* Please note: if you want to test the XAudio2Fx wrappers with MS XAudio2 (e.g. by not registering the IXAudio2 interface below),
	   you also need to change XAPOBase.cpp to make everything work */
	register_faudio_dll(DllHandle, *CLSID_XAudio2[XAUDIO2_VERSION]);
	register_faudio_dll(DllHandle, *CLSID_AudioReverb[XAUDIO2_VERSION]);
	register_faudio_dll(DllHandle, *CLSID_AudioVolumeMeter[XAUDIO2_VERSION]);
#endif
	return S_OK;
}

extern "C" HRESULT __stdcall DllUnregisterServer(void)
{
#if XAUDIO2_VERSION <= 7
	unregister_faudio_dll(DllHandle, *CLSID_XAudio2[XAUDIO2_VERSION]);
	unregister_faudio_dll(DllHandle, *CLSID_AudioReverb[XAUDIO2_VERSION]);
	unregister_faudio_dll(DllHandle, *CLSID_AudioVolumeMeter[XAUDIO2_VERSION]);
#endif
	return S_OK;
}

#endif // XAUDIO2_VERSION <= 7
