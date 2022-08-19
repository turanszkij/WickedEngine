#include "wiConfig.h"
#include "wiHelper.h"

namespace wi::config
{
	bool Section::Has(const std::string& name) const
	{
		return values.find(name) != values.end();
	}

	bool Section::GetBool(const std::string& name) const
	{
		auto it = values.find(name);
		if (it == values.end())
		{
			return 0;
		}
		if (!it->second.compare("true"))
		{
			return true;
		}
		if (!it->second.compare("false"))
		{
			return false;
		}
		return std::stoi(it->second) != 0;
	}
	int Section::GetInt(const std::string& name) const
	{
		auto it = values.find(name);
		if (it == values.end())
		{
			return 0;
		}
		if (!it->second.compare("true"))
		{
			return 1;
		}
		if (!it->second.compare("false"))
		{
			return 0;
		}
		return std::stoi(it->second);
	}
	float Section::GetFloat(const std::string& name) const
	{
		auto it = values.find(name);
		if (it == values.end())
		{
			return 0;
		}
		if (!it->second.compare("true"))
		{
			return 1;
		}
		if (!it->second.compare("false"))
		{
			return 0;
		}
		return std::stof(it->second);
	}
	std::string Section::GetText(const std::string& name) const
	{
		auto it = values.find(name);
		if (it == values.end())
		{
			return "";
		}
		return it->second;
	}

	void Section::Set(const std::string& name, bool value)
	{
		values[name] = value ? "true" : "false";
	}
	void Section::Set(const std::string& name, int value)
	{
		values[name] = std::to_string(value);
	}
	void Section::Set(const std::string& name, float value)
	{
		values[name] = std::to_string(value);
	}
	void Section::Set(const std::string& name, const std::string& value)
	{
		values[name] = value;
	}

	bool File::Open(const std::string& filename)
	{
		wi::vector<uint8_t> filedata;
		if (!wi::helper::FileRead(filename, filedata))
			return false;

		std::string text = std::string(filedata.begin(), filedata.end());
		std::string key;
		std::string value;
		std::string section_label;
		std::string section_id;
		std::string comment;
		Section* current_section = this;
		enum class State
		{
			Key,
			Value,
			Section,
			Comment,
		} state = State::Key;
		for (auto c : text)
		{
			switch (c)
			{
			case '\r':
				break;
			case ';':
			case '#':
				state = State::Comment;
				comment += c;
				break;
			case ' ':
			case '\t':
				if (state == State::Value && !value.empty())
				{
					value += c;
				}
				else if (state == State::Comment)
				{
					comment += c;
				}
				break;
			case '\n':
				opened_order.emplace_back();
				opened_order.back().section_label = section_label;
				section_label.clear();
				opened_order.back().section_id = section_id;
				opened_order.back().comment = comment;
				if (!key.empty())
				{
					opened_order.back().key = key;
					current_section->Set(key, value);
				}
				key.clear();
				value.clear();
				comment.clear();
				state = State::Key;
				break;
			case '=':
				if (state == State::Key)
				{
					state = State::Value;
				}
				else if (state == State::Value)
				{
					value += c;
				}
				else if (state == State::Comment)
				{
					comment += c;
				}
				break;
			case '[':
				if (state == State::Key)
				{
					state = State::Section;
					section_label.clear();
					section_id.clear();
				}
				else if (state == State::Value)
				{
					value += c;
				}
				else if (state == State::Comment)
				{
					comment += c;
				}
				break;
			case ']':
				if (state == State::Section)
				{
					sections[section_id] = Section();
					current_section = &sections[section_id];
					state = State::Key;
				}
				else if (state == State::Value)
				{
					value += c;
				}
				else if (state == State::Comment)
				{
					comment += c;
				}
				break;
			default:
				switch (state)
				{
				case State::Key:
					key += c;
					break;
				case State::Value:
					value += c;
					break;
				case State::Section:
					section_label += c;
					section_id += c;
					break;
				case State::Comment:
					comment += c;
					break;
				default:
					break;
				}
				break;
			}
		}
		return true;
	}
	void File::CommitChanges()
	{
		std::string text;
		for (auto& line : opened_order)
		{
			if (line.section_id.empty())
			{
				if (!line.key.empty())
				{
					text += line.key + " = " + values[line.key];
				}
			}
			else
			{
				if (!line.section_label.empty())
				{
					text += "[" + line.section_label + "]";
				}
				if (!line.key.empty())
				{
					text += line.key + " = " + sections[line.section_id].GetText(line.key);
				}
			}
			text += line.comment + "\n";
		}
		wi::helper::FileWrite(filename, (const uint8_t*)text.c_str(), text.length());
	}
	Section& File::GetSection(const std::string& name)
	{
		auto it = sections.find(name);
		if (it == sections.end())
		{
			return *this;
		}
		return it->second;
	}
}
