#include "postProcessHF.hlsli"
static const float strength = 4.f;
static const float falloff  = -0.8f;
static const float rad = 0.1f;
static const float darkness = 1.2f;

#define NUM_SAMPLES	 16
static const float invSamples = 1.0 / (float)NUM_SAMPLES;

// AO sampling directions 
static const float3 AO_SAMPLES[ NUM_SAMPLES ] = 
{
#if NUM_SAMPLES==16
    float3(0.355512, -0.709318, -0.102371 ),
	float3(0.534186, 0.71511, -0.115167 ),
	float3(-0.87866, 0.157139, -0.115167 ),
	float3(0.140679, -0.475516, -0.0639818 ),
	float3(-0.0796121, 0.158842, -0.677075 ),
	float3(-0.0759516, -0.101676, -0.483625 ),
	float3(0.12493, -0.0223423, -0.483625 ),
	float3(-0.0720074, 0.243395, -0.967251 ),
	float3(-0.207641, 0.414286, 0.187755 ),
	float3(-0.277332, -0.371262, 0.187755 ),
	float3(0.63864, -0.114214, 0.262857 ),
	float3(-0.184051, 0.622119, 0.262857 ),
	float3(0.110007, -0.219486, 0.435574 ),
	float3(0.235085, 0.314707, 0.696918 ),
	float3(-0.290012, 0.0518654, 0.522688 ),
	float3(0.0975089, -0.329594, 0.609803 )
#elif NUM_SAMPLES==10
	float3(-0.010735935, 0.01647018, 0.0062425877),
	float3(-0.06533369, 0.3647007, -0.13746321),
	float3(-0.6539235, -0.016726388, -0.53000957),
	float3(0.40958285, 0.0052428036, -0.5591124),
	float3(-0.1465366, 0.09899267, 0.15571679),
	float3(-0.44122112, -0.5458797, 0.04912532),
	float3(0.03755566, -0.10961345, -0.33040273),
	float3(0.019100213, 0.29652783, 0.066237666),
	float3(0.8765323, 0.011236004, 0.28265962),
	float3(0.29264435, -0.40794238, 0.15964167)
#endif
};



float4 main( VertextoPixel input ):SV_Target
{
	//float4 normap = xNormalMap.SampleLevel( Sampler, input.tex, 0 );
	float4 normap = loadNormal(input.tex.xy);

	clip(normap.a?-1:1);

	float3 normal = normap.xyz;

	float3 fres = normalize(xMaskTex.Load(int3((64 * input.tex.xy * 400) % 64, 0)).xyz * 2.0 - 1.0);

	//float2 depthMapSize;
	//xSceneDepthMap.GetDimensions(depthMapSize.x,depthMapSize.y);
	//float  depth  = ( xSceneDepthMap.Load(int4(depthMapSize*input.tex.xy,0,0)).r );
	float depth = loadDepth(input.tex.xy);

	float3 ep = float3( input.tex, depth );
	float bl = 0.0;
	float radD = rad / depth;

	float3 ray;
	float3 occFrag;
	float  depthDiff;

	[unroll]
	for( int i = 0; i < NUM_SAMPLES; ++i )
	{
		ray = radD * reflect(AO_SAMPLES[i],fres);
		float2 newTex = ep.xy + ray.xy;

		//occFrag = xNormalMap.SampleLevel(Sampler, newTex,0).xyz;
		occFrag = loadNormal(newTex).xyz;
		if(!occFrag.x && !occFrag.y && !occFrag.z)
			break;

		depthDiff = ( depth - ( xSceneDepthMap.SampleLevel(Sampler,newTex,0).r ) );

		bl += step( falloff, depthDiff ) * (1.0 - saturate( dot( occFrag.xyz, normal ) ) ) 
			* (1-smoothstep( falloff,strength,depthDiff ) );

	}

	float ao = 1.0 - bl * invSamples*darkness;

	return saturate( float4( ao, ao, ao, 1.0 ) );
}