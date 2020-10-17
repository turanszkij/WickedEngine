#include "MainComponent_BindLua.h"
#include "RenderPath_BindLua.h"
#include "RenderPath3D_BindLua.h"
#include "RenderPath2D_BindLua.h"
#include "LoadingScreen_BindLua.h"
#include "wiProfiler.h"

const char MainComponent_BindLua::className[] = "MainComponent";

Luna<MainComponent_BindLua>::FunctionType MainComponent_BindLua::methods[] = {
	lunamethod(MainComponent_BindLua, GetActivePath),
	lunamethod(MainComponent_BindLua, SetActivePath),
	lunamethod(MainComponent_BindLua, SetFrameSkip),
	lunamethod(MainComponent_BindLua, SetTargetFrameRate),
	lunamethod(MainComponent_BindLua, SetFrameRateLock),
	lunamethod(MainComponent_BindLua, SetInfoDisplay),
	lunamethod(MainComponent_BindLua, SetWatermarkDisplay),
	lunamethod(MainComponent_BindLua, SetFPSDisplay),
	lunamethod(MainComponent_BindLua, SetResolutionDisplay),
	{ NULL, NULL }
};
Luna<MainComponent_BindLua>::PropertyType MainComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

MainComponent_BindLua::MainComponent_BindLua(MainComponent* component) :component(component)
{
}

MainComponent_BindLua::MainComponent_BindLua(lua_State *L)
{
	component = nullptr;
}

int MainComponent_BindLua::GetActivePath(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetActivePath() component is empty!");
		return 0;
	}

	//return 3d component if the active one is of that type
	RenderPath3D* comp3D = dynamic_cast<RenderPath3D*>(component->GetActivePath());
	if (comp3D != nullptr)
	{
		Luna<RenderPath3D_BindLua>::push(L, new RenderPath3D_BindLua(comp3D));
		return 1;
	}

	//return loading component if the active one is of that type
	LoadingScreen* compLoad = dynamic_cast<LoadingScreen*>(component->GetActivePath());
	if (compLoad != nullptr)
	{
		Luna<LoadingScreen_BindLua>::push(L, new LoadingScreen_BindLua(compLoad));
		return 1;
	}

	//return 2d component if the active one is of that type
	RenderPath2D* comp2D = dynamic_cast<RenderPath2D*>(component->GetActivePath());
	if (comp2D != nullptr)
	{
		Luna<RenderPath2D_BindLua>::push(L, new RenderPath2D_BindLua(comp2D));
		return 1;
	}

	//return component if the active one is of that type
	RenderPath* comp = dynamic_cast<RenderPath*>(component->GetActivePath());
	if (comp != nullptr)
	{
		Luna<RenderPath_BindLua>::push(L, new RenderPath_BindLua(comp));
		return 1;
	}

	wiLua::SError(L, "GetActivePath() Warning: type of active component not registered!");
	return 0;
}
int MainComponent_BindLua::SetActivePath(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetActivePath(RenderPath component) component is empty!");
		return 0;
	}

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float fadeSeconds = 0;
		wiColor fadeColor = wiColor(0, 0, 0, 255);
		if (argc > 1)
		{
			fadeSeconds = wiLua::SGetFloat(L, 2);
			if (argc > 2)
			{
				fadeColor.setR((uint8_t)wiLua::SGetInt(L, 3));
				if (argc > 3)
				{
					fadeColor.setG((uint8_t)wiLua::SGetInt(L, 4));
					if (argc > 4)
					{
						fadeColor.setB((uint8_t)wiLua::SGetInt(L, 5));
					}
				}
			}
		}

		RenderPath3D_BindLua* comp3D = Luna<RenderPath3D_BindLua>::lightcheck(L, 1);
		if (comp3D != nullptr)
		{
			component->ActivatePath(comp3D->component, fadeSeconds, fadeColor);
			return 0;
		}

		LoadingScreen_BindLua* compLoad = Luna<LoadingScreen_BindLua>::lightcheck(L, 1);
		if (compLoad != nullptr)
		{
			component->ActivatePath(compLoad->component, fadeSeconds, fadeColor);
			return 0;
		}

		RenderPath2D_BindLua* comp2D = Luna<RenderPath2D_BindLua>::lightcheck(L, 1);
		if (comp2D != nullptr)
		{
			component->ActivatePath(comp2D->component, fadeSeconds, fadeColor);
			return 0;
		}

		RenderPath_BindLua* comp = Luna<RenderPath_BindLua>::lightcheck(L, 1);
		if (comp != nullptr)
		{
			component->ActivatePath(comp->component, fadeSeconds, fadeColor);
			return 0;
		}
	}
	else
	{
		wiLua::SError(L, "SetActivePath(RenderPath component, opt int fadeFrames,fadeColorR,fadeColorG,fadeColorB) not enought arguments!");
		return 0;
	}
	return 0;
}
int MainComponent_BindLua::SetFrameSkip(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFrameSkip(bool enabled) component is empty!");
		return 0;
	}

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->setFrameSkip(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetFrameSkip(bool enabled) not enought arguments!");
	return 0;
}
int MainComponent_BindLua::SetTargetFrameRate(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetTargetFrameRate(float value) component is empty!");
		return 0;
	}

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->setTargetFrameRate(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetTargetFrameRate(float value) not enought arguments!");
	return 0;
}
int MainComponent_BindLua::SetFrameRateLock(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFrameRateLock(bool enabled) component is empty!");
		return 0;
	}

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->setFrameRateLock(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetFrameRateLock(bool enabled) not enought arguments!");
	return 0;
}
int MainComponent_BindLua::SetInfoDisplay(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetInfoDisplay() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->infoDisplay.active = wiLua::SGetBool(L, 1);
	}
	else
		wiLua::SError(L, "SetInfoDisplay(bool active) not enough arguments!");
	return 0;
}
int MainComponent_BindLua::SetWatermarkDisplay(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetWatermarkDisplay() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->infoDisplay.watermark = wiLua::SGetBool(L, 1);
	}
	else
		wiLua::SError(L, "SetWatermarkDisplay(bool active) not enough arguments!");
	return 0;
}
int MainComponent_BindLua::SetFPSDisplay(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFPSDisplay() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->infoDisplay.fpsinfo = wiLua::SGetBool(L, 1);
	}
	else
		wiLua::SError(L, "SetFPSDisplay(bool active) not enough arguments!");
	return 0;
}
int MainComponent_BindLua::SetResolutionDisplay(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetResolutionDisplay() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->infoDisplay.resolution = wiLua::SGetBool(L, 1);
	}
	else
		wiLua::SError(L, "SetResolutionDisplay(bool active) not enough arguments!");
	return 0;
}


int SetProfilerEnabled(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiProfiler::SetEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetProfilerEnabled(bool active) not enough arguments!");

	return 0;
}

void MainComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<MainComponent_BindLua>::Register(wiLua::GetLuaState()); 
		
		wiLua::RegisterFunc("SetProfilerEnabled", SetProfilerEnabled);
	}
}
