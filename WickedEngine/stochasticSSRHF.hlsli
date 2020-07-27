
#ifndef WI_STOCHASTICSSR_HF
#define WI_STOCHASTICSSR_HF

// Shared SSR settings:
static const float SSRMaxRoughness = 1.0f; // Specify max roughness, this can improve performance in complex scenes.
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

#endif // WI_STOCHASTICSSR_HF