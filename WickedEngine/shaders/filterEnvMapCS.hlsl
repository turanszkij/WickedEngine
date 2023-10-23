#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifdef __PSSL__
// TODO: register spilling issue
#pragma warning (disable:7203)
#endif // __PSSL__

PUSHCONSTANT(push, FilterEnvmapPushConstants);

float D_GGX(float NdotH, float roughness)
{
	float a = NdotH * roughness;
	float k = roughness / (1.0 - NdotH * NdotH + a * a);
	return k * k * (1.0 / PI);
}

// From "Real Shading in UnrealEngine 4" by Brian Karis, page 4
//	https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float4 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	// Additional PDF:
	//	https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/shaders/ibl_filtering.frag
	float pdf = D_GGX(CosTheta, a);
	// Apply the Jacobian to obtain a pdf that is parameterized by l
	// see https://bruop.github.io/ibl/
	// Typically you'd have the following:
	// float pdf = D_GGX(NoH, roughness) * NoH / (4.0 * VoH);
	// but since V = N => VoH == NoH
	pdf /= 4.0;
	
	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi); 
	H.z = CosTheta;
	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize(cross(UpVector, N));
	float3 TangentY = cross(N, TangentX);
	// Tangent to world space
	return float4(TangentX * H.x + TangentY * H.y + N * H.z, pdf);
}

// Mipmap Filtered Samples (GPU Gems 3, 20.4)
// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
float computeLod(float pdf, uint width, uint sampleCount)
{
	// // Solid angle of current sample -- bigger for less likely samples
	// float omegaS = 1.0 / (float(u_sampleCount) * pdf);
	// // Solid angle of texel
	// // note: the factor of 4.0 * MATH_PI 
	// float omegaP = 4.0 * MATH_PI / (6.0 * float(u_width) * float(u_width));
	// // Mip level is determined by the ratio of our sample's solid angle to a texel's solid angle 
	// // note that 0.5 * log2 is equivalent to log4
	// float lod = 0.5 * log2(omegaS / omegaP);

	// babylon introduces a factor of K (=4) to the solid angle ratio
	// this helps to avoid undersampling the environment map
	// this does not appear in the original formulation by Jaroslav Krivanek and Mark Colbert
	// log4(4) == 1
	// lod += 1.0;

	// We achieved good results by using the original formulation from Krivanek & Colbert adapted to cubemaps

	// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
	float lod = 0.5 * log2(6.0 * float(width) * float(width) / (float(sampleCount) * pdf));


	return lod;
}

static const uint THREAD_OFFLOAD = 16;
groupshared float4 shared_colors[GENERATEMIPCHAIN_2D_BLOCK_SIZE][GENERATEMIPCHAIN_2D_BLOCK_SIZE][THREAD_OFFLOAD];

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, THREAD_OFFLOAD)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
	if (DTid.x >= push.filterResolution.x || DTid.y >= push.filterResolution.y)
		return;
	
	TextureCube input = bindless_cubemaps[push.texture_input];
	RWTexture2DArray<float4> output = bindless_rwtextures2DArray[push.texture_output];

	float2 uv = (DTid.xy + 0.5f) * push.filterResolution_rcp.xy;
	uint face = DTid.z / THREAD_OFFLOAD;
	float3 N = normalize(uv_to_cubemap(uv, face));
	float3 V = N;

	float4 col = 0;
	uint threadstart = DTid.z % THREAD_OFFLOAD;
	
	for (uint i = threadstart; i < push.filterRayCount; i += THREAD_OFFLOAD)
	{
		float2 Xi = hammersley2d(i, push.filterRayCount);
		float4 importanceSample = ImportanceSampleGGX(Xi, push.filterRoughness, N);
		float3 H = importanceSample.xyz;
		float pdf = importanceSample.w;
		float3 L = 2 * dot(V, H) * H - V;
			
		float NoL = saturate(dot(N, L));
		if (NoL > 0)
		{
			uint2 dim;
			input.GetDimensions(dim.x, dim.y); // input to computeLod needs to be resolution of top mip, not the current filter resolution
			float lod = computeLod(pdf, dim.x, push.filterRayCount);
			col += input.SampleLevel(sampler_linear_clamp, L, lod) * NoL;
		}
	}

	shared_colors[GTid.x][GTid.y][threadstart] = col;
	GroupMemoryBarrierWithGroupSync();

	if(threadstart == 0)
	{
		float4 accum = 0;
		for (uint j = 0; j < THREAD_OFFLOAD;++j)
		{
			accum += shared_colors[GTid.x][GTid.y][j];
		}
		accum /= accum.a;
		output[uint3(DTid.xy, face)] = accum;
	}

}
