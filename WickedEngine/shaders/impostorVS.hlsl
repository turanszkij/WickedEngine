#include "globals.hlsli"
#include "impostorHF.hlsli"

struct ImpostorPush
{
	uint instanceOffset;
};
PUSHCONSTANT(push, ImpostorPush);

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(-1, 1, 0),
	float3(1, -1, 0),
	float3(1, 1, 0),
};

ByteAddressBuffer impostorBuffer : register(t0);

VSOut main(uint fakeIndex : SV_VERTEXID)
{
	const uint vertexID = fakeIndex % 6;
	const uint instanceID = fakeIndex / 6;

	ShaderMeshInstancePointer poi = impostorBuffer.Load<ShaderMeshInstancePointer>(push.instanceOffset + instanceID * sizeof(ShaderMeshInstancePointer));
	ShaderMeshInstance instance = load_instance(poi.GetInstanceIndex());

	float3 pos = BILLBOARD[vertexID];
	float2 uv = float2(pos.xy * float2(0.5f, -0.5f) + 0.5f);
	uint slice = poi.GetFrustumIndex() * impostorCaptureAngles * 3;

	// We rotate the billboard to face camera, but unlike emitted particles, 
	//	they don't rotate according to camera rotation, but the camera position relative
	//	to the impostor (at least for now)
	float3 origin = instance.center;
	float3 up = float3(0, 1, 0);
	float3 face = GetCamera().position - origin;
	face.y = 0; // only rotate around Y axis!
	face = normalize(face);
	float3 right = normalize(cross(face, up));
	pos = mul(pos, float3x3(right, up, face));

	pos *= instance.radius;
	pos += instance.center;

	// Decide which slice to show according to billboard facing direction:
	float angle = acos(dot(face.xz, float2(0, 1))) / PI;
	if (cross(face, float3(0, 0, 1)).y < 0)
	{
		angle = 2 - angle;
	}
	angle *= 0.5f;
	angle = saturate(angle - 0.0001);
	slice += uint(angle * impostorCaptureAngles) * 3;

	VSOut Out;
	Out.pos3D = pos;
	Out.pos = mul(GetCamera().view_projection, float4(Out.pos3D, 1));
	Out.uv = uv;
	Out.slice = slice;
	Out.dither = poi.GetDither();
	Out.instanceColor = instance.color;

	return Out;
}
