#pragma once
#include "CommonInclude.h"

#include <string>

class wiHashString
{
private:
	std::string str;
	size_t hash;
public:
	wiHashString(const char* value = "");
	wiHashString(const std::string& value);
	~wiHashString();

	const std::string& GetString() const { return str; }
	size_t GetHash() const { return hash; }
};

bool operator==(const wiHashString& a, const wiHashString& b);

namespace std
{
	template <>
	struct hash<wiHashString>
	{
		size_t operator()(const wiHashString& k) const
		{
			return k.GetHash();
		}
	};
}

