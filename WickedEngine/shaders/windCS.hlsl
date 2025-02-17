#include "globals.hlsli"

RWTexture3D<float> output : register(u0);
RWTexture3D<float> output_prev : register(u1);

float compute_wind(float3 position, float time)
{
	position += time;
	const ShaderWind wind = GetWeather().wind;

	position.xyz *= wind.wavesize;

	float waveoffset = 0;
	waveoffset += noise_gradient_3D(position.xyz);
	waveoffset += noise_gradient_3D(position.xyz / 2.0);
	waveoffset += noise_gradient_3D(position.xyz / 4.0);
	waveoffset += noise_gradient_3D(position.xyz / 8.0);
	waveoffset += noise_gradient_3D(position.xyz / 16.0);
	waveoffset += noise_gradient_3D(position.xyz / 32.0);
	waveoffset *= wind.randomness;

	return sin(mad(time, wind.speed, waveoffset));
}

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 position = (float3)DTid;

	output[DTid] = compute_wind(position, GetTime());
	output_prev[DTid] = compute_wind(position, GetTimePrev());
}
