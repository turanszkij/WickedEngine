#include "globals.hlsli"
#include "impostorHF.hlsli"

static const float2 BILLBOARD[] = {
	float2(-1, -1),
	float2(1, -1),
	float2(-1, 1),
	float2(1, 1),
};

Buffer<float4> vb_pos : register(t0);
ByteAddressBuffer impostor_data : register(t2);

struct ImpostorPush
{
	uint camera_index;
	uint shadow;
};
PUSHCONSTANT(push, ImpostorPush);

VSOut main(uint vertexID : SV_VertexID, out uint VPIndex : SV_ViewportArrayIndex)
{
	VPIndex = push.camera_index;
	ShaderCamera camera = GetCameraIndexed(push.camera_index);

	uint impostorID = vertexID / 4u;
	uint2 data = impostor_data.Load2(impostorID * sizeof(uint2));
	float3 pos3D = vb_pos[vertexID].xyz;

	[branch]
	if (push.shadow)
	{
		// Directional shadow reuses the main camera billboards, but they are rotated to face the light (shadow camera):
		const float3 corner0 = vb_pos[impostorID * 4u + 0].xyz;
		const float3 corner3 = vb_pos[impostorID * 4u + 3].xyz;
		const float3 center = (corner0 + corner3) * 0.5;
		const float radius = length(corner3 - corner0) * 0.35355339; // half diagonal / sqrt(2)
		const float3 face = -camera.forward;
		float3 right = cross(face, float3(0, 1, 0));
		right = dot(right, right) > 0.000001 ? normalize(right) : float3(1, 0, 0); // light straight above
		const float3 up = cross(right, face);
		const float2 corner = BILLBOARD[vertexID % 4u];
		pos3D = center + (corner.x * right + corner.y * up) * radius;
	}

	VSOut Out;
	Out.clip = dot(float4(pos3D, 1), camera.clip_plane);
	Out.pos = mul(camera.view_projection, float4(pos3D, 1));
	Out.uv = float2(BILLBOARD[vertexID % 4u] * float2(0.5f, -0.5f) + 0.5f);
	Out.slice = data.x & 0xFFFFFF;
	Out.dither = float((data.x >> 24u) & 0xFF) / 255.0;
	Out.instanceColor = data.y;

	PrimitiveID prim;
	prim.init();
	prim.primitiveIndex = vertexID / 2u;
	prim.instanceIndex = GetScene().impostorInstanceOffset;
	prim.subsetIndex = 0;
	Out.primitiveID = prim.pack();

	return Out;
}
