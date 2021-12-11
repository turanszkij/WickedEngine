#ifndef WI_IMPOSTOR_HF
#define WI_IMPOSTOR_HF

struct VSOut
{
	float4 pos						: SV_POSITION;
	float2 uv						: TEXCOORD;
	uint slice						: SLICE;
	nointerpolation float dither	: DITHER;
	float3 pos3D					: WORLDPOSITION;
	uint instanceID					: INSTANCEID;
};

Texture2DArray<float4> impostorTex : register(t1);

#endif // WI_IMPOSTOR_HF
