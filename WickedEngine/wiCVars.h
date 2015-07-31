#pragma once
#include "CommonInclude.h"
#include "wiHelper.h"
#include "wiThreadSafeManager.h"

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
		string data;
		Data_Type type;
		Variable(const string& d = "", Data_Type t = TEXT):data(d),type(t){}

		bool isValid() const
		{
			return type != EMPTY && !data.empty();
		}
		string get() const
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
	typedef map<string,Variable> container;
	container variables;

	static wiCVars* globalVars;
public:
	wiCVars();
	~wiCVars();
	static wiCVars* GetGlobal();

	const Variable get(const string& name);
	bool set(const string& name, const string& value);
	bool add(const string& name, const string& value, Data_Type newType = Data_Type::TEXT); 
	bool del(const string& name);
	bool CleanUp();
};

