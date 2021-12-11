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

	ShaderMeshInstancePointer poi = impostorBuffer.Load<ShaderMeshInstancePointer>(push.instanceOffset + instanceID * 8);
	ShaderMeshInstance instance = load_instance(poi.instanceID);
	ShaderMesh mesh = load_mesh(instance.meshIndex);
	float3 extents = mesh.aabb_max - mesh.aabb_min;
	float radius = max(extents.x, max(extents.y, extents.z)) * 0.5;

	float3 pos = BILLBOARD[vertexID];
	float2 uv = float2(pos.xy * float2(0.5f, -0.5f) + 0.5f);
	uint slice = poi.GetFrustumIndex();
	pos *= radius;

	// We rotate the billboard to face camera, but unlike emitted particles, 
	//	they don't rotate according to camera rotation, but the camera position relative
	//	to the impostor (at least for now)
	float3 origin = mul(instance.transform.GetMatrix(), float4(0, 0, 0, 1)).xyz;
	float3 up = normalize(mul((float3x3)instance.transform.GetMatrix(), float3(0, 1, 0)));
	float3 face = mul((float3x3)instance.transform.GetMatrix(), GetCamera().position - origin);
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
	angle = saturate(angle - 0.0001);
	slice += uint(angle * impostorCaptureAngles);

	VSOut Out;
	Out.pos3D = mul(instance.transform.GetMatrix(), float4(pos, 1)).xyz;
	Out.pos = mul(GetCamera().view_projection, float4(Out.pos3D, 1));
	Out.uv = uv;
	Out.slice = slice;
	Out.dither = poi.GetDither();
	Out.instanceID = poi.instanceID;

	return Out;
}
