inline void affectWind(inout float3 pos, in float3 wind, in float3 time, in float value, in uint randV, in uint randW, in float waveSize){
	wind = sin(time+(pos.x+pos.y+pos.z))*wind.xyz*0.1*value;
	//if(randV%randW) wind=-wind;
	pos+=wind;
}

