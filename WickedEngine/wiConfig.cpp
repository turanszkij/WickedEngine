#include "wiConfig.h"
#include "wiHelper.h"

#include <unordered_set>
#include <mutex>

namespace wi::config
{
	bool Section::Has(const char* name) const
	{
		return values.find(name) != values.end();
	}

	bool Section::GetBool(const char* name) const
	{
		const auto it = values.find(name);
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
		if (it->second.empty())
		{
			return 0;
		}
		return std::stoi(it->second) != 0;
	}
	int Section::GetInt(const char* name) const
	{
		const auto it = values.find(name);
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
		if (it->second.empty())
		{
			return 0;
		}
		return std::stoi(it->second);
	}
	uint32_t Section::GetUint(const char* name) const
	{
		const auto it = values.find(name);
		if (it == values.end())
		{
			return 0u;
		}
		if (!it->second.compare("true"))
		{
			return 1u;
		}
		if (!it->second.compare("false"))
		{
			return 0u;
		}
		if (it->second.empty())
		{
			return 0u;
		}
		return (uint32_t)std::stoul(it->second);
	}
	float Section::GetFloat(const char* name) const
	{
		const auto it = values.find(name);
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
		if (it->second.empty())
		{
			return 0;
		}
		return std::stof(it->second);
	}
	std::string Section::GetText(const char* name) const
	{
		const auto it = values.find(name);
		if (it == values.end())
		{
			return "";
		}
		return it->second;
	}

	std::vector<std::string> Section::GetTextArray(const char* name) const
	{
		std::vector<std::string> result;
		const auto it = values.find(name);
		if (it == values.end())
		{
			return result;
		}
		const std::string& str = it->second;
		if (str.empty())
		{
			return result;
		}
		size_t start = 0;
		size_t end = str.find(',');
		while (end != std::string::npos)
		{
			std::string token = str.substr(start, end - start);
			// Trim leading/trailing whitespace
			size_t first = token.find_first_not_of(" \t");
			size_t last = token.find_last_not_of(" \t");
			if (first != std::string::npos)
				result.push_back(token.substr(first, last - first + 1));
			else
				result.push_back("");
			start = end + 1;
			end = str.find(',', start);
		}
		// Last token
		std::string token = str.substr(start);
		size_t first = token.find_first_not_of(" \t");
		size_t last = token.find_last_not_of(" \t");
		if (first != std::string::npos)
			result.push_back(token.substr(first, last - first + 1));
		else
			result.push_back("");
		return result;
	}
	std::vector<int> Section::GetIntArray(const char* name) const
	{
		std::vector<int> result;
		std::vector<std::string> strings = GetTextArray(name);
		result.reserve(strings.size());
		for (const auto& s : strings)
		{
			if (!s.empty())
				result.push_back(std::stoi(s));
			else
				result.push_back(0);
		}
		return result;
	}
	std::vector<float> Section::GetFloatArray(const char* name) const
	{
		std::vector<float> result;
		std::vector<std::string> strings = GetTextArray(name);
		result.reserve(strings.size());
		for (const auto& s : strings)
		{
			if (!s.empty())
				result.push_back(std::stof(s));
			else
				result.push_back(0.0f);
		}
		return result;
	}
	size_t Section::GetCount(const char* name) const
	{
		const auto it = values.find(name);
		if (it == values.end() || it->second.empty())
		{
			return 0;
		}
		size_t count = 1;
		for (char c : it->second)
		{
			if (c == ',')
				count++;
		}
		return count;
	}

	void Section::Set(const char* name, bool value)
	{
		values[name] = value ? "true" : "false";
	}
	void Section::Set(const char* name, int value)
	{
		values[name] = std::to_string(value);
	}
	void Section::Set(const char* name, uint32_t value)
	{
		values[name] = std::to_string(value);
	}
	void Section::Set(const char* name, float value)
	{
		values[name] = std::to_string(value);
	}
	void Section::Set(const char* name, const char* value)
	{
		values[name] = value;
	}
	void Section::Set(const char* name, const std::string& value)
	{
		values[name] = value;
	}
	void Section::Set(const char* name, const std::vector<std::string>& arr)
	{
		std::string result;
		for (size_t i = 0; i < arr.size(); ++i)
		{
			if (i > 0)
				result += ", ";
			result += arr[i];
		}
		values[name] = result;
	}
	void Section::Set(const char* name, const std::vector<int>& arr)
	{
		std::string result;
		for (size_t i = 0; i < arr.size(); ++i)
		{
			if (i > 0)
				result += ", ";
			result += std::to_string(arr[i]);
		}
		values[name] = result;
	}
	void Section::Set(const char* name, const std::vector<float>& arr)
	{
		std::string result;
		for (size_t i = 0; i < arr.size(); ++i)
		{
			if (i > 0)
				result += ", ";
			result += std::to_string(arr[i]);
		}
		values[name] = result;
	}

