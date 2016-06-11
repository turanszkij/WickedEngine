#ifndef _PACK_HF_
#define _PACK_HF_
#include "globals.hlsli"

// Helper functions to compress and uncompress two floats

float Pack(float2 input, int precision = 4096)
{
	float2 output = input;
	output.x = floor(output.x * (precision - 1));
	output.y = floor(output.y * (precision - 1));

	return (output.x * precision) + output.y;
}

float2 Unpack(float input, int precision = 4096)
{
	float2 output = float2(0, 0);

	output.y = input % precision;
	output.x = floor(input / precision);

	return output / (precision - 1);
}


// Normal vector compressors

#define NORMALMAPCOMPRESSOR 1

#if NORMALMAPCOMPRESSOR == 0
// Reconstruct Z
float2 encode(float3 n)
{
	return float2(n.xy*0.5 + 0.5);
}
float3 decode(float2 enc)
{
	float3 n;
	n.xy = enc * 2 - 1;
	n.z = sqrt(1 - dot(n.xy, n.xy));
	return n;
}

#elif NORMALMAPCOMPRESSOR == 1

// Spherical coordinates
float2 encode(float3 n)
{
	return (float2(atan2(n.y, n.x) / PI, n.z) + 1.0)*0.5;
}
float3 decode(float2 enc)
{
	float2 ang = enc * 2 - 1;
	float2 scth;
	sincos(ang.x * PI, scth.x, scth.y);
	float2 scphi = float2(sqrt(1.0 - ang.y*ang.y), ang.y);
	return float3(scth.y*scphi.x, scth.x*scphi.x, scphi.y);
}

#else

// Spheremap
float2 encode(float3 n)
{
	float2 enc = normalize(n.xy) * (sqrt(-n.z*0.5 + 0.5));
	enc = enc*0.5 + 0.5;
	return enc;
}
float3 decode(float2 enc)
{
	float4 nn = float4(enc,0,0)*float4(2, 2, 0, 0) + float4(-1, -1, 1, -1);
	float l = dot(nn.xyz, -nn.xyw);
	nn.z = l;
	nn.xy *= sqrt(l);
	return nn.xyz * 2 + float3(0, 0, -1);
}

#endif

#endif // _PACK_HF_