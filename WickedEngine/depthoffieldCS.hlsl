#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// Implementation based on Jorge Jimenez Siggraph 2014 Next Generation Post Processing in Call of Duty Advanced Warfare

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_presort, float4, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, float4, 0);

#define DOF_RING_COUNT 4

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


#define DOF_SPREAD_TOE_POWER 2
float SpreadToe(float offsetCoc, float spreadCmp) 
{
	return offsetCoc <= 1.0 ? pow(spreadCmp, DOF_SPREAD_TOE_POWER) : spreadCmp; 
}
float SpreadCmp(float offsetCoc, float sampleCoc, float pixelToSampleUnitsScale) 
{
	return SpreadToe(offsetCoc, saturate(pixelToSampleUnitsScale * sampleCoc - offsetCoc + 1.0));
}

#define DOF_SINGLE_PIXEL_RADIUS 0.7071 // length( float2(0.5, 0.5 ) )
float SampleAlpha(float sampleCoc)
{
    return min(rcp(PI * sampleCoc * sampleCoc), rcp(PI * DOF_SINGLE_PIXEL_RADIUS * DOF_SINGLE_PIXEL_RADIUS));
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float maxcoc = neighborhood_mindepth_maxcoc[DTid.xy / DEPTHOFFIELD_TILESIZE].y;

    const uint ringCount = clamp(ceil(pow(maxcoc, 0.5f) * DOF_RING_COUNT), 1, DOF_RING_COUNT);
    const float spreadScale = ringCount / maxcoc;
    const float2 ringScale = float(ringCount) / float(DOF_RING_COUNT) * maxcoc * xPPResolution_rcp;

    const float4 center_color = input[DTid.xy];
    const float3 center_presort = texture_presort[DTid.xy].rgb;
    const float center_coc = center_presort.r;
    const float center_backgroundWeight = center_presort.g;
    const float center_foregroundWeight = center_presort.b;

    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

    float4 background = center_backgroundWeight * float4(center_color.rgb, 1);
    float4 foreground = center_foregroundWeight * float4(center_color.rgb, 1);
    for (uint j = 0; j < ringCount; ++j)
    {
        for (uint i = ringSampleCount[j]; i < ringSampleCount[j + 1]; ++i)
        {
            const float offsetCoc = disc[i].z;
            const float2 uv2 = uv + ringScale * disc[i].xy;
            const float4 color = float4(input.SampleLevel(sampler_point_clamp, uv2, 0).rgb, 1);
            const float3 presort = texture_presort.SampleLevel(sampler_point_clamp, uv2, 0).rgb;
            const float coc = presort.r;
            const float spreadCmp = SpreadCmp(offsetCoc, coc, spreadScale);
            const float backgroundWeight = presort.g * spreadCmp;
            const float foregroundWeight = presort.b * spreadCmp;
            background += backgroundWeight * color;
            foreground += foregroundWeight * color;
        }
    }
    background.rgb *= rcp(max(0.00001, background.a));
    foreground.rgb *= rcp(max(0.00001, foreground.a));

    float alpha = saturate(2 * ringNormFactor[ringCount] * rcp(SampleAlpha(maxcoc)) * foreground.a);
    float4 color = float4(lerp(background.rgb, foreground.rgb, alpha), center_color.a);

    output[DTid.xy] = color;
    //output[DTid.xy] = alpha;
    //output[DTid.xy] = maxcoc;
    //output[DTid.xy] = float(ringCount) / float(DOF_RING_COUNT);
}
