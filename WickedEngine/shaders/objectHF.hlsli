#ifndef WI_OBJECTSHADER_HF
#define WI_OBJECTSHADER_HF

#ifdef TRANSPARENT
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
#else
#define SHADOW_MASK_ENABLED
#endif

#if !defined(TRANSPARENT) && !defined(PREPASS)
#define DISABLE_ALPHATEST
#endif

#ifdef PLANARREFLECTION
#define DISABLE_ENVMAPS
#define DISABLE_VOXELGI
#endif

#ifdef WATER
#define DISABLE_VOXELGI
#endif

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

	float4 GetPosition()
	{
		return float4(bindless_buffers[GetMesh().vb_pos_nor_wind].Load<float3>(vertexID * sizeof(uint4)), 1);
	}
	float3 GetNormal()
	{
		const uint normal_wind = bindless_buffers[GetMesh().vb_pos_nor_wind].Load<uint4>(vertexID * sizeof(uint4)).w;
		float3 normal;
		normal.x = (float)((normal_wind >> 0u) & 0xFF) / 255.0 * 2 - 1;
		normal.y = (float)((normal_wind >> 8u) & 0xFF) / 255.0 * 2 - 1;
		normal.z = (float)((normal_wind >> 16u) & 0xFF) / 255.0 * 2 - 1;
		return normal;
	}
	float GetWindWeight()
	{
		const uint normal_wind = bindless_buffers[GetMesh().vb_pos_nor_wind].Load<uint4>(vertexID * sizeof(uint4)).w;
		return ((normal_wind >> 24u) & 0xFF) / 255.0;
	}

	float4 GetUVSets()
	{
		[branch]
		if (GetMesh().vb_uvs < 0)
			return 0;
		return unpack_half4(bindless_buffers[GetMesh().vb_uvs].Load2(vertexID * sizeof(uint2)));
	}

	ShaderMeshInstancePointer GetInstancePointer()
	{
		if (push.instances >= 0)
			return bindless_buffers[push.instances].Load<ShaderMeshInstancePointer>(push.instance_offset + instanceID * sizeof(ShaderMeshInstancePointer));

		ShaderMeshInstancePointer poi;
		poi.init();
		return poi;
	}

	float2 GetAtlasUV()
	{
		[branch]
		if (GetMesh().vb_atl < 0)
			return 0;
		return unpack_half2(bindless_buffers[GetMesh().vb_atl].Load(vertexID * sizeof(uint)));
	}

	float4 GetVertexColor()
	{
		[branch]
		if (GetMesh().vb_col < 0)
			return 1;
		return unpack_rgba(bindless_buffers[GetMesh().vb_col].Load(vertexID * sizeof(uint)));
	}

	float4 GetTangent()
	{
		[branch]
		if (GetMesh().vb_tan < 0)
			return 0;
		return unpack_utangent(bindless_buffers[GetMesh().vb_tan].Load(vertexID * sizeof(uint))) * 2 - 1;
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
	float2 atlas;
	float4 color;
	float3 normal;
	float4 tangent;
	uint emissiveColor;

	inline void create(in ShaderMaterial material, in VertexInput input)
	{
		position = input.GetPosition();
		color = GetMaterial().baseColor * unpack_rgba(input.GetInstance().color);
		color.a *= 1 - input.GetInstancePointer().GetDither();
		emissiveColor = input.GetInstance().emissive;

		[branch]
		if (material.IsUsingVertexColors())
		{
			color *= input.GetVertexColor();
		}
		
		normal = normalize(mul((float3x3)input.GetInstance().transformInverseTranspose.GetMatrix(), input.GetNormal()));

		tangent = input.GetTangent();
		tangent.xyz = normalize(mul((float3x3)input.GetInstance().transformInverseTranspose.GetMatrix(), tangent.xyz));

		uvsets = input.GetUVSets();
		uvsets.xy = mad(uvsets.xy, material.texMulAdd.xy, material.texMulAdd.zw);

		atlas = input.GetAtlasUV();

		position = mul(input.GetInstance().transform.GetMatrix(), position);

#ifndef DISABLE_WIND
		[branch]
		if (material.IsUsingWind())
		{
			position.xyz += compute_wind(position.xyz, input.GetWindWeight());
		}
#endif // DISABLE_WIND
	}
};

