#pragma once
#include "WickedEngine.h"

class VariableManager
{
public:
	enum Data_Type{
		TEXT,
		NUMBER,
	};
private:
	struct Variable{
		string data;
		Data_Type type;
		Variable(const string d, Data_Type t):data(d),type(t){}
	};
	typedef unordered_map<string,Variable*> container;
	static container variables;
public:
	static void SetUp();

	static const Variable* get(const string& name);
	static bool add(const string& name, const string& value, Data_Type newType = Data_Type::TEXT); 
	static bool del(const string& name);
	static bool CleanUp();
};

