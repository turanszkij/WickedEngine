#include "wiNetwork_BindLua.h"
#include "wiNetwork.h"
#include "wiHelper.h"

using namespace std;

const char wiNetwork_BindLua::className[] = "Network";

Luna<wiNetwork_BindLua>::FunctionType wiNetwork_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<wiNetwork_BindLua>::PropertyType wiNetwork_BindLua::properties[] = {
	{ NULL, NULL }
};

void wiNetwork_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiNetwork_BindLua>::Register(wiLua::GetLuaState());

		wiLua::RunText("network = Network()");
	}
}

