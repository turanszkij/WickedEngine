#include "lightHF.hlsli"
#include "pointLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{
	float4 light = float4(lightColor.rgb,1);
	float2 screenPos;
		screenPos.x = PSIn.pos2D.x/PSIn.pos2D.w/2.0f + 0.5f;
		screenPos.y = -PSIn.pos2D.y/PSIn.pos2D.w/2.0f + 0.5f;
	float depth = depthMap.SampleLevel(Sampler,screenPos,0);

	[branch]if(depth<zFarP){	

		float4 norU = normalMap.SampleLevel(Sampler,screenPos,0);
		bool unshaded = isUnshaded(norU.w);

		[branch]if(!unshaded){
			float4 material = materialMap.SampleLevel(Sampler,screenPos,0);
			float specular = material.w*specularMaximumIntensity;
			uint specular_power = material.z;
			float3 normal = norU.xyz;
			bool toonshaded = isToon(norU.w);

			float3 pos3D = getPosition(screenPos,depth);
			float3 eyevector = normalize(PSIn.cam.xyz-pos3D.xyz);
	
			float3 lightDir;
			float attenuation;
			float lightInt = pointLight(pos3D, normal, lightDir, attenuation, toonshaded);
			light *= lightInt;

			//float rimLight = saturate(1.0f-specular) * saturate(pow(saturate(1.0f-dot(normal,eyevector)),3));
			//light.rgb+=rimLight;

			//SPECULAR
			applySpecular(light, lightColor * lightInt, normal, eyevector, lightDir, 1, specular_power, specular, toonshaded);
			//else 
			//	light.a=0;

			light*=attenuation;

		}
		else
			light=float4(1,1,1,1);
	
		//float3 lv = pos3D.xyz-lightPos.xyz;
		//light=xTextureSh.Sample(Sampler,lv);
		//light=lightColor;

	}

	return clamp(light,0,inf);
}