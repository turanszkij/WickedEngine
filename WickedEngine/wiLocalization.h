#pragma once
#include "CommonInclude.h"
#include "wiUnorderedMap.h"
#include "wiVector.h"

#include <string>

namespace wi
{
	struct Localization
	{
		struct Entry
		{
			std::string id_text;
			std::string text;
			std::string hint;
		};
		wi::vector<Entry> entries;
		wi::unordered_map<size_t, size_t> lookup;
		wi::unordered_map<std::string, Localization> sections;

		Localization& GetSection(const char* name);
		Localization& GetSection(const std::string& name);
		const Localization* CheckSection(const char* name) const;
		const Localization* CheckSection(const std::string& name) const;

		void Add(size_t id, const char* text, const char* hint = nullptr);
		void Add(const char* id, const char* text);

		const char* Get(size_t id) const;
		const char* Get(const char* id) const;

		bool Import(const std::string& filename);
		bool Export(const std::string& filename) const;
	};
}
