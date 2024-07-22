#ifndef WI_OBJECTSHADER_HF
#define WI_OBJECTSHADER_HF

#ifdef TRANSPARENT
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
#else
#define SHADOW_MASK_ENABLED
#endif // TRANSPARENT

#if !defined(TRANSPARENT) && !defined(PREPASS)
#define DISABLE_ALPHATEST
#endif // !defined(TRANSPARENT) && !defined(PREPASS)

#ifdef PLANARREFLECTION
#define DISABLE_ENVMAPS
#define DISABLE_VOXELGI
#endif // PLANARREFLECTION

#ifdef WATER
#define DISABLE_ENVMAPS
#define DISABLE_VOXELGI
#endif // WATER

//#define LIGHTMAP_QUALITY_BICUBIC

#ifdef DISABLE_ALPHATEST
#define EARLY_DEPTH_STENCIL
#endif // DISABLE_ALPHATEST

#ifdef EARLY_DEPTH_STENCIL
#define SVT_FEEDBACK
#endif // EARLY_DEPTH_STENCIL

#ifdef TERRAINBLENDED
#define TEXTURE_SLOT_NONUNIFORM
#endif // TERRAINBLENDED

#include "globals.hlsli"
#include "brdf.hlsli"
#include "lightingHF.hlsli"
#include "skyAtmosphere.hlsli"
#include "fogHF.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "ShaderInterop_DDGI.h"
#include "shadingHF.hlsli"

// DEFINITIONS
//////////////////

PUSHCONSTANT(push, ObjectPushConstants);

inline ShaderGeometry GetMesh()
{
	return load_geometry(push.geometryIndex);
}
inline ShaderMaterial GetMaterial()
{
	return load_material(push.materialIndex);
}

#define sampler_objectshader bindless_samplers[GetMaterial().sampler_descriptor]

// Use these to compile this file as shader prototype:
//#define OBJECTSHADER_COMPILE_VS				- compile vertex shader prototype
//#define OBJECTSHADER_COMPILE_PS				- compile pixel shader prototype

// Use these to define the expected layout for the shader:
//#define OBJECTSHADER_LAYOUT_SHADOW			- layout for shadow pass
//#define OBJECTSHADER_LAYOUT_SHADOW_TEX		- layout for shadow pass and alpha test or transparency
//#define OBJECTSHADER_LAYOUT_PREPASS			- layout for prepass
//#define OBJECTSHADER_LAYOUT_PREPASS_TEX		- layout for prepass and alpha test or dithering
//#define OBJECTSHADER_LAYOUT_COMMON			- layout for common passes

// Use these to enable features for the shader:
//	(Some of these are enabled automatically with OBJECTSHADER_LAYOUT defines)
//#define OBJECTSHADER_USE_CLIPPLANE				- shader will be clipped according to camera clip planes
//#define OBJECTSHADER_USE_COLOR					- shader will use colors (material color, vertex color...)
//#define OBJECTSHADER_USE_DITHERING				- shader will use dithered transparency
//#define OBJECTSHADER_USE_UVSETS					- shader will sample textures with uv sets
//#define OBJECTSHADER_USE_ATLAS					- shader will use atlas
//#define OBJECTSHADER_USE_NORMAL					- shader will use normals
//#define OBJECTSHADER_USE_AO						- shader will use ambient occlusion
//#define OBJECTSHADER_USE_WETMAP					- shader will use wetmap
//#define OBJECTSHADER_USE_TANGENT					- shader will use tangents, normal mapping
//#define OBJECTSHADER_USE_POSITION3D				- shader will use world space positions
//#define OBJECTSHADER_USE_EMISSIVE					- shader will use emissive
//#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX	- shader will use dynamic render target slice selection
//#define OBJECTSHADER_USE_VIEWPORTARRAYINDEX		- shader will use dynamic viewport selection
//#define OBJECTSHADER_USE_NOCAMERA					- shader will not use camera space transform
//#define OBJECTSHADER_USE_INSTANCEINDEX			- shader will use instance ID


#ifdef OBJECTSHADER_LAYOUT_SHADOW
#endif // OBJECTSHADER_LAYOUT_SHADOW

#ifdef OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_INSTANCEINDEX
#define OBJECTSHADER_USE_UVSETS
#endif // OBJECTSHADER_LAYOUT_SHADOW_TEX

#ifdef OBJECTSHADER_LAYOUT_PREPASS
#define OBJECTSHADER_USE_CLIPPLANE
#define OBJECTSHADER_USE_INSTANCEINDEX
#endif // OBJECTSHADER_LAYOUT_SHADOW

#ifdef OBJECTSHADER_LAYOUT_PREPASS_TEX
#define OBJECTSHADER_USE_CLIPPLANE
#define OBJECTSHADER_USE_UVSETS
#define OBJECTSHADER_USE_DITHERING
#define OBJECTSHADER_USE_INSTANCEINDEX
#endif // OBJECTSHADER_LAYOUT_SHADOW_TEX

