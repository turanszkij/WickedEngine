#ifndef WHELPER
#define WHELPER

#include "CommonInclude.h"

#include <string>
#include <vector>

namespace wiHelper
{
	string toUpper(const string& s);

	bool readByteData(const string& fileName, BYTE** data, size_t& dataSize);

	void messageBox(const string& msg, const string& caption = "Warning!");

	void screenshot(const string& name = "");

	string getCurrentDateTimeAsString();

	string GetApplicationDirectory();

	string GetOriginalWorkingDirectory();

	string GetWorkingDirectory();

	bool SetWorkingDirectory(const string& path);

	void GetFilesInDirectory(vector<string> &out, const string &directory);

	void SplitPath(const string& fullPath, string& dir, string& fileName);

	string GetFileNameFromPath(const string& fullPath);

	string GetDirectoryFromPath(const string& fullPath);

	void Sleep(float milliseconds);
};

#endif
