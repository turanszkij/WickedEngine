#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);


// CRT TV screen shader, source: https://www.shadertoy.com/view/XsjSzR

// Emulated input resolution.
#if 0
  // Fix resolution to set amount.
  #define res (float2(320.0/1.0,160.0/1.0))
#else
  // Optimize for resize.
  #define res (postprocess.resolution.xy/6.0)
#endif

// Hardness of scanline.
//  -8.0 = soft
// -16.0 = medium
static const float hardScan=-10.0;

// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
static const float hardPix=-3.0;

// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
static const float2 warp=float2(1.0/32.0,1.0/24.0); 

// Amount of shadow mask.
static const float maskDark=0.5;
static const float maskLight=1.5;

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
float3 ToLinear(float3 c){return float3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
float3 ToSrgb(float3 c){return float3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
float3 Fetch(float2 pos,float2 off)
{
	pos=floor(pos*res+off)/res;
	if(max(abs(pos.x-0.5),abs(pos.y-0.5))>0.5)
		return float3(0.0,0.0,0.0);
	return ToLinear(input.SampleLevel(sampler_linear_clamp, pos.xy, 0).rgb);
}

// Distance in emulated pixels to nearest texel.
float2 Dist(float2 pos)
{
	pos=pos*res;
	return -((pos-floor(pos))-0.5);
}

// 1D Gaussian.
float Gaus(float pos,float scale)
{
	return exp2(scale*pos*pos);
}

// 3-tap Gaussian filter along horz line.
float3 Horz3(float2 pos,float off)
{
	float3 b=Fetch(pos,float2(-1.0,off));
	float3 c=Fetch(pos,float2( 0.0,off));
	float3 d=Fetch(pos,float2( 1.0,off));
	float dst=Dist(pos).x;
	// Convert distance to weight.
	float scale=hardPix;
	float wb=Gaus(dst-1.0,scale);
	float wc=Gaus(dst+0.0,scale);
	float wd=Gaus(dst+1.0,scale);
	// Return filtered sample.
	return (b*wb+c*wc+d*wd)/(wb+wc+wd);
}

// 5-tap Gaussian filter along horz line.
float3 Horz5(float2 pos,float off)
{
	float3 a=Fetch(pos,float2(-2.0,off));
	float3 b=Fetch(pos,float2(-1.0,off));
	float3 c=Fetch(pos,float2( 0.0,off));
	float3 d=Fetch(pos,float2( 1.0,off));
	float3 e=Fetch(pos,float2( 2.0,off));
	float dst=Dist(pos).x;
	// Convert distance to weight.
	float scale=hardPix;
	float wa=Gaus(dst-2.0,scale);
	float wb=Gaus(dst-1.0,scale);
	float wc=Gaus(dst+0.0,scale);
	float wd=Gaus(dst+1.0,scale);
	float we=Gaus(dst+2.0,scale);
	// Return filtered sample.
	return (a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);
}

// Return scanline weight.
float Scan(float2 pos,float off)
{
	float dst=Dist(pos).y;
	return Gaus(dst+off,hardScan);
}

// Allow nearest three lines to effect pixel.
float3 Tri(float2 pos)
{
	float3 a=Horz3(pos,-1.0);
	float3 b=Horz5(pos, 0.0);
	float3 c=Horz3(pos, 1.0);
	float wa=Scan(pos,-1.0);
	float wb=Scan(pos, 0.0);
	float wc=Scan(pos, 1.0);
	return a*wa+b*wb+c*wc;
}

// Distortion of scanlines, and end of screen alpha.
float2 Warp(float2 pos)
{
	pos=pos*2.0-1.0;
	pos*=float2(1.0+(pos.y*pos.y)*warp.x,1.0+(pos.x*pos.x)*warp.y);
	return pos*0.5+0.5;
}

// Shadow mask.
float3 Mask(float2 pos)
{
	pos.x+=pos.y*3.0;
	float3 mask=float3(maskDark,maskDark,maskDark);
	pos.x=frac(pos.x/6.0);
	if(pos.x<0.333)
		mask.r=maskLight;
	else if(pos.x<0.666)
		mask.g=maskLight;
	else
		mask.b=maskLight;
	return mask;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 color = 0;

	float2 uv = (DTid.xy + 0.5) * postprocess.resolution_rcp.xy;
	//uv = Warp(uv);
	color.rgb=Tri(uv)*Mask(DTid.xy);
	color.rgb *= 1 + sin(GetTime() * 100) * postprocess.params0.x;
	color.rgb=ToSrgb(color.rgb);

	output[DTid.xy] = color;
}
