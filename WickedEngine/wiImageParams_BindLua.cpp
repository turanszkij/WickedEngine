#include "wiImageParams_BindLua.h"
#include "wiMath_BindLua.h"

namespace wi::lua
{
	Luna<ImageParams_BindLua>::FunctionType ImageParams_BindLua::methods[] = {
		lunamethod(ImageParams_BindLua, GetPos),
		lunamethod(ImageParams_BindLua, GetSize),
		lunamethod(ImageParams_BindLua, GetPivot),
		lunamethod(ImageParams_BindLua, GetColor),
		lunamethod(ImageParams_BindLua, GetOpacity),
		lunamethod(ImageParams_BindLua, GetFade),
		lunamethod(ImageParams_BindLua, GetRotation),
		lunamethod(ImageParams_BindLua, GetTexOffset),
		lunamethod(ImageParams_BindLua, GetTexOffset2),
		lunamethod(ImageParams_BindLua, GetDrawRect),
		lunamethod(ImageParams_BindLua, GetDrawRect2),
		lunamethod(ImageParams_BindLua, IsDrawRectEnabled),
		lunamethod(ImageParams_BindLua, IsDrawRect2Enabled),
		lunamethod(ImageParams_BindLua, IsMirrorEnabled),
		lunamethod(ImageParams_BindLua, IsBackgroundBlurEnabled),
		lunamethod(ImageParams_BindLua, IsBackgroundEnabled),

		lunamethod(ImageParams_BindLua, SetPos),
		lunamethod(ImageParams_BindLua, SetSize),
		lunamethod(ImageParams_BindLua, SetPivot),
		lunamethod(ImageParams_BindLua, SetColor),
		lunamethod(ImageParams_BindLua, SetOpacity),
		lunamethod(ImageParams_BindLua, SetFade),
		lunamethod(ImageParams_BindLua, SetStencil),
		lunamethod(ImageParams_BindLua, SetStencilRefMode),
		lunamethod(ImageParams_BindLua, SetBlendMode),
		lunamethod(ImageParams_BindLua, SetQuality),
		lunamethod(ImageParams_BindLua, SetSampleMode),
		lunamethod(ImageParams_BindLua, SetRotation),
		lunamethod(ImageParams_BindLua, SetTexOffset),
		lunamethod(ImageParams_BindLua, SetTexOffset2),
		lunamethod(ImageParams_BindLua, EnableDrawRect),
		lunamethod(ImageParams_BindLua, EnableDrawRect2),
		lunamethod(ImageParams_BindLua, DisableDrawRect),
		lunamethod(ImageParams_BindLua, DisableDrawRect2),
		lunamethod(ImageParams_BindLua, EnableMirror),
		lunamethod(ImageParams_BindLua, DisableMirror),
		lunamethod(ImageParams_BindLua, EnableBackgroundBlur),
		lunamethod(ImageParams_BindLua, DisableBackgroundBlur),
		lunamethod(ImageParams_BindLua, EnableBackground),
		lunamethod(ImageParams_BindLua, DisableBackground),
		{ nullptr, nullptr }
	};
	Luna<ImageParams_BindLua>::PropertyType ImageParams_BindLua::properties[] = {
		lunaproperty(ImageParams_BindLua, Pos),
		lunaproperty(ImageParams_BindLua, Size),
		lunaproperty(ImageParams_BindLua, Pivot),
		lunaproperty(ImageParams_BindLua, Color),
		lunaproperty(ImageParams_BindLua, Opacity),
		lunaproperty(ImageParams_BindLua, Fade),
		lunaproperty(ImageParams_BindLua, Rotation),
		lunaproperty(ImageParams_BindLua, TexOffset),
		lunaproperty(ImageParams_BindLua, TexOffset2),
		{ nullptr, nullptr }
	};


	ImageParams_BindLua::ImageParams_BindLua(const wi::image::Params& params) :params(params)
	{
	}

