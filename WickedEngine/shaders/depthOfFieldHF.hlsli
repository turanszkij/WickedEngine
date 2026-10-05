#ifndef WI_DEPTHOFFIELD_HF
#define WI_DEPTHOFFIELD_HF

PUSHCONSTANT(postprocess, PostProcess);

inline float get_coc(in float linear_depth)
{
    return min(dof_maxcoc, dof_cocscale * GetCamera().aperture_size * pow(abs(1 - GetCamera().focal_length / (linear_depth * GetCamera().z_far)), 2.0f));
}

// The half resolution passes (prefilter, main, postfilter) work on tonemapped colors, and the
//	upsample converts back to HDR. Averaged linearly, a very bright surface dominates every sample
//	disc it touches, so a slightly defocused bright object next to a blurred background turns into a
//	solid band of its own color out to the nearest sample ring, with a hard edge that jumps as the
//	rings and tiles shift (same idea as HDR_CORRECTION in temporalaaCS.hlsl).
#define DOF_HDR_CORRECTION

inline float3 dof_to_working_color(float3 color)
{
#ifdef DOF_HDR_CORRECTION
	return tonemap(color);
#else
	return color;
#endif // DOF_HDR_CORRECTION
}
inline float3 dof_from_working_color(float3 color)
{
#ifdef DOF_HDR_CORRECTION
	return inverse_tonemap(min(color, 0.996)); // the texture format can round to 1, which would map to infinity
#else
	return color;
#endif // DOF_HDR_CORRECTION
}

#define DOF_DEPTH_SCALE_FOREGROUND (GetCamera().z_far * 1.5)
float2 DepthCmp2(float depth, float closestTileDepth)
{
    float d = DOF_DEPTH_SCALE_FOREGROUND * (depth - closestTileDepth);
    float2 depthCmp;
    depthCmp.x = smoothstep(0.0, 1.0, d); // Background
    depthCmp.y = 1.0 - depthCmp.x; // Foreground
    return depthCmp;

}

#define DOF_SINGLE_PIXEL_RADIUS 0.7071 // length( float2(0.5, 0.5 ) )
float SampleAlpha(float sampleCoc)
{
    return min(rcp(PI * sampleCoc * sampleCoc), rcp(PI * DOF_SINGLE_PIXEL_RADIUS * DOF_SINGLE_PIXEL_RADIUS));
}

#define DOF_SPREAD_TOE_POWER 2
float SpreadToe(float offsetCoc, float spreadCmp)
{
    return offsetCoc <= 1.0 ? pow(spreadCmp, DOF_SPREAD_TOE_POWER) : spreadCmp;
}
float SpreadCmp(float offsetCoc, float sampleCoc, float pixelToSampleUnitsScale)
{
    return SpreadToe(offsetCoc, saturate(pixelToSampleUnitsScale * sampleCoc - offsetCoc + 1.0));
}

#define DOF_MAX_RING_COUNT 4
#define DOF_RING_COUNT 3
static const float3 disc[] =
{
    {  0.2310, -0.0957, 1.0 },
    { -0.2310,  0.0957, 1.0 },
    { -0.2310, -0.0957, 1.0 },
    {  0.2310,  0.0957, 1.0 },
    { -0.0957, -0.2310, 1.0 },
    {  0.0957,  0.2310, 1.0 },
    {  0.0957, -0.2310, 1.0 },
    { -0.0957,  0.2310, 1.0 },
    { -0.3536,  0.3536, 2.0 },
    {  0.3536, -0.3536, 2.0 },
    {  0.4619, -0.1913, 2.0 },
    { -0.4619,  0.1913, 2.0 },
    { -0.5000,  0.0000, 2.0 },
    {  0.5000,  0.0000, 2.0 },
    { -0.4619, -0.1913, 2.0 },
    {  0.4619,  0.1913, 2.0 },
    { -0.3536, -0.3536, 2.0 },
    {  0.3536,  0.3536, 2.0 },
    { -0.1913, -0.4619, 2.0 },
    {  0.1913,  0.4619, 2.0 },
    { -0.0000, -0.5000, 2.0 },
    {  0.0000,  0.5000, 2.0 },
    {  0.1913, -0.4619, 2.0 },
    { -0.1913,  0.4619, 2.0 },
    {  0.4566, -0.5950, 3.0 },
    { -0.4566,  0.5950, 3.0 },
    { -0.5950,  0.4566, 3.0 },
    {  0.5950, -0.4566, 3.0 },
    {  0.6929, -0.2870, 3.0 },
    { -0.6929,  0.2870, 3.0 },
    { -0.7436,  0.0979, 3.0 },
    {  0.7436, -0.0979, 3.0 },
    { -0.7436, -0.0979, 3.0 },
    {  0.7436,  0.0979, 3.0 },
    { -0.6929, -0.2870, 3.0 },
    {  0.6929,  0.2870, 3.0 },
    { -0.5950, -0.4566, 3.0 },
    {  0.5950,  0.4566, 3.0 },
    {  0.4566,  0.5950, 3.0 },
    { -0.4566, -0.5950, 3.0 },
    { -0.2870, -0.6929, 3.0 },
    {  0.2870,  0.6929, 3.0 },
    { -0.0979, -0.7436, 3.0 },
    {  0.0979,  0.7436, 3.0 },
    { -0.0979,  0.7436, 3.0 },
    {  0.0979, -0.7436, 3.0 },
    {  0.2870, -0.6929, 3.0 },
    { -0.2870,  0.6929, 3.0 },
    { -0.5556,  0.8315, 4.0 },
    {  0.5556, -0.8315, 4.0 },
    { -0.7071,  0.7071, 4.0 },
    {  0.7071, -0.7071, 4.0 },
    {  0.8315, -0.5556, 4.0 },
    { -0.8315,  0.5556, 4.0 },
    {  0.9239, -0.3827, 4.0 },
    { -0.9239,  0.3827, 4.0 },
    {  0.9808, -0.1951, 4.0 },
    { -0.9808,  0.1951, 4.0 },
    { -1.0000,  0.0000, 4.0 },
    {  1.0000,  0.0000, 4.0 },
    { -0.9808, -0.1951, 4.0 },
    {  0.9808,  0.1951, 4.0 },
    { -0.9239, -0.3827, 4.0 },
    {  0.9239,  0.3827, 4.0 },
    {  0.8315,  0.5556, 4.0 },
    { -0.8315, -0.5556, 4.0 },
    { -0.7071, -0.7071, 4.0 },
    {  0.7071,  0.7071, 4.0 },
    { -0.5556, -0.8315, 4.0 },
    {  0.5556,  0.8315, 4.0 },
    { -0.3827, -0.9239, 4.0 },
    {  0.3827,  0.9239, 4.0 },
    {  0.1951,  0.9808, 4.0 },
    { -0.1951, -0.9808, 4.0 },
    { -0.0000, -1.0000, 4.0 },
    {  0.0000,  1.0000, 4.0 },
    {  0.1951, -0.9808, 4.0 },
    { -0.1951,  0.9808, 4.0 },
    {  0.3827, -0.9239, 4.0 },
    { -0.3827,  0.9239, 4.0 },
};

static const uint ringSampleCount[] = { 0, 8, 24, 48, 80 };
static const float ringNormFactor[] = { 1.0, 0.111111, 0.040000, 0.020408, 0.012346 };

#endif // WI_DEPTHOFFIELD_HF
