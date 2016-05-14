#include "postProcessHF.hlsli"
#include "reconstructPositionHF.hlsli"


// Ne lepegessunk 0-kat, akkor mar inkabb kicsit tobbet inkabb
static const float	g_fMinRayStep = 0.01f;
// Hany lepesben keressunk durvan
static const int	g_iMaxSteps = 16;
// Durva kereses fazisban mennyire durvuljon minden lepesben
static const float	g_fRayStep = 1.18f;
// Ha durva talalat van akkor finomitsuk azt ennyi lepesben
static const int	g_iNumBinarySearchSteps = 16;
// Tavoli tukorkepek fadelesehez (view space)
static const float	g_fSearchDist = 80.f;
static const float	g_fSearchDistInv = 1.0f / g_fSearchDist;
// Minel kisebb, annal pontosabb tukrozodes talalatok lesznek csak megtartva
static const float  g_fRayhitThreshold = 0.9f;

bool bInsideScreen(in float2 vCoord)
{
	if (vCoord.x < 0 || vCoord.x > 1 || vCoord.y < 0 || vCoord.y > 1)
		return false;
	return true;
}

float4 SSRBinarySearch(float3 vDir, inout float3 vHitCoord)
{
	float fDepth;

	for (int i = 0; i < g_iNumBinarySearchSteps; i++)
	{
		float4 vProjectedCoord = mul(float4(vHitCoord, 1.0f), g_xCamera_Proj);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		fDepth = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);
		float fDepthDiff = vHitCoord.z - fDepth;

		if (fDepthDiff <= 0.0f)
			vHitCoord += vDir;

		vDir *= 0.5f;
		vHitCoord -= vDir;
	}

	float4 vProjectedCoord = mul(float4(vHitCoord, 1.0f), g_xCamera_Proj);
	vProjectedCoord.xy /= vProjectedCoord.w;
	vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	fDepth = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);
	float fDepthDiff = vHitCoord.z - fDepth;

	return float4(vProjectedCoord.xy, fDepth, abs(fDepthDiff) < g_fRayhitThreshold ? 1.0f : 0.0f);
}

float4 SSRRayMarch(float3 vDir, inout float3 vHitCoord)
{
	float fDepth;

	for (int i = 0; i < g_iMaxSteps; i++)
	{
		vHitCoord += vDir;

		float4 vProjectedCoord = mul(float4(vHitCoord, 1.0f), g_xCamera_Proj);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		fDepth = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);

		float fDepthDiff = vHitCoord.z - fDepth;

		[branch]
		if (fDepthDiff > 0.0f)
			return SSRBinarySearch(vDir, vHitCoord);

		vDir *= g_fRayStep;

	}

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

float4 main(VertexToPixelPostProcess input) : SV_Target
{
	float4 o = 1;

	float4 vNormZ;
	vNormZ.rgb = texture_gbuffer1.Load(int3(input.pos.xy, 0)).rgb;
	vNormZ.a = texture_depth.Load(int3(input.pos.xy, 0));

	float2 vRougnessSpec = loadVelocity(input.tex).zw; //specular_power,specular intensity
													   //float fSpecularModifier = (1 - clamp(vRougnessSpec.x, 0, 256) / 256.f)*vRougnessSpec.y;

	float3 vWorldPos = getPosition(input.tex, vNormZ.a);


	//Reflection vector
	float3 vViewPos = mul(float4(vWorldPos.xyz, 1), g_xCamera_View).xyz;
	float3 vViewNor = mul(float4(vNormZ.xyz, 0), g_xCamera_View).xyz;
	float3 vReflectDir = normalize(reflect(normalize(vViewPos.xyz), normalize(vViewNor.xyz)));


	//Raycast
	float3 vHitPos = vViewPos;

	float4 vCoords = SSRRayMarch(vReflectDir /** max( g_fMinRayStep, vViewPos.z )*/, vHitPos);

	float2 vCoordsEdgeFact = float2(1, 1) - pow(saturate(abs(vCoords.xy - float2(0.5f, 0.5f)) * 2), 8);
	float fScreenEdgeFactor = saturate(min(vCoordsEdgeFact.x, vCoordsEdgeFact.y));

	//if (!bInsideScreen(vCoords.xy))
	//	fScreenEdgeFactor = 0;


	//Color
	float fReflectionIntensity =
		saturate(
			pow(vRougnessSpec.y, 2) *															//specular fade
			fScreenEdgeFactor *																	// screen fade
			saturate(vReflectDir.z)																// camera facing fade
			//* saturate((g_fSearchDist - distance(vViewPos, vHitPos)) * g_fSearchDistInv)	// reflected object distance fade
			* vCoords.w																			// rayhit binary fade
			);


	float3 vReflectionColor = xTexture.SampleLevel(sampler_linear_clamp, vCoords.xy, 0).rgb;
	float3 vSceneColor = xTexture.Load(int3(input.pos.xy,0)).rgb;

	o = float4(lerp(vSceneColor, vReflectionColor, fReflectionIntensity), 1);

	return o;

}
