#include "VariableManager.h"

VariableManager::container VariableManager::variables;

void VariableManager::SetUp()
{
}

const VariableManager::Variable* VariableManager::get(const string& name)
{
	container::iterator it = variables.find(name);
	if(it!=variables.end())
		return it->second;
	else return nullptr;
}
bool VariableManager::add(const string& name, const string& value, Data_Type newType)
{
	if(get(name))
		return false;
	variables.insert< pair<string,Variable*> >( pair<string,Variable*>(name,new Variable(value,newType)) );
} 
bool VariableManager::del(const string& name)
{
	Variable* var=nullptr;
	if(!var)
		return false;
	delete(var);
	variables.erase(name);
	return true;
}
bool VariableManager::CleanUp()
{
	return true;
}
