#include "objectHF.hlsli"


struct ConstantOutputType
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;

	float3 f3B0 : POSITION0;
	float3 f3B1 : POSITION1;
	float3 f3B2 : POSITION2;

	float4 f4N0 : NORMAL0;
	float4 f4N1 : NORMAL1;
	float4 f4N2 : NORMAL2;
};

struct HullOutputType
{
	float3 pos								: POSITION;
	float3 tex								: TEXCOORD0;
	float3 nor								: NORMAL;
	float3 vel								: TEXCOORD1;
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
    PixelInputType Out = (PixelInputType)0;


    float4 vertexPosition;
	float4 vertexNormal;
	float2 vertexTex;
	float3 vertexVel;

    //New vertex returned from tessallator, average it
	vertexTex      = uvwCoord.z * patch[0].tex.xy + uvwCoord.x * patch[1].tex.xy + uvwCoord.y * patch[2].tex.xy;
	vertexVel	   = uvwCoord.z * patch[0].vel + uvwCoord.x * patch[1].vel + uvwCoord.y * patch[2].vel;

	
	// The barycentric coordinates
    float fU = uvwCoord.x;
    float fV = uvwCoord.y;
    float fW = uvwCoord.z;

    
    // Compute position
    vertexPosition = float4(PhongGeometry(fU, fV, fW, input),1);
    // Compute normal
    vertexNormal   = PhongNormal(fU, fV, fW, input);
	
	Out.clip = dot(vertexPosition, g_xClipPlane);

	////DISPLACEMENT
	//if(xDisplace[(uint)patch[0].tex.z]) vertexPosition.xyz += vertexNormal * (-1+dispMap.SampleLevel( sampler_objectshader,vertexTex,0 ).r) *0.4;
	//

	
	Out.pos = Out.pos2D = mul( vertexPosition, g_xCamera_VP );
	Out.pos3D = vertexPosition.xyz;
	Out.tex = vertexTex.xy;
	Out.nor = normalize(vertexNormal.xyz);
	/*float2x3 tanbin = tangentBinormal(Out.nor);
	Out.tan=tanbin[0];
	Out.bin=tanbin[1];*/

	Out.ReflectionMapSamplingPos = mul(vertexPosition, g_xFrame_MainCamera_ReflVP);
	/*Out.ShadowMapSamplingPos[0] = mul(vertexPosition,  xShViewOprojection[0] );
	Out.ShadowMapSamplingPos[1] = mul(vertexPosition,  xShViewOprojection[1] );
	Out.ShadowMapSamplingPos[2] = mul(vertexPosition,  xShViewOprojection[2] );*/


	
	//Out.Position3D = vertexPosition.xyz;
	//Out.EyeVec = normalize(xCamPos.xyz - vertexPosition.xyz);
	
	
	//Out.vel = vertexVel;
	//Out.vel += (Out.pos.xyz-mul(vertexPosition,xPrevViewProjection).xyz)*0.10;
	//Out.mat = mat;

	Out.ao = vertexNormal.w;

	Out.instanceColor = patch[0].instanceColor.rgb;
	Out.dither = patch[0].dither;


	return Out;
}