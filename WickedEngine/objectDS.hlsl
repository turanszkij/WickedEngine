#include "objectHF.hlsli"


struct ConstantOutputType
{
	float fTessFactor[3] : SV_TessFactor;
    float fInsideTessFactor : SV_InsideTessFactor;

	float4 pos0 : POSITION0;
	float4 pos1 : POSITION1;
	float4 pos2 : POSITION2;

	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;

	float4 uvsets0 : UVSETS0;
	float4 uvsets1 : UVSETS1;
	float4 uvsets2 : UVSETS2;

	float4 atlas0 : ATLAS0;
	float4 atlas1 : ATLAS1;
	float4 atlas2 : ATLAS2;

	float4 nor0 : NORMAL0;
	float4 nor1 : NORMAL1;
	float4 nor2 : NORMAL2;

	float4 tan0 : TANGENT0;
	float4 tan1 : TANGENT1;
	float4 tan2 : TANGENT2;

	float4 posPrev0 : POSITIONPREV0;
	float4 posPrev1 : POSITIONPREV1;
	float4 posPrev2 : POSITIONPREV2;
};

struct HullOutputType
{
	float4 pos								: POSITION;
	float4 color							: COLOR;
	float4 uvsets							: UVSETS;
	float4 atlas							: ATLAS;
	float4 nor								: NORMAL;
	float4 tan								: TANGENT;
	float4 posPrev							: POSITIONPREV;
};




//=================================================================================================================================
// Utility phong functions
//=================================================================================================================================
float3 project(float3 p, float3 c, float3 n)
{
    return p - dot(p - c, n) * n;
}

// Computes the position of a point in the Phong Tessellated triangle
float3 PhongGeometry(float u, float v, float w, ConstantOutputType hsc)
{
	// Find local space point
	float3 p = w * hsc.pos0.xyz + u * hsc.pos1.xyz + v * hsc.pos2.xyz;
	// Find projected vectors
	float3 c0 = project(p, hsc.pos0.xyz, hsc.nor0.xyz);
	float3 c1 = project(p, hsc.pos1.xyz, hsc.nor1.xyz);
	float3 c2 = project(p, hsc.pos2.xyz, hsc.nor2.xyz);
	// Interpolate
	float3 q = w * c0 + u * c1 + v * c2;
	// For blending between tessellated and untessellated model:
	//float3 r = LERP(p, q, alpha);
	return q;
}
float3 PhongGeometryPrev(float u, float v, float w, ConstantOutputType hsc)
{
	// Find local space point
	float3 p = w * hsc.posPrev0.xyz + u * hsc.posPrev1.xyz + v * hsc.posPrev2.xyz;
	// Find projected vectors
	float3 c0 = project(p, hsc.posPrev0.xyz, hsc.nor0.xyz);
	float3 c1 = project(p, hsc.posPrev1.xyz, hsc.nor1.xyz);
	float3 c2 = project(p, hsc.posPrev2.xyz, hsc.nor2.xyz);
	// Interpolate
	float3 q = w * c0 + u * c1 + v * c2;
	// For blending between tessellated and untessellated model:
	//float3 r = LERP(p, q, alpha);
	return q;
}

// Computes the normal of a point in the Phong Tessellated triangle
float3 PhongNormal(float u, float v, float w, ConstantOutputType hsc)
{
    // Interpolate
    return normalize(w * hsc.nor0.xyz + u * hsc.nor1.xyz + v * hsc.nor2.xyz);
}
float4 PhongTangent(float u, float v, float w, ConstantOutputType hsc)
{
	// Interpolate
	return normalize(w * hsc.tan0 + u * hsc.tan1 + v * hsc.tan2);
}

////////////////////////////////////////////////////////////////////////////////
// Domain Shader
////////////////////////////////////////////////////////////////////////////////
[domain("tri")]
PixelInputType main(ConstantOutputType input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<HullOutputType, 3> patch)
{
    PixelInputType Out;


	float4 vertexPosition;
	float4 vertexPositionPrev;
	float3 vertexNormal;
	float4 vertexTangent;
	float4 vertexUvsets;
	float2 vertexAtlas;
	float4 vertexColor;

    //New vertex returned from tessallator, average it
	vertexUvsets = uvwCoord.z * patch[0].uvsets + uvwCoord.x * patch[1].uvsets + uvwCoord.y * patch[2].uvsets;
	vertexAtlas = uvwCoord.z * patch[0].atlas.xy + uvwCoord.x * patch[1].atlas.xy + uvwCoord.y * patch[2].atlas.xy;
	vertexColor = uvwCoord.z * patch[0].color + uvwCoord.x * patch[1].color + uvwCoord.y * patch[2].color;

	
	// The barycentric coordinates
    float fU = uvwCoord.x;
    float fV = uvwCoord.y;
    float fW = uvwCoord.z;

    
    // Compute position
	vertexPosition = float4(PhongGeometry(fU, fV, fW, input), 1);
	vertexPositionPrev = float4(PhongGeometryPrev(fU, fV, fW, input), 1);
    // Compute normal
	vertexNormal = PhongNormal(fU, fV, fW, input);
	vertexTangent = PhongTangent(fU, fV, fW, input);

	// Displacement
	float3 displacement = -g_xMaterial.displacementMapping * vertexNormal.xyz;
	[branch]
	if (g_xMaterial.uvset_displacementMap >= 0)
	{
		const float2 UV_displacementMap = g_xMaterial.uvset_displacementMap == 0 ? vertexUvsets.xy : vertexUvsets.zw;
		displacement *= 1 - texture_displacementmap.SampleLevel(sampler_linear_wrap, UV_displacementMap, 0).rrr;
	}
	vertexPosition.xyz += displacement;
	vertexPositionPrev.xyz += displacement;
	
	Out.clip = dot(vertexPosition, g_xClipPlane);
	
	Out.pos = mul(g_xCamera_VP, vertexPosition);
	Out.pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, vertexPositionPrev);
	Out.pos3D = vertexPosition.xyz;
	Out.uvsets = vertexUvsets;
	Out.atl = vertexAtlas;
	Out.color = vertexColor;
	Out.nor = vertexNormal;
	Out.tan = vertexTangent;

	return Out;
}