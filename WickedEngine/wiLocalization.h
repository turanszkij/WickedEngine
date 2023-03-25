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
		std::string section_hint;

		// Check if localization contains anything
		bool IsValid() const { return !entries.empty() || !sections.empty(); }

		// Get access to a section, identified by name
		Localization& GetSection(const char* name);
		Localization& GetSection(const std::string& name);

		// Check if a section exists, identified by name.
		//	returns section pointer if exists, nullptr otherwise
		const Localization* CheckSection(const char* name) const;
		const Localization* CheckSection(const std::string& name) const;

		// Sets a hint string to a section, to add some context to editors
		void SetSectionHint(const char* text);
		void SetSectionHint(const std::string& text);

		// Adds a new entry if it doesn't exist yet, or update existing one
		void Add(size_t id, const char* text, const char* hint = nullptr);

		// Get localized string for an entry
		const char* Get(size_t id) const;

		// Import XML file into this Localization
		bool Import(const std::string& filename);

		// Export this localization into an XML file
		bool Export(const std::string& filename) const;
	};
}
