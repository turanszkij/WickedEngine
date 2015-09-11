#include "MainComponent_BindLua.h"
#include "RenderableComponent_BindLua.h"
#include "Renderable3DComponent_BindLua.h"
#include "Renderable2DComponent_BindLua.h"
#include "DeferredRenderableComponent_BindLua.h"
#include "ForwardRenderableComponent_BindLua.h"
#include "LoadingScreenComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"

const char MainComponent_BindLua::className[] = "MainComponent";

Luna<MainComponent_BindLua>::FunctionType MainComponent_BindLua::methods[] = {
	lunamethod(MainComponent_BindLua, GetContent),
	lunamethod(MainComponent_BindLua, GetActiveComponent),
	lunamethod(MainComponent_BindLua, SetActiveComponent),
	lunamethod(MainComponent_BindLua, SetFrameSkip),
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
		ForwardRenderableComponent_BindLua* compFwd3D = Luna<ForwardRenderableComponent_BindLua>::lightcheck(L, 1);
		if (compFwd3D != nullptr)
		{
			component->activateComponent(compFwd3D->component);
			return 0;
		}

		DeferredRenderableComponent_BindLua* compDef3D = Luna<DeferredRenderableComponent_BindLua>::lightcheck(L, 1);
		if (compDef3D != nullptr)
		{
			component->activateComponent(compDef3D->component);
			return 0;
		}

		Renderable3DComponent_BindLua* comp3D = Luna<Renderable3DComponent_BindLua>::lightcheck(L, 1);
		if (comp3D != nullptr)
		{
			component->activateComponent(comp3D->component);
			return 0;
		}

		LoadingScreenComponent_BindLua* compLoad = Luna<LoadingScreenComponent_BindLua>::lightcheck(L, 1);
		if (compLoad != nullptr)
		{
			component->activateComponent(compLoad->component);
			return 0;
		}

		Renderable2DComponent_BindLua* comp2D = Luna<Renderable2DComponent_BindLua>::lightcheck(L, 1);
		if (comp2D != nullptr)
		{
			component->activateComponent(comp2D->component);
			return 0;
		}

		RenderableComponent_BindLua* comp = Luna<RenderableComponent_BindLua>::lightcheck(L, 1);
		if (comp != nullptr)
		{
			component->activateComponent(comp->component);
			return 0;
		}
	}
	else
	{
		wiLua::SError(L, "SetActiveComponent(RenderableComponent component) not enought arguments!");
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

void MainComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<MainComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
