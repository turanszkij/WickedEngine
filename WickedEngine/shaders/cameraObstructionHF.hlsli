#ifndef WI_CAMERA_OBSTRUCTION_HF
#define WI_CAMERA_OBSTRUCTION_HF
// Shared scalar math for pixel prepass and native policy regressions. The
// aperture is a finite, feathered elliptical cone, NOT an entire-instance fade.
#ifdef __cplusplus
#include "ShaderInterop.h"
#include <cmath>
#endif
inline float CameraObstructionSaturate(float value)
{ return value < 0 ? 0 : (value > 1 ? 1 : value); }
inline float CameraObstructionDither(float3 position, float3 eye, float4 focusRadius, float amount, float facing)
{
#ifdef __cplusplus
    using std::sqrt;
#endif
    if (amount <= 0 || focusRadius.w <= 0) return 0;
    float ax=focusRadius.x-eye.x, ay=focusRadius.y-eye.y, az=focusRadius.z-eye.z;
    float length2=ax*ax+ay*ay+az*az;
    if (length2 < 0.0001f) return 0;
    float px=position.x-eye.x, py=position.y-eye.y, pz=position.z-eye.z;
    float t=(px*ax+py*ay+pz*az)/length2;
    if (t <= 0 || t >= 1.04f) return 0;
    float inverseLength=1.0f/sqrt(length2);
    ax*=inverseLength; ay*=inverseLength; az*=inverseLength;
    // Camera-horizontal right with a finite fallback for vertical orbit.
    float horizontal=sqrt(ax*ax+az*az);
    float rx=horizontal>0.001f ? az/horizontal : 1;
    float rz=horizontal>0.001f ? -ax/horizontal : 0;
    float ux=ay*rz, uy=az*rx-ax*rz, uz=-ay*rx;
    float side=px*rx+pz*rz;
    float vertical=(px*ux+py*uy+pz*uz)/2.25f;
    float radius=focusRadius.w*(0.35f+0.65f*CameraObstructionSaturate(t));
    float radial=sqrt(side*side+vertical*vertical)/radius;
    float feather=CameraObstructionSaturate((1-radial)/0.30f);
    feather=feather*feather*(3-2*feather);
    float endFeather=CameraObstructionSaturate((1.04f-t)/0.07f);
    float nearFeather=CameraObstructionSaturate(t/0.03f);
    float edge=1-CameraObstructionSaturate(facing);
    edge=edge*edge*edge*edge;
    // Leave a ghost of the trunk face, with substantially stronger grazing
    // silhouettes. No emissive tint or full-tree outline postprocess.
    return CameraObstructionSaturate(amount)*feather*endFeather*nearFeather*(0.80f-0.45f*edge);
}
#endif
