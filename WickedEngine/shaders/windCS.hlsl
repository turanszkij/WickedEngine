#include "globals.hlsli"

RWTexture3D<float> output : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 position = (float3)DTid;

	const float time = GetTime();
	position += time;
	const ShaderWind wind = GetWeather().wind;

	float randomness_amount = 0;
	randomness_amount += noise_gradient_3D(position.xyz);
	randomness_amount += noise_gradient_3D(position.xyz / 2.0f);
	randomness_amount += noise_gradient_3D(position.xyz / 4.0f);
	randomness_amount += noise_gradient_3D(position.xyz / 8.0f);
	randomness_amount += noise_gradient_3D(position.xyz / 16.0f);
	randomness_amount += noise_gradient_3D(position.xyz / 32.0f);
	randomness_amount *= wind.randomness;

	float direction_amount = dot(position.xyz, wind.direction);
	float waveoffset = mad(direction_amount, wind.wavesize, randomness_amount);

	output[DTid] = sin(mad(time, wind.speed, waveoffset));
}
