#ifndef WI_HAIRPARTICLE_HF
#define WI_HAIRPARTICLE_HF
#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

#define HairGetInstance() (load_instance(xHairInstanceIndex))
#define HairGetGeometry() (load_geometry(HairGetInstance().geometryOffset))
#define HairGetMaterial() (load_material(HairGetGeometry().materialIndex))

struct VertexToPixel
{
	precise float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
	half4 nor_wet : NORMAL_WET;
	float clip : SV_ClipDistance0;
	nointerpolation float fade : DITHERFADE;
	nointerpolation uint primitiveID : PRIMITIVEID;
	
	inline float3 GetPos3D()
	{
		return GetCamera().screen_to_world(pos);
	}

	inline float3 GetViewVector()
	{
		return GetCamera().screen_to_nearplane(pos) - GetPos3D(); // ortho support, cannot use cameraPos!
	}

	inline half GetDither()
	{
		return fade;
	}
};

#endif // WI_HAIRPARTICLE_HF
