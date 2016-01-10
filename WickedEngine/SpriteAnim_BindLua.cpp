#include "SpriteAnim_BindLua.h"
#include "Vector_BindLua.h"

const char SpriteAnim_BindLua::className[] = "SpriteAnim";

Luna<SpriteAnim_BindLua>::FunctionType SpriteAnim_BindLua::methods[] = {
	lunamethod(SpriteAnim_BindLua, SetRot),
	lunamethod(SpriteAnim_BindLua, SetRotation),
	lunamethod(SpriteAnim_BindLua, SetOpacity),
	lunamethod(SpriteAnim_BindLua, SetFade),
	lunamethod(SpriteAnim_BindLua, SetRepeatable),
	lunamethod(SpriteAnim_BindLua, SetVelocity),
	lunamethod(SpriteAnim_BindLua, SetScaleX),
	lunamethod(SpriteAnim_BindLua, SetScaleY),
	lunamethod(SpriteAnim_BindLua, SetMovingTexAnim),
	lunamethod(SpriteAnim_BindLua, SetDrawRecAnim),

	lunamethod(SpriteAnim_BindLua, GetRot),
	lunamethod(SpriteAnim_BindLua, GetRotation),
	lunamethod(SpriteAnim_BindLua, GetOpacity),
	lunamethod(SpriteAnim_BindLua, GetFade),
	lunamethod(SpriteAnim_BindLua, GetRepeatable),
	lunamethod(SpriteAnim_BindLua, GetVelocity),
	lunamethod(SpriteAnim_BindLua, GetScaleX),
	lunamethod(SpriteAnim_BindLua, GetScaleY),
	lunamethod(SpriteAnim_BindLua, GetMovingTexAnim),
	lunamethod(SpriteAnim_BindLua, GetDrawRecAnim),
	{ NULL, NULL }
};
Luna<SpriteAnim_BindLua>::PropertyType SpriteAnim_BindLua::properties[] = {
	{ NULL, NULL }
};

SpriteAnim_BindLua::SpriteAnim_BindLua(const wiSprite::Anim& anim) :anim(anim)
{
}

SpriteAnim_BindLua::SpriteAnim_BindLua(lua_State *L)
{
	anim = wiSprite::Anim();
}

SpriteAnim_BindLua::~SpriteAnim_BindLua()
{
}


int SpriteAnim_BindLua::SetRot(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.rot = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRot(float rot) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetRotation(lua_State *L)
{
	return SetRot(L);
}
int SpriteAnim_BindLua::SetOpacity(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.opa = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetOpacity(float val) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetFade(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.fad = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetFade(float val) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetRepeatable(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.repeatable = wiLua::SGetBool(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRepeatable(float val) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetVelocity(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMStoreFloat3(&anim.vel, vec->vector);
		}
		else 
		{
			wiLua::SError(L, "SetVelocity(Vector val) argument is not a Vector!");
		}
	}
	else
	{
		wiLua::SError(L, "SetVelocity(Vector val) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetScaleX(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.scaleX = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetScaleX(float val) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetScaleY(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.scaleY = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetScaleY(float val) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetMovingTexAnim(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		MovingTexData_BindLua* data = Luna<MovingTexData_BindLua>::lightcheck(L, 1);
		if (data != nullptr)
		{
			anim.movingTexAnim = data->data;
		}
		else
		{
			wiLua::SError(L, "SetMovingTexAnim(MovingTexAnim data) argument is not a MovingTexAnim!");
		}
	}
	else
	{
		wiLua::SError(L, "SetMovingTexAnim(MovingTexAnim data) not enough arguments!");
	}
	return 0;
}
int SpriteAnim_BindLua::SetDrawRecAnim(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		DrawRecData_BindLua* data = Luna<DrawRecData_BindLua>::lightcheck(L, 1);
		if (data != nullptr)
		{
			anim.drawRecAnim = data->data;
		}
		else
		{
			wiLua::SError(L, "SetDrawRecAnim(DrawRecAnim data) argument is not a DrawRecAnim!");
		}
	}
	else
	{
		wiLua::SError(L, "SetDrawRecAnim(DrawRecAnim data) not enough arguments!");
	}
	return 0;
}

int SpriteAnim_BindLua::GetRot(lua_State *L)
{
	wiLua::SSetFloat(L, anim.rot);
	return 1;
}
int SpriteAnim_BindLua::GetRotation(lua_State *L)
{
	return GetRot(L);
}
int SpriteAnim_BindLua::GetOpacity(lua_State *L)
{
	wiLua::SSetFloat(L, anim.opa);
	return 1;
}
int SpriteAnim_BindLua::GetFade(lua_State *L)
{
	wiLua::SSetFloat(L, anim.fad);
	return 1;
}
int SpriteAnim_BindLua::GetRepeatable(lua_State *L)
{
	wiLua::SSetBool(L, anim.repeatable);
	return 1;
}
int SpriteAnim_BindLua::GetVelocity(lua_State *L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&anim.vel)));
	return 1;
}
int SpriteAnim_BindLua::GetScaleX(lua_State *L)
{
	wiLua::SSetFloat(L, anim.scaleX);
	return 1;
}
int SpriteAnim_BindLua::GetScaleY(lua_State *L)
{
	wiLua::SSetFloat(L, anim.scaleY);
	return 1;
}
int SpriteAnim_BindLua::GetMovingTexAnim(lua_State *L)
{
	Luna<MovingTexData_BindLua>::push(L, new MovingTexData_BindLua(anim.movingTexAnim));
	return 1;
}
int SpriteAnim_BindLua::GetDrawRecAnim(lua_State *L)
{
	Luna<DrawRecData_BindLua>::push(L, new DrawRecData_BindLua(anim.drawRecAnim));
	return 1;
}

void SpriteAnim_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<SpriteAnim_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
		Luna<MovingTexData_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
		Luna<DrawRecData_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char MovingTexData_BindLua::className[] = "MovingTexAnim";

Luna<MovingTexData_BindLua>::FunctionType MovingTexData_BindLua::methods[] = {
	lunamethod(MovingTexData_BindLua, SetSpeedX),
	lunamethod(MovingTexData_BindLua, SetSpeedY),

	lunamethod(MovingTexData_BindLua, GetSpeedX),
	lunamethod(MovingTexData_BindLua, GetSpeedY),
	{ NULL, NULL }
};
Luna<MovingTexData_BindLua>::PropertyType MovingTexData_BindLua::properties[] = {
	{ NULL, NULL }
};

MovingTexData_BindLua::MovingTexData_BindLua(const wiSprite::Anim::MovingTexData& data) :data(data)
{
}

MovingTexData_BindLua::MovingTexData_BindLua(lua_State *L)
{
	data = wiSprite::Anim::MovingTexData();
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.speedX = wiLua::SGetFloat(L, 1);
		if (argc > 1)
		{
			data.speedY = wiLua::SGetFloat(L, 2);
		}
	}
}

MovingTexData_BindLua::~MovingTexData_BindLua()
{
}

int MovingTexData_BindLua::SetSpeedX(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.speedX = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetSpeedX(float val) not enough arguments!");
	}
	return 0;
}
int MovingTexData_BindLua::SetSpeedY(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.speedY = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetSpeedY(float val) not enough arguments!");
	}
	return 0;
}

int MovingTexData_BindLua::GetSpeedX(lua_State *L)
{
	wiLua::SSetFloat(L, data.speedX);
	return 1;
}
int MovingTexData_BindLua::GetSpeedY(lua_State *L)
{
	wiLua::SSetFloat(L, data.speedY);
	return 1;
}



const char DrawRecData_BindLua::className[] = "DrawRecAnim";

Luna<DrawRecData_BindLua>::FunctionType DrawRecData_BindLua::methods[] = {
	lunamethod(DrawRecData_BindLua, SetOnFrameChangeWait),
	lunamethod(DrawRecData_BindLua, SetFrameCount),
	lunamethod(DrawRecData_BindLua, SetJumpX),
	lunamethod(DrawRecData_BindLua, SetJumpY),
	lunamethod(DrawRecData_BindLua, SetSizX),
	lunamethod(DrawRecData_BindLua, SetSizY),

	lunamethod(DrawRecData_BindLua, GetOnFrameChangeWait),
	lunamethod(DrawRecData_BindLua, GetFrameCount),
	lunamethod(DrawRecData_BindLua, GetJumpX),
	lunamethod(DrawRecData_BindLua, GetJumpY),
	lunamethod(DrawRecData_BindLua, GetSizX),
	lunamethod(DrawRecData_BindLua, GetSizY),
	{ NULL, NULL }
};
Luna<DrawRecData_BindLua>::PropertyType DrawRecData_BindLua::properties[] = {
	{ NULL, NULL }
};

DrawRecData_BindLua::DrawRecData_BindLua(const wiSprite::Anim::DrawRecData& data) :data(data)
{
}

DrawRecData_BindLua::DrawRecData_BindLua(lua_State *L)
{
	data = wiSprite::Anim::DrawRecData();
}

DrawRecData_BindLua::~DrawRecData_BindLua()
{
}

int DrawRecData_BindLua::SetOnFrameChangeWait(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.onFrameChangeWait = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetOnFrameChangeWait(float val) not enough arguments!");
	}
	return 0;
}
int DrawRecData_BindLua::SetFrameCount(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.frameCount = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetFrameCount(float val) not enough arguments!");
	}
	return 0;
}
int DrawRecData_BindLua::SetJumpX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.jumpX = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetJumpX(float val) not enough arguments!");
	}
	return 0;
}
int DrawRecData_BindLua::SetJumpY(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.jumpY = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetJumpY(float val) not enough arguments!");
	}
	return 0;
}
int DrawRecData_BindLua::SetSizX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.sizX = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetSizX(float val) not enough arguments!");
	}
	return 0;
}
int DrawRecData_BindLua::SetSizY(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		data.sizY = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetSizY(float val) not enough arguments!");
	}
	return 0;
}

int DrawRecData_BindLua::GetOnFrameChangeWait(lua_State* L)
{
	wiLua::SSetFloat(L, data.onFrameChangeWait);
	return 1;
}
int DrawRecData_BindLua::GetFrameCount(lua_State* L)
{
	wiLua::SSetFloat(L, data.frameCount);
	return 1;
}
int DrawRecData_BindLua::GetJumpX(lua_State* L)
{
	wiLua::SSetFloat(L, data.jumpX);
	return 1;
}
int DrawRecData_BindLua::GetJumpY(lua_State* L)
{
	wiLua::SSetFloat(L, data.jumpY);
	return 1;
}
int DrawRecData_BindLua::GetSizX(lua_State* L)
{
	wiLua::SSetFloat(L, data.sizX);
	return 1;
}
int DrawRecData_BindLua::GetSizY(lua_State* L)
{
	wiLua::SSetFloat(L, data.sizY);
	return 1;
}