#ifdef OBJECTSHADER_LAYOUT_COMMON
#define OBJECTSHADER_USE_CLIPPLANE
#define OBJECTSHADER_USE_UVSETS
#define OBJECTSHADER_USE_ATLAS
#define OBJECTSHADER_USE_COLOR
#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_AO
#define OBJECTSHADER_USE_WETMAP
#define OBJECTSHADER_USE_TANGENT
#define OBJECTSHADER_USE_POSITION3D
#define OBJECTSHADER_USE_EMISSIVE
#define OBJECTSHADER_USE_INSTANCEINDEX
#endif // OBJECTSHADER_LAYOUT_COMMON

struct VertexInput
{
	uint vertexID : SV_VertexID;
	uint instanceID : SV_InstanceID;

	float4 GetPositionWind()
	{
		return bindless_buffers_float4[GetMesh().vb_pos_wind][vertexID];
	}

	float4 GetUVSets()
	{
		[branch]
		if (GetMesh().vb_uvs < 0)
			return 0;
		return lerp(GetMesh().uv_range_min.xyxy, GetMesh().uv_range_max.xyxy, bindless_buffers_float4[GetMesh().vb_uvs][vertexID]);
	}

	ShaderMeshInstancePointer GetInstancePointer()
	{
		if (push.instances >= 0)
			return bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + instanceID * sizeof(ShaderMeshInstancePointer));

		ShaderMeshInstancePointer poi;
		poi.init();
		return poi;
	}

	half2 GetAtlasUV()
	{
		[branch]
		if (GetMesh().vb_atl < 0)
			return 0;
		return (half2)bindless_buffers_float2[GetMesh().vb_atl][vertexID];
	}

	half4 GetVertexColor()
	{
		[branch]
		if (GetMesh().vb_col < 0)
			return 1;
		return (half4)bindless_buffers_float4[GetMesh().vb_col][vertexID];
	}
	
	float3 GetNormal()
	{
		[branch]
		if (GetMesh().vb_nor < 0)
			return 0;
		return bindless_buffers_float4[GetMesh().vb_nor][vertexID].xyz;
	}

	half4 GetTangent()
	{
		[branch]
		if (GetMesh().vb_tan < 0)
			return 0;
		return (half4)bindless_buffers_float4[GetMesh().vb_tan][vertexID];
	}

	ShaderMeshInstance GetInstance()
	{
		if (push.instances >= 0)
			return load_instance(GetInstancePointer().GetInstanceIndex());

		ShaderMeshInstance inst;
		inst.init();
		return inst;
	}

	half GetVertexAO()
	{
		[branch]
		if (GetInstance().vb_ao < 0)
			return 1;
		return (half)bindless_buffers_float[GetInstance().vb_ao][vertexID];
	}

	half GetWetmap()
	{
		[branch]
		if (GetInstance().vb_wetmap < 0)
			return 0;
		return (half)bindless_buffers_float[GetInstance().vb_wetmap][vertexID];
	}
};


struct VertexSurface
{
	float4 position;
	float4 uvsets;
	half2 atlas;
	half4 color;
	float3 normal;
	half4 tangent;
	half ao;
	half wet;

	inline void create(in ShaderMaterial material, in VertexInput input)
	{
		float4 pos_wind = input.GetPositionWind();
		position = float4(pos_wind.xyz, 1);
		normal = input.GetNormal();
		color = half4(GetMaterial().GetBaseColor() * input.GetInstance().GetColor());
		color.a *= half(1 - input.GetInstancePointer().GetDither());

		[branch]
		if (material.IsUsingVertexColors())
		{
			color *= input.GetVertexColor();
		}

		[branch]
		if (material.IsUsingVertexAO())
		{
			ao = input.GetVertexAO();
		}
		else
		{
			ao = 1;
		}

		normal = rotate_vector(normal, input.GetInstance().quaternion);
		normal = any(normal) ? normalize(normal) : 0;

		tangent = input.GetTangent();
		tangent.xyz = rotate_vector(tangent.xyz, (half4)input.GetInstance().quaternion);
		tangent.xyz = any(tangent.xyz) ? normalize(tangent.xyz) : 0;
		
		uvsets = input.GetUVSets();

		atlas = input.GetAtlasUV();

		position = mul(input.GetInstance().transform.GetMatrix(), position);

		wet = input.GetWetmap();

#ifndef DISABLE_WIND
		[branch]
		if (material.IsUsingWind())
		{
			position.xyz += sample_wind(position.xyz, pos_wind.w);
		}
#endif // DISABLE_WIND
	}
};

