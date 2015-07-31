#include "wiCVars.h"

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
		STATICMUTEX.lock();
		if (globalVars == nullptr)
			globalVars = new wiCVars();
		STATICMUTEX.unlock();
	}
	return globalVars;
}


const wiCVars::Variable wiCVars::get(const string& name)
{
	container::iterator it = variables.find(name);
	if(it!=variables.end())
		return it->second;
	else return Variable::Invalid();
}
bool wiCVars::set(const string& name, const string& value)
{
	if (!get(name).isValid())
		return false;

	MUTEX.lock();
	container::iterator it = variables.find(name);
	if (it != variables.end())
		variables[name].data = value;
	MUTEX.unlock();

	return true;
}
bool wiCVars::add(const string& name, const string& value, Data_Type newType)
{
	if(get(name).isValid())
		return false;
	MUTEX.lock();
	variables.insert< pair<string,Variable> >( pair<string,Variable>(name,Variable(value,newType)) );
	MUTEX.unlock();
	return true;
} 
bool wiCVars::del(const string& name)
{
	if (!get(name).isValid())
		return false;
	MUTEX.lock();
	variables.erase(name);
	MUTEX.unlock();
	return true;
}
bool wiCVars::CleanUp()
{
	MUTEX.lock();
	variables.clear();
	MUTEX.unlock();
	return true;
}
