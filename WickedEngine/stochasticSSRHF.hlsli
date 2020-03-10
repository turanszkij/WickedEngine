
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

static const float2 offset[9] =
{
    float2(-2.0, -2.0),
    float2(0.0, -2.0),
    float2(2.0, -2.0),
    float2(-2.0, 0.0),
    float2(0.0, 0.0),
    float2(2.0, 0.0),
    float2(-2.0, 2.0),
    float2(0.0, 2.0),
    float2(2.0, 2.0)
};


uint3 Rand3DPCG16(int3 p)
{
    uint3 v = uint3(p);

    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

	// only top 16 bits are well shuffled
    return v >> 16u;
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