struct PixelInput
{
	precise float4 pos : SV_POSITION;

#ifdef OBJECTSHADER_USE_CLIPPLANE
	float clip : SV_ClipDistance0;
#endif // OBJECTSHADER_USE_CLIPPLANE

#if defined(OBJECTSHADER_USE_INSTANCEINDEX) || defined(OBJECTSHADER_USE_DITHERING)
	uint instanceIndex_dither : INSTANCEINDEX_DITHER;
#endif // OBJECTSHADER_USE_INSTANCEINDEX || OBJECTSHADER_USE_DITHERING

#ifdef OBJECTSHADER_USE_UVSETS
	float4 uvsets : UVSETS;
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_COLOR
	half4 color : COLOR;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_TANGENT
	half4 tan : TANGENT;
#endif // OBJECTSHADER_USE_TANGENT

#ifdef OBJECTSHADER_USE_NORMAL
	float3 nor : NORMAL;
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_ATLAS
	half2 atl : ATLAS;
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_POSITION3D
	float3 pos3D : WORLDPOSITION;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_AO
	half ao : AMBIENT_OCCLUSION;
#endif // OBJECTSHADER_USE_AO

#ifdef OBJECTSHADER_USE_WETMAP
	half wet : WET;
#endif // OBJECTSHADER_USE_WETMAP

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#ifdef VPRT_EMULATION
	uint RTIndex : RTINDEX;
#else
	uint RTIndex : SV_RenderTargetArrayIndex;
#endif // VPRT_EMULATION
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
#ifdef VPRT_EMULATION
	uint VPIndex : VPINDEX;
#else
	uint VPIndex : SV_ViewportArrayIndex;
#endif // VPRT_EMULATION
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX

#ifdef OBJECTSHADER_USE_INSTANCEINDEX
	inline uint GetInstanceIndex()
	{
		return instanceIndex_dither & 0xFFFFFF;
	}
#endif // OBJECTSHADER_USE_INSTANCEINDEX

#ifdef OBJECTSHADER_USE_DITHERING
	inline half GetDither()
	{
		return half((instanceIndex_dither >> 24u) / 255.0);
	}
#endif // OBJECTSHADER_USE_DITHERING
	
#ifdef OBJECTSHADER_USE_UVSETS
	inline float4 GetUVSets()
	{
		float4 ret = uvsets;
		ret.xy = mad(ret.xy, GetMaterial().texMulAdd.xy, GetMaterial().texMulAdd.zw);
		return ret;
	}
#endif // OBJECTSHADER_USE_UVSETS
};


// OBJECT SHADER PROTOTYPE
///////////////////////////

#ifdef OBJECTSHADER_COMPILE_VS

// Vertex shader base:
PixelInput main(VertexInput input)
{
	PixelInput Out;

	VertexSurface surface;
	surface.create(GetMaterial(), input);

	Out.pos = surface.position;

#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pos = mul(GetCamera().view_projection, Out.pos);
#endif // OBJECTSHADER_USE_NOCAMERA

#ifdef OBJECTSHADER_USE_CLIPPLANE
	Out.clip = dot(surface.position, GetCamera().clip_plane);
#endif // OBJECTSHADER_USE_CLIPPLANE

#if defined(OBJECTSHADER_USE_INSTANCEINDEX) || defined(OBJECTSHADER_USE_DITHERING)
	Out.instanceIndex_dither = 0;
#ifdef OBJECTSHADER_USE_INSTANCEINDEX
	Out.instanceIndex_dither |= input.GetInstancePointer().GetInstanceIndex() & 0xFFFFFF;
#endif // OBJECTSHADER_USE_INSTANCEINDEX
#ifdef OBJECTSHADER_USE_DITHERING
	Out.instanceIndex_dither |= (uint(surface.color.a * 255u) & 0xFF) << 24u;
#endif // OBJECTSHADER_USE_DITHERING
#endif // OBJECTSHADER_USE_INSTANCEINDEX || OBJECTSHADER_USE_DITHERING

#ifdef OBJECTSHADER_USE_POSITION3D
	Out.pos3D = surface.position.xyz;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_COLOR
	Out.color = surface.color;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_UVSETS
	Out.uvsets = surface.uvsets;
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_ATLAS
	Out.atl = surface.atlas;
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_NORMAL
	Out.nor = surface.normal;
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_AO
	Out.ao = surface.ao;
#endif // OBJECTSHADER_USE_AO

#ifdef OBJECTSHADER_USE_WETMAP
	Out.wet = surface.wet;
#endif // OBJECTSHADER_USE_WETMAP

#ifdef OBJECTSHADER_USE_TANGENT
	Out.tan = surface.tangent;
#endif // OBJECTSHADER_USE_TANGENT

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
	const uint frustum_index = input.GetInstancePointer().GetCameraIndex();
	Out.RTIndex = GetCamera(frustum_index).output_index;
#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pos = mul(GetCamera(frustum_index).view_projection, surface.position);
#endif // OBJECTSHADER_USE_NOCAMERA
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	const uint frustum_index = input.GetInstancePointer().GetCameraIndex();
	Out.VPIndex = GetCamera(frustum_index).output_index;
#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pos = mul(GetCamera(frustum_index).view_projection, surface.position);
#endif // OBJECTSHADER_USE_NOCAMERA
#endif // OBJECTSHADER_USE_VIEWPORTARRAYINDEX

	return Out;
}

#endif // OBJECTSHADER_COMPILE_VS


#ifdef OBJECTSHADER_COMPILE_PS

