#ifndef WI_BITONIC_SORT_COMPUTE_HF
#define WI_BITONIC_SORT_COMPUTE_HF

CBUFFER(CB,0)
{
	unsigned int g_iLevel;
	unsigned int g_iLevelMask;
	unsigned int g_iWidth;
	unsigned int g_iHeight;
};


ByteAddressBuffer Input : register(t0);

RWByteAddressBuffer Data : register(u0);

static const uint _stride = 4; // using 32 bit uints


#endif // WI_BITONIC_SORT_COMPUTE_HF