struct PixelInput
{
	precise float4 pos : SV_POSITION;

#ifdef OBJECTSHADER_USE_INSTANCEINDEX
	uint instanceIndex : INSTANCEINDEX;
#endif // OBJECTSHADER_USE_INSTANCEINDEX

#ifdef OBJECTSHADER_USE_CLIPPLANE
	float  clip : SV_ClipDistance0;
#endif // OBJECTSHADER_USE_CLIPPLANE

#ifdef OBJECTSHADER_USE_DITHERING
	nointerpolation float dither : DITHER;
#endif // OBJECTSHADER_USE_DITHERING

#ifdef OBJECTSHADER_USE_EMISSIVE
	uint emissiveColor : EMISSIVECOLOR;
#endif // OBJECTSHADER_USE_EMISSIVE

#ifdef OBJECTSHADER_USE_COLOR
	float4 color : COLOR;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_UVSETS
	float4 uvsets : UVSETS;
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_ATLAS
	float2 atl : ATLAS;
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_NORMAL
	float3 nor : NORMAL;
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_TANGENT
	float4 tan : TANGENT;
#endif // OBJECTSHADER_USE_TANGENT

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
};


// OBJECT SHADER PROTOTYPE
///////////////////////////

#ifdef OBJECTSHADER_COMPILE_VS