// Possible switches:
//	PREPASS				-	assemble object shader for depth prepass rendering
//	DEPTHONLY			-	assemble object shader for depth prepass rendering with no return
//	TRANSPARENT			-	assemble object shader for tiled forward transparent rendering
//	ENVMAPRENDERING		-	modify object shader for envmap rendering
//	PLANARREFLECTION	-	include planar reflection sampling
//	PARALLAXOCCLUSIONMAPPING					-	include parallax occlusion mapping computation
//	WATER				-	include specialized water shader code
//  ... and other material type specific defines

#if defined(__PSSL__) && defined(PREPASS) && !defined(DEPTHONLY)
#pragma PSSL_target_output_format (target 0 FMT_32_R)
#endif // __PSSL__ && PREPASS

#ifdef EARLY_DEPTH_STENCIL
[earlydepthstencil]
#endif // EARLY_DEPTH_STENCIL

// entry point:
#ifdef PREPASS
#ifdef DEPTHONLY
void main(PixelInput input, in uint primitiveID : SV_PrimitiveID, out uint coverage : SV_Coverage)
#else
uint main(PixelInput input, in uint primitiveID : SV_PrimitiveID, out uint coverage : SV_Coverage) : SV_Target
#endif // DEPTHONLY
#else
float4 main(PixelInput input, in bool is_frontface : SV_IsFrontFace) : SV_Target
#endif // PREPASS


