#ifndef WI_HAIRPARTICLE_HF
#define WI_HAIRPARTICLE_HF
#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

ShaderMeshInstance HairGetInstance()
{
	return load_instance(xHairInstanceIndex);
}
ShaderGeometry HairGetGeometry()
{
	return load_geometry(HairGetInstance().geometryOffset);
}
ShaderMaterial HairGetMaterial()
{
	return load_material(HairGetGeometry().materialIndex);
}

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float clip : SV_ClipDistance0;
	float2 tex : TEXCOORD;
	nointerpolation float fade : DITHERFADE;
	uint primitiveID : PRIMITIVEID;
	half3 nor : NORMAL;
	half wet : WET;
	
	inline float3 GetPos3D()
	{
		ShaderCamera camera = GetCamera();
		const float2 ScreenCoord = pos.xy * camera.internal_resolution_rcp; // use pixel center!
		const float z = camera.IsOrtho() ? (1 - pos.z) : inverse_lerp(camera.z_near, camera.z_far, pos.w);
		return camera.frustum_corners.position_from_screen(ScreenCoord, z);
	}

	inline float3 GetViewVector()
	{
		ShaderCamera camera = GetCamera();
		const float2 ScreenCoord = pos.xy * camera.internal_resolution_rcp; // use pixel center!
		return camera.frustum_corners.position_near(ScreenCoord) - GetPos3D(); // ortho support, cannot use cameraPos!
	}
};

#endif // WI_HAIRPARTICLE_HF
