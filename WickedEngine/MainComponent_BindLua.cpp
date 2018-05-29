#include "MainComponent_BindLua.h"
#include "RenderableComponent_BindLua.h"
#include "Renderable3DComponent_BindLua.h"
#include "Renderable2DComponent_BindLua.h"
#include "DeferredRenderableComponent_BindLua.h"
#include "ForwardRenderableComponent_BindLua.h"
#include "TiledForwardRenderableComponent_BindLua.h"
#include "TiledDeferredRenderableComponent_BindLua.h"
#include "LoadingScreenComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"
#include "wiProfiler.h"

const char MainComponent_BindLua::className[] = "MainComponent";

Luna<MainComponent_BindLua>::FunctionType MainComponent_BindLua::methods[] = {
	lunamethod(MainComponent_BindLua, GetContent),
	lunamethod(MainComponent_BindLua, GetActiveComponent),
	lunamethod(MainComponent_BindLua, SetActiveComponent),
	lunamethod(MainComponent_BindLua, SetFrameSkip),
	lunamethod(MainComponent_BindLua, SetInfoDisplay),
	lunamethod(MainComponent_BindLua, SetWatermarkDisplay),
	lunamethod(MainComponent_BindLua, SetFPSDisplay),
	lunamethod(MainComponent_BindLua, SetCPUDisplay),
	lunamethod(MainComponent_BindLua, SetResolutionDisplay),
	lunamethod(MainComponent_BindLua, SetColorGradePaletteDisplay),
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

MainComponent_BindLua::~MainComponent_BindLua()
{
}

int MainComponent_BindLua::GetContent(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetContent() component is empty!");
		return 0;
	}
	Luna<wiResourceManager_BindLua>::push(L, new wiResourceManager_BindLua(&component->Content));
	return 1;
}
int MainComponent_BindLua::GetActiveComponent(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetActiveComponent() component is empty!");
		return 0;
	}

	//return deferred 3d component if the active one is of that type
	DeferredRenderableComponent* compDef3D = dynamic_cast<DeferredRenderableComponent*>(component->getActiveComponent());
	if (compDef3D != nullptr)
	{
		Luna<DeferredRenderableComponent_BindLua>::push(L, new DeferredRenderableComponent_BindLua(compDef3D));
		return 1;
	}

	//return tiled deferred 3d component if the active one is of that type
	TiledDeferredRenderableComponent* compTDef3D = dynamic_cast<TiledDeferredRenderableComponent*>(component->getActiveComponent());
	if (compTDef3D != nullptr)
	{
		Luna<TiledDeferredRenderableComponent_BindLua>::push(L, new TiledDeferredRenderableComponent_BindLua(compTDef3D));
		return 1;
	}

	//return tiled forward 3d component if the active one is of that type
	TiledForwardRenderableComponent* compTFwd3D = dynamic_cast<TiledForwardRenderableComponent*>(component->getActiveComponent());
	if (compTFwd3D != nullptr)
	{
		Luna<TiledForwardRenderableComponent_BindLua>::push(L, new TiledForwardRenderableComponent_BindLua(compTFwd3D));
		return 1;
	}

	//return forward 3d component if the active one is of that type
	ForwardRenderableComponent* compFwd3D = dynamic_cast<ForwardRenderableComponent*>(component->getActiveComponent());
	if (compFwd3D != nullptr)
	{
		Luna<ForwardRenderableComponent_BindLua>::push(L, new ForwardRenderableComponent_BindLua(compFwd3D));
		return 1;
	}

	//return 3d component if the active one is of that type
	Renderable3DComponent* comp3D = dynamic_cast<Renderable3DComponent*>(component->getActiveComponent());
	if (comp3D != nullptr)
	{
		Luna<Renderable3DComponent_BindLua>::push(L, new Renderable3DComponent_BindLua(comp3D));
		return 1;
	}

	//return loading component if the active one is of that type
	LoadingScreenComponent* compLoad = dynamic_cast<LoadingScreenComponent*>(component->getActiveComponent());
	if (compLoad != nullptr)
	{
		Luna<LoadingScreenComponent_BindLua>::push(L, new LoadingScreenComponent_BindLua(compLoad));
		return 1;
	}

	//return 2d component if the active one is of that type
	Renderable2DComponent* comp2D = dynamic_cast<Renderable2DComponent*>(component->getActiveComponent());
	if (comp2D != nullptr)
	{
		Luna<Renderable2DComponent_BindLua>::push(L, new Renderable2DComponent_BindLua(comp2D));
		return 1;
	}

	//return component if the active one is of that type
	RenderableComponent* comp = dynamic_cast<RenderableComponent*>(component->getActiveComponent());
	if (comp != nullptr)
	{
		Luna<RenderableComponent_BindLua>::push(L, new RenderableComponent_BindLua(comp));
		return 1;
	}

	wiLua::SError(L, "GetActiveComponent() Warning: type of active component not registered!");
	return 0;
}
int MainComponent_BindLua::SetActiveComponent(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetActiveComponent(RenderableComponent component) component is empty!");
		return 0;
	}

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int fadeFrames = 0;
		wiColor fadeColor = wiColor(0, 0, 0, 255);
		if (argc > 1)
		{
			fadeFrames = wiLua::SGetInt(L, 2);
			if (argc > 2)
			{
				fadeColor.r = wiLua::SGetInt(L, 3);
				if (argc > 3)
				{
					fadeColor.g = wiLua::SGetInt(L, 4);
					if (argc > 4)
					{
						fadeColor.b = wiLua::SGetInt(L, 5);
					}
				}
			}
		}

		ForwardRenderableComponent_BindLua* compFwd3D = Luna<ForwardRenderableComponent_BindLua>::lightcheck(L, 1);
		if (compFwd3D != nullptr)
		{
			component->activateComponent(compFwd3D->component, fadeFrames, fadeColor);
			return 0;
		}

		DeferredRenderableComponent_BindLua* compDef3D = Luna<DeferredRenderableComponent_BindLua>::lightcheck(L, 1);
		if (compDef3D != nullptr)
		{
			component->activateComponent(compDef3D->component, fadeFrames, fadeColor);
			return 0;
		}

		TiledDeferredRenderableComponent_BindLua* compTDef3D = Luna<TiledDeferredRenderableComponent_BindLua>::lightcheck(L, 1);
		if (compTDef3D != nullptr)
		{
			component->activateComponent(compTDef3D->component, fadeFrames, fadeColor);
			return 0;
		}

		TiledForwardRenderableComponent_BindLua* compTFwd3D = Luna<TiledForwardRenderableComponent_BindLua>::lightcheck(L, 1);
		if (compTFwd3D != nullptr)
		{
			component->activateComponent(compTFwd3D->component, fadeFrames, fadeColor);
			return 0;
		}

		Renderable3DComponent_BindLua* comp3D = Luna<Renderable3DComponent_BindLua>::lightcheck(L, 1);
		if (comp3D != nullptr)
		{
			component->activateComponent(comp3D->component, fadeFrames, fadeColor);
			return 0;
		}

		LoadingScreenComponent_BindLua* compLoad = Luna<LoadingScreenComponent_BindLua>::lightcheck(L, 1);
		if (compLoad != nullptr)
		{
			component->activateComponent(compLoad->component, fadeFrames, fadeColor);
			return 0;
		}

		Renderable2DComponent_BindLua* comp2D = Luna<Renderable2DComponent_BindLua>::lightcheck(L, 1);
		if (comp2D != nullptr)
		{
			component->activateComponent(comp2D->component, fadeFrames, fadeColor);
			return 0;
		}

		RenderableComponent_BindLua* comp = Luna<RenderableComponent_BindLua>::lightcheck(L, 1);
		if (comp != nullptr)
		{
			component->activateComponent(comp->component, fadeFrames, fadeColor);
			return 0;
		}
	}
	else
	{
		wiLua::SError(L, "SetActiveComponent(RenderableComponent component, opt int fadeFrames,fadeColorR,fadeColorG,fadeColorB) not enought arguments!");
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
int MainComponent_BindLua::SetCPUDisplay(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetCPUDisplay() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->infoDisplay.cpuinfo = wiLua::SGetBool(L, 1);
	}
	else
		wiLua::SError(L, "SetCPUDisplay(bool active) not enough arguments!");
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
int MainComponent_BindLua::SetColorGradePaletteDisplay(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetColorGradePaletteDisplay() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->colorGradingPaletteDisplayEnabled = wiLua::SGetBool(L, 1);
	}
	else
		wiLua::SError(L, "SetColorGradePaletteDisplay(bool active) not enough arguments!");
	return 0;
}


int SetProfilerEnabled(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiProfiler::GetInstance().ENABLED = wiLua::SGetBool(L, 1);
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
		Luna<MainComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState()); 
		
		wiLua::GetGlobal()->RegisterFunc("SetProfilerEnabled", SetProfilerEnabled);
	}
}
