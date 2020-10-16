#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectInputLayoutHF.hlsli"

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(-1, 1, 0),
	float3(1, -1, 0),
	float3(1, 1, 0),
};

RAWBUFFER(instanceBuffer, TEXSLOT_ONDEMAND21);

VSOut main(uint fakeIndex : SV_VERTEXID)
{
	const uint vertexID = fakeIndex % 6;
	const uint instanceID = fakeIndex / 6;

	uint byteOffset = (uint)g_xColor.x + instanceID * 64;

	Input_Instance instance;
	instance.mat0 = asfloat(instanceBuffer.Load4(byteOffset + 0));
	instance.mat1 = asfloat(instanceBuffer.Load4(byteOffset + 16));
	instance.mat2 = asfloat(instanceBuffer.Load4(byteOffset + 32));
	instance.userdata = instanceBuffer.Load4(byteOffset + 48);

	float4x4 WORLD = MakeWorldMatrixFromInstance(instance);

	float3 pos = BILLBOARD[vertexID];
	float3 tex = float3(pos.xy * float2(0.5f, -0.5f) + 0.5f, instance.userdata.y);

	// We rotate the billboard to face camera, but unlike emitted particles, 
	//	they don't rotate according to camera rotation, but the camera position relative
	//	to the impostor (at least for now)
	float3 origin = mul(WORLD, float4(0, 0, 0, 1)).xyz;
	float3 up = normalize(mul((float3x3)WORLD, float3(0, 1, 0)));
	float3 face = mul((float3x3)WORLD, g_xCamera_CamPos - origin);
	face.y = 0; // only rotate around Y axis!
	face = normalize(face);
	float3 right = normalize(cross(face, up));
	pos = mul(pos, float3x3(right, up, face));

	// Decide which slice to show according to billboard facing direction:
	float angle = acos(dot(face.xz, float2(0, 1))) / PI;
	if (cross(face, float3(0, 0, 1)).y < 0)
	{
		angle = 2 - angle;
	}
	angle *= 0.5f;
	tex.z += floor(angle * impostorCaptureAngles);

	float4 color_dither = unpack_rgba(instance.userdata.x);

	VSOut Out;
	Out.pos3D = mul(WORLD, float4(pos, 1)).xyz;
	Out.pos = mul(g_xCamera_VP, float4(Out.pos3D, 1));
	Out.tex = tex;
	Out.dither = 1 - color_dither.a;
	Out.instanceColor = pack_rgba(float4(color_dither.rgb, 1));
	Out.pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, float4(Out.pos3D, 1));

	return Out;
}
