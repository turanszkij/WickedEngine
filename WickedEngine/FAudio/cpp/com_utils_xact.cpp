#include "com_utils.h"
#include <string.h> /* memcmp */

/* GUIDs */
const IID IID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 00, 00, 00, 00, 00, 00, 0x46}};
const IID IID_IClassFactory = {0x00000001, 0x0000, 0x0000, {0xC0, 00, 00, 00, 00, 00, 00, 0x46}};

const IID IID_IXACT3Engine3_0 = {0x9e33f661, 0x2d07, 0x43ec, {0x97, 0x04, 0xbb, 0xcb, 0x71, 0xa5, 0x49, 0x72}};
const IID IID_IXACT3Engine3_1 = {0xe72c1b9a, 0xd717, 0x41c0, {0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49}};
const IID IID_IXACT3Engine3_2 = {0xe72c1b9a, 0xd717, 0x41c0, {0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49}};
const IID IID_IXACT3Engine3_3 = {0xe72c1b9a, 0xd717, 0x41c0, {0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49}};
const IID IID_IXACT3Engine3_4 = {0xe72c1b9a, 0xd717, 0x41c0, {0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49}};
const IID IID_IXACT3Engine3_5 = {0xb1ee676a, 0xd9cd, 0x4d2a, {0x89, 0xa8, 0xfa, 0x53, 0xeb, 0x9e, 0x48, 0x0b}};
const IID IID_IXACT3Engine3_6 = {0xb1ee676a, 0xd9cd, 0x4d2a, {0x89, 0xa8, 0xfa, 0x53, 0xeb, 0x9e, 0x48, 0x0b}};
const IID IID_IXACT3Engine3_7 = {0xb1ee676a, 0xd9cd, 0x4d2a, {0x89, 0xa8, 0xfa, 0x53, 0xeb, 0x9e, 0x48, 0x0b}};

const IID CLSID_XACTEngine3_0 = {0x3b80ee2a, 0xb0f5, 0x4780, {0x9e, 0x30, 0x90, 0xcb, 0x39, 0x68, 0x5b, 0x03}};
const IID CLSID_XACTEngine3_1 = {0x962f5027, 0x99be, 0x4692, {0xa4, 0x68, 0x85, 0x80, 0x2c, 0xf8, 0xde, 0x61}};
const IID CLSID_XACTEngine3_2 = {0xd3332f02, 0x3dd0, 0x4de9, {0x9a, 0xec, 0x20, 0xd8, 0x5c, 0x41, 0x11, 0xb6}};
const IID CLSID_XACTEngine3_3 = {0x94c1affa, 0x66e7, 0x4961, {0x95, 0x21, 0xcf, 0xde, 0xf3, 0x12, 0x8d, 0x4f}};
const IID CLSID_XACTEngine3_4 = {0x0977d092, 0x2d95, 0x4e43, {0x8d, 0x42, 0x9d, 0xdc, 0xc2, 0x54, 0x5e, 0xd5}};
const IID CLSID_XACTEngine3_5 = {0x074b110f, 0x7f58, 0x4743, {0xae, 0xa5, 0x12, 0xf1, 0x5b, 0x50, 0x74, 0xed}};
const IID CLSID_XACTEngine3_6 = {0x248d8a3b, 0x6256, 0x44d3, {0xa0, 0x18, 0x2a, 0xc9, 0x6c, 0x45, 0x9f, 0x47}};
const IID CLSID_XACTEngine3_7 = {0xbcc782bc, 0x6492, 0x4c22, {0x8c, 0x35, 0xf5, 0xd7, 0x2f, 0xe7, 0x3c, 0x6e}};

const IID *CLSID_XACTEngine[] =
{
	&CLSID_XACTEngine3_0,
	&CLSID_XACTEngine3_1,
	&CLSID_XACTEngine3_2,
	&CLSID_XACTEngine3_3,
	&CLSID_XACTEngine3_4,
	&CLSID_XACTEngine3_5,
	&CLSID_XACTEngine3_6,
	&CLSID_XACTEngine3_7,
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
void *CreateXACT3EngineInternal(void);

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

extern "C" HRESULT __stdcall DllCanUnloadNow() 
{ 
	return S_FALSE; 
}

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	IClassFactory *factory = NULL;

	if (guid_equals(rclsid, *CLSID_XACTEngine[XACT3_VERSION]))
	{
		factory = new ClassFactory<IID_IXACT3Engine, CreateXACT3EngineInternal>();
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
	register_faudio_dll(DllHandle, *CLSID_XACTEngine[XACT3_VERSION]);
	return S_OK;
}

extern "C" HRESULT __stdcall DllUnregisterServer(void)
{
	unregister_faudio_dll(DllHandle, *CLSID_XACTEngine[XACT3_VERSION]);
	return S_OK;
}
