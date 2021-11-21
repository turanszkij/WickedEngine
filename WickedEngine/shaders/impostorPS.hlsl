#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(VSOut input) : SV_Target
{
	float3 uv_col = float3(input.uv, input.slice);
	float3 uv_nor = uv_col;
	uv_nor.z += impostorCaptureAngles;
	float3 uv_sur = uv_nor;
	uv_sur.z += impostorCaptureAngles;

	ShaderMeshInstance instance = load_instance(input.instanceID);

	float4 color = impostorTex.Sample(sampler_linear_clamp, uv_col) * unpack_rgba(instance.color);
	float3 N = impostorTex.Sample(sampler_linear_clamp, uv_nor).rgb * 2 - 1;
	float4 surfaceparams = impostorTex.Sample(sampler_linear_clamp, uv_sur);

	float ao = surfaceparams.r;
	float roughness = surfaceparams.g;
	float metalness = surfaceparams.b;
	float reflectance = surfaceparams.a;

	float3 V = GetCamera().position - input.pos3D;
	float dist = length(V);
	V /= dist;

	Surface surface;
	surface.init();
	surface.roughness = roughness;
	surface.albedo = lerp(lerp(color.rgb, float3(0, 0, 0), reflectance), float3(0, 0, 0), metalness);
	surface.f0 = lerp(lerp(float3(0, 0, 0), float3(1, 1, 1), reflectance), color.rgb, metalness);
	surface.P = input.pos3D;
	surface.N = N;
	surface.V = V;
	surface.pixel = input.pos.xy;
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	TiledLighting(surface, lighting);

	ApplyLighting(surface, lighting, color);

	ApplyFog(dist, GetCamera().position, V, color);

	return color;
}
