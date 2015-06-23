#ifndef WHELPER
#define WHELPER

#include "WickedEngine.h"
#include <locale>

static class WickedHelper{
public:
	static string toUpper(const string& s);

	static bool readByteData(const string& fileName, BYTE** data, size_t& dataSize);

	static void warningMessage(const string& msg);
};

#endif
