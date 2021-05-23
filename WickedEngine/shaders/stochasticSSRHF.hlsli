#ifndef WI_STOCHASTICSSR_HF
#define WI_STOCHASTICSSR_HF

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



#ifndef HLSL5
// AMD code below

/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

static const uint g_samples_per_quad = 1; // 1, 2 or 4
static const bool g_temporal_variance_guided_tracing_enabled = true;

static const float g_roughness_sigma_min = 0.001f;
static const float g_roughness_sigma_max = 0.01f;
static const float g_depth_sigma = 0.02f;

uint PackFloat16(min16float2 v) {
	uint2 p = f32tof16(float2(v));
	return p.x | (p.y << 16);
}

min16float2 UnpackFloat16(uint a) {
	float2 tmp = f16tof32(
		uint2(a & 0xFFFF, a >> 16));
	return min16float2(tmp);
}

uint PackRayCoords(uint2 ray_coord, bool copy_horizontal, bool copy_vertical, bool copy_diagonal) {
	uint ray_x_15bit = ray_coord.x & 0b111111111111111;
	uint ray_y_14bit = ray_coord.y & 0b11111111111111;
	uint copy_horizontal_1bit = copy_horizontal ? 1 : 0;
	uint copy_vertical_1bit = copy_vertical ? 1 : 0;
	uint copy_diagonal_1bit = copy_diagonal ? 1 : 0;

	uint packed = (copy_diagonal_1bit << 31) | (copy_vertical_1bit << 30) | (copy_horizontal_1bit << 29) | (ray_y_14bit << 15) | (ray_x_15bit << 0);
	return packed;
}

void UnpackRayCoords(uint packed, out uint2 ray_coord, out bool copy_horizontal, out bool copy_vertical, out bool copy_diagonal) {
	ray_coord.x = (packed >> 0) & 0b111111111111111;
	ray_coord.y = (packed >> 15) & 0b11111111111111;
	copy_horizontal = (packed >> 29) & 0b1;
	copy_vertical = (packed >> 30) & 0b1;
	copy_diagonal = (packed >> 31) & 0b1;
}

// Transforms origin to uv space
// Mat must be able to transform origin from its current space into clip space.
float3 ProjectPosition(float3 origin, float4x4 mat) {
	float4 projected = mul(mat, float4(origin, 1));
	projected.xyz /= projected.w;
	projected.xy = 0.5 * projected.xy + 0.5;
	projected.y = (1 - projected.y);
	return projected.xyz;
}

// Origin and direction must be in the same space and mat must be able to transform from that space into clip space.
float3 ProjectDirection(float3 origin, float3 direction, float3 screen_space_origin, float4x4 mat) {
	float3 offsetted = ProjectPosition(origin + direction, mat);
	return offsetted - screen_space_origin;
}

// Mat must be able to transform origin from texture space to a linear space.
float3 InvProjectPosition(float3 coord, float4x4 mat) {
	coord.y = (1 - coord.y);
	coord.xy = 2 * coord.xy - 1;
	float4 projected = mul(mat, float4(coord, 1));
	projected.xyz /= projected.w;
	return projected.xyz;
}

//=== FFX_DNSR_Reflections_ override functions ===


bool FFX_DNSR_Reflections_IsGlossyReflection(float roughness) {
	return roughness <= 0.5;
}

bool FFX_DNSR_Reflections_IsMirrorReflection(float roughness) {
	return roughness < 0.0001;
}

float3 FFX_DNSR_Reflections_ScreenSpaceToViewSpace(float3 screen_uv_coord) {
	return InvProjectPosition(screen_uv_coord, g_xCamera_InvP);
}

float3 FFX_DNSR_Reflections_ViewSpaceToWorldSpace(float4 view_space_coord) {
	return mul(g_xCamera_InvV, view_space_coord).xyz;
}

float3 FFX_DNSR_Reflections_WorldSpaceToScreenSpacePrevious(float3 world_coord) {
	return ProjectPosition(world_coord, g_xCamera_PrevVP);
}
#endif

#endif // WI_STOCHASTICSSR_HF