// Vertex shader base:
PixelInput main(VertexInput input)
{
	PixelInput Out;

#ifdef OBJECTSHADER_USE_INSTANCEINDEX
	Out.instanceIndex = input.GetInstancePointer().GetInstanceIndex();
#endif // OBJECTSHADER_USE_INSTANCEINDEX

	VertexSurface surface;
	surface.create(GetMaterial(), input);

	Out.pos = surface.position;

#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pos = mul(GetCamera().view_projection, Out.pos);
#endif // OBJECTSHADER_USE_NOCAMERA

#ifdef OBJECTSHADER_USE_CLIPPLANE
	Out.clip = dot(surface.position, GetCamera().clip_plane);
#endif // OBJECTSHADER_USE_CLIPPLANE

#ifdef OBJECTSHADER_USE_POSITION3D
	Out.pos3D = surface.position.xyz;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_COLOR
	Out.color = surface.color;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_DITHERING
	Out.dither = surface.color.a;
#endif // OBJECTSHADER_USE_DITHERING

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

#ifdef OBJECTSHADER_USE_EMISSIVE
	Out.emissiveColor = surface.emissiveColor;
#endif // OBJECTSHADER_USE_EMISSIVE

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
	const uint frustum_index = input.GetInstancePointer().GetFrustumIndex();
	Out.RTIndex = GetCamera(frustum_index).output_index;
#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pos = mul(GetCamera(frustum_index).view_projection, surface.position);
#endif // OBJECTSHADER_USE_NOCAMERA
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX

#ifdef OBJECTSHADER_USE_VIEWPORTARRAYINDEX
	const uint frustum_index = input.GetInstancePointer().GetFrustumIndex();
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

#ifndef DISABLE_ALPHATEST
#ifndef TRANSPARENT
#ifndef ENVMAPRENDERING
#ifdef OBJECTSHADER_USE_DITHERING
	// apply dithering:
	clip(dither(pixel + GetTemporalAASampleRotation()) - (1 - input.dither));
#endif // OBJECTSHADER_USE_DITHERING
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT
#endif // ENVMAPRENDERING


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
	float3x3 TBN = compute_tangent_frame(surface.N, surface.P, input.uvsets.xy);
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
		float2 uv = GetMaterial().textures[DISPLACEMENTMAP].GetUVSet() == 0 ? input.uvsets.xy : input.uvsets.zw;
		float2 uv_dx = ddx_coarse(uv);
		float2 uv_dy = ddy_coarse(uv);

		ParallaxOcclusionMapping_Impl(
			input.uvsets,
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
		surface.baseColor *= GetMaterial().textures[BASECOLORMAP].Sample(sampler_objectshader, input.uvsets);
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
		surface.bumpColor = float3(GetMaterial().textures[NORMALMAP].Sample(sampler_objectshader, input.uvsets).rg, 1);
		surface.bumpColor = surface.bumpColor * 2 - 1;
		surface.bumpColor.rg *= GetMaterial().normalMapStrength;
	}
#endif // OBJECTSHADER_USE_TANGENT
#endif // WATER

	surface.layerMask = GetMaterial().layerMask & load_instance(input.instanceIndex).layerMask;


	float4 surfaceMap = 1;
#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().textures[SURFACEMAP].IsValid())
	{
		surfaceMap = GetMaterial().textures[SURFACEMAP].Sample(sampler_objectshader, input.uvsets);
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
		specularMap = GetMaterial().textures[SPECULARMAP].Sample(sampler_objectshader, input.uvsets);
	}
#endif // OBJECTSHADER_USE_UVSETS



	surface.create(GetMaterial(), surface.baseColor, surfaceMap, specularMap);


	// Emissive map:
	surface.emissiveColor = GetMaterial().GetEmissive();

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (any(surface.emissiveColor) && GetMaterial().textures[EMISSIVEMAP].IsValid())
	{
		float4 emissiveMap = GetMaterial().textures[EMISSIVEMAP].Sample(sampler_objectshader, input.uvsets);
		surface.emissiveColor *= emissiveMap.rgb * emissiveMap.a;
	}
#endif // OBJECTSHADER_USE_UVSETS



#ifdef OBJECTSHADER_USE_EMISSIVE
	surface.emissiveColor *= Unpack_R11G11B10_FLOAT(input.emissiveColor);
#endif // OBJECTSHADER_USE_EMISSIVE


#ifdef OBJECTSHADER_USE_UVSETS
	// Secondary occlusion map:
	[branch]
	if (GetMaterial().IsOcclusionEnabled_Secondary() && GetMaterial().textures[OCCLUSIONMAP].IsValid())
	{
		surface.occlusion *= GetMaterial().textures[OCCLUSIONMAP].Sample(sampler_objectshader, input.uvsets).r;
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
		float2 anisotropyTexture = GetMaterial().textures[ANISOTROPYMAP].Sample(sampler_objectshader, input.uvsets).rg * 2 - 1;
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
		surface.sheen.color = GetMaterial().textures[SHEENCOLORMAP].Sample(sampler_objectshader, input.uvsets).rgb;
	}
	[branch]
	if (GetMaterial().textures[SHEENROUGHNESSMAP].IsValid())
	{
		surface.sheen.roughness = GetMaterial().textures[SHEENROUGHNESSMAP].Sample(sampler_objectshader, input.uvsets).a;
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
		surface.clearcoat.factor = GetMaterial().textures[CLEARCOATMAP].Sample(sampler_objectshader, input.uvsets).r;
	}
	[branch]
	if (GetMaterial().textures[CLEARCOATROUGHNESSMAP].IsValid())
	{
		surface.clearcoat.roughness = GetMaterial().textures[CLEARCOATROUGHNESSMAP].Sample(sampler_objectshader, input.uvsets).g;
	}
#ifdef OBJECTSHADER_USE_TANGENT
	[branch]
	if (GetMaterial().textures[CLEARCOATNORMALMAP].IsValid())
	{
		float3 clearcoatNormalMap = float3(GetMaterial().textures[CLEARCOATNORMALMAP].Sample(sampler_objectshader, input.uvsets).rg, 1);
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



#ifdef WATER

	//NORMALMAP
	float2 bumpColor0 = 0;
	float2 bumpColor1 = 0;
	float2 bumpColor2 = 0;
	[branch]
	if (GetMaterial().textures[NORMALMAP].IsValid())
	{
		Texture2D texture_normalmap = bindless_textures[GetMaterial().textures[NORMALMAP].texture_descriptor];
		const float2 UV_normalMap = GetMaterial().textures[NORMALMAP].GetUVSet() == 0 ? input.uvsets.xy : input.uvsets.zw;
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
		reflectionUV.xy = clipspace_to_uv(reflectionUV.xy);
		lighting.indirect.specular += bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_mirror, reflectionUV.xy + surface.bumpColor.rg, 0).rgb * surface.F;
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
			float transmissionMap = GetMaterial().textures[TRANSMISSIONMAP].Sample(sampler_objectshader, input.uvsets).r;
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
	LightMapping(load_instance(input.instanceIndex).lightmap, input.atl, lighting, surface);
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


	float4 color = surface.baseColor;

#ifdef WATER
	[branch]
	if (GetCamera().texture_refraction_index >= 0)
	{
		// WATER REFRACTION
		Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
		float sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy + surface.bumpColor.rg, 0) * GetCamera().z_far;
		float depth_difference = sampled_lineardepth - lineardepth;
		surface.refraction.rgb = texture_refraction.SampleLevel(sampler_linear_mirror, ScreenCoord.xy + surface.bumpColor.rg * saturate(0.5 * depth_difference), 0).rgb;
		if (depth_difference < 0)
		{
			// Fix cutoff by taking unperturbed depth diff to fill the holes with fog:
			sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy, 0) * GetCamera().z_far;
			depth_difference = sampled_lineardepth - lineardepth;
		}
		// WATER FOG:
		surface.refraction.a = 1 - saturate(color.a * 0.1 * depth_difference);
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
	prim.instanceIndex = input.instanceIndex;
	prim.subsetIndex = push.geometryIndex - load_instance(input.instanceIndex).geometryOffset;
	return prim.pack();
#else
	return color;
#endif // PREPASS
}


#endif // OBJECTSHADER_COMPILE_PS



#endif // WI_OBJECTSHADER_HF

