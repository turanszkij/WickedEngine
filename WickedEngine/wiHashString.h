#pragma once
#include "CommonInclude.h"

class wiHashString
{
private:
	string str;
	size_t hash;
public:
	wiHashString(const char* value = "");
	wiHashString(const string& value);
	~wiHashString();

	const string& GetString() const { return str; }
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

