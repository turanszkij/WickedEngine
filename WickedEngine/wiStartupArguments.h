#pragma once

#include <string>
#include <set>

class wiStartupArguments
{
public:
	static std::set<std::string> params;

	static void Parse(const wchar_t* args);
	static bool HasArgument(const std::string& value);
};

