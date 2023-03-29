#include "wiLocalization.h"
#include "wiHelper.h"
#include "wiBacklog.h"

#include "Utility/pugixml.hpp"

namespace wi
{
	Localization& Localization::GetSection(const char* name)
	{
		return GetSection(std::string(name));
	}
	Localization& Localization::GetSection(const std::string& name)
	{
		return sections[name];
	}
	const Localization* Localization::CheckSection(const char* name) const
	{
		return CheckSection(std::string(name));
	}
	const Localization* Localization::CheckSection(const std::string& name) const
	{
		auto it = sections.find(name);
		if (it != sections.end())
		{
			return &it->second;
		}
		return nullptr;
	}
	void Localization::SetSectionHint(const char* text)
	{
		SetSectionHint(std::string(text));
	}
	void Localization::SetSectionHint(const std::string& text)
	{
		section_hint = text;
	}

	void Localization::Add(size_t id, const char* text, const char* hint)
	{
		auto it = lookup.find(id);
		Entry* entry = nullptr;
		if (it == lookup.end())
		{
			// Add new entry:
			lookup[id] = entries.size();
			entry = &entries.emplace_back();
		}
		else
		{
			// Update existing entry:
			entry = &entries[it->second];
		}

		entry->text = text;
		entry->id_text = std::to_string(id);
		if (hint != nullptr)
		{
			entry->hint = hint;
		}
	}

	const char* Localization::Get(size_t id) const
	{
		auto it = lookup.find(id);
		if (it == lookup.end())
			return nullptr;
		size_t index = it->second;
		const Entry& entry = entries[index];
		return entry.text.c_str();
	}

	void recursive_import(Localization& section, pugi::xml_node& node)
	{
		if (strcmp(node.name(), "entry") == 0)
		{
			const char* hint = nullptr;
			if (node.attribute("hint"))
			{
				hint = node.attribute("hint").as_string();
			}
			section.Add(node.attribute("id").as_ullong(), node.text().as_string(), hint);
			for (pugi::xml_node child : node.children())
			{
				recursive_import(section, child);
			}
		}
		if (strcmp(node.name(), "section") == 0)
		{
			Localization& child_section = section.GetSection(node.attribute("name").as_string());
			if (node.attribute("hint"))
			{
				child_section.section_hint = node.attribute("hint").as_string();
			}
			for (pugi::xml_node child : node.children())
			{
				recursive_import(child_section, child);
			}
		}
	}
	void recursive_export(const Localization& section, pugi::xml_node& node)
	{
		for (auto& entry : section.entries)
		{
			pugi::xml_node child_node = node.append_child("entry");
			child_node.append_attribute("id") = entry.id_text.c_str();
			if (!entry.hint.empty())
			{
				child_node.append_attribute("hint") = entry.hint.c_str();
			}
			child_node.append_child(pugi::node_pcdata).set_value(entry.text.c_str());
		}

		for (auto& it : section.sections)
		{
			const Localization& child_section = it.second;
			pugi::xml_node section_node = node.append_child("section");
			section_node.append_attribute("name") = it.first.c_str();
			if (!child_section.section_hint.empty())
			{
				section_node.append_attribute("hint") = child_section.section_hint.c_str();
			}
			recursive_export(child_section, section_node);
		}
	}

	bool Localization::Import(const std::string& filename)
	{
		wi::vector<uint8_t> filedata;
		bool success = wi::helper::FileRead(filename, filedata);
		if (!success)
			return false;

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_buffer_inplace(filedata.data(), filedata.size());
		if (result.status != pugi::xml_parse_status::status_ok)
		{
			wi::backlog::post("XML error in " + filename + " at offset = " + std::to_string(result.offset) + ": " + result.description(), wi::backlog::LogLevel::Warning);
			return false;
		}

		for (pugi::xml_node child : doc.children())
		{
			recursive_import(*this, child);
		}

		return true;
	}
	bool Localization::Export(const std::string& filename) const
	{
		pugi::xml_document doc;
		doc.append_child(pugi::node_comment).set_value("This file was created by Wicked Engine for language localization.");
		doc.append_child(pugi::node_comment).set_value("You can import this file with the wi::Localization::Import() function.");

		recursive_export(*this, doc);

		return doc.save_file(filename.c_str());
	}
}
