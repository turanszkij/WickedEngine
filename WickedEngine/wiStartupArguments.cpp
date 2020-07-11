#include "wiStartupArguments.h"
#include "wiHelper.h"

#include <vector>
#include <sstream>
#include <iterator>
#include <set>

using namespace std;

namespace wiStartupArguments
{
	set<string> params;

	void Parse(const wchar_t* args)
	{
		wstring from = args;
		string to;
		wiHelper::StringConvert(from, to);

		istringstream iss(to);

		params =
		{
			istream_iterator<string>{iss},
			istream_iterator<string>{}
		};

	}

	void Parse(int argc, char *argv[])
    {
	    for (int i=1; i<argc; i++)
        {
	        params.insert(std::string(argv[i]));
        }
    }

	bool HasArgument(const std::string& value)
	{
		return params.find(value) != params.end();
	}

}
