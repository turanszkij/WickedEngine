#ifndef _BITONIC_SORT_COMPUTE_HF_
#define _BITONIC_SORT_COMPUTE_HF_

CBUFFER(CB,0)
{
	unsigned int g_iLevel;
	unsigned int g_iLevelMask;
	unsigned int g_iWidth;
	unsigned int g_iHeight;
};


RAWBUFFER(Input, 0);
RWRAWBUFFER(Data, 0);

static const uint _stride = 4; // using 32 bit uints


#endif // _BITONIC_SORT_COMPUTE_HF_
