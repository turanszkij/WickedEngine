#ifndef WHELPER
#define WHELPER

#include "WickedEngine.h"
#include <locale>

static class WickedHelper{
public:
	static string toUpper(const string& s);

	static bool readByteData(const string& fileName, BYTE** data, size_t& dataSize);

	static void messageBox(const string& msg, const string& caption = "Warning!", HWND hWnd = nullptr);
};

#endif
