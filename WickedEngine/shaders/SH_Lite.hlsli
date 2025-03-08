// Note: this file is modified for Wicked Engine from the original
//	- half precision storage
//	- ps5 shader compiler support

//=================================================================================================
//
//  SHforHLSL - Spherical harmonics suppport library for HLSL, by MJP
//  https://github.com/TheRealMJP/SHforHLSL
//  https://therealmjp.github.io/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
//
// This header is intended to be included directly from HLSL code, or similar. It is a simplified
// version of SH.hlsli, and does not use templates, operator overloads, or other HLSL 2021+
// features. It implements types and utility functions for working with low-order spherical
// harmonics, focused on use cases for graphics.
//
// Currently this library has support for L1 (2 bands, 4 coefficients) and
// L2 (3 bands, 9 coefficients) SH. Depending on the author and material you're reading, you may
// see L1 referred to as both first-order or second-order, and L2 referred to as second-order
// or third-order. Ravi Ramamoorthi tends to refer to three bands as second-order, and
// Peter-Pike Sloan tends to refer to three bands as third-order. This library always uses L1 and
// L2 for clarity.
//
// The core SH types all use 32-bit halfs for storing coefficients, with either 1 scalar or 3
// halfs for separate RGB coefficients.
//
// Example #1: integrating and projecting radiance onto L2 SH
//
// SH::L2_RGB radianceSH = SH::L2_RGB::Zero();
// for(int32_t sampleIndex = 0; sampleIndex < NumSamples; ++sampleIndex)
// {
//     half2 u1u2 = Randomhalf2(sampleIndex, NumSamples);
//     half3 sampleDirection = SampleDirectionSphere(u1u2);
//     half3 sampleRadiance = CalculateIncomingRadiance(sampleDirection);
//     radianceSH += SH::ProjectOntoL2(sampleDirection, sampleRadiance);
// }
// radianceSH *= 1.0 / (NumSamples * SampleDirectionSphere_PDF());
//
// Example #2: calculating diffuse lighting for a surface from radiance projected onto L2 SH
//
// SH::L2_RGB radianceSH = FetchRadianceSH(surfacePosition);
// half3 diffuseLighting = SH::CalculateIrradiance(radianceSH, surfaceNormal) * (diffuseAlbedo / Pi);
//
//=================================================================================================

#ifndef SH_LITE_HLSLI_
#define SH_LITE_HLSLI_

namespace SH
{

// Constants
static const half Pi = 3.141592654;
static const half SqrtPi = sqrt(Pi);

static const half CosineA0 = Pi;
static const half CosineA1 = (2.0 * Pi) / 3.0;
static const half CosineA2 = (0.25 * Pi);

static const half BasisL0 = 1 / (2 * SqrtPi);
static const half BasisL1 = sqrt(3) / (2 * SqrtPi);
static const half BasisL2_MN2 = sqrt(15) / (2 * SqrtPi);
static const half BasisL2_MN1 = sqrt(15) / (2 * SqrtPi);
static const half BasisL2_M0 = sqrt(5) / (4 * SqrtPi);
static const half BasisL2_M1 = sqrt(15) / (2 * SqrtPi);
static const half BasisL2_M2 = sqrt(15) / (4 * SqrtPi);

// Core SH types containing the coefficients
struct L1
{
	static const uint NumCoefficients = 4;

	half C[NumCoefficients];

	static L1 Zero()
	{
		return (L1)0;
	}
	static L1 Create(half c0, half c1, half c2, half c3)
	{
		L1 ret;
		ret.C[0] = c0;
		ret.C[1] = c1;
		ret.C[2] = c2;
		ret.C[3] = c3;
		return ret;
	}
	
	struct Packed
	{
		uint C[NumCoefficients / 2];
	};
};

L1 Unpack(L1::Packed packed)
{
	L1 ret;
	ret.C[0] = f16tof32(packed.C[0]);
	ret.C[1] = f16tof32(packed.C[0] >> 16u);
	ret.C[2] = f16tof32(packed.C[1]);
	ret.C[3] = f16tof32(packed.C[1] >> 16u);
	return ret;
}
L1::Packed Pack(L1 sh)
{
	L1::Packed ret;
	ret.C[0] = f32tof16(sh.C[0]) | (f32tof16(sh.C[1]) << 16u);
	ret.C[1] = f32tof16(sh.C[2]) | (f32tof16(sh.C[3]) << 16u);
	return ret;
}

struct L1_RGB
{
	static const uint NumCoefficients = 4;

	half3 C[NumCoefficients];

	static L1_RGB Zero()
	{
		return (L1_RGB)0;
	}
	
