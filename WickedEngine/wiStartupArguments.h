#pragma once

#include <string>

namespace wiStartupArguments
{
	void Parse(const wchar_t* args);
    void Parse(int argc, char *argv[]);
	bool HasArgument(const std::string& value);
}
