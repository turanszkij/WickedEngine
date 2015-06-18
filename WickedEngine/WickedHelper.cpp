#include "WickedHelper.h"

std::string WickedHelper::toUpper(const std::string& s)
{
	std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += std::toupper(s.at(i), loc);
		}
		return result;
}