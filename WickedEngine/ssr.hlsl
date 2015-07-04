#include "postProcessHF.hlsli"

cbuffer prop:register(b0){
	float4 xAmbient;
	float4 xHorizon;
	float4x4 matProjInv;
	float4 xFogSEH;
};
#include "reconstructPositionHF.hlsli"

static const float	g_fRayStep = 0.05f;
static const float	g_fMinRayStep = 0.02f;
static const int	g_iMaxSteps = 50;
static const float	g_fSearchDist = 30.f;
static const float	g_fSearchDistInv = 1.0f / g_fSearchDist;
static const int	g_iNumBinarySearchSteps = 15;
static const float	g_fMaxDDepth = 1.4f;
static const float	g_fMaxDDepthInv = 1.0f / g_fMaxDDepth;

static const float	g_fReflectionSpecularFalloffExponent = 2.0f;

float3 SSRBinarySearch(float3 vDir, inout float3 vHitCoord, out float fDepthDiff)
{
	float fDepth;

	for (int i = 0; i < g_iNumBinarySearchSteps; i++)
	{
		float4 vProjectedCoord = mul(float4(vHitCoord, 1.0f), matProj);
			vProjectedCoord.xy *= float2(1, -1);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * 0.5 + 0.5;

		fDepth = loadMask(vProjectedCoord.xy).r; //contains linearDepth!

		fDepthDiff = vHitCoord.z - fDepth;

		[branch]
		if (fDepthDiff < 0.0f)
			vHitCoord += vDir;

		vDir *= 0.5f;
		vHitCoord -= vDir;
	}

	float4 vProjectedCoord = mul(float4(vHitCoord, 1.0f), matProj);
		vProjectedCoord.xy *= float2(1, -1);
	vProjectedCoord.xy /= vProjectedCoord.w;
	vProjectedCoord.xy = vProjectedCoord.xy * 0.5 + 0.5;

	return float3(vProjectedCoord.xy, fDepth);
}

float4 SSRRayMarch(float3 vDir, inout float3 vHitCoord, out float fDepthDiff)
{
	vDir *= g_fRayStep;

	float fDepth;

	for (int i = 0; i < g_iMaxSteps; i++)
	{
		float fPrevDepth = vHitCoord.z;

		vHitCoord += vDir;

		float4 vProjectedCoord = mul(float4(vHitCoord, 1.0f), matProj);
			vProjectedCoord.xy *= float2(1, -1);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * 0.5 + 0.5;

		fDepth = loadMask(vProjectedCoord.xy).r; //contains linearDepth!

		fDepthDiff = fPrevDepth - fDepth;

		[branch]
		if (fDepthDiff > 0.0f && fDepthDiff < g_fMaxDDepth)
			return float4(SSRBinarySearch(vDir, vHitCoord, fDepthDiff), 1.0f);

	}

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

float4 main( VertextoPixel input ) : SV_Target
{
	float4 o = 1;

	//float3 vSceneColor = loadScene(input.tex).rgb;

	float4 vNormZ;
	vNormZ.rgb = loadNormal(input.tex).rgb;
	vNormZ.a = loadDepth(input.tex);

	float2 vRougnessSpec = loadVelocity(input.tex).zw; //specular_power,specular intensity
	//float fSpecularModifier = (1 - clamp(vRougnessSpec.x, 0, 256) / 256.f)*vRougnessSpec.y;

	float3 vWorldPos = getPosition(input.tex, vNormZ.a);


	//if (vRougnessSpec.y == 0.0f)
	//{
	//	//o.rgb = vSceneColor;
	//	o.rgb = float4(0, 0, 0, 0);
	//	return o;
	//}

	//clip(fSpecularModifier < 0.01f ? -1 : 1);

	//Reflection vector
	float3 vViewPos = mul(float4(vWorldPos.xyz, 1), matView).xyz;
	float3 vViewNor = mul(float4(vNormZ.xyz, 0), matView).xyz;
	float3 vReflectDir = normalize(reflect(normalize(vViewPos.xyz), normalize(vViewNor.xyz)));


	//Raycast
	float3 vHitPos = vViewPos;
	float fDepthDiff;

	float4 vCoords = SSRRayMarch(vReflectDir * max(g_fMinRayStep, vViewPos.z), vHitPos, fDepthDiff);

	float2 vCoordsDiff = abs( vCoords.xy - 0.5f );

	float fScreenEdgeFactor = saturate( 1.0f - ( vCoordsDiff.x + vCoordsDiff.y ) );

	//Color
	float fReflectionIntensity =
			pow(vRougnessSpec.y, g_fReflectionSpecularFalloffExponent) *						//specular fade
			fScreenEdgeFactor *																	//screen fade
			saturate(vReflectDir.z)																//camera facing fade
			* saturate( ( g_fSearchDist - length( vViewPos - vHitPos ) ) * g_fSearchDistInv )	//reflected object distance fade
			* vCoords.w																			//rayhit binary fade
			;
	//float3 vReflectionColor = loadScene(vCoords.xy).rgb;

	//float3 vReflectionColor=0;

	//static const float range = 2.5f;
	//float2 dim;
	//xTexture.GetDimensions(dim.x, dim.y);
	//dim /= 2.f;
	//float samples = 0;
	//for (float x = -range; x <= range; x += 1.f){
	//	for (float y = -range; y <= range; y += 1.f){
	//		vReflectionColor += xTexture.SampleLevel(Sampler, vCoords.xy+float2(x,y)/dim, 4);
	//		samples += 1.f;
	//	}
	//}
	//vReflectionColor /= samples;

	float3 vReflectionColor = xTexture.SampleLevel(Sampler, vCoords.xy, 3);

	o = float4(vReflectionColor, fReflectionIntensity);

	return o;

}
