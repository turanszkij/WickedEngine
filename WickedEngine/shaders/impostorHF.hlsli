#ifndef WI_IMPOSTOR_HF
#define WI_IMPOSTOR_HF

struct VSOut
{
	precise float4 pos				: SV_Position;
	float clip						: SV_ClipDistance0;
	float2 uv						: TEXCOORD;
	uint slice						: SLICE;
	nointerpolation float dither	: DITHER;
	uint instanceColor				: COLOR;
	uint primitiveID				: PRIMITIVEID;
	
	inline float3 GetPos3D()
	{
		return GetCamera().screen_to_world(pos);
	}

	inline float3 GetViewVector()
	{
		return GetCamera().screen_to_nearplane(pos) - GetPos3D(); // ortho support, cannot use cameraPos!
	}

	inline half GetDither()
	{
		return dither;
	}
};

Texture2DArray<float4> impostorTex : register(t1);

#endif // WI_IMPOSTOR_HF
