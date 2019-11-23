#pragma once

#ifdef _WIN32

#define NOMINMAX
#include <SDKDDKVer.h>
#include <windows.h>

#ifdef WINSTORE_SUPPORT
#include <Windows.UI.Core.h>
#endif // WINSTORE_SUPPORT

#endif // _WIN32
