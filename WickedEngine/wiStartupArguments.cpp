#include "wiStartupArguments.h"

#include <vector>
#include <sstream>
#include <iterator>

using namespace std;

set<string> wiStartupArguments::params;

void wiStartupArguments::Parse(const wchar_t* args)
{
	wstring tmp = args;
	string tmp1(tmp.begin(), tmp.end());

	istringstream iss(tmp1);

	params = 
	{ 
		istream_iterator<string>{iss},
		istream_iterator<string>{} 
	};

}

bool wiStartupArguments::HasArgument(const std::string& value)
{
	return params.find(value) != params.end();
}
