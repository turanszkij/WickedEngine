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

#define LIGHTMAP_QUALITY_BICUBIC

#ifdef DISABLE_ALPHATEST
#define EARLY_DEPTH_STENCIL
#endif // DISABLE_ALPHATEST

#ifdef EARLY_DEPTH_STENCIL
#define SVT_FEEDBACK
#endif // EARLY_DEPTH_STENCIL


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

#define sampler_objectshader			bindless_samplers[GetMaterial().sampler_descriptor]

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

	min16float2 GetAtlasUV()
	{
		[branch]
		if (GetMesh().vb_atl < 0)
			return 0;
		return (min16float2)bindless_buffers_float2[GetMesh().vb_atl][vertexID];
	}

	min16float4 GetVertexColor()
	{
		[branch]
		if (GetMesh().vb_col < 0)
			return 1;
		return (min16float4)bindless_buffers_float4[GetMesh().vb_col][vertexID];
	}
	
	min16float3 GetNormal()
	{
		[branch]
		if (GetMesh().vb_nor < 0)
			return 0;
		return (min16float3)bindless_buffers_float4[GetMesh().vb_nor][vertexID].xyz;
	}

	min16float4 GetTangent()
	{
		[branch]
		if (GetMesh().vb_tan < 0)
			return 0;
		return (min16float4)bindless_buffers_float4[GetMesh().vb_tan][vertexID];
	}

	ShaderMeshInstance GetInstance()
	{
		if (push.instances >= 0)
			return load_instance(GetInstancePointer().GetInstanceIndex());

		ShaderMeshInstance inst;
		inst.init();
		return inst;
	}
};


struct VertexSurface
{
	float4 position;
	float4 uvsets;
	min16float2 atlas;
	min16float4 color;
	min16float3 normal;
	min16float4 tangent;