// Pixel shader base:
{
	const float depth = input.pos.z;
	const float lineardepth = input.pos.w;
	const uint2 pixel = input.pos.xy; // no longer pixel center!
	const float2 ScreenCoord = input.pos.xy * GetCamera().internal_resolution_rcp; // use pixel center!

#ifdef OBJECTSHADER_USE_UVSETS
	float4 uvsets = input.GetUVSets();
#endif // OBJECTSHADER_USE_UVSETS

#ifdef TILEDFORWARD
	write_mipmap_feedback(push.materialIndex, ddx_coarse(uvsets), ddy_coarse(uvsets));
#endif // TILEDFORWARD

#ifndef DISABLE_ALPHATEST
#ifndef TRANSPARENT
#ifndef ENVMAPRENDERING
#ifdef OBJECTSHADER_USE_DITHERING
	// apply dithering:
	clip(dither(pixel + GetTemporalAASampleRotation()) - (1 - input.GetDither()));
#endif // OBJECTSHADER_USE_DITHERING
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT
#endif // ENVMAPRENDERING

#ifdef OBJECTSHADER_USE_INSTANCEINDEX
	ShaderMeshInstance meshinstance = load_instance(input.GetInstanceIndex());
#endif // OBJECTSHADER_USE_INSTANCEINDEX

	ShaderMaterial material = GetMaterial();

	Surface surface;
	surface.init();


#ifdef OBJECTSHADER_USE_NORMAL
	if (is_frontface == false)
	{
		input.nor = -input.nor;
	}
	surface.N = normalize(input.nor);
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_AO
	surface.occlusion = input.ao;
#endif // OBJECTSHADER_USE_AO

#ifdef OBJECTSHADER_USE_POSITION3D
	surface.P = input.pos3D;
	surface.V = GetCamera().position - surface.P;
	float dist = length(surface.V);
	surface.V /= dist;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_TANGENT
	if (is_frontface == false)
	{
		input.tan = -input.tan;
	}
	surface.T = input.tan;
	surface.T.w = surface.T.w < 0 ? -1 : 1;
	half3 bitangent = cross(surface.T.xyz, input.nor) * surface.T.w;
	float3x3 TBN = float3x3(surface.T.xyz, bitangent, input.nor); // unnormalized TBN! http://www.mikktspace.com/
	
	surface.T.xyz = normalize(surface.T.xyz);

#ifdef PARALLAXOCCLUSIONMAPPING
	[branch]
	if (material.textures[DISPLACEMENTMAP].IsValid())
	{
		Texture2D tex = bindless_textures[material.textures[DISPLACEMENTMAP].texture_descriptor];
		float2 uv = material.textures[DISPLACEMENTMAP].GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
		float2 uv_dx = ddx_coarse(uv);
		float2 uv_dy = ddy_coarse(uv);

		ParallaxOcclusionMapping_Impl(
			uvsets,
			surface.V,
			TBN,
			material.GetParallaxOcclusionMapping(),
			tex,
			uv,
			uv_dx,
			uv_dy
		);
	}
#endif // PARALLAXOCCLUSIONMAPPING

#endif // OBJECTSHADER_USE_TANGENT



#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
#ifdef PREPASS
	if (material.textures[BASECOLORMAP].IsValid())
#else
	if (material.textures[BASECOLORMAP].IsValid() && (GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
#endif // PREPASS
	{
		surface.baseColor *= (half4)material.textures[BASECOLORMAP].Sample(sampler_objectshader, uvsets);
	}
	
	[branch]
	if (material.textures[TRANSPARENCYMAP].IsValid())
	{
		surface.baseColor.a *= (half)material.textures[TRANSPARENCYMAP].Sample(sampler_objectshader, uvsets).r;
	}
#endif // OBJECTSHADER_USE_UVSETS


#ifdef OBJECTSHADER_USE_COLOR
	surface.baseColor *= input.color;
#endif // OBJECTSHADER_USE_COLOR


#ifdef TRANSPARENT
#ifndef DISABLE_ALPHATEST
	// Alpha test is only done for transparents
	//	- Prepass will write alpha coverage mask
	//	- Opaque will use [earlydepthstencil] and COMPARISON_EQUAL depth test on top of depth prepass
	clip(surface.baseColor.a - material.GetAlphaTest() - meshinstance.GetAlphaTest());
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT


#ifndef WATER
#ifdef OBJECTSHADER_USE_TANGENT
	[branch]
	if (material.textures[NORMALMAP].IsValid())
	{
		surface.bumpColor = half3((half2)material.textures[NORMALMAP].Sample(sampler_objectshader, uvsets).rg, 1);
		surface.bumpColor = surface.bumpColor * 2 - 1;
		surface.bumpColor.rg *= material.GetNormalMapStrength();
	}
#endif // OBJECTSHADER_USE_TANGENT
#endif // WATER

	surface.layerMask = material.layerMask & meshinstance.layerMask;


	half4 surfaceMap = 1;
#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (material.textures[SURFACEMAP].IsValid())
	{
		surfaceMap = (half4)material.textures[SURFACEMAP].Sample(sampler_objectshader, uvsets);
	}
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_EMISSIVE
	// Emissive map:
	surface.emissiveColor = material.GetEmissive();

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (any(surface.emissiveColor) && material.textures[EMISSIVEMAP].IsValid())
	{
		half4 emissiveMap = (half4)material.textures[EMISSIVEMAP].Sample(sampler_objectshader, uvsets);
		surface.emissiveColor *= emissiveMap.rgb * emissiveMap.a;
	}
#endif // OBJECTSHADER_USE_UVSETS

	surface.emissiveColor *= meshinstance.GetEmissive();
#endif // OBJECTSHADER_USE_EMISSIVE

#ifdef OBJECTSHADER_USE_UVSETS
#ifdef TERRAINBLENDED
	[branch]
	if (material.GetTerrainBlendRcp() > 0)
	{
		// Blend object into terrain material:
		ShaderTerrain terrain = GetScene().terrain;
		[branch]
		if(terrain.chunk_buffer >= 0)
		{
			int2 chunk_coord = floor((surface.P.xz - terrain.center_chunk_pos.xz) / terrain.chunk_size);
			if(chunk_coord.x >= -terrain.chunk_buffer_range && chunk_coord.x <= terrain.chunk_buffer_range && chunk_coord.y >= -terrain.chunk_buffer_range && chunk_coord.y <= terrain.chunk_buffer_range)
			{
				uint chunk_idx = flatten2D(chunk_coord + terrain.chunk_buffer_range, terrain.chunk_buffer_range * 2 + 1);
				ShaderTerrainChunk chunk = bindless_structured_terrain_chunks[terrain.chunk_buffer][chunk_idx];
				
				[branch]
				if(chunk.heightmap >= 0)
				{
					Texture2D terrain_heightmap = bindless_textures[NonUniformResourceIndex(chunk.heightmap)];
					float2 chunk_min = terrain.center_chunk_pos.xz + chunk_coord * terrain.chunk_size;
					float2 chunk_max = terrain.center_chunk_pos.xz + terrain.chunk_size + chunk_coord * terrain.chunk_size;
					float2 terrain_uv = saturate(inverse_lerp(chunk_min, chunk_max, surface.P.xz));
					float terrain_height0 = terrain_heightmap.SampleLevel(sampler_linear_clamp, terrain_uv, 0).r;
					float terrain_height1 = terrain_heightmap.SampleLevel(sampler_linear_clamp, terrain_uv, 0, int2(1, 0)).r;
					float terrain_height2 = terrain_heightmap.SampleLevel(sampler_linear_clamp, terrain_uv, 0, int2(0, 1)).r;
					float3 P0 = float3(0, terrain_height0, 0); 
					float3 P1 = float3(1, terrain_height1, 0); 
					float3 P2 = float3(0, terrain_height2, 1);
					float3 terrain_normal = normalize(cross(P2 - P0, P1 - P0));
					float terrain_height = lerp(terrain.min_height, terrain.max_height, terrain_height0);
					float object_height = surface.P.y;
					float diff = (object_height - terrain_height) * material.GetTerrainBlendRcp();
					float blend = 1 - pow(saturate(diff), 2);
					//blend *= lerp(1, saturate((noise_gradient_3D(surface.P * 2) * 0.5 + 0.5) * 2), saturate(diff));
					//terrain_uv = lerp(saturate(inverse_lerp(chunk_min, chunk_max, surface.P.xz - surface.N.xz * diff)), terrain_uv, saturate(surface.N.y)); // uv stretching improvement: stretch in normal direction if normal gets horizontal
					ShaderMaterial terrain_material = load_material(chunk.materialID);
					terrain_uv = mad(terrain_uv, terrain_material.texMulAdd.xy, terrain_material.texMulAdd.zw);
					float4 terrain_baseColor = terrain_material.textures[BASECOLORMAP].Sample(sampler_objectshader, terrain_uv.xyxy);
					float4 terrain_bumpColor = terrain_material.textures[NORMALMAP].Sample(sampler_objectshader, terrain_uv.xyxy);
					float4 terrain_surfaceMap = terrain_material.textures[SURFACEMAP].Sample(sampler_objectshader, terrain_uv.xyxy);
					float3 terrain_emissiveMap = terrain_material.textures[EMISSIVEMAP].Sample(sampler_objectshader, terrain_uv.xyxy).rgb;
					surface.baseColor = lerp(surface.baseColor, terrain_baseColor, blend);
					surface.bumpColor = lerp(surface.bumpColor, terrain_bumpColor.rgb * 2 - 1, blend);
					surfaceMap = lerp(surfaceMap, terrain_surfaceMap, blend);
					surface.emissiveColor += terrain_emissiveMap * terrain_material.GetEmissive() * blend;
					input.nor = lerp(input.nor, terrain_normal, blend);
					TBN[2] = input.nor;
					surface.N = normalize(input.nor);
				}
			}
		}
	}
#endif // TERRAINBLENDED
#endif // OBJECTSHADER_USE_UVSETS

	[branch]
	if (!material.IsUsingSpecularGlossinessWorkflow())
	{
		// Premultiply these before evaluating decals:
		surfaceMap.g *= material.GetRoughness();
		surfaceMap.b *= material.GetMetalness();
		surfaceMap.a *= material.GetReflectance();
	}

#ifdef TILEDFORWARD
	const uint flat_tile_index = GetFlatTileIndex(pixel);
#endif // TILEDFORWARD

#ifndef PREPASS
#ifndef WATER
#ifdef FORWARD
	ForwardDecals(surface, surfaceMap, sampler_objectshader);
#endif // FORWARD

#ifdef TILEDFORWARD
	TiledDecals(surface, flat_tile_index, surfaceMap, sampler_objectshader);
#endif // TILEDFORWARD
#endif // WATER
#endif // PREPASS


#ifndef WATER
#ifdef OBJECTSHADER_USE_TANGENT
	[branch]
	if (any(surface.bumpColor))
	{
		surface.N = normalize(mul(surface.bumpColor, TBN));
	}
#endif // OBJECTSHADER_USE_TANGENT
#endif // WATER


	half4 specularMap = 1;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (material.textures[SPECULARMAP].IsValid())
	{
		specularMap = (half4)material.textures[SPECULARMAP].Sample(sampler_objectshader, uvsets);
	}
#endif // OBJECTSHADER_USE_UVSETS


	surface.create(material, surface.baseColor, surfaceMap, specularMap);
	
	

#ifdef OBJECTSHADER_USE_WETMAP
	if(input.wet > 0)
	{
		surface.albedo = lerp(surface.albedo, 0, input.wet);
		surface.roughness = clamp(surface.roughness * sqr(1 - input.wet), 0.01, 1);
		surface.N = normalize(lerp(surface.N, input.nor, input.wet));
	}
#endif // OBJECTSHADER_USE_WETMAP


#ifdef OBJECTSHADER_USE_UVSETS
	// Secondary occlusion map:
	[branch]
	if (material.IsOcclusionEnabled_Secondary() && material.textures[OCCLUSIONMAP].IsValid())
	{
		surface.occlusion *= material.textures[OCCLUSIONMAP].Sample(sampler_objectshader, uvsets).r;
	}
#endif // OBJECTSHADER_USE_UVSETS


#ifndef PREPASS
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
#ifndef CARTOON
	[branch]
	if (GetCamera().texture_ao_index >= 0)
	{
		surface.occlusion *= (half)bindless_textures_float[GetCamera().texture_ao_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	}
#endif // CARTOON
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // PREPASS


#ifdef ANISOTROPIC
	surface.aniso.strength = material.GetAnisotropy();
	surface.aniso.direction = half2(material.GetAnisotropyCos(), material.GetAnisotropySin());

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (material.textures[ANISOTROPYMAP].IsValid())
	{
		half2 anisotropyTexture = (half2)material.textures[ANISOTROPYMAP].Sample(sampler_objectshader, uvsets).rg * 2 - 1;
		surface.aniso.strength *= length(anisotropyTexture);
		surface.aniso.direction = mul(half2x2(surface.aniso.direction.x, surface.aniso.direction.y, -surface.aniso.direction.y, surface.aniso.direction.x), normalize(anisotropyTexture));
	}
#endif // OBJECTSHADER_USE_UVSETS

	surface.aniso.T = normalize(mul(TBN, half3(surface.aniso.direction, 0)));

#endif // ANISOTROPIC


#ifdef SHEEN
	surface.sheen.color = material.GetSheenColor();
	surface.sheen.roughness = material.GetSheenRoughness();

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (material.textures[SHEENCOLORMAP].IsValid())
	{
		surface.sheen.color = (half3)material.textures[SHEENCOLORMAP].Sample(sampler_objectshader, uvsets).rgb;
	}
	[branch]
	if (material.textures[SHEENROUGHNESSMAP].IsValid())
	{
		surface.sheen.roughness = (half)material.textures[SHEENROUGHNESSMAP].Sample(sampler_objectshader, uvsets).a;
	}
#endif // OBJECTSHADER_USE_UVSETS
#endif // SHEEN


#ifdef CLEARCOAT
	surface.clearcoat.factor = material.GetClearcoat();
	surface.clearcoat.roughness = material.GetClearcoatRoughness();
	surface.clearcoat.N = input.nor;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (material.textures[CLEARCOATMAP].IsValid())
	{
		surface.clearcoat.factor = (half)material.textures[CLEARCOATMAP].Sample(sampler_objectshader, uvsets).r;
	}
	[branch]
	if (material.textures[CLEARCOATROUGHNESSMAP].IsValid())
	{
		surface.clearcoat.roughness = (half)material.textures[CLEARCOATROUGHNESSMAP].Sample(sampler_objectshader, uvsets).g;
	}
#ifdef OBJECTSHADER_USE_TANGENT
	[branch]
	if (material.textures[CLEARCOATNORMALMAP].IsValid())
	{
		half3 clearcoatNormalMap = half3(material.textures[CLEARCOATNORMALMAP].Sample(sampler_objectshader, uvsets).rg, 1);
		clearcoatNormalMap = clearcoatNormalMap * 2 - 1;
		surface.clearcoat.N = mul(clearcoatNormalMap, TBN);
	}
#endif // OBJECTSHADER_USE_TANGENT

	surface.clearcoat.N = normalize(surface.clearcoat.N);

#endif // OBJECTSHADER_USE_UVSETS
#endif // CLEARCOAT

	surface.sss = material.GetSSS();
	surface.sss_inv = material.GetSSSInverse();

#ifdef WATER
	surface.extinction = material.GetSheenColor().rgb; // Note: sheen color is repurposed as extinction color for water
#endif // WATER

	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.update();

	half3 ambient = GetAmbient(surface.N);
	ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);

	
	half4 color = surface.baseColor;

#ifdef WATER
	//NORMALMAP
	half2 bumpColor0 = 0;
	half2 bumpColor1 = 0;
	half2 bumpColor2 = 0;
	[branch]
	if (material.textures[NORMALMAP].IsValid())
	{
		Texture2D texture_normalmap = bindless_textures[material.textures[NORMALMAP].texture_descriptor];
		const float2 UV_normalMap = material.textures[NORMALMAP].GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
		bumpColor0 = 2 * (half2)texture_normalmap.Sample(sampler_objectshader, UV_normalMap - material.texMulAdd.ww).rg - 1;
		bumpColor1 = 2 * (half2)texture_normalmap.Sample(sampler_objectshader, UV_normalMap + material.texMulAdd.zw).rg - 1;
	}
	[branch]
	if (GetCamera().texture_waterriples_index >= 0)
	{
		bumpColor2 = (half2)bindless_textures_float2[GetCamera().texture_waterriples_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).rg;
	}
	surface.bumpColor = half3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * material.GetRefraction();
	surface.N = normalize(lerp(surface.N, mul(normalize(surface.bumpColor), TBN), material.GetNormalMapStrength()));
	surface.bumpColor.rg *= material.GetNormalMapStrength();

	[branch]
	if (GetCamera().texture_reflection_index >= 0)
	{
		//REFLECTION
		float4 reflectionUV = mul(GetCamera().reflection_view_projection, float4(surface.P, 1));
		reflectionUV.xy /= reflectionUV.w;
		reflectionUV.xy = clipspace_to_uv(reflectionUV.xy) + surface.bumpColor.rg;
		half3 reflectiveColor = (half3)bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_mirror, reflectionUV.xy, 0).rgb;
		[branch]
		if(GetCamera().texture_reflection_depth_index >= 0)
		{
			float reflectiveDepth = bindless_textures[GetCamera().texture_reflection_depth_index].SampleLevel(sampler_point_clamp, reflectionUV.xy, 0).r;
			float3 reflectivePosition = reconstruct_position(reflectionUV.xy, reflectiveDepth, GetCamera().reflection_inverse_view_projection);
			float4 water_plane = GetCamera().reflection_plane;
			float water_depth = -dot(float4(reflectivePosition, 1), water_plane);
			reflectiveColor.rgb = lerp(color.rgb, reflectiveColor.rgb, saturate(exp(-water_depth * color.a)));
		}
		lighting.indirect.specular += reflectiveColor * surface.F;
	}
#endif // WATER



#ifdef TRANSPARENT
	[branch]
	if (material.GetTransmission() > 0)
	{
		surface.transmission = material.GetTransmission();

#ifdef OBJECTSHADER_USE_UVSETS
		[branch]
		if (material.textures[TRANSMISSIONMAP].IsValid())
		{
			half transmissionMap = (half)material.textures[TRANSMISSIONMAP].Sample(sampler_objectshader, uvsets).r;
			surface.transmission *= transmissionMap;
		}
#endif // OBJECTSHADER_USE_UVSETS

		[branch]
		if (GetCamera().texture_refraction_index >= 0)
		{
			Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
			float2 size;
			float mipLevels;
			texture_refraction.GetDimensions(0, size.x, size.y, mipLevels);
			const float2 normal2D = mul((float3x3)GetCamera().view, surface.N.xyz).xy;
			float2 perturbatedRefrTexCoords = ScreenCoord.xy + normal2D * material.GetRefraction();
			half4 refractiveColor = texture_refraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, surface.roughness * mipLevels);
			surface.refraction.rgb = surface.albedo * refractiveColor.rgb;
			surface.refraction.a = surface.transmission;
		}
	}
#endif // TRANSPARENT


#ifdef OBJECTSHADER_USE_ATLAS
	LightMapping(meshinstance.lightmap, input.atl, lighting, surface);
#endif // OBJECTSHADER_USE_ATLAS


#ifdef PLANARREFLECTION
	lighting.indirect.specular += PlanarReflection(surface, surface.bumpColor.rg) * surface.F;
#endif


#ifdef FORWARD
	ForwardLighting(surface, lighting);
#endif // FORWARD


#ifdef TILEDFORWARD
	TiledLighting(surface, lighting, flat_tile_index);
#endif // TILEDFORWARD


#ifndef WATER
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
#ifndef CARTOON
	[branch]
	if (GetCamera().texture_ssr_index >= 0)
	{
		half4 ssr = (half4)bindless_textures[GetCamera().texture_ssr_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0);
		lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb * surface.F, ssr.a);
	}
	[branch]
	if (GetCamera().texture_ssgi_index >= 0)
	{
		surface.ssgi = (half3)bindless_textures[GetCamera().texture_ssgi_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).rgb;
	}
