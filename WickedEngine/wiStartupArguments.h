#pragma once

#include <string>

namespace wi::startup_arguments
{
	void Parse(const wchar_t* args);
    void Parse(int argc, char *argv[]);
	bool HasArgument(const std::string& value);
}
