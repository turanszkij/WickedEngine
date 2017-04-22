#ifndef _TONEMAP_H_
#define _TONEMAP_H_

// Reinhard operator
float3 tonemap(float3 x)
{
	return x / (x + 1);
}
float3 inverseTonemap(float3 x)
{
	return x / (1 - x);
}

#endif // _TONEMAP_H_