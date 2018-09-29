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

float GetAngle(float2 a, float2 b)
{
	float dot = a.x*b.x + a.y*b.y;      // dot product
	float det = a.x*b.y - a.y*b.x;		// determinant
	float angle = atan2(det, dot);		// atan2(y, x) or atan2(sin, cos)
	if (angle < 0)
	{
		angle += PI * 2;
	}
	return angle;
}

VSOut main(uint fakeIndex : SV_VERTEXID)
{
	const uint vertexID = fakeIndex % 6;
	const uint instanceID = fakeIndex / 6;

	uint byteOffset = (uint)g_xColor.x + instanceID * 64;

	Input_Instance instance;
	instance.wi0 = asfloat(instanceBuffer.Load4(byteOffset + 0));
	instance.wi1 = asfloat(instanceBuffer.Load4(byteOffset + 16));
	instance.wi2 = asfloat(instanceBuffer.Load4(byteOffset + 32));
	instance.color_dither = asfloat(instanceBuffer.Load4(byteOffset + 48));

	float4x4 WORLD = MakeWorldMatrixFromInstance(instance);

	float3 pos = BILLBOARD[vertexID];
	float3 tex = float3(pos.xy * float2(0.5f, -0.5f) + 0.5f, instance.color_dither.x); // here color_dither.x is texture arrayindex for now...

	// We rotate the billboard to face camera, but unlike emitted particles, 
	//	they don't rotate according to camera rotation, but the camera position relative
	//	to the impostor (at least for now)
	float3 origin = mul(float4(0, 0, 0, 1), WORLD).xyz;
	float3 up = normalize(mul(float3(0, 1, 0), (float3x3)WORLD));
	float3 face = normalize(g_xCamera_CamPos - origin);
	float3 right = normalize(cross(face, up));
	pos = mul(pos, float3x3(right, up, face));

	// Decide which slice to show according to billboard facing direction (todo fix this up!):
	float angle = GetAngle(face.xz, float2(-1, 0)) / PI / 2.0f;
	tex.z += floor((1 - saturate(angle)) * impostorCaptureAngles);

	VSOut Out;
	Out.pos3D = mul(float4(pos, 1), WORLD).xyz;
	Out.pos = mul(float4(Out.pos3D, 1), g_xCamera_VP);
	Out.tex = tex;
	Out.dither = instance.color_dither.a;
	Out.instanceColor = 0xFFFFFFFF; // todo
	Out.nor = face;
	Out.pos2D = Out.pos;
	Out.pos2DPrev = mul(float4(Out.pos3D, 1), g_xFrame_MainCamera_PrevVP);

	return Out;
}
