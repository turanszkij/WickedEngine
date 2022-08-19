#pragma once
#include "CommonInclude.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace wi::config
{
	struct Section
	{
		// Check whether the key exists:
		bool Has(const std::string& name) const;

		// Get the associated value for the key:
		bool GetBool(const std::string& name) const;
		int GetInt(const std::string& name) const;
		float GetFloat(const std::string& name) const;
		std::string GetText(const std::string& name) const;

		// Set the associated value for the key:
		void Set(const std::string& name, bool value);
		void Set(const std::string& name, int value);
		void Set(const std::string& name, float value);
		void Set(const std::string& name, const char* value);
		void Set(const std::string& name, const std::string& value);

	protected:
		std::unordered_map<std::string, std::string> values;
	};

	struct File : public Section
	{
		// Open a config file (.ini format)
		bool Open(const std::string& filename);
		// Write back the config file with the current keys and values
		void Commit();
		// Get access to a named section. If it doesn't exist, then the root section will be returned
		Section& GetSection(const char* name);
		Section& GetSection(const std::string& name);

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
