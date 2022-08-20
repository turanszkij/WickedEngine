#pragma once
#include "CommonInclude.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace wi::config
{
	struct File;
	struct Section
	{
		friend struct File;

		// Check whether the key exists:
		bool Has(const char* name) const;

		// Get the associated value for the key:
		bool GetBool(const char* name) const;
		int GetInt(const char* name) const;
		float GetFloat(const char* name) const;
		std::string GetText(const char* name) const;

		// Set the associated value for the key:
		void Set(const char* name, bool value);
		void Set(const char* name, int value);
		void Set(const char* name, float value);
		void Set(const char* name, const char* value);
		void Set(const char* name, const std::string& value);

	protected:
		std::unordered_map<std::string, std::string> values;
	};

	struct File : public Section
	{
		// Open a config file (.ini format)
		bool Open(const char* filename);
		// Write back the config file with the current keys and values
		void Commit();
		// Get access to a named section. If it doesn't exist, then it will be created
		Section& GetSection(const char* name);

	private:
		std::string filename;
		std::unordered_map<std::string, Section> sections;
		struct Line
		{
			std::string section_label;
			std::string section_id;
			std::string key;
			std::string comment;
		};
		std::vector<Line> opened_order;
	};
}