#endif // CARTOON
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // WATER

#ifdef WATER
	[branch]
	if (GetCamera().texture_refraction_index >= 0)
	{
		// Water refraction:
		float4 water_plane = GetCamera().reflection_plane;
		const float camera_above_water = dot(float4(GetCamera().position, 1), water_plane) < 0; 
		Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
		// First sample using full perturbation:
		float2 refraction_uv = ScreenCoord.xy + surface.bumpColor.rg;
		float refraction_depth = find_max_depth(refraction_uv, 2, 2);
		float3 refraction_position = reconstruct_position(refraction_uv, refraction_depth);
		float water_depth = -dot(float4(refraction_position, 1), water_plane);
		if(camera_above_water)
			water_depth = -water_depth;
		if(water_depth <= 0)
		{
			// Above water, fill holes by taking unperturbed sample:
			refraction_uv = ScreenCoord.xy;
		}
		else
		{
			// Below water, compute perturbation according to first sample water depth:
			refraction_uv = ScreenCoord.xy + surface.bumpColor.rg * saturate(1 - exp(-water_depth));
		}
		surface.refraction.rgb = (half3)texture_refraction.SampleLevel(sampler_linear_mirror, refraction_uv, 0).rgb;
		// Recompute depth params again with actual perturbation:
		refraction_depth = texture_depth.SampleLevel(sampler_point_clamp, refraction_uv, 0);
		refraction_position = reconstruct_position(refraction_uv, refraction_depth);
		water_depth = max(water_depth, -dot(float4(refraction_position, 1), water_plane));
		if(camera_above_water)
			water_depth = -water_depth;
		// Water fog computation:
		float waterfog = saturate(exp(-water_depth * color.a));
		float3 transmittance = saturate(exp(-water_depth * surface.extinction * color.a));
		surface.refraction.a = waterfog;
		surface.refraction.rgb *= transmittance;
		color.a = 1;
	}
#endif // WATER

	ApplyLighting(surface, lighting, color);


#ifdef UNLIT
	color = surface.baseColor;
#endif // UNLIT


// Transparent objects has been rendered separately from opaque, so let's apply it now.
// Must also be applied before fog since fog is layered over.
#ifdef TRANSPARENT
	ApplyAerialPerspective(ScreenCoord, surface.P, color);
#endif // TRANSPARENT


#ifdef OBJECTSHADER_USE_POSITION3D
	ApplyFog(dist, surface.V, color);
#endif // OBJECTSHADER_USE_POSITION3D


	color = saturateMediump(color);


	// end point:
#ifdef PREPASS
	coverage = AlphaToCoverage(color.a, material.GetAlphaTest() + meshinstance.alphaTest, input.pos);
#ifndef DEPTHONLY
	PrimitiveID prim;
	prim.primitiveIndex = primitiveID;
	prim.instanceIndex = input.GetInstanceIndex();
	prim.subsetIndex = push.geometryIndex - meshinstance.geometryOffset;
	return prim.pack();
#endif // DEPTHONLY
#else
	return color;
#endif // PREPASS
}


#endif // OBJECTSHADER_COMPILE_PS



#endif // WI_OBJECTSHADER_HF

