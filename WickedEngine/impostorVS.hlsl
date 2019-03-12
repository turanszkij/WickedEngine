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

RAWBUFFER(instanceBuffer, TEXSLOT_ONDEMAND0);

VSOut main(uint fakeIndex : SV_VERTEXID)
{
	const uint vertexID = fakeIndex % 6;
	const uint instanceID = fakeIndex / 6;

	uint byteOffset = (uint)g_xColor.x + instanceID * 64;

	Input_Instance instance;
	instance.mat0 = asfloat(instanceBuffer.Load4(byteOffset + 0));
	instance.mat1 = asfloat(instanceBuffer.Load4(byteOffset + 16));
	instance.mat2 = asfloat(instanceBuffer.Load4(byteOffset + 32));
	instance.color = asfloat(instanceBuffer.Load4(byteOffset + 48));

	float4x4 WORLD = MakeWorldMatrixFromInstance(instance);

	float3 pos = BILLBOARD[vertexID];
	float3 tex = float3(pos.xy * float2(0.5f, -0.5f) + 0.5f, instance.color.x); // here color.x is texture arrayindex for now...

	// We rotate the billboard to face camera, but unlike emitted particles, 
	//	they don't rotate according to camera rotation, but the camera position relative
	//	to the impostor (at least for now)
	float3 origin = mul(float4(0, 0, 0, 1), WORLD).xyz;
	float3 up = normalize(mul(float3(0, 1, 0), (float3x3)WORLD));
	float3 face = normalize(g_xCamera_CamPos - origin);
	float3 right = normalize(cross(face, up));
	pos = mul(pos, float3x3(right, up, face));

	// Decide which slice to show according to billboard facing direction:
	float angle = acos(dot(face.xz, float2(0, -1))) / PI;
	if (cross(face, float3(0, 0, -1)).y < 0)
	{
		angle = 2 - angle;
	}
	angle *= 0.5f;
	tex.z += floor(saturate(angle) * impostorCaptureAngles);

	VSOut Out;
	Out.pos3D = mul(float4(pos, 1), WORLD).xyz;
	Out.pos = mul(float4(Out.pos3D, 1), g_xCamera_VP);
	Out.tex = tex;
	Out.dither = 1 - instance.color.a;
	Out.instanceColor = 0xFFFFFFFF; // todo
	Out.pos2D = Out.pos;
	Out.pos2DPrev = mul(float4(Out.pos3D, 1), g_xFrame_MainCamera_PrevVP);

	return Out;
}