	struct Packed
	{
		uint C[3 * NumCoefficients / 2];
	};
};

L1_RGB Unpack(L1_RGB::Packed packed)
{
	L1_RGB ret;
	ret.C[0] = half3(f16tof32(packed.C[0]), f16tof32(packed.C[0] >> 16u), f16tof32(packed.C[1]));
	ret.C[1] = half3(f16tof32(packed.C[1] >> 16u), f16tof32(packed.C[2]), f16tof32(packed.C[2] >> 16u));
	ret.C[2] = half3(f16tof32(packed.C[3]), f16tof32(packed.C[3] >> 16u), f16tof32(packed.C[4]));
	ret.C[3] = half3(f16tof32(packed.C[4] >> 16u), f16tof32(packed.C[5]), f16tof32(packed.C[5] >> 16u));
	return ret;
}
L1_RGB::Packed Pack(L1_RGB sh)
{
	L1_RGB::Packed ret;
	ret.C[0] = f32tof16(sh.C[0].x) | (f32tof16(sh.C[0].y) << 16u);
	ret.C[1] = f32tof16(sh.C[0].z) | (f32tof16(sh.C[1].x) << 16u);
	ret.C[2] = f32tof16(sh.C[1].y) | (f32tof16(sh.C[1].z) << 16u);
	ret.C[3] = f32tof16(sh.C[2].x) | (f32tof16(sh.C[2].y) << 16u);
	ret.C[4] = f32tof16(sh.C[2].z) | (f32tof16(sh.C[3].x) << 16u);
	ret.C[5] = f32tof16(sh.C[3].y) | (f32tof16(sh.C[3].z) << 16u);
	return ret;
}

struct L2
{
	static const uint NumCoefficients = 9;

	half C[NumCoefficients];

	static L2 Zero()
	{
		return (L2)0;
	}
};

struct L2_RGB
{
	static const uint NumCoefficients = 9;

	half3 C[NumCoefficients];

