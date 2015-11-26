#include "wiNetwork_BindLua.h"
#include "wiClient.h"
#include "wiServer.h"
#include "wiHelper.h"

const char wiClient_BindLua::className[] = "Client";

Luna<wiClient_BindLua>::FunctionType wiClient_BindLua::methods[] = {
	lunamethod(wiClient_BindLua,Poll),
	{ NULL, NULL }
};
Luna<wiClient_BindLua>::PropertyType wiClient_BindLua::properties[] = {
	{ NULL, NULL }
};


wiClient_BindLua::wiClient_BindLua(lua_State* L)
{
	string name = "CLIENT-", ipaddress = "127.0.0.1";
	name += wiHelper::getCurrentDateTimeAsString();
	int port = 65000;

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		name = wiLua::SGetString(L, 1);
		if (argc > 1)
		{
			ipaddress = wiLua::SGetString(L, 2);
			if (argc > 2)
			{
				port = wiLua::SGetInt(L, 3);
			}
		}
	}

	client = new wiClient(name, ipaddress, port);
}


wiClient_BindLua::~wiClient_BindLua()
{
	SAFE_DELETE(client);
}

int wiClient_BindLua::Poll(lua_State* L)
{
	int i = 0;
	client->Poll(i);
	return 0;
}

void wiClient_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiClient_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}


const char wiServer_BindLua::className[] = "Server";

Luna<wiServer_BindLua>::FunctionType wiServer_BindLua::methods[] = {
	lunamethod(wiServer_BindLua,Poll),
	{ NULL, NULL }
};
Luna<wiServer_BindLua>::PropertyType wiServer_BindLua::properties[] = {
	{ NULL, NULL }
};

wiServer_BindLua::wiServer_BindLua(lua_State* L)
{
	string name = "SERVER-", ipaddress = "0.0.0.0";
	name+=wiHelper::getCurrentDateTimeAsString();
	int port = 65000;

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		name = wiLua::SGetString(L, 1);
		if (argc > 1)
		{
			ipaddress = wiLua::SGetString(L, 2);
			if (argc > 2)
			{
				port = wiLua::SGetInt(L, 3);
			}
		}
	}

	server = new wiServer(name, ipaddress, port);
}

wiServer_BindLua::~wiServer_BindLua()
{
	SAFE_DELETE(server);
}

int wiServer_BindLua::Poll(lua_State* L)
{
	int i = 0;
	server->Poll(i);
	return 0;
}

void wiServer_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiServer_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}