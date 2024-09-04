#include "globals.hlsli"

struct Output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

Output main(uint vertexID : SV_VertexID)
{
	ShaderMeshInstance inst = load_instance(g_xPaintDecalInstanceID);
	ShaderGeometry geometry = load_geometry(inst.geometryOffset);
	
	float3 pos = 0;
	[branch]
	if(geometry.vb_pos_wind >= 0)
	{
		pos = bindless_buffers_float4[geometry.vb_pos_wind][vertexID].xyz;
	}
	pos = mul(inst.transform.GetMatrix(), float4(pos, 1)).xyz;
	
	float3 nor = 0;
	[branch]
	if(geometry.vb_nor >= 0)
	{
		nor = bindless_buffers_float4[geometry.vb_nor][vertexID].xyz;
	}
	nor = rotate_vector(nor, inst.quaternion);
	
	float2 uv = 0;
	[branch]
	if(geometry.vb_uvs >= 0)
	{
		uv = bindless_buffers_float4[geometry.vb_uvs][vertexID].xy;
	}
	
	Output output;
	output.pos = float4(uv, 0, 1);
	output.pos.xy = output.pos.xy * 2 - 1;
	output.pos.y *= -1;

	output.uv = uv;
	
	output.pos3D = pos;

	output.normal = nor;
	
	return output;
}
