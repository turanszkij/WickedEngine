#pragma once
#include "CommonInclude.h"

#include <string>

class wiHashString
{
private:
	std::string str;
	size_t hash;
public:
	wiHashString(const std::string& value = "") : str(value), hash(std::hash<std::string>{}(value)) {}
	wiHashString(const char* value) : wiHashString(std::string(value)) {}

	constexpr const std::string& GetString() const { return str; }
	constexpr size_t GetHash() const { return hash; }
};

constexpr bool operator==(const wiHashString& a, const wiHashString& b)
{
	return a.GetHash() == b.GetHash();
}

namespace std
{
	template <>
	struct hash<wiHashString>
	{
		constexpr size_t operator()(const wiHashString& k) const
		{
			return k.GetHash();
		}
	};
}

