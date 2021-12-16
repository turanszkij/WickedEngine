/*
BSD LICENSE

Copyright (c) 2019-2020 Superluminal. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "PerformanceAPI_capi.h"

#ifdef __cplusplus
	#define PERFORMANCEAPI_API inline
#else
	#define PERFORMANCEAPI_API static inline
#endif

#ifdef PERFORMANCEAPI_ENABLED
	typedef HMODULE PerformanceAPI_ModuleHandle;
#else
	typedef void* PerformanceAPI_ModuleHandle;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load the PerformanceAPI functions from the specified DLL path. If any part of this fails, the output 
 * outFunctions will be zero-initialized. 
 *
 * @param inPathToDLL	The path to the PerformanceAPI DLL. Note: The DLL at the specified path must match the architecture (i.e. x86 or x64) of the program this API is used in.
 * @param outFunctions	Pointer to a PerformanceAPI_Functions struct that will be filled with the correct function pointers to use the API. Filled with null pointers if the load failed for whatever reason.
 *
 * @return A handle to the module if the module was successfully loaded and the API retrieved; NULL otherwise. This can be used to free the module through PerformanceAPI_Free if needed.
 */	 
PERFORMANCEAPI_API PerformanceAPI_ModuleHandle PerformanceAPI_LoadFrom(const wchar_t* inPathToDLL, PerformanceAPI_Functions* outFunctions)
{
	// Zero-initialize functions and copy to the output. This ensures we can return from this function at any point,
	// without leaving the user in a state where the output is only partially initialized.
	PerformanceAPI_Functions functions = { 0 };
	*outFunctions = functions;

	// If the API is not enabled (i.e. non-Windows or explicitly disabled by the user), we don't try to initialize any of the functions.
	// In this case the user will be left with a default (zero) initialized functions struct.
#if PERFORMANCEAPI_ENABLED
	HMODULE module = LoadLibraryW(inPathToDLL);
	if (module == NULL)
		return NULL;

	PerformanceAPI_GetAPI_Func getAPI = (PerformanceAPI_GetAPI_Func)((void*)GetProcAddress(module, "PerformanceAPI_GetAPI"));
	if (getAPI == NULL)
	{
		FreeLibrary(module);
		return NULL;
	}

	if (getAPI(PERFORMANCEAPI_VERSION, outFunctions) == 0)
	{
		FreeLibrary(module);
		return NULL;
	}

	return module;
#else
	return NULL;
#endif
}

/**
 * Free the PerformanceAPI module that was previously loaded through PerformanceAPI_LoadFrom. After this function is called, you can no longer use the function pointers 
 * in the PerformanceAPI_Functions struct that you previously retrieved through PerformanceAPI_LoadFrom.
 *
 * @param inModule The module to free
 */
PERFORMANCEAPI_API void PerformanceAPI_Free(PerformanceAPI_ModuleHandle* inModule)
{
#if PERFORMANCEAPI_ENABLED
	FreeLibrary(*inModule);
#endif
}

#ifdef __cplusplus
} // extern "C"
#endif
