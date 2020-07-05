#include "wiImageParams_BindLua.h"
#include "Vector_BindLua.h"

const char wiImageParams_BindLua::className[] = "ImageParams";

Luna<wiImageParams_BindLua>::FunctionType wiImageParams_BindLua::methods[] = {
	lunamethod(wiImageParams_BindLua, GetPos),
	lunamethod(wiImageParams_BindLua, GetSize),
	lunamethod(wiImageParams_BindLua, GetPivot),
	lunamethod(wiImageParams_BindLua, GetColor),
	lunamethod(wiImageParams_BindLua, GetOpacity),
	lunamethod(wiImageParams_BindLua, GetFade),
	lunamethod(wiImageParams_BindLua, GetRotation),
	lunamethod(wiImageParams_BindLua, GetTexOffset),
	lunamethod(wiImageParams_BindLua, GetTexOffset2),
	lunamethod(wiImageParams_BindLua, GetDrawRect),
	lunamethod(wiImageParams_BindLua, GetDrawRect2),
	lunamethod(wiImageParams_BindLua, IsDrawRectEnabled),
	lunamethod(wiImageParams_BindLua, IsDrawRect2Enabled),
	lunamethod(wiImageParams_BindLua, IsMirrorEnabled),
	lunamethod(wiImageParams_BindLua, IsBackgroundBlurEnabled),

	lunamethod(wiImageParams_BindLua, SetPos),
	lunamethod(wiImageParams_BindLua, SetSize),
	lunamethod(wiImageParams_BindLua, SetPivot),
	lunamethod(wiImageParams_BindLua, SetColor),
	lunamethod(wiImageParams_BindLua, SetOpacity),
	lunamethod(wiImageParams_BindLua, SetFade),
	lunamethod(wiImageParams_BindLua, SetStencil),
	lunamethod(wiImageParams_BindLua, SetStencilRefMode),
	lunamethod(wiImageParams_BindLua, SetBlendMode),
	lunamethod(wiImageParams_BindLua, SetQuality),
	lunamethod(wiImageParams_BindLua, SetSampleMode),
	lunamethod(wiImageParams_BindLua, SetRotation),
	lunamethod(wiImageParams_BindLua, SetTexOffset),
	lunamethod(wiImageParams_BindLua, SetTexOffset2),
	lunamethod(wiImageParams_BindLua, EnableDrawRect),
	lunamethod(wiImageParams_BindLua, EnableDrawRect2),
	lunamethod(wiImageParams_BindLua, DisableDrawRect),
	lunamethod(wiImageParams_BindLua, DisableDrawRect2),
	lunamethod(wiImageParams_BindLua, EnableMirror),
	lunamethod(wiImageParams_BindLua, DisableMirror),
	lunamethod(wiImageParams_BindLua, EnableBackgroundBlur),
	lunamethod(wiImageParams_BindLua, DisableBackgroundBlur),
	{ NULL, NULL }
};
Luna<wiImageParams_BindLua>::PropertyType wiImageParams_BindLua::properties[] = {
	{ NULL, NULL }
};


wiImageParams_BindLua::wiImageParams_BindLua(const wiImageParams& params) :params(params)
{
}

int wiImageParams_BindLua::GetPos(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&params.pos)));
	return 1;
}
int wiImageParams_BindLua::GetSize(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&params.siz)));
	return 1;
}
int wiImageParams_BindLua::GetPivot(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&params.pivot)));
	return 1;
}
int wiImageParams_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&params.color)));
	return 1;
}
int wiImageParams_BindLua::GetOpacity(lua_State* L)
{
	wiLua::SSetFloat(L, params.opacity);
	return 1;
}
int wiImageParams_BindLua::GetFade(lua_State* L)
{
	wiLua::SSetFloat(L, params.fade);
	return 1;
}
int wiImageParams_BindLua::GetRotation(lua_State* L)
{
	wiLua::SSetFloat(L, params.rotation);
	return 1;
}
int wiImageParams_BindLua::GetTexOffset(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&params.texOffset)));
	return 1;
}
int wiImageParams_BindLua::GetTexOffset2(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&params.texOffset2)));
	return 1;
}
int wiImageParams_BindLua::GetDrawRect(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&params.drawRect)));
	return 1;
}
int wiImageParams_BindLua::GetDrawRect2(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&params.drawRect2)));
	return 1;
}
int wiImageParams_BindLua::IsDrawRectEnabled(lua_State* L)
{
	wiLua::SSetBool(L, params.isDrawRectEnabled());
	return 1;
}
int wiImageParams_BindLua::IsDrawRect2Enabled(lua_State* L)
{
	wiLua::SSetBool(L, params.isDrawRect2Enabled());
	return 1;
}
int wiImageParams_BindLua::IsMirrorEnabled(lua_State* L)
{
	wiLua::SSetBool(L, params.isMirrorEnabled());
	return 1;
}
int wiImageParams_BindLua::IsBackgroundBlurEnabled(lua_State* L)
{
	wiLua::SSetBool(L, params.isBackgroundBlurEnabled());
	return 1;
}