	bool File::Open(const char* filename)
	{
		this->filename = filename; // even if file couldn't be loaded, we remember the filename so it can be created on commit
		if (!wi::helper::FileExists(filename))
			return false;
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

		bool flushed = false;
		auto flush_line = [&]() {
			if (flushed)
				return;
			flushed = true;
			while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
			{
				// trailing whitespaces from values are removed and added as comment:
				comment = value.back() + comment;
				value.pop_back();
			}
			opened_order.emplace_back();
			opened_order.back().section_label = section_label;
			section_label.clear();
			opened_order.back().section_id = section_id;
			opened_order.back().comment = comment;
			if (!key.empty())
			{
				opened_order.back().key = key;
				current_section->Set(key.c_str(), value);
			}
			key.clear();
			value.clear();
			comment.clear();
			state = State::Key;
		};

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
				flushed = false; // force flush
				flush_line();
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

		flush_line();

		return true;
	}
	void File::Commit()
	{
		std::unordered_map<Section*, std::unordered_set<std::string>> committed_values;
		Section* section = this;
		std::string text;
		for (auto& line : opened_order)
		{
			if (line.section_id.empty())
			{
				if (!line.key.empty())
				{
					text += line.key + " = " + values[line.key];
					committed_values[section].insert(line.key);
					text += line.comment + "\n"; // Newline only added when there's a key-value pair
				}
				else if (!line.comment.empty())
				{
					text += line.comment + "\n"; // Standalone comment line
				}
			}
			else
			{
				if (!line.section_label.empty())
				{
					// Commit unformatted left over values for previous section:
					for (auto& it : section->values)
					{
						if (committed_values[section].count(it.first) == 0)
						{
							text += it.first + " = " + it.second + "\n";
							committed_values[section].insert(it.first);
						}
					}
					// Begin new section:
					section = &sections[line.section_label];
					// Ensure proper line termination before section
					if (!text.empty() && text.back() != '\n')
						text += "\n";
					// Add blank line between sections (but not at file start)
					if (!text.empty())
						text += "\n";
					text += "[" + line.section_label + "]";
					text += line.comment + "\n";
				}
				else if (!line.key.empty())
				{
					text += line.key + " = " + sections[line.section_id].GetText(line.key.c_str());
					committed_values[section].insert(line.key);
					text += line.comment + "\n"; // Newline only added when there's a key-value pair
				}
				else if (!line.comment.empty())
				{
					text += line.comment + "\n"; // Standalone comment line
				}
			}
		}

		// Commit left over unformatted sections and values:
		for (auto& it : values)
		{
			if (committed_values[this].count(it.first) == 0)
			{
				text += it.first + " = " + it.second + "\n";
				committed_values[this].insert(it.first);
			}
		}
		for (auto& it : sections)
		{
			if (it.second.values.empty())
				continue;
			if (committed_values.count(&it.second) == 0)
			{
				// Add blank line before section (but not at file start)
				if (!text.empty())
					text += "\n";
				text += "[" + it.first + "]\n";
				committed_values[&it.second] = {};
			}
			for (auto& it2 : it.second.values)
			{
				if (committed_values[&it.second].count(it2.first) == 0)
				{
					text += it2.first + " = " + it2.second + "\n";
					committed_values[&it.second].insert(it2.first);
				}
			}
		}
		std::scoped_lock lck(locker);
		wi::helper::FileWrite(filename, (const uint8_t*)text.c_str(), text.length());
	}
	Section& File::GetSection(const char* name)
	{
		return sections[name];
	}
	bool File::HasSection(const char* name) const
	{
		return sections.find(name) != sections.end();
	}
}
