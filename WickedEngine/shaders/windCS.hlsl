#include "globals.hlsli"

RWTexture3D<float> output : register(u0);
RWTexture3D<float> output_prev : register(u1);

float compute_wind(float3 position, float time)
{
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

	return sin(mad(time, wind.speed, waveoffset));
}

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 position = (float3)DTid;

	output[DTid] = compute_wind(position, GetTime());
	output_prev[DTid] = compute_wind(position, GetTimePrev());
}
