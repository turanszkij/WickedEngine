#pragma once
#include "CommonInclude.h"
#include "wiHelper.h"
#include "wiThreadSafeManager.h"

#include <string>
#include <map>

class wiCVars : public wiThreadSafeManager
{
public:
	enum Data_Type{
		EMPTY,
		TEXT,
		INTEGER,
		FLOAT,
		BOOLEAN,
		CVAR_DATATYPE_COUNT
	};
private:
	struct Variable{
		std::string data;
		Data_Type type;
		Variable(const std::string& d = "", Data_Type t = TEXT):data(d),type(t){}

		bool isValid() const
		{
			return type != EMPTY && !data.empty();
		}
		std::string get() const
		{
			return data;
		}
		int getInt() const 
		{
			return atoi(data.c_str());
		}
		double getFloat() const
		{
			return (double)atof(data.c_str());
		}
		bool getBool() const
		{
			if (atoi(data.c_str()) != 0)
				return true;
			if (!wiHelper::toUpper(data).compare("TRUE"))
				return true;
			return false;
		}
		bool equals(const Variable& other)
		{
			if (type != other.type)
			{
				return false;
			}
			switch (type)
			{
			case wiCVars::TEXT:
				return !data.compare(other.data);
				break;
			case wiCVars::INTEGER:
				return getInt() == other.getInt();
				break;
			case wiCVars::FLOAT:
				return abs(getFloat() - other.getFloat()) < DBL_EPSILON;
				break;
			case wiCVars::BOOLEAN:
				return getBool() == other.getBool();
				break;
			default:
				break;
			}

			return false;
		}
		static Variable Invalid()
		{
			return Variable("", EMPTY);
		}
	};
	typedef std::map<std::string,Variable> container;
	container variables;

	static wiCVars* globalVars;
public:
	wiCVars();
	~wiCVars();
	static wiCVars* GetGlobal();

	const Variable get(const std::string& name);
	bool set(const std::string& name, const std::string& value);
	bool add(const std::string& name, const std::string& value, Data_Type newType = Data_Type::TEXT); 
	bool del(const std::string& name);
	bool CleanUp();
};

