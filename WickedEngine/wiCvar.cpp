#include "wiCvar.h"

wiCvar::container wiCvar::variables;

void wiCvar::SetUp()
{
}

const wiCvar::Variable* wiCvar::get(const string& name)
{
	container::iterator it = variables.find(name);
	if(it!=variables.end())
		return it->second;
	else return nullptr;
}
bool wiCvar::add(const string& name, const string& value, Data_Type newType)
{
	if(get(name))
		return false;
	variables.insert< pair<string,Variable*> >( pair<string,Variable*>(name,new Variable(value,newType)) );
} 
bool wiCvar::del(const string& name)
{
	Variable* var=nullptr;
	if(!var)
		return false;
	delete(var);
	variables.erase(name);
	return true;
}
bool wiCvar::CleanUp()
{
	return true;
}
