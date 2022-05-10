#define SURFACE_LOAD_QUAD_DERIVATIVES
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "surfaceHF.hlsli"
#include "objectHF.hlsli"

ConstantBuffer<ShaderTypeBin> bin : register(b10);
StructuredBuffer<uint> binned_pixels : register(t0);

RWTexture2D<float4> output : register(u0);
RWTexture2D<float2> output_normal : register(u1);
RWTexture2D<unorm float> output_roughness : register(u2);
RWStructuredBuffer<uint2> output_pixel_data_common : register(u3);
RWStructuredBuffer<uint4> output_pixel_data_varying : register(u4);

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID)
{
	if (DTid.x >= bin.count)
		return;

	const uint bin_data_index = bin.offset + DTid.x;
	uint2 pixel = unpack_pixel(binned_pixels[bin_data_index]);
	uint primitiveID = texture_primitiveID[pixel];
	if (!any(primitiveID))
	{
		return;
	}

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	float3 rayDirection_quad_x = CreateCameraRay(uv_to_clipspace(((float2)pixel + float2(1, 0) + 0.5) * GetCamera().internal_resolution_rcp)).Direction;
	float3 rayDirection_quad_y = CreateCameraRay(uv_to_clipspace(((float2)pixel + float2(0, 1) + 0.5) * GetCamera().internal_resolution_rcp)).Direction;

	PrimitiveID prim;
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();

	[branch]
	if (!surface.load(prim, ray.Origin, ray.Direction, rayDirection_quad_x, rayDirection_quad_y))
	{
		return;
	}
	surface.pixel = pixel.xy;
	surface.screenUV = uv;

	Lighting lighting;
	lighting.create(0, 0, 0, 0);
	TiledLighting(surface, lighting); // decals only

#ifdef UNLIT
	output[pixel] = surface.baseColor;
	return;
#else
	output[pixel] = float4(surface.albedo, 1);
#endif // UNLIT

#ifdef CLEARCOAT
	output_normal[pixel] = encode_oct(surface.clearcoat.N);
	output_roughness[pixel] = surface.clearcoat.roughness;
#else
	output_normal[pixel] = encode_oct(surface.N);
	output_roughness[pixel] = surface.roughness;
#endif // CLEARCOAT

	output_pixel_data_common[bin_data_index].x = pack_rgba(float4(surface.f0, surface.occlusion));
	output_pixel_data_common[bin_data_index].y = Pack_R11G11B10_FLOAT(surface.emissiveColor);



	// Pack surface parameters that are varying per shader type:

#ifdef ANISOTROPIC
	output_pixel_data_varying[bin_data_index].xy = pack_half4(surface.T);
#endif // ANISOTROPIC

#ifdef PLANARREFLECTION
	output_pixel_data_varying[bin_data_index].x = pack_half2(surface.bumpColor.rg);
#endif

#ifdef SHEEN
	output_pixel_data_varying[bin_data_index].x = pack_rgba(float4(surface.sheen.color, surface.sheen.roughness));
#endif // SHEEN

#ifdef CLEARCOAT
	output_pixel_data_varying[bin_data_index].y = pack_half2(encode_oct(surface.N));
	output_pixel_data_varying[bin_data_index].z = pack_rgba(float4(surface.roughness, 0, 0, 0));
#endif // CLEARCOAT

}
