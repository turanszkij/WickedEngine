#ifndef PACK_HLSL
#define PACK_HLSL

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

#endif //PACK_HLSL