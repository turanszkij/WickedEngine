#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#ifdef SUPPORT_WINDOWS7
#include <winsdkver.h>
#define  _WIN32_WINNT   0x0601 //WIN7

#elif
#include <SDKDDKVer.h>

#endif
