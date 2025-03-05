#ifndef CAPSULE_SHADOW_HF
#define CAPSULE_SHADOW_HF

// Source: https://www.shadertoy.com/view/3stcD4

float acosFast(float x) {
    // Lagarde 2014, "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // This is the approximation of degree 1, with a max absolute error of 9.0x10^-3
    float y = abs(x);
    float p = -0.1565827 * y + 1.570796;
    p *= sqrt(1.0 - y);
    return x >= 0.0 ? p : PI - p;
}

float acosFastPositive(float x) {
    // Lagarde 2014, "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    float p = -0.1565827 * x + 1.570796;
    return p * sqrt(1.0 - x);
}

float sphericalCapsIntersection(float cosCap1, float cosCap2, float cap2, float cosDistance) {
    // Oat and Sander 2007, "Ambient Aperture Lighting"
    // Approximation mentioned by Jimenez et al. 2016
    float r1 = acosFastPositive(cosCap1);
    float r2 = cap2;
    float d  = acosFast(cosDistance);

    // We work with cosine angles, replace the original paper's use of
    // cos(min(r1, r2)_ with max(cosCap1, cosCap2)
    // We also remove a multiplication by 2 * PI to simplify the computation
    // since we divide by 2 * PI at the call site

    if (min(r1, r2) <= max(r1, r2) - d) {
        return 1.0 - max(cosCap1, cosCap2);
    } else if (r1 + r2 <= d) {
        return 0.0;
    }

    float delta = abs(r1 - r2);
    float x = 1.0 - saturate((d - delta) / max(r1 + r2 - delta, 0.0001));
    // simplified smoothstep()
    float area = sqr(x) * (-2.0 * x + 3.0);
    return area * (1.0 - max(cosCap1, cosCap2));
}

float directionalOcclusionSphere(in float3 pos, in float4 sphere, in float4 cone) {
    float3 occluder = sphere.xyz - pos;
    float occluderLength2 = dot(occluder, occluder);
    float3 occluderDir = occluder * rsqrt(occluderLength2);

    float cosPhi = dot(occluderDir, cone.xyz);
    // sqr(sphere.w) should be a uniform --> capsuleRadius^2
    float cosTheta = sqrt(occluderLength2 / (sqr(sphere.w) + occluderLength2));
    float cosCone = cos(cone.w);

    return 1.0 - sphericalCapsIntersection(cosTheta, cosCone, cone.w, cosPhi) / (1.0 - cosCone);
}

float directionalOcclusionCapsule(in float3 pos, in float3 capsuleA, in float3 capsuleB, in float capsuleRadius, in float4 cone) {
    float3 Ld = capsuleB - capsuleA;
    float3 L0 = capsuleA - pos;
    float a = dot(cone.xyz, Ld);
    float t = saturate(dot(L0, a * cone.xyz - Ld) / (dot(Ld, Ld) - a * a));
    float3 posToRay = capsuleA + t * Ld;

    return directionalOcclusionSphere(pos, float4(posToRay, capsuleRadius), cone);
}

#endif // CAPSULE_SHADOW_HF