	int ImageParams_BindLua::GetPos(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&params.pos));
		return 1;
	}
	int ImageParams_BindLua::GetSize(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat2(&params.siz));
		return 1;
	}
	int ImageParams_BindLua::GetPivot(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat2(&params.pivot));
		return 1;
	}
	int ImageParams_BindLua::GetColor(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat4(&params.color));
		return 1;
	}
	int ImageParams_BindLua::GetOpacity(lua_State* L)
	{
		wi::lua::SSetFloat(L, params.opacity);
		return 1;
	}
	int ImageParams_BindLua::GetFade(lua_State* L)
	{
		wi::lua::SSetFloat(L, params.fade);
		return 1;
	}
	int ImageParams_BindLua::GetRotation(lua_State* L)
	{
		wi::lua::SSetFloat(L, params.rotation);
		return 1;
	}
	int ImageParams_BindLua::GetTexOffset(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat2(&params.texOffset));
		return 1;
	}
	int ImageParams_BindLua::GetTexOffset2(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat2(&params.texOffset2));
		return 1;
	}
	int ImageParams_BindLua::GetDrawRect(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat4(&params.drawRect));
		return 1;
	}
	int ImageParams_BindLua::GetDrawRect2(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat4(&params.drawRect2));
		return 1;
	}
	int ImageParams_BindLua::IsDrawRectEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, params.isDrawRectEnabled());
		return 1;
	}
	int ImageParams_BindLua::IsDrawRect2Enabled(lua_State* L)
	{
		wi::lua::SSetBool(L, params.isDrawRect2Enabled());
		return 1;
	}
	int ImageParams_BindLua::IsMirrorEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, params.isMirrorEnabled());
		return 1;
	}
	int ImageParams_BindLua::IsBackgroundBlurEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, params.isBackgroundEnabled());
		return 1;
	}
	int ImageParams_BindLua::IsBackgroundEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, params.isBackgroundEnabled());
		return 1;
	}

	int ImageParams_BindLua::SetPos(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMStoreFloat3(&params.pos, XMLoadFloat4(&vector->data));
			}
		}
		else
		{
			wi::lua::SError(L, "SetPos(Vector pos) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetSize(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMStoreFloat2(&params.siz, XMLoadFloat4(&vector->data));
			}
		}
		else
		{
			wi::lua::SError(L, "SetSize(Vector size) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetPivot(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMStoreFloat2(&params.pivot, XMLoadFloat4(&vector->data));
			}
		}
		else
		{
			wi::lua::SError(L, "SetPivot(Vector value) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetColor(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (param != nullptr)
			{
				XMStoreFloat4(&params.color, XMLoadFloat4(&param->data));
			}
		}
		else
		{
			wi::lua::SError(L, "SetColor(Vector value) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetOpacity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.opacity = wi::lua::SGetFloat(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetOpacity(float x) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetFade(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.fade = wi::lua::SGetFloat(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetFade(float x) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetStencil(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			params.stencilComp = (wi::image::STENCILMODE)wi::lua::SGetInt(L, 1);
			params.stencilRef = (uint8_t)wi::lua::SGetInt(L, 2);
		}
		else
		{
			wi::lua::SError(L, "SetStencil(int stencilmode,stencilref) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetStencilRefMode(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.stencilRefMode = (wi::image::STENCILREFMODE)wi::lua::SGetInt(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetStencilRefMode(int stencilrefmode) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetBlendMode(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.blendFlag = (wi::enums::BLENDMODE)wi::lua::SGetInt(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetBlendMode(int blendMode) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetQuality(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.quality = (wi::image::QUALITY)wi::lua::SGetInt(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetQuality(int quality) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetSampleMode(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.sampleFlag = (wi::image::SAMPLEMODE)wi::lua::SGetInt(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetSampleMode(int sampleMode) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetRotation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			params.rotation = wi::lua::SGetFloat(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetRotation(float x) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetTexOffset(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMStoreFloat2(&params.texOffset, XMLoadFloat4(&vector->data));
			}
		}
		else
		{
			wi::lua::SError(L, "SetTexOffset(Vector value) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::SetTexOffset2(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMStoreFloat2(&params.texOffset2, XMLoadFloat4(&vector->data));
			}
		}
		else
		{
			wi::lua::SError(L, "SetTexOffset2(Vector value) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::EnableDrawRect(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMFLOAT4 drawrect;
				XMStoreFloat4(&drawrect, XMLoadFloat4(&vector->data));
				params.enableDrawRect(drawrect);
			}
		}
		else
		{
			wi::lua::SError(L, "EnableDrawRect(Vector value) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::EnableDrawRect2(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				XMFLOAT4 drawrect2;
				XMStoreFloat4(&drawrect2, XMLoadFloat4(&vector->data));
				params.enableDrawRect2(drawrect2);
			}
		}
		else
		{
			wi::lua::SError(L, "EnableDrawRect2(Vector value) not enough arguments!");
		}
		return 0;
	}
	int ImageParams_BindLua::DisableDrawRect(lua_State* L)
	{
		params.disableDrawRect();
		return 0;
	}
	int ImageParams_BindLua::DisableDrawRect2(lua_State* L)
	{
		params.disableDrawRect2();
		return 0;
	}
	int ImageParams_BindLua::EnableMirror(lua_State* L)
	{
		params.enableMirror();
		return 0;
	}
	int ImageParams_BindLua::DisableMirror(lua_State* L)
	{
		params.disableMirror();
		return 0;
	}
	int ImageParams_BindLua::EnableBackgroundBlur(lua_State* L)
	{
		params.enableBackground();
		return 0;
	}
	int ImageParams_BindLua::DisableBackgroundBlur(lua_State* L)
	{
		params.disableBackground();
		return 0;
	}
	int ImageParams_BindLua::EnableBackground(lua_State* L)
	{
		params.enableBackground();
		return 0;
	}
	int ImageParams_BindLua::DisableBackground(lua_State* L)
	{
		params.disableBackground();
		return 0;
	}

	ImageParams_BindLua::ImageParams_BindLua(lua_State* L)
	{
		float x = 0, y = 0, w = 100, h = 100;
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			if (argc < 3)//w
			{
				w = wi::lua::SGetFloat(L, 1);
				if (argc > 1)//h
				{
					h = wi::lua::SGetFloat(L, 2);
				}
			}
			else {//x,y,w
				x = wi::lua::SGetFloat(L, 1);
				y = wi::lua::SGetFloat(L, 2);
				w = wi::lua::SGetFloat(L, 3);
				if (argc > 3)//h
				{
					h = wi::lua::SGetFloat(L, 4);
				}
			}
		}
		params = wi::image::Params(x, y, w, h);
	}


	void ImageParams_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<ImageParams_BindLua>::Register(wi::lua::GetLuaState());

			wi::lua::RunText(R"(
STENCILMODE_DISABLED		= 0
STENCILMODE_EQUAL			= 1
STENCILMODE_LESS			= 2
STENCILMODE_LESSEQUAL		= 3
STENCILMODE_GREATER			= 4
STENCILMODE_GREATEREQUAL	= 5
STENCILMODE_NOT				= 6
STENCILMODE_ALWAYS			= 7

STENCILREFMODE_ENGINE		= 0
STENCILREFMODE_USER			= 1
STENCILREFMODE_ALL			= 2

SAMPLEMODE_CLAMP			= 0
SAMPLEMODE_WRAP				= 1
SAMPLEMODE_MIRROR			= 2

QUALITY_NEAREST				= 0
QUALITY_LINEAR				= 1
QUALITY_ANISOTROPIC			= 2
QUALITY_BICUBIC				= 3

BLENDMODE_OPAQUE			= 0
BLENDMODE_ALPHA				= 1
BLENDMODE_PREMULTIPLIED		= 2
BLENDMODE_ADDITIVE			= 3
)");

		}
	}

}
