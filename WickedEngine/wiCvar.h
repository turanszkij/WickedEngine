#pragma once
#include "CommonInclude.h"

class wiCvar
{
public:
	enum Data_Type{
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
		Variable(const string d, Data_Type t):data(d),type(t){}
	};
	typedef map<string,Variable*> container;
	static container variables;
public:
	static void SetUp();

	static const Variable* get(const string& name);
	static bool add(const string& name, const string& value, Data_Type newType = Data_Type::TEXT); 
	static bool del(const string& name);
	static bool CleanUp();
};

