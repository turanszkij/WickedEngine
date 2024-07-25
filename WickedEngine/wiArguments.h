#pragma once

#include <string>

// This can be used to parse and retrieve startup/command arguments of the application
namespace wi::arguments
{
	void Parse(const wchar_t* args);
	void Parse(int argc, char* argv[]);
	bool HasArgument(const std::string& value);
}
