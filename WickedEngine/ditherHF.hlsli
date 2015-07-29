#ifndef DITHER_HF
#define DITHER_HF

inline float ditherMask(in float2 pixel, in float param)
{
	float2 dither = frac(pixel.xy * 0.5f);
	return step(param, abs(dither.x - dither.y));
}

#endif //DITHER_HF