	static L2_RGB Zero()
	{
		return (L2_RGB)0;
	}
};

// Sum two sets of SH coefficients
L1 Add(L1 a, L1 b)
{
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        a.C[i] += b.C[i];
    return a;
}

L1_RGB Add(L1_RGB a, L1_RGB b)
{
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        a.C[i] += b.C[i];
    return a;
}

L2 Add(L2 a, L2 b)
{
    [unroll]
    for(uint i = 0; i < L2::NumCoefficients; ++i)
        a.C[i] += b.C[i];
    return a;
}

L2_RGB Add(L2_RGB a, L2_RGB b)
{
    [unroll]
    for(uint i = 0; i < L2_RGB::NumCoefficients; ++i)
        a.C[i] += b.C[i];
    return a;
}

// Substract two sets of SH coefficients
L1 Subtract(L1 a, L1 b)
{
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        a.C[i] -= b.C[i];
    return a;
}

L1_RGB Subtract(L1_RGB a, L1_RGB b)
{
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        a.C[i] -= b.C[i];
    return a;
}

L2 Subtract(L2 a, L2 b)
{
    [unroll]
    for(uint i = 0; i < L2::NumCoefficients; ++i)
        a.C[i] -= b.C[i];
    return a;
}

L2_RGB Subtract(L2_RGB a, L2_RGB b)
{
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        a.C[i] -= b.C[i];
    return a;
}

// Multiply a set of SH coefficients by a single value
L1 Multiply(L1 a, half b)
{
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        a.C[i] *= b;
    return a;
}

L1_RGB Multiply(L1_RGB a, half3 b)
{
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        a.C[i] *= b;
    return a;
}

L2 Multiply(L2 a, half b)
{
    [unroll]
    for(uint i = 0; i < L2::NumCoefficients; ++i)
        a.C[i] *= b;
    return a;
}

L2_RGB Multiply(L2_RGB a, half3 b)
{
    [unroll]
    for(uint i = 0; i < L2_RGB::NumCoefficients; ++i)
        a.C[i] *= b;
    return a;
}

// Divide a set of SH coefficients by a single value
L1 Divide(L1 a, half b)
{
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        a.C[i] /= b;
    return a;
}

L1_RGB Divide(L1_RGB a, half3 b)
{
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        a.C[i] /= b;
    return a;
}

L2 Divide(L2 a, half b)
{
    [unroll]
    for(uint i = 0; i < L2::NumCoefficients; ++i)
        a.C[i] /= b;
    return a;
}

L2_RGB Divide(L2_RGB a, half3 b)
{
    [unroll]
    for(uint i = 0; i < L2_RGB::NumCoefficients; ++i)
        a.C[i] /= b;
    return a;
}

// Truncates a set of L2 coefficients to produce a set of L1 coefficients
L1 L2toL1(L2 sh)
{
    L1 result;
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        result.C[i] = sh.C[i];
    return result;
}

L1_RGB L2toL1(L2_RGB sh)
{
    L1_RGB result;
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        result.C[i] = sh.C[i];
    return result;
}

// Converts from scalar to RGB SH coefficients
L1_RGB ToRGB(L1 sh)
{
    L1_RGB result;
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        result.C[i] = sh.C[i];
    return result;
}

L2_RGB ToRGB(L2 sh)
{
    L2_RGB result;
    [unroll]
    for(uint i = 0; i < L2::NumCoefficients; ++i)
        result.C[i] = sh.C[i];
    return result;
}

// Linear interpolation
L1 Lerp(L1 x, L1 y, half s)
{
    return Add(Multiply(x, 1.0 - s), Multiply(y, s));
}

L1_RGB Lerp(L1_RGB x, L1_RGB y, half s)
{
    return Add(Multiply(x, 1.0 - s), Multiply(y, s));
}

L2 Lerp(L2 x, L2 y, half s)
{
    return Add(Multiply(x, 1.0 - s), Multiply(y, s));
}

L2_RGB Lerp(L2_RGB x, L2_RGB y, half s)
{
    return Add(Multiply(x, 1.0 - s), Multiply(y, s));
}

// Projects a value in a single direction onto a set of L1 SH coefficients
L1 ProjectOntoL1(half3 direction, half value)
{
    L1 sh;

    // L0
    sh.C[0] = BasisL0 * value;

    // L1
    sh.C[1] = BasisL1 * direction.y * value;
    sh.C[2] = BasisL1 * direction.z * value;
    sh.C[3] = BasisL1 * direction.x * value;

    return sh;
}

L1_RGB ProjectOntoL1_RGB(half3 direction, half3 value)
{
    L1_RGB sh;

    // L0
    sh.C[0] = BasisL0 * value;

    // L1
    sh.C[1] = BasisL1 * direction.y * value;
    sh.C[2] = BasisL1 * direction.z * value;
    sh.C[3] = BasisL1 * direction.x * value;

    return sh;
}

// Projects a value in a single direction onto a set of L2 SH coefficients
L2 ProjectOntoL2(half3 direction, half value)
{
    L2 sh;

    // L0
    sh.C[0] = BasisL0 * value;

    // L1
    sh.C[1] = BasisL1 * direction.y * value;
    sh.C[2] = BasisL1 * direction.z * value;
    sh.C[3] = BasisL1 * direction.x * value;

    // L2
    sh.C[4] = BasisL2_MN2 * direction.x * direction.y * value;
    sh.C[5] = BasisL2_MN1 * direction.y * direction.z * value;
    sh.C[6] = BasisL2_M0 * (3.0 * direction.z * direction.z - 1.0) * value;
    sh.C[7] = BasisL2_M1 * direction.x * direction.z * value;
    sh.C[8] = BasisL2_M2 * (direction.x * direction.x - direction.y * direction.y) * value;

    return sh;
}

L2_RGB ProjectOntoL2_RGB(half3 direction, half3 value)
{
    L2_RGB sh;

    // L0
    sh.C[0] = BasisL0 * value;

    // L1
    sh.C[1] = BasisL1 * direction.y * value;
    sh.C[2] = BasisL1 * direction.z * value;
    sh.C[3] = BasisL1 * direction.x * value;

    // L2
    sh.C[4] = BasisL2_MN2 * direction.x * direction.y * value;
    sh.C[5] = BasisL2_MN1 * direction.y * direction.z * value;
    sh.C[6] = BasisL2_M0 * (3.0 * direction.z * direction.z - 1.0) * value;
    sh.C[7] = BasisL2_M1 * direction.x * direction.z * value;
    sh.C[8] = BasisL2_M2 * (direction.x * direction.x - direction.y * direction.y) * value;

    return sh;
}

// Calculates the dot product of two sets of L1 SH coefficients
half DotProduct(L1 a, L1 b)
{
    half result = 0.0;
    [unroll]
    for(uint i = 0; i < L1::NumCoefficients; ++i)
        result += a.C[i] * b.C[i];

    return result;
}

half3 DotProduct(L1_RGB a, L1_RGB b)
{
    half3 result = 0.0;
    [unroll]
    for(uint i = 0; i < L1_RGB::NumCoefficients; ++i)
        result += a.C[i] * b.C[i];

    return result;
}

// Calculates the dot product of two sets of L2 SH coefficients
half DotProduct(L2 a, L2 b)
{
    half result = 0.0;
    [unroll]
    for(uint i = 0; i < L2::NumCoefficients; ++i)
        result += a.C[i] * b.C[i];

    return result;
}

half3 DotProduct(L2_RGB a, L2_RGB b)
{
    half3 result = 0.0;
    [unroll]
    for(uint i = 0; i < L2_RGB::NumCoefficients; ++i)
        result += a.C[i] * b.C[i];

    return result;
}

// Projects a delta in a direction onto SH and calculates the dot product with a set of L1 SH coefficients.
// Can be used to "look up" a value from SH coefficients in a particular direction.
half Evaluate(L1 sh, half3 direction)
{
    L1 projectedDelta = ProjectOntoL1(direction, 1.0);
    return DotProduct(projectedDelta, sh);
}

half3 Evaluate(L1_RGB sh, half3 direction)
{
    L1_RGB projectedDelta = ProjectOntoL1_RGB(direction, 1.0);
    return DotProduct(projectedDelta, sh);
}

// Projects a delta in a direction onto SH and calculates the dot product with a set of L2 SH coefficients.
// Can be used to "look up" a value from SH coefficients in a particular direction.
half Evaluate(L2 sh, half3 direction)
{
    L2 projectedDelta = ProjectOntoL2(direction, 1.0);
    return DotProduct(projectedDelta, sh);
}

half3 Evaluate(L2_RGB sh, half3 direction)
{
    L2_RGB projectedDelta = ProjectOntoL2_RGB(direction, 1.0);
    return DotProduct(projectedDelta, sh);
}

// Convolves a set of L1 SH coefficients with a set of L1 zonal harmonics
L1 ConvolveWithZH(L1 sh, half2 zh)
{
    // L0
    sh.C[0] *= zh.x;

    // L1
    sh.C[1] *= zh.y;
    sh.C[2] *= zh.y;
    sh.C[3] *= zh.y;

    return sh;
}

L1_RGB ConvolveWithZH(L1_RGB sh, half2 zh)
{
    // L0
    sh.C[0] *= zh.x;

    // L1
    sh.C[1] *= zh.y;
    sh.C[2] *= zh.y;
    sh.C[3] *= zh.y;

    return sh;
}

// Convolves a set of L2 SH coefficients with a set of L2 zonal harmonics
L2 ConvolveWithZH(L2 sh, half3 zh)
{
    // L0
    sh.C[0] *= zh.x;

    // L1
    sh.C[1] *= zh.y;
    sh.C[2] *= zh.y;
    sh.C[3] *= zh.y;

    // L2
    sh.C[4] *= zh.z;
    sh.C[5] *= zh.z;
    sh.C[6] *= zh.z;
    sh.C[7] *= zh.z;
    sh.C[8] *= zh.z;

    return sh;
}

L2_RGB ConvolveWithZH(L2_RGB sh, half3 zh)
{
    // L0
    sh.C[0] *= zh.x;

    // L1
    sh.C[1] *= zh.y;
    sh.C[2] *= zh.y;
    sh.C[3] *= zh.y;

    // L2
    sh.C[4] *= zh.z;
    sh.C[5] *= zh.z;
    sh.C[6] *= zh.z;
    sh.C[7] *= zh.z;
    sh.C[8] *= zh.z;

    return sh;
}

// Convolves a set of L1 SH coefficients with a cosine lobe. See [2]
L1 ConvolveWithCosineLobe(L1 sh)
{
    return ConvolveWithZH(sh, half2(CosineA0, CosineA1));
}

L1_RGB ConvolveWithCosineLobe(L1_RGB sh)
{
    return ConvolveWithZH(sh, half2(CosineA0, CosineA1));
}

// Convolves a set of L2 SH coefficients with a cosine lobe. See [2]
L2 ConvolveWithCosineLobe(L2 sh)
{
    return ConvolveWithZH(sh, half3(CosineA0, CosineA1, CosineA2));
}

L2_RGB ConvolveWithCosineLobe(L2_RGB sh)
{
    return ConvolveWithZH(sh, half3(CosineA0, CosineA1, CosineA2));
}

// Computes the "optimal linear direction" for a set of SH coefficients, AKA the "dominant" direction. See [0].
half3 OptimalLinearDirection(L1 sh)
{
    return normalize(half3(sh.C[3], sh.C[1], sh.C[2]));
}

half3 OptimalLinearDirection(L1_RGB sh)
{
    half3 direction = 0.0;
    for(uint i = 0; i < 3; ++i)
    {
        direction.x += sh.C[3][i];
        direction.y += sh.C[1][i];
        direction.z += sh.C[2][i];
    }
    return normalize(direction);
}

// Computes the direction and color of a directional light that approximates a set of L1 SH coefficients. See [0].
void ApproximateDirectionalLight(L1 sh, out half3 direction, out half intensity)
{
    direction = OptimalLinearDirection(sh);
    L1 dirSH = ProjectOntoL1(direction, 1.0);
    dirSH.C[0] = 0.0;
    intensity = DotProduct(dirSH, sh) * (867.0 / (316.0 * Pi));
}

void ApproximateDirectionalLight(L1_RGB sh, out half3 direction, out half3 color)
{
    direction = OptimalLinearDirection(sh);
    L1_RGB dirSH = ProjectOntoL1_RGB(direction, 1.0);
    dirSH.C[0] = 0.0;
    color = DotProduct(dirSH, sh) * (867.0 / (316.0 * Pi));
}

// Calculates the irradiance from a set of SH coefficients containing projected radiance.
// Convolves the radiance with a cosine lobe, and then evaluates the result in the given normal direction.
// Note that this does not scale the irradiance by 1 / Pi: if using this result for Lambertian diffuse,
// you will want to include the divide-by-pi that's part of the Lambertian BRDF.
// For example: half3 diffuse = CalculateIrradiance(sh, normal) * diffuseAlbedo / Pi;
half CalculateIrradiance(L1 sh, half3 normal)
{
    L1 convolved = ConvolveWithCosineLobe(sh);
    return Evaluate(convolved, normal);
}

half3 CalculateIrradiance(L1_RGB sh, half3 normal)
{
    L1_RGB convolved = ConvolveWithCosineLobe(sh);
    return Evaluate(convolved, normal);
}

// Calculates the irradiance from a set of SH coefficients containing projected radiance.
// Convolves the radiance with a cosine lobe, and then evaluates the result in the given normal direction.
// Note that this does not scale the irradiance by 1 / Pi: if using this result for Lambertian diffuse,
// you will want to include the divide-by-pi that's part of the Lambertian BRDF.
// For example: half3 diffuse = CalculateIrradiance(sh, normal) * diffuseAlbedo / Pi;
half CalculateIrradiance(L2 sh, half3 normal)
{
    L2 convolved = ConvolveWithCosineLobe(sh);
    return Evaluate(convolved, normal);
}

half3 CalculateIrradiance(L2_RGB sh, half3 normal)
{
    L2_RGB convolved = ConvolveWithCosineLobe(sh);
    return Evaluate(convolved, normal);
}

// Calculates the irradiance from a set of L1 SH coeffecients using the non-linear fit from [1]
// Note that this does not scale the irradiance by 1 / Pi: if using this result for Lambertian diffuse,
// you will want to include the divide-by-pi that's part of the Lambertian BRDF.
// For example: half3 diffuse = CalculateIrradianceGeomerics(sh, normal) * diffuseAlbedo / Pi;
half CalculateIrradianceGeomerics(L1 sh, half3 normal)
{
    half R0 = max(sh.C[0], 0.00001);

    half3 R1 = 0.5 * half3(sh.C[3], sh.C[1], sh.C[2]);
    half lenR1 = max(length(R1), 0.00001);

    half q = 0.5 * (1.0 + dot(R1 / lenR1, normal));

    half p = 1.0 + 2.0 * lenR1 / R0;
    half a = (1.0 - lenR1 / R0) / (1.0 + lenR1 / R0);

    return R0 * (a + (1.0 - a) * (p + 1.0) * pow(abs(q), p));
}

half3 CalculateIrradianceGeomerics(L1_RGB sh, half3 normal)
{
    L1 shr = L1::Create(sh.C[0].x, sh.C[1].x, sh.C[2].x, sh.C[3].x);
    L1 shg = L1::Create(sh.C[0].y, sh.C[1].y, sh.C[2].y, sh.C[3].y);
    L1 shb = L1::Create(sh.C[0].z, sh.C[1].z, sh.C[2].z, sh.C[3].z);

    return half3(CalculateIrradianceGeomerics(shr, normal), CalculateIrradianceGeomerics(shg, normal), CalculateIrradianceGeomerics(shb, normal));
}

// Calculates the irradiance from a set of L1 SH coefficientions by 'hallucinating" L3 zonal harmonics. See [4].
half CalculateIrradianceL1ZH3Hallucinate(L1 sh, half3 normal)
{
    const half3 zonalAxis = normalize(half3(sh.C[3], sh.C[1], sh.C[2]));

    half ratio = abs(dot(half3(sh.C[3], sh.C[1], sh.C[2]), zonalAxis)) / sh.C[0];

    const half zonalL2Coeff = sh.C[0] * (0.08 * ratio + 0.6 * ratio * ratio);

    const half fZ = dot(zonalAxis, normal);
    const half zhDir = sqrt(5.0 / (16.0 * Pi)) * (3.0 * fZ * fZ - 1.0);

    const half baseIrradiance = CalculateIrradiance(sh, normal);

    return baseIrradiance + ((Pi * 0.25) * zonalL2Coeff * zhDir);
}

half3 CalculateIrradianceL1ZH3Hallucinate(L1_RGB sh, half3 normal)
{
    const half3 lumCoefficients = half3(0.2126, 0.7152, 0.0722);
    const half3 zonalAxis = normalize(half3(dot(sh.C[3], lumCoefficients), dot(sh.C[1], lumCoefficients), dot(sh.C[2], lumCoefficients)));

    half3 ratio;
    for(uint i = 0; i < 3; ++i)
        ratio[i] = abs(dot(half3(sh.C[3][i], sh.C[1][i], sh.C[2][i]), zonalAxis)) / sh.C[0][i];

    const half3 zonalL2Coeff = sh.C[0] * (0.08 * ratio + 0.6 * ratio * ratio);

    const half fZ = dot(zonalAxis, normal);
    const half zhDir = sqrt(5.0 / (16.0 * Pi)) * (3.0 * fZ * fZ - 1.0);

    const half3 baseIrradiance = CalculateIrradiance(sh, normal);

    return baseIrradiance + ((Pi * 0.25) * zonalL2Coeff * zhDir);
}

// Approximates a GGX lobe with a given roughness/alpha as L1 zonal harmonics, using a fitted curve
half2 ApproximateGGXAsL1ZH(half ggxAlpha)
{
    const half l1Scale = 1.66711256633276 / (1.65715038133932 + ggxAlpha);
    return half2(1.0, l1Scale);
}

// Approximates a GGX lobe with a given roughness/alpha as L2 zonal harmonics, using a fitted curve
half3 ApproximateGGXAsL2ZH(half ggxAlpha)
{
    const half l1Scale = 1.66711256633276 / (1.65715038133932 + ggxAlpha);
    const half l2Scale = 1.56127990596116 / (0.96989757593282 + ggxAlpha) - 0.599972342361123;
    return half3(1.0, l1Scale, l2Scale);
}

// Convolves a set of L1 SH coefficients with a GGX lobe for a given roughness/alpha
L1 ConvolveWithGGX(L1 sh, half ggxAlpha)
{
    return ConvolveWithZH(sh, ApproximateGGXAsL1ZH(ggxAlpha));
}

L1_RGB ConvolveWithGGX(L1_RGB sh, half ggxAlpha)
{
    return ConvolveWithZH(sh, ApproximateGGXAsL1ZH(ggxAlpha));
}

// Convolves a set of L2 SH coefficients with a GGX lobe for a given roughness/alpha
L2 ConvolveWithGGX(L2 sh, half ggxAlpha)
{
    return ConvolveWithZH(sh, ApproximateGGXAsL2ZH(ggxAlpha));
}

L2_RGB ConvolveWithGGX(L2_RGB sh, half ggxAlpha)
{
    return ConvolveWithZH(sh, ApproximateGGXAsL2ZH(ggxAlpha));
}

// Given a set of L1 SH coefficients represnting incoming radiance, determines a directional light
// direction, color, and modified roughness value that can be used to compute an approximate specular term. See [5]
void ExtractSpecularDirLight(L1 shRadiance, half sqrtRoughness, out half3 lightDir, out half lightIntensity, out half modifiedSqrtRoughness)
{
    half3 avgL1 = half3(shRadiance.C[3], shRadiance.C[1], shRadiance.C[2]);
    avgL1 *= 0.5;
    half avgL1len = length(avgL1);

    lightDir = avgL1 / avgL1len;
    lightIntensity = Evaluate(shRadiance, lightDir) * Pi;
    modifiedSqrtRoughness = saturate(sqrtRoughness / sqrt(avgL1len));
}

void ExtractSpecularDirLight(L1_RGB shRadiance, half sqrtRoughness, out half3 lightDir, out half3 lightColor, out half modifiedSqrtRoughness)
{
    half3 avgL1 = half3(dot(shRadiance.C[3] / shRadiance.C[0], 0.333), dot(shRadiance.C[1] / shRadiance.C[0], 0.333), dot(shRadiance.C[2] / shRadiance.C[0], 0.333));
    avgL1 *= 0.5;
    half avgL1len = length(avgL1);

    lightDir = avgL1 / avgL1len;
    lightColor = Evaluate(shRadiance, lightDir) * Pi;
    modifiedSqrtRoughness = saturate(sqrtRoughness / sqrt(avgL1len));
}

// Rotates a set of L1 coefficients by a rotation matrix. Adapted from DirectX::XMSHRotate [3]
L1 Rotate(L1 sh, half3x3 rotation)
{
    L1 result;

    // L0
    result.C[0] = sh.C[0];

    // L1
    half3 dir = half3(sh.C[3], sh.C[1], sh.C[2]);
    dir = mul(dir, rotation);
    result.C[3] = dir.x;
    result.C[1] = dir.y;
    result.C[2] = dir.z;

    return result;
}

L1_RGB Rotate(L1_RGB sh, half3x3 rotation)
{
    L1_RGB result;

    // L0
    result.C[0] = sh.C[0];

    // L1
    [unroll]
    for(uint i = 0; i < 3; ++i)
    {
        half3 dir = half3(sh.C[3][i], sh.C[1][i], sh.C[2][i]);
        dir = mul(dir, rotation);
        result.C[3][i] = dir.x;
        result.C[1][i] = dir.y;
        result.C[2][i] = dir.z;
    }


    return result;
}

// Rotates a set of L2 coefficients by a rotation matrix. Adapted from DirectX::XMSHRotate [3]
L2 Rotate(L2 sh, half3x3 rotation)
{
    // The basis vectors used in DXSH are slightly different than ours,
    // the X and Z are flipped relative to what's used above in ProjectOntoL1/L2.
    // Hence there are several negations here to adapt the code work for us.
    const half r00 = rotation._m00;
    const half r10 = rotation._m01;
    const half r20 = -rotation._m02;

    const half r01 = rotation._m10;
    const half r11 = rotation._m11;
    const half r21 = -rotation._m12;

    const half r02 = -rotation._m20;
    const half r12 = -rotation._m21;
    const half r22 = rotation._m22;

    L2 result;

    // L0
    result.C[0] = sh.C[0];

    // L1
    result.C[1] = (r11 * sh.C[1] - r12 * sh.C[2] + r10 * sh.C[3]);
    result.C[2] = (-r21 * sh.C[1] + r22 * sh.C[2] - r20 * sh.C[3]);
    result.C[3] = (r01 * sh.C[1] - r02 * sh.C[2] + r00 * sh.C[3]);

    // L2
    const half t41 = r01 * r00;
    const half t43 = r11 * r10;
    const half t48 = r11 * r12;
    const half t50 = r01 * r02;
    const half t55 = r02 * r02;
    const half t57 = r22 * r22;
    const half t58 = r12 * r12;
    const half t61 = r00 * r02;
    const half t63 = r10 * r12;
    const half t68 = r10 * r10;
    const half t70 = r01 * r01;
    const half t72 = r11 * r11;
    const half t74 = r00 * r00;
    const half t76 = r21 * r21;
    const half t78 = r20 * r20;

    const half v173 = 0.1732050808e1;
    const half v577 = 0.5773502693e0;
    const half v115 = 0.1154700539e1;
    const half v288 = 0.2886751347e0;
    const half v866 = 0.8660254040e0;

    half r[25];
    r[0] = r11 * r00 + r01 * r10;
    r[1] = -r01 * r12 - r11 * r02;
    r[2] =  v173 * r02 * r12;
    r[3] = -r10 * r02 - r00 * r12;
    r[4] = r00 * r10 - r01 * r11;
    r[5] = - r11 * r20 - r21 * r10;
    r[6] = r11 * r22 + r21 * r12;
    r[7] = -v173 * r22 * r12;
    r[8] = r20 * r12 + r10 * r22;
    r[9] = -r10 * r20 + r11 * r21;
    r[10] = -v577 * (t41 + t43) + v115 * r21 * r20;
    r[11] = v577 * (t48 + t50) - v115 * r21 * r22;
    r[12] = -0.5 * (t55 + t58) + t57;
    r[13] = v577 * (t61 + t63) - v115 * r20 * r22;
    r[14] =  v288 * (t70 - t68 + t72 - t74) - v577 * (t76 - t78);
    r[15] = -r01 * r20 -  r21 * r00;
    r[16] = r01 * r22 + r21 * r02;
    r[17] = -v173 * r22 * r02;
    r[18] = r00 * r22 + r20 * r02;
    r[19] = -r00 * r20 + r01 * r21;
    r[20] = t41 - t43;
    r[21] = -t50 + t48;
    r[22] =  v866 * (t55 - t58);
    r[23] = t63 - t61;
    r[24] = 0.5 * (t74 - t68 - t70 +  t72);

    for(uint i = 0; i < 5; ++i)
    {
        const uint base = i * 5;
        result.C[4 + i] = (r[base + 0] * sh.C[4] + r[base + 1] * sh.C[5] +
                           r[base + 2] * sh.C[6] + r[base + 3] * sh.C[7] +
                           r[base + 4] * sh.C[8]);
    }

    return result;
}

L2_RGB Rotate(L2_RGB sh, half3x3 rotation)
{
    // The basis vectors used in DXSH are slightly different than ours,
    // the X and Z are flipped relative to what's used above in ProjectOntoL1/L2.
    // Hence there are several negations here to adapt the code work for us.
    const half r00 = rotation._m00;
    const half r10 = rotation._m01;
    const half r20 = -rotation._m02;

    const half r01 = rotation._m10;
    const half r11 = rotation._m11;
    const half r21 = -rotation._m12;

    const half r02 = -rotation._m20;
    const half r12 = -rotation._m21;
    const half r22 = rotation._m22;

    L2_RGB result;

    // L0
    result.C[0] = sh.C[0];

    // L1
    result.C[1] = (r11 * sh.C[1] - r12 * sh.C[2] + r10 * sh.C[3]);
    result.C[2] = (-r21 * sh.C[1] + r22 * sh.C[2] - r20 * sh.C[3]);
    result.C[3] = (r01 * sh.C[1] - r02 * sh.C[2] + r00 * sh.C[3]);

    // L2
    const half t41 = r01 * r00;
    const half t43 = r11 * r10;
    const half t48 = r11 * r12;
    const half t50 = r01 * r02;
    const half t55 = r02 * r02;
    const half t57 = r22 * r22;
    const half t58 = r12 * r12;
    const half t61 = r00 * r02;
    const half t63 = r10 * r12;
    const half t68 = r10 * r10;
    const half t70 = r01 * r01;
    const half t72 = r11 * r11;
    const half t74 = r00 * r00;
    const half t76 = r21 * r21;
    const half t78 = r20 * r20;

    const half v173 = 0.1732050808e1;
    const half v577 = 0.5773502693e0;
    const half v115 = 0.1154700539e1;
    const half v288 = 0.2886751347e0;
    const half v866 = 0.8660254040e0;

    half r[25];
    r[0] = r11 * r00 + r01 * r10;
    r[1] = -r01 * r12 - r11 * r02;
    r[2] =  v173 * r02 * r12;
    r[3] = -r10 * r02 - r00 * r12;
    r[4] = r00 * r10 - r01 * r11;
    r[5] = - r11 * r20 - r21 * r10;
    r[6] = r11 * r22 + r21 * r12;
    r[7] = -v173 * r22 * r12;
    r[8] = r20 * r12 + r10 * r22;
    r[9] = -r10 * r20 + r11 * r21;
    r[10] = -v577 * (t41 + t43) + v115 * r21 * r20;
    r[11] = v577 * (t48 + t50) - v115 * r21 * r22;
    r[12] = -0.5 * (t55 + t58) + t57;
    r[13] = v577 * (t61 + t63) - v115 * r20 * r22;
    r[14] =  v288 * (t70 - t68 + t72 - t74) - v577 * (t76 - t78);
    r[15] = -r01 * r20 -  r21 * r00;
    r[16] = r01 * r22 + r21 * r02;
    r[17] = -v173 * r22 * r02;
    r[18] = r00 * r22 + r20 * r02;
    r[19] = -r00 * r20 + r01 * r21;
    r[20] = t41 - t43;
    r[21] = -t50 + t48;
    r[22] =  v866 * (t55 - t58);
    r[23] = t63 - t61;
    r[24] = 0.5 * (t74 - t68 - t70 +  t72);

    for(uint i = 0; i < 5; ++i)
    {
        const uint base = i * 5;
        result.C[4 + i] = (r[base + 0] * sh.C[4] + r[base + 1] * sh.C[5] +
                           r[base + 2] * sh.C[6] + r[base + 3] * sh.C[7] +
                           r[base + 4] * sh.C[8]);
    }

    return result;
}

} // namespace SH

// References:
//
// [0] Stupid SH Tricks by Peter-Pike Sloan - https://www.ppsloan.org/publications/StupidSH36.pdf
// [1] Converting SH Radiance to Irradiance by Graham Hazel - https://grahamhazel.com/blog/2017/12/22/converting-sh-radiance-to-irradiance/
// [2] An Efficient Representation for Irradiance Environment Maps by Ravi Ramamoorthi and Pat Hanrahan - https://cseweb.ucsd.edu/~ravir/6998/papers/envmap.pdf
// [3] SHMath by Chuck Walbourn (originally written by Peter-Pike Sloan) - https://walbourn.github.io/spherical-harmonics-math/
// [4] ZH3: Quadratic Zonal Harmonics by Thomas Roughton, Peter-Pike Sloan, Ari Silvennoinen, Michal Iwanicki, and Peter Shirley - https://torust.me/ZH3.pdf
// [5] Precomputed Global Illumination in Frostbite by Yuriy O'Donnell - https://www.ea.com/frostbite/news/precomputed-global-illumination-in-frostbite

#endif // SH_LITE_HLSLI_
