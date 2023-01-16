#include "wiArguments.h"
#include "wiHelper.h"
#include "wiUnorderedSet.h"

#include <sstream>
#include <iterator>

namespace wi::arguments
{
	wi::vector<std::string> params;

	void Parse(const wchar_t* args)
	{
		std::wstring from = args;
		std::string to;
		wi::helper::StringConvert(from, to);

		std::istringstream iss(to);

		params =
		{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		};

	}

	void Parse(int argc, char *argv[])
    {
		for (int i = 1; i < argc; i++)
		{
			params.push_back(std::string(argv[i]));
		}
    }

	bool HasArgument(const std::string& value)
	{
		for(auto& param : params)
		{
			if(param == value)
				return true;
		}
		return false;
	}

	wi::vector<std::string>* GetParameters()
	{
		return &params;
	}
}
