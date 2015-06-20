#ifndef NORMALSCOMPRESS
#define NORMALSCOMPRESS

float2 encodeNormal (float3 n)
{
    return n.xy*0.5+0.5;
}
float3 decodeNormal (float2 enc)
{
    float3 n;
    n.xy = enc*2-1;
    n.z = sqrt(1-dot(n.xy, n.xy));
    return n;
}

#endif