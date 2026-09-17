#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

Buffer<uint> primitiveBuffer : register(t0);

struct HairShadowPush
{
	uint camera_index;
};
PUSHCONSTANT(push, HairShadowPush);

VertexToPixel main(uint vid : SV_VertexID, out uint VPIndex : SV_ViewportArrayIndex)
{
	VPIndex = push.camera_index;
	ShaderCamera camera = GetCameraIndexed(push.camera_index);
	ShaderMeshInstance inst = HairGetInstance();
	ShaderGeometry geometry = HairGetGeometry();

	VertexToPixel Out;
	Out.primitiveID = vid / 3;

	uint vertexID = primitiveBuffer[vid];
	float4 pos_wind = bindless_buffers_float4[descriptor_index(geometry.vb_pos_wind)][vertexID];
	float3 position = mul(inst.transform.GetMatrix(), float4(pos_wind.xyz, 1)).xyz;
	float3 normal = normalize(bindless_buffers_float4[descriptor_index(geometry.vb_nor)][vertexID].xyz);
	float4 uvsets = bindless_buffers_float4[descriptor_index(geometry.vb_uvs)][vertexID];

	Out.fade = saturate(distance(position.xyz, camera.position.xyz) / xHairViewDistance);
	Out.fade = saturate(Out.fade - 0.8f) * 5.0f; // fade will be on edge and inwards 20%

	Out.pos = float4(position, 1);
	Out.clip = dot(Out.pos, camera.clip_plane);
	Out.pos = mul(camera.view_projection, Out.pos);

	Out.nor_wet = half4(normal, (half)bindless_buffers_float[descriptor_index(inst.vb_wetmap)][vertexID]);
	Out.tex = uvsets.xy;
	
	return Out;
}
