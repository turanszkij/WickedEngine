#include "wiCVars.h"

using namespace std;

wiCVars* wiCVars::globalVars = nullptr;

wiCVars::wiCVars()
{
	wiThreadSafeManager();
}
wiCVars::~wiCVars()
{
	CleanUp();
}
wiCVars* wiCVars::GetGlobal()
{
	if (globalVars == nullptr)
	{
		LOCK_STATIC();
		if (globalVars == nullptr)
			globalVars = new wiCVars();
		UNLOCK_STATIC();
	}
	return globalVars;
}


const wiCVars::Variable wiCVars::get(const std::string& name)
{
	container::iterator it = variables.find(name);
	if(it!=variables.end())
		return it->second;
	else return Variable::Invalid();
}
bool wiCVars::set(const std::string& name, const std::string& value)
{
	if (!get(name).isValid())
		return false;

	LOCK();
	container::iterator it = variables.find(name);
	if (it != variables.end())
		variables[name].data = value;
	UNLOCK();

	return true;
}
bool wiCVars::add(const std::string& name, const std::string& value, Data_Type newType)
{
	if(get(name).isValid())
		return false;
	LOCK();
	variables.insert< pair<string,Variable> >( pair<string,Variable>(name,Variable(value,newType)) );
	UNLOCK();
	return true;
} 
bool wiCVars::del(const std::string& name)
{
	if (!get(name).isValid())
		return false;
	LOCK();
	variables.erase(name);
	UNLOCK();
	return true;
}
bool wiCVars::CleanUp()
{
	LOCK();
	variables.clear();
	UNLOCK();
	return true;
}
