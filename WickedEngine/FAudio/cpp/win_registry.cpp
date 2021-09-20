#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h> /* snprintf */

static bool registry_write_string(HKEY reg, const char *name, const char *value, size_t len) 
{
	HRESULT hr = RegSetValueExA(
		reg, 
		name, 
		0, 
		REG_SZ, 
		reinterpret_cast<const BYTE *> (value), 
		static_cast<DWORD>((len + 1) * sizeof(char))
	);
	return hr == ERROR_SUCCESS;
}

static const char *base_key() 
{
	if (sizeof(void *) == 4) 
	{	
		return "Software\\Classes\\WOW6432Node\\CLSID";
	} 
	else 
	{
		return "Software\\Classes\\CLSID";
	}
}

static void format_clsid(REFIID clsid, char *value, size_t max) 
{
	snprintf(value, max, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
		clsid.Data1, clsid.Data2, clsid.Data3,
		clsid.Data4[0], clsid.Data4[1], clsid.Data4[2], clsid.Data4[3],
		clsid.Data4[4], clsid.Data4[5], clsid.Data4[6], clsid.Data4[7]);
}

extern "C" HRESULT register_faudio_dll(void *DllHandle, REFIID clsid) 
{
// not your typical COM register functions because we're "hijacking" the existing XAudio2 registration
//	- COM classes are historically registered under HKEY_CLASSES_ROOT
//  - on modern Windows version this is a merged from on HKEY_LOCAL_MACHINE/Software/Classes and HKEY_CURRENT_USER/Software/Classes
//	- the system-wide configuration is stored under HKEY_LOCAL_MACHINE
//	- we override the registration for the XAudio2 DLLs for the current user
	
	char str_clsid[40];
	format_clsid(clsid, str_clsid, sizeof(str_clsid) / sizeof(char));

	char key[2048];
	snprintf(key, sizeof(key) / sizeof(char), "%s\\%s\\InProcServer32", base_key(), str_clsid);

	// open registry (creating key if it does not exist)
	HKEY registry_key = NULL;
	if (RegCreateKeyExA(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_WRITE, NULL, &registry_key, NULL)) 
	{
		return E_FAIL;
	}

	// retrieve path to the dll
	char path[2048];
	DWORD len = GetModuleFileNameA(reinterpret_cast<HMODULE>(DllHandle), path, sizeof(key) / sizeof(char));

	// update registry
	registry_write_string(registry_key, NULL, path, len);
	RegCloseKey(registry_key);
	
	return S_OK;
}

extern "C" HRESULT unregister_faudio_dll(void *DllHandle, REFIID clsid) 
{
	// open registry 
	HKEY registry_key = NULL;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, base_key(), 0, DELETE | KEY_SET_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &registry_key)) 
	{
		return E_FAIL;
	}

	char str_clsid[40];
	format_clsid(clsid, str_clsid, sizeof(str_clsid) / sizeof(char));

	// delete key
	HRESULT hr = RegDeleteTreeA(registry_key, str_clsid);
	RegCloseKey(registry_key);
	return S_OK;
}
