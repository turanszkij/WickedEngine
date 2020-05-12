#pragma once

#include <string>

namespace wiStartupArguments
{
	void Parse(const wchar_t* args);
	bool HasArgument(const std::string& value);
}
