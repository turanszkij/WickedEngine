#ifndef WI_STOCHASTICSSR_HF
#define WI_STOCHASTICSSR_HF

// Blue noise resources:
STRUCTUREDBUFFER(sobolSequenceBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(scramblingTileBuffer, uint, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(rankingTileBuffer, uint, TEXSLOT_ONDEMAND3);

// Stochastic Screen Space Reflections reference:
// https://www.ea.com/frostbite/news/stochastic-screen-space-reflections


#define GGX_SAMPLE_VISIBLE

// Bias used on GGX importance sample when denoising, to remove part of the tale that create a lot more noise.
#define GGX_IMPORTANCE_SAMPLE_BIAS 0.1

// Shared SSR settings:
static const float SSRMaxRoughness = 1.0f; // Specify max roughness, this can improve performance in complex scenes.
static const float SSRIntensity = 1.0f;
static const float SSRResolveConeMip = 1.0f; // Control overall filtering of the importance sampling. 
static const float SSRResolveSpatialSize = 3.0f; // Seems to work best with the temporal pass in the [-3;3] range
static const float SSRBlendScreenEdgeFade = 5.0f;

// Temporary
static const float BRDFBias = 0.7f;


float ComputeRoughnessMaskScale(in float maxRoughness)
{
    float MaxRoughness = clamp(maxRoughness, 0.01f, 1.0f);
    
    float roughnessMaskScale = -2.0f / MaxRoughness;
    return roughnessMaskScale * 1.0f; // 2.0f & 1.0f
}

float GetRoughnessFade(in float roughness, in float maxRoughness)
{
    float roughnessMaskScale = ComputeRoughnessMaskScale(maxRoughness);
    return min(roughness * roughnessMaskScale + 2, 1.0f);
}

float GetRoughness(float roughness)
{
    return max(roughness, 0.02f);
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// Fast RNG inspired by PCG (Permuted Congruential Generator) - Based on Epic Games (Unreal Engine)
// Returns three elements with 16 random bits each (0-0xffff (65535)).
uint3 Rand_PCG16(int3 i)
{
    // Epic Games had good results by interpreting signed values as unsigned.
    uint3 r = uint3(i);

    // Linear congruential generator
    // A simple but very fast pseudorandom number generator
    // see: https://en.wikipedia.org/wiki/Linear_congruential_generator
    r = r * 1664525u + 1013904223u; // LCG set from 'Numerical Recipes'

    // Final shuffle
    // In the original PCG code, they used xorshift for their final shuffle.
    // According to Epic Games, they would do simple Feistel steps instead since xorshift is expensive.
    // They would then use r.x, r.y and r.z as parts to create something persistence with few instructions.
    r.x += r.y * r.z;
    r.y += r.z * r.x;
    r.z += r.x * r.y;
    
    r.x += r.y * r.z;
    r.y += r.z * r.x;
    r.z += r.x * r.y;

	// PCG would then shuffle the top 16 bits thoroughly.
    return r >> 16u;
}

// Hammersley sequence manipulated by a random value and returns top 16 bits
float2 HammersleyRandom16(uint idx, uint num, uint2 random)
{
    // Reverse Bits 32
    uint bits = idx;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    float E1 = frac(float(idx / num) + float(random.x) * 1.52587890625e-5); // / 0xffff (rcp(65536) )
    float E2 = float((bits >> 16) ^ random.y) * 1.52587890625e-5; // Shift reverse bits by 16 and compare bits with random
    return float2(E1, E2);
}

float2 HammersleyRandom16(uint idx, uint2 random)
{
    uint bits = idx;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    float E1 = frac(float(random.x) * 1.52587890625e-5); // / 0xffff (rcp(65536) )
    float E2 = float((bits >> 16) ^ random.y) * 1.52587890625e-5; // Shift reverse bits by 16 and compare bits with random
    return float2(E1, E2);
}

// Brian Karis, Epic Games "Real Shading in Unreal Engine 4"
float4 ImportanceSampleGGX(float2 Xi, float Roughness)
{
    float m = Roughness * Roughness;
    float m2 = m * m;
		
    float Phi = 2 * PI * Xi.x;
				 
    float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
    float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));
				 
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
		
    float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
    float D = m2 / (PI * d * d);
    float pdf = D * CosTheta;

    return float4(H, pdf);
}

// [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
// http://jcgt.org/published/0006/01/01/
float3x3 GetTangentBasis(float3 TangentZ)
{
    const float Sign = TangentZ.z >= 0 ? 1 : -1;
    const float a = -rcp(Sign + TangentZ.z);
    const float b = TangentZ.x * TangentZ.y * a;
	
    float3 TangentX = { 1 + Sign * a * pow(TangentZ.x, 2), Sign * b, -Sign * TangentZ.x };
    float3 TangentY = { b, Sign + a * pow(TangentZ.y, 2), -TangentZ.y };

    return float3x3(TangentX, TangentY, TangentZ);
}

float3 TangentToWorld(float3 vec, float3 tangentZ)
{
    return mul(vec, GetTangentBasis(tangentZ));
}

float4 TangentToWorld(float4 H, float3 tangentZ)
{
    return float4(mul(H.xyz, GetTangentBasis(tangentZ)), H.w);
}

float3 WorldToTangent(float3 vec, float3 tangentZ)
{
    return mul(GetTangentBasis(tangentZ), vec);
}


float2 SampleDisk(float2 Xi)
{
	float theta = 2 * PI * Xi.x;
	float radius = sqrt(Xi.y);
	return radius * float2(cos(theta), sin(theta));
}

// Adapted from: "Sampling the GGX Distribution of Visible Normals", by E. Heitz
// http://jcgt.org/published/0007/04/01/paper.pdf
float4 ImportanceSampleVisibleGGX(float2 diskXi, float roughness, float3 V)
{
	float alphaRoughness = roughness * roughness;
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;

	// Transform the view direction to hemisphere configuration
	float3 Vh = normalize(float3(alphaRoughness * V.xy, V.z));

	// Orthonormal basis
	// tangent0 is orthogonal to N.
	float3 tangent0 = (Vh.z < 0.9999) ? normalize(cross(float3(0, 0, 1), Vh)) : float3(1, 0, 0);
	float3 tangent1 = cross(Vh, tangent0);

	float2 p = diskXi;
	float s = 0.5 + 0.5 * Vh.z;
	p.y = (1 - s) * sqrt(1 - p.x * p.x) + s * p.y;

	// Reproject onto hemisphere
	float3 H;
	H = p.x * tangent0;
	H += p.y * tangent1;
	H += sqrt(saturate(1 - dot(p, p))) * Vh;

	// Transform the normal back to the ellipsoid configuration
	H = normalize(float3(alphaRoughness * H.xy, max(0.0, H.z)));

	float NdotV = V.z;
	float NdotH = H.z;
	float VdotH = dot(V, H);

	// Microfacet Distribution
	float f = (NdotH * alphaRoughnessSq - NdotH) * NdotH + 1;
	float D = alphaRoughnessSq / (PI * f * f);

	// Smith Joint masking function
	float SmithGGXMasking = 2.0 * NdotV / (sqrt(NdotV * (NdotV - NdotV * alphaRoughnessSq) + alphaRoughnessSq) + NdotV);

	// D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
	float PDF = SmithGGXMasking * VdotH * D / NdotV;

	return float4(H, PDF);
}

// "A Low-Discrepancy Sampler that Distributes Monte Carlo Errors as a Blue Noise in Screen Space" by Heitz et al.
float BNDSequenceSample(uint2 pixelCoord, uint sampleIndex, uint sampleDimension)
{
	pixelCoord = pixelCoord & 127u;
	sampleIndex = sampleIndex & 255u;
	sampleDimension = sampleDimension & 255u;

	// xor index based on optimized ranking
	const uint rankedSampleIndex = sampleIndex ^ rankingTileBuffer[sampleDimension + (pixelCoord.x + pixelCoord.y * 128u) * 8u];

	// Fetch value in sequence
	uint value = sobolSequenceBuffer[sampleDimension + rankedSampleIndex * 256u];

	// If the dimension is optimized, xor sequence value based on optimized scrambling
	value = value ^ scramblingTileBuffer[(sampleDimension % 8u) + (pixelCoord.x + pixelCoord.y * 128u) * 8u];

	// Convert to float and return
	return (value + 0.5f) / 256.0f;
}

#endif // WI_STOCHASTICSSR_HF
