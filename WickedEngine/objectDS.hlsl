#include "objectHF.hlsli"


struct ConstantOutputType
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;

	float3 f3B0 : POSITION0;
	float3 f3B1 : POSITION1;
	float3 f3B2 : POSITION2;

	float3 f3PrevB0 : POSITIONPREV0;
	float3 f3PrevB1 : POSITIONPREV1;
	float3 f3PrevB2 : POSITIONPREV2;

	float4 f4N0 : NORMAL0;
	float4 f4N1 : NORMAL1;
	float4 f4N2 : NORMAL2;
};

struct HullOutputType
{
	float3 pos								: POSITION;
	float3 posPrev							: POSITIONPREV;
	float4 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
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
	float3 p = w * hsc.f3B0 + u * hsc.f3B1 + v * hsc.f3B2;
	// Find projected vectors
	float3 c0 = project(p, hsc.f3B0, hsc.f4N0.xyz);
	float3 c1 = project(p, hsc.f3B1, hsc.f4N1.xyz);
	float3 c2 = project(p, hsc.f3B2, hsc.f4N2.xyz);
	// Interpolate
	float3 q = w * c0 + u * c1 + v * c2;
	// For blending between tessellated and untessellated model:
	//float3 r = LERP(p, q, alpha);
	return q;
}
float3 PhongGeometryPrev(float u, float v, float w, ConstantOutputType hsc)
{
	// Find local space point
	float3 p = w * hsc.f3PrevB0 + u * hsc.f3PrevB1 + v * hsc.f3PrevB2;
	// Find projected vectors
	float3 c0 = project(p, hsc.f3PrevB0, hsc.f4N0.xyz);
	float3 c1 = project(p, hsc.f3PrevB1, hsc.f4N1.xyz);
	float3 c2 = project(p, hsc.f3PrevB2, hsc.f4N2.xyz);
	// Interpolate
	float3 q = w * c0 + u * c1 + v * c2;
	// For blending between tessellated and untessellated model:
	//float3 r = LERP(p, q, alpha);
	return q;
}

// Computes the normal of a point in the Phong Tessellated triangle
float4 PhongNormal(float u, float v, float w, ConstantOutputType hsc)
{
    // Interpolate
    return normalize(w * hsc.f4N0 + u * hsc.f4N1 + v * hsc.f4N2);
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
	float4 vertexNormal;
	float4 vertexTex;

    //New vertex returned from tessallator, average it
	vertexTex = uvwCoord.z * patch[0].tex + uvwCoord.x * patch[1].tex + uvwCoord.y * patch[2].tex;

	
	// The barycentric coordinates
    float fU = uvwCoord.x;
    float fV = uvwCoord.y;
    float fW = uvwCoord.z;

    
    // Compute position
	vertexPosition = float4(PhongGeometry(fU, fV, fW, input), 1);
	vertexPositionPrev = float4(PhongGeometryPrev(fU, fV, fW, input), 1);
    // Compute normal
	vertexNormal = PhongNormal(fU, fV, fW, input);

	// Displacement
	float3 displacement = 1 - xDisplacementMap.SampleLevel(sampler_linear_wrap, vertexTex.xy, 0).rrr;
	displacement *= vertexNormal.xyz;
	displacement *= 0.1f; // todo: param
	vertexPosition.xyz += displacement;
	vertexPositionPrev.xyz += displacement;
	
	Out.clip = dot(vertexPosition, g_xClipPlane);
	
	Out.pos = Out.pos2D = mul( vertexPosition, g_xCamera_VP );
	Out.pos2DPrev = mul(vertexPositionPrev, g_xFrame_MainCamera_PrevVP);
	Out.pos3D = vertexPosition.xyz;
	Out.tex = vertexTex.xy;
	Out.nor = normalize(vertexNormal.xyz);
	Out.nor2D = mul(Out.nor.xyz, (float3x3)g_xCamera_View).xy;
	Out.atl = vertexTex.zw;

	Out.ReflectionMapSamplingPos = mul(vertexPosition, g_xFrame_MainCamera_ReflVP);

	Out.instanceColor = patch[0].instanceColor.rgb;
	Out.dither = patch[0].dither;


	return Out;
}