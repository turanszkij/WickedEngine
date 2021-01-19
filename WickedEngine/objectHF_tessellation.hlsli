#include "objectHF.hlsli"

struct ConstantOutputType
{
	float fTessFactor[3] : SV_TessFactor;
	float fInsideTessFactor : SV_InsideTessFactor;
};

#ifdef OBJECTSHADER_COMPILE_HS

ConstantOutputType PatchConstantFunction(InputPatch<PixelInput, 3> I)
{
	ConstantOutputType Out;

	const float MODIFIER = 0.6f;

	float fDistance;
	float3 f3MidPoint;
	// Edge 0
	f3MidPoint = (I[2].pos.xyz + I[0].pos.xyz) / 2.0f;
	fDistance = distance(f3MidPoint, g_xCamera_CamPos.xyz) * MODIFIER - xTessellationFactors.z;
	Out.fTessFactor[0] = xTessellationFactors.x * (1.0f - clamp((fDistance / xTessellationFactors.w), 0.0f, 1.0f - (1.0f / xTessellationFactors.x)));
	// Edge 1
	f3MidPoint = (I[0].pos.xyz + I[1].pos.xyz) / 2.0f;
	fDistance = distance(f3MidPoint, g_xCamera_CamPos.xyz) * MODIFIER - xTessellationFactors.z;
	Out.fTessFactor[1] = xTessellationFactors.x * (1.0f - clamp((fDistance / xTessellationFactors.w), 0.0f, 1.0f - (1.0f / xTessellationFactors.x)));
	// Edge 2
	f3MidPoint = (I[1].pos.xyz + I[2].pos.xyz) / 2.0f;
	fDistance = distance(f3MidPoint, g_xCamera_CamPos.xyz) * MODIFIER - xTessellationFactors.z;
	Out.fTessFactor[2] = xTessellationFactors.x * (1.0f - clamp((fDistance / xTessellationFactors.w), 0.0f, 1.0f - (1.0f / xTessellationFactors.x)));
	// Inside
	Out.fInsideTessFactor = (Out.fTessFactor[0] + Out.fTessFactor[1] + Out.fTessFactor[2]) / 3.0f;

	return Out;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
PixelInput main(InputPatch<PixelInput, 3> patch, uint pointID : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
	return patch[pointID];
}
#endif // OBJECTSHADER_COMPILE_HS



#ifdef OBJECTSHADER_COMPILE_DS

float3 project(float3 p, float3 c, float3 n)
{
	return p - dot(p - c, n) * n;
}

[domain("tri")]
PixelInput main(ConstantOutputType input, float3 uvw : SV_DomainLocation, const OutputPatch<PixelInput, 3> patch)
{
	PixelInput Out = patch[0];

	const float u = uvw.x;
	const float v = uvw.y;
	const float w = uvw.z;

	// Phong smoothing - positions
	{
		float3 p = w * patch[0].pos.xyz + u * patch[1].pos.xyz + v * patch[2].pos.xyz;
		float3 c0 = project(p, patch[0].pos.xyz, patch[0].nor.xyz);
		float3 c1 = project(p, patch[1].pos.xyz, patch[1].nor.xyz);
		float3 c2 = project(p, patch[2].pos.xyz, patch[2].nor.xyz);
		Out.pos.xyz = w * c0 + u * c1 + v * c2;
	}

#ifdef OBJECTSHADER_USE_POSITIONPREV
	// Phong smoothing - previous frame positions
	{
		float3 p = w * patch[0].pre.xyz + u * patch[1].pre.xyz + v * patch[2].pre.xyz;
		float3 c0 = project(p, patch[0].pre.xyz, patch[0].nor.xyz);
		float3 c1 = project(p, patch[1].pre.xyz, patch[1].nor.xyz);
		float3 c2 = project(p, patch[2].pre.xyz, patch[2].nor.xyz);
		Out.pre.xyz = w * c0 + u * c1 + v * c2;
	}
#endif // OBJECTSHADER_USE_POSITIONPREV

#ifdef OBJECTSHADER_USE_COLOR
	Out.color = w * patch[0].color + u * patch[1].color + v * patch[2].color;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_UVSETS
	Out.uvsets = w * patch[0].uvsets + u * patch[1].uvsets + v * patch[2].uvsets;
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_ATLAS
	Out.atl = w * patch[0].atl + u * patch[1].atl + v * patch[2].atl;
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_NORMAL
	Out.nor = normalize(w * patch[0].nor + u * patch[1].nor + v * patch[2].nor);
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_TANGENT
	Out.tan = normalize(w * patch[0].tan + u * patch[1].tan + v * patch[2].tan);
#endif // OBJECTSHADER_USE_TANGENT

#ifdef OBJECTSHADER_USE_POSITION3D
	Out.pos3D = Out.pos.xyz;
#endif // OBJECTSHADER_USE_POSITION3D




#ifdef OBJECTSHADER_USE_CLIPPLANE
	Out.clip = dot(Out.pos, g_xCamera_ClipPlane);
#endif // OBJECTSHADER_USE_CLIPPLANE

#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pos = mul(g_xCamera_VP, Out.pos);
#endif // OBJECTSHADER_USE_NOCAMERA

#ifdef OBJECTSHADER_USE_POSITIONPREV
#ifndef OBJECTSHADER_USE_NOCAMERA
	Out.pre = mul(g_xCamera_PrevVP, Out.pre);
#endif // OBJECTSHADER_USE_NOCAMERA
#endif // OBJECTSHADER_USE_POSITIONPREV

	return Out;
}

#endif // OBJECTSHADER_COMPILE_DS