	inline void create(in ShaderMaterial material, in VertexInput input)
	{
		float4 pos_wind = input.GetPositionWind();
		position = float4(pos_wind.xyz, 1);
		normal = input.GetNormal();
		color = min16float4(GetMaterial().baseColor * unpack_rgba(input.GetInstance().color));
		color.a *= min16float(1 - input.GetInstancePointer().GetDither());

		[branch]
		if (material.IsUsingVertexColors())
		{
			color *= input.GetVertexColor();
		}

		normal = mul((min16float3x3)input.GetInstance().transformInverseTranspose.GetMatrix(), normal);

		tangent = input.GetTangent();
		tangent.xyz = mul((min16float3x3)input.GetInstance().transformInverseTranspose.GetMatrix(), tangent.xyz);

		// Note: normalization must happen when normal is exported as half precision for interpolator!
		normal = any(normal) ? normalize(normal) : 0;
		tangent = any(tangent) ? normalize(tangent) : 0;
		
		uvsets = input.GetUVSets();

		atlas = input.GetAtlasUV();

		position = mul(input.GetInstance().transform.GetMatrix(), position);

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
	min16float4 color : COLOR;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_TANGENT
	min16float4 tan : TANGENT;
#endif // OBJECTSHADER_USE_TANGENT

#ifdef OBJECTSHADER_USE_NORMAL
	min16float3 nor : NORMAL;
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_ATLAS
	min16float2 atl : ATLAS;
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_POSITION3D
	float3 pos3D : WORLDPOSITION;
#endif // OBJECTSHADER_USE_POSITION3D

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
	inline min16float GetDither()
	{
		return min16float((instanceIndex_dither >> 24u) / 255.0);
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
//	TRANSPARENT			-	assemble object shader for forward or tile forward transparent rendering
//	ENVMAPRENDERING		-	modify object shader for envmap rendering
//	PLANARREFLECTION	-	include planar reflection sampling
//	PARALLAXOCCLUSIONMAPPING					-	include parallax occlusion mapping computation
//	WATER				-	include specialized water shader code

#if defined(__PSSL__) && defined(PREPASS)
#pragma PSSL_target_output_format (target 0 FMT_32_R)
#endif // __PSSL__ && PREPASS

#ifdef EARLY_DEPTH_STENCIL
[earlydepthstencil]
#endif // EARLY_DEPTH_STENCIL

// entry point:
#ifdef PREPASS
uint main(PixelInput input, in uint primitiveID : SV_PrimitiveID, out uint coverage : SV_Coverage) : SV_Target
#else
float4 main(PixelInput input, in bool is_frontface : SV_IsFrontFace) : SV_Target
#endif // PREPASS


// Pixel shader base:
{
	const float depth = input.pos.z;
	const float lineardepth = input.pos.w;
	const uint2 pixel = input.pos.xy;
	const float2 ScreenCoord = pixel * GetCamera().internal_resolution_rcp;

#ifdef OBJECTSHADER_USE_UVSETS
	float4 uvsets = input.GetUVSets();
#endif // OBJECTSHADER_USE_UVSETS

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


	Surface surface;
	surface.init();


#ifdef OBJECTSHADER_USE_NORMAL
	if (is_frontface == false)
	{
		input.nor = -input.nor;
	}
	surface.N = normalize(input.nor);
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_POSITION3D
	surface.P = input.pos3D;
	surface.V = GetCamera().position - surface.P;
	float dist = length(surface.V);
	surface.V /= dist;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_TANGENT
#if 0
	float3x3 TBN = compute_tangent_frame(surface.N, surface.P, uvsets.xy);
#else
	if (is_frontface == false)
	{
		input.tan = -input.tan;
	}
	surface.T = input.tan;
	surface.T.xyz = normalize(surface.T.xyz);
	float3 binormal = normalize(cross(surface.T.xyz, surface.N) * surface.T.w);
	float3x3 TBN = float3x3(surface.T.xyz, binormal, surface.N);
#endif

#ifdef PARALLAXOCCLUSIONMAPPING
	[branch]
	if (GetMaterial().textures[DISPLACEMENTMAP].IsValid())
	{
		Texture2D tex = bindless_textures[GetMaterial().textures[DISPLACEMENTMAP].texture_descriptor];
		float2 uv = GetMaterial().textures[DISPLACEMENTMAP].GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
		float2 uv_dx = ddx_coarse(uv);
		float2 uv_dy = ddy_coarse(uv);

		ParallaxOcclusionMapping_Impl(
			uvsets,
			surface.V,
			TBN,
			GetMaterial(),
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
	if (GetMaterial().textures[BASECOLORMAP].IsValid())
#else
	if (GetMaterial().textures[BASECOLORMAP].IsValid() && (GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
#endif // PREPASS
	{
		surface.baseColor *= GetMaterial().textures[BASECOLORMAP].Sample(sampler_objectshader, uvsets);
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
	clip(surface.baseColor.a - GetMaterial().alphaTest);
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT


#ifndef WATER
#ifdef OBJECTSHADER_USE_TANGENT
	[branch]
	if (GetMaterial().normalMapStrength > 0 && GetMaterial().textures[NORMALMAP].IsValid())
	{
		surface.bumpColor = float3(GetMaterial().textures[NORMALMAP].Sample(sampler_objectshader, uvsets).rg, 1);
		surface.bumpColor = surface.bumpColor * 2 - 1;
		surface.bumpColor.rg *= GetMaterial().normalMapStrength;
	}
#endif // OBJECTSHADER_USE_TANGENT
#endif // WATER

	surface.layerMask = GetMaterial().layerMask & meshinstance.layerMask;


	float4 surfaceMap = 1;
#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().textures[SURFACEMAP].IsValid())
	{
		surfaceMap = GetMaterial().textures[SURFACEMAP].Sample(sampler_objectshader, uvsets);
	}
#endif // OBJECTSHADER_USE_UVSETS


	[branch]
	if (!GetMaterial().IsUsingSpecularGlossinessWorkflow())
	{
		// Premultiply these before evaluating decals:
		surfaceMap.g *= GetMaterial().roughness;
		surfaceMap.b *= GetMaterial().metalness;
		surfaceMap.a *= GetMaterial().reflectance;
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
		surface.N = normalize(lerp(surface.N, mul(surface.bumpColor, TBN), length(surface.bumpColor)));
	}
#endif // OBJECTSHADER_USE_TANGENT
#endif // WATER


	float4 specularMap = 1;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().textures[SPECULARMAP].IsValid())
	{
		specularMap = GetMaterial().textures[SPECULARMAP].Sample(sampler_objectshader, uvsets);
	}
#endif // OBJECTSHADER_USE_UVSETS



	surface.create(GetMaterial(), surface.baseColor, surfaceMap, specularMap);




#ifdef OBJECTSHADER_USE_EMISSIVE
	// Emissive map:
	surface.emissiveColor = GetMaterial().GetEmissive();

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (any(surface.emissiveColor) && GetMaterial().textures[EMISSIVEMAP].IsValid())
	{
		float4 emissiveMap = GetMaterial().textures[EMISSIVEMAP].Sample(sampler_objectshader, uvsets);
		surface.emissiveColor *= emissiveMap.rgb * emissiveMap.a;
	}
#endif // OBJECTSHADER_USE_UVSETS

	surface.emissiveColor *= Unpack_R11G11B10_FLOAT(meshinstance.emissive);
#endif // OBJECTSHADER_USE_EMISSIVE


#ifdef OBJECTSHADER_USE_UVSETS
	// Secondary occlusion map:
	[branch]
	if (GetMaterial().IsOcclusionEnabled_Secondary() && GetMaterial().textures[OCCLUSIONMAP].IsValid())
	{
		surface.occlusion *= GetMaterial().textures[OCCLUSIONMAP].Sample(sampler_objectshader, uvsets).r;
	}
#endif // OBJECTSHADER_USE_UVSETS


#ifndef PREPASS
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
#ifndef CARTOON
	[branch]
	if (GetCamera().texture_ao_index >= 0)
	{
		surface.occlusion *= bindless_textures_float[GetCamera().texture_ao_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	}
#endif // CARTOON
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // PREPASS


#ifdef ANISOTROPIC
	surface.aniso.strength = GetMaterial().anisotropy_strength;
	surface.aniso.direction = float2(GetMaterial().anisotropy_rotation_cos, GetMaterial().anisotropy_rotation_sin);

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().textures[ANISOTROPYMAP].IsValid())
	{
		float2 anisotropyTexture = GetMaterial().textures[ANISOTROPYMAP].Sample(sampler_objectshader, uvsets).rg * 2 - 1;
		surface.aniso.strength *= length(anisotropyTexture);
		surface.aniso.direction = mul(float2x2(surface.aniso.direction.x, surface.aniso.direction.y, -surface.aniso.direction.y, surface.aniso.direction.x), normalize(anisotropyTexture));
	}
#endif // OBJECTSHADER_USE_UVSETS

	surface.aniso.T = normalize(mul(TBN, float3(surface.aniso.direction, 0)));

#endif // ANISOTROPIC


#ifdef SHEEN
	surface.sheen.color = GetMaterial().GetSheenColor();
	surface.sheen.roughness = GetMaterial().sheenRoughness;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().textures[SHEENCOLORMAP].IsValid())
	{
		surface.sheen.color = GetMaterial().textures[SHEENCOLORMAP].Sample(sampler_objectshader, uvsets).rgb;
	}
	[branch]
	if (GetMaterial().textures[SHEENROUGHNESSMAP].IsValid())
	{
		surface.sheen.roughness = GetMaterial().textures[SHEENROUGHNESSMAP].Sample(sampler_objectshader, uvsets).a;
	}
#endif // OBJECTSHADER_USE_UVSETS
#endif // SHEEN


#ifdef CLEARCOAT
	surface.clearcoat.factor = GetMaterial().clearcoat;
	surface.clearcoat.roughness = GetMaterial().clearcoatRoughness;
	surface.clearcoat.N = input.nor;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().textures[CLEARCOATMAP].IsValid())
	{
		surface.clearcoat.factor = GetMaterial().textures[CLEARCOATMAP].Sample(sampler_objectshader, uvsets).r;
	}
	[branch]
	if (GetMaterial().textures[CLEARCOATROUGHNESSMAP].IsValid())
	{
		surface.clearcoat.roughness = GetMaterial().textures[CLEARCOATROUGHNESSMAP].Sample(sampler_objectshader, uvsets).g;
	}
#ifdef OBJECTSHADER_USE_TANGENT
	[branch]
	if (GetMaterial().textures[CLEARCOATNORMALMAP].IsValid())
	{
		float3 clearcoatNormalMap = float3(GetMaterial().textures[CLEARCOATNORMALMAP].Sample(sampler_objectshader, uvsets).rg, 1);
		clearcoatNormalMap = clearcoatNormalMap * 2 - 1;
		surface.clearcoat.N = mul(clearcoatNormalMap, TBN);
	}
#endif // OBJECTSHADER_USE_TANGENT

	surface.clearcoat.N = normalize(surface.clearcoat.N);

#endif // OBJECTSHADER_USE_UVSETS
#endif // CLEARCOAT


	surface.sss = GetMaterial().subsurfaceScattering;
	surface.sss_inv = GetMaterial().subsurfaceScattering_inv;

	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.update();

	float3 ambient = GetAmbient(surface.N);
	ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);

	
	float4 color = surface.baseColor;

#ifdef WATER
	//NORMALMAP
	float2 bumpColor0 = 0;
	float2 bumpColor1 = 0;
	float2 bumpColor2 = 0;
	[branch]
	if (GetMaterial().textures[NORMALMAP].IsValid())
	{
		Texture2D texture_normalmap = bindless_textures[GetMaterial().textures[NORMALMAP].texture_descriptor];
		const float2 UV_normalMap = GetMaterial().textures[NORMALMAP].GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
		bumpColor0 = 2 * texture_normalmap.Sample(sampler_objectshader, UV_normalMap - GetMaterial().texMulAdd.ww).rg - 1;
		bumpColor1 = 2 * texture_normalmap.Sample(sampler_objectshader, UV_normalMap + GetMaterial().texMulAdd.zw).rg - 1;
	}
	[branch]
	if (GetCamera().texture_waterriples_index >= 0)
	{
		bumpColor2 = bindless_textures_float2[GetCamera().texture_waterriples_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).rg;
	}
	surface.bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * GetMaterial().refraction;
	surface.N = normalize(lerp(surface.N, mul(normalize(surface.bumpColor), TBN), GetMaterial().normalMapStrength));
	surface.bumpColor *= GetMaterial().normalMapStrength;

	[branch]
	if (GetCamera().texture_reflection_index >= 0)
	{
		//REFLECTION
		float4 reflectionUV = mul(GetCamera().reflection_view_projection, float4(surface.P, 1));
		reflectionUV.xy /= reflectionUV.w;
		reflectionUV.xy = clipspace_to_uv(reflectionUV.xy) + surface.bumpColor.rg;
		float3 reflectiveColor = bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_mirror, reflectionUV.xy, 0).rgb;
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
	if (GetMaterial().transmission > 0)
	{
		surface.transmission = GetMaterial().transmission;

#ifdef OBJECTSHADER_USE_UVSETS
		[branch]
		if (GetMaterial().textures[TRANSMISSIONMAP].IsValid())
		{
			float transmissionMap = GetMaterial().textures[TRANSMISSIONMAP].Sample(sampler_objectshader, uvsets).r;
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
			float2 perturbatedRefrTexCoords = ScreenCoord.xy + normal2D * GetMaterial().refraction;
			float4 refractiveColor = texture_refraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, surface.roughness * mipLevels);
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
		float4 ssr = bindless_textures[GetCamera().texture_ssr_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0);
		lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb * surface.F, ssr.a);
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
		surface.refraction.rgb = texture_refraction.SampleLevel(sampler_linear_mirror, refraction_uv, 0).rgb;
		// Recompute depth params again with actual perturbation:
		refraction_depth = texture_depth.SampleLevel(sampler_point_clamp, refraction_uv, 0);
		refraction_position = reconstruct_position(refraction_uv, refraction_depth);
		water_depth = max(water_depth, -dot(float4(refraction_position, 1), water_plane));
		if(camera_above_water)
			water_depth = -water_depth;
		// Water fog computation:
		surface.refraction.a = saturate(exp(-water_depth * color.a));
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


	color = clamp(color, 0, 65000);


	// end point:
#ifdef PREPASS
	coverage = AlphaToCoverage(color.a, GetMaterial().alphaTest, input.pos);

	PrimitiveID prim;
	prim.primitiveIndex = primitiveID;
	prim.instanceIndex = input.GetInstanceIndex();
	prim.subsetIndex = push.geometryIndex - meshinstance.geometryOffset;
	return prim.pack();
#else
	return color;
#endif // PREPASS
}


#endif // OBJECTSHADER_COMPILE_PS



#endif // WI_OBJECTSHADER_HF