int wiImageParams_BindLua::SetPos(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat3(&params.pos, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetPos(Vector pos) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetSize(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat2(&params.siz, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetSize(Vector size) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetPivot(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat2(&params.pivot, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetPivot(Vector value) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			XMStoreFloat4(&params.color, param->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetColor(Vector value) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetOpacity(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.opacity = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetOpacity(float x) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetFade(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.fade = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetFade(float x) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetStencil(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		params.stencilComp = (STENCILMODE)wiLua::SGetInt(L, 1);
		params.stencilRef = (uint8_t)wiLua::SGetInt(L, 2);
	}
	else
	{
		wiLua::SError(L, "SetStencil(int stencilmode,stencilref) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetStencilRefMode(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.stencilRefMode = (STENCILREFMODE)wiLua::SGetInt(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetStencilRefMode(int stencilrefmode) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetBlendMode(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.blendFlag = (BLENDMODE)wiLua::SGetInt(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetBlendMode(int blendMode) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetQuality(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.quality = (QUALITY)wiLua::SGetInt(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetQuality(int quality) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetSampleMode(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.sampleFlag = (SAMPLEMODE)wiLua::SGetInt(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetSampleMode(int sampleMode) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetRotation(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		params.rotation = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRotation(float x) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetTexOffset(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat2(&params.texOffset, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetTexOffset(Vector value) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::SetTexOffset2(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat2(&params.texOffset2, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetTexOffset2(Vector value) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::EnableDrawRect(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMFLOAT4 drawrect;
			XMStoreFloat4(&drawrect, vector->vector);
			params.enableDrawRect(drawrect);
		}
	}
	else
	{
		wiLua::SError(L, "EnableDrawRect(Vector value) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::EnableDrawRect2(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMFLOAT4 drawrect2;
			XMStoreFloat4(&drawrect2, vector->vector);
			params.enableDrawRect2(drawrect2);
		}
	}
	else
	{
		wiLua::SError(L, "EnableDrawRect2(Vector value) not enough arguments!");
	}
	return 0;
}
int wiImageParams_BindLua::DisableDrawRect(lua_State* L)
{
	params.disableDrawRect();
	return 0;
}
int wiImageParams_BindLua::DisableDrawRect2(lua_State* L)
{
	params.disableDrawRect2();
	return 0;
}
int wiImageParams_BindLua::EnableMirror(lua_State* L)
{
	params.enableMirror();
	return 0;
}
int wiImageParams_BindLua::DisableMirror(lua_State* L)
{
	params.disableMirror();
	return 0;
}
int wiImageParams_BindLua::EnableBackgroundBlur(lua_State* L)
{
	params.enableBackgroundBlur();
	return 0;
}
int wiImageParams_BindLua::DisableBackgroundBlur(lua_State* L)
{
	params.disableBackgroundBlur();
	return 0;
}

wiImageParams_BindLua::wiImageParams_BindLua(lua_State *L)
{
	float x = 0, y = 0, w = 100, h = 100;
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		if (argc < 3)//w
		{
			w = wiLua::SGetFloat(L, 1);
			if (argc > 1)//h
			{
				h = wiLua::SGetFloat(L, 2);
			}
		}
		else{//x,y,w
			x = wiLua::SGetFloat(L, 1);
			y = wiLua::SGetFloat(L, 2);
			w = wiLua::SGetFloat(L, 3);
			if (argc > 3)//h
			{
				h = wiLua::SGetFloat(L, 4);
			}
		}
	}
	params = wiImageParams(x, y, w, h);
}


void wiImageParams_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiImageParams_BindLua>::Register(wiLua::GetLuaState());

		wiLua::RunText("STENCILMODE_DISABLED		= 0");
		wiLua::RunText("STENCILMODE_EQUAL			= 1");
		wiLua::RunText("STENCILMODE_LESS			= 2");
		wiLua::RunText("STENCILMODE_LESSEQUAL		= 3");
		wiLua::RunText("STENCILMODE_GREATER		= 4");
		wiLua::RunText("STENCILMODE_GREATEREQUAL	= 5");
		wiLua::RunText("STENCILMODE_NOT			= 6");
		wiLua::RunText("STENCILMODE_ALWAYS			= 7");

		wiLua::RunText("STENCILREFMODE_ENGINE		= 0");
		wiLua::RunText("STENCILREFMODE_USER		= 1");
		wiLua::RunText("STENCILREFMODE_ALL			= 2");

		wiLua::RunText("SAMPLEMODE_CLAMP			= 0");
		wiLua::RunText("SAMPLEMODE_WRAP			= 1");
		wiLua::RunText("SAMPLEMODE_MIRROR			= 2");

		wiLua::RunText("QUALITY_NEAREST			= 0");
		wiLua::RunText("QUALITY_LINEAR				= 1");
		wiLua::RunText("QUALITY_ANISOTROPIC		= 2");
		wiLua::RunText("QUALITY_BICUBIC			= 3");

		wiLua::RunText("BLENDMODE_OPAQUE			= 0");
		wiLua::RunText("BLENDMODE_ALPHA			= 1");
		wiLua::RunText("BLENDMODE_PREMULTIPLIED	= 2");
		wiLua::RunText("BLENDMODE_ADDITIVE			= 3");
	}
}

