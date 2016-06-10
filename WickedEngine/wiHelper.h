#ifndef WHELPER
#define WHELPER

#include "CommonInclude.h"
#include <locale>

namespace wiHelper
{
	string toUpper(const string& s);

	bool readByteData(const string& fileName, BYTE** data, size_t& dataSize);

	void messageBox(const string& msg, const string& caption = "Warning!");

	void screenshot(const string& name = "");

	string getCurrentDateTimeAsString();

	string GetWorkingDirectory();

	void GetFilesInDirectory(vector<string> &out, const string &directory);
};

#endif
