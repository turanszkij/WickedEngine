#include "lightHF.hlsli"
#include "spotLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{
	float4 light = float4(lightColor.rgb,1);
	float2 screenPos = float2(1,-1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;
	float depth = depthMap.SampleLevel(sampler_point_clamp,screenPos,0);
	//clip(depth<1?1:-1);

		

	
	[branch]if(depth<g_xCamera_ZFarP){

		float4 norU = normalMap.SampleLevel(sampler_point_clamp,screenPos,0);
		bool unshaded = isUnshaded(norU.w);

		[branch]if(!unshaded){
			float4 material = materialMap.SampleLevel(sampler_point_clamp,screenPos,0);
			float specular = material.w*specularMaximumIntensity;
			uint specular_power = material.z;
			float3 normal = norU.xyz;
			bool toonshaded = isToon(norU.w);


			float3 pos3D = getPosition(screenPos,depth);
			float3 eyevector = normalize(g_xCamera_CamPos.xyz-pos3D.xyz);

			float attenuation;
			float3 lightToPixel;
			float lightInt = spotLight(pos3D, normal, attenuation, lightToPixel, toonshaded);
			light.rgb *= lightInt;

		
			//float rimLight = saturate(1.0f-specular) * saturate(pow(saturate(1.0f-dot(normal,eyevector)),3));
			//light.rgb+=rimLight;
	
			//SPECULAR
			applySpecular(light, lightColor * lightInt, normal, eyevector, lightToPixel, 1, specular_power, specular, toonshaded);
			//else 
			//	light.a=0;

			light*=attenuation;
		}
		else
			light=float4(1,1,1,1);

	}


	return max(light, 0);
}