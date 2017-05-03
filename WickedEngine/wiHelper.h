#ifndef WHELPER
#define WHELPER

#include "CommonInclude.h"

#include <string>
#include <vector>

namespace wiHelper
{
	std::string toUpper(const std::string& s);

	bool readByteData(const std::string& fileName, BYTE** data, size_t& dataSize);

	void messageBox(const std::string& msg, const std::string& caption = "Warning!");

	void screenshot(const std::string& name = "");

	std::string getCurrentDateTimeAsString();

	std::string GetApplicationDirectory();

	std::string GetOriginalWorkingDirectory();

	std::string GetWorkingDirectory();

	bool SetWorkingDirectory(const std::string& path);

	void GetFilesInDirectory(std::vector<std::string> &out, const std::string &directory);

	void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName);

	std::string GetFileNameFromPath(const std::string& fullPath);

	std::string GetDirectoryFromPath(const std::string& fullPath);

	void Sleep(float milliseconds);
};

#endif
