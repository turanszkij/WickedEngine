#include "effectHF_PS.hlsli"
#include "globalsHF.hlsli"
#include "globals.hlsli"

struct VSOut{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

struct PSOut{
	float4 col : SV_TARGET0;
	float4 nor : SV_TARGET1;
	float4 vel : SV_TARGET2;
};

PSOut main(VSOut PSIn)
{
	float2 ScreenCoord, ScreenCoordPrev;
	ScreenCoord.x = PSIn.pos2D.x / PSIn.pos2D.w / 2.0f + 0.5f;
	ScreenCoord.y = -PSIn.pos2D.y / PSIn.pos2D.w / 2.0f + 0.5f;
	ScreenCoordPrev.x = PSIn.pos2DPrev.x / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	ScreenCoordPrev.y = -PSIn.pos2DPrev.y / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	float2 vel = ScreenCoord - ScreenCoordPrev;

	static const float overBright = 1.005f;
	float3 inNor = normalize(PSIn.nor);
	float3 nor = inNor*overBright;
	float3 sun = 0;
	float3 col = 0;
	//[branch]if(xFx.x==0){
		col = /*lerp( xHorizon.rgb,*/ enviroTex.SampleLevel( texSampler,nor,0 ).rgb/*, saturate(nor.y/0.3f) )*/;
		sun = clamp(pow(abs(dot(g_xWorld_SunDir.xyz,nor)),256)*g_xWorld_SunColor.rgb, 0, inf);
	//}
	//else
	//{
	//	sun = clamp(pow(abs(dot(g_xWorld_SunDir.xyz,nor)),64)*g_xWorld_SunColor.rgb, 0, inf);
	//}
	//return float4( col+sun ,1);

	PSOut Out = (PSOut)0;
	Out.col = float4(col + sun, 1);
	Out.nor = float4(inNor,0);
	Out.vel = float4(vel, 0, 0);
	return Out;
}

//struct VSOut{
//	float4 pos : SV_POSITION;
//	float3 tex : TEXCOORD0;
//};
//
//float4 main(VSOut PSIn):SV_TARGET
//{
//	//float sun = pow( saturate( dot(PSIn.tex,normalize(xSun.xyz)) ),2 )<0.5?0:1;
//	return enviroTex.Sample( texSampler,PSIn.tex );
//}

//struct VertextoPixel
//{
//	float4 pos				: SV_POSITION;
//	float2 tex				: TEXCOORD0;
//	float3 Position3D		: TEXCOORD1;
//	//float4 SunPos			: TEXCOORD2;
//	float3 CamPos			: TEXCOORD2;
//	float  AtmospherePos	: TEXCOORD3;
//	//float3 vel				: TEXCOORD5;
//};
///*struct PixelOutputType
//{
//	float4 col	: SV_TARGET0;
//	float4 nor	: SV_TARGET1;
//	float4 spe	: SV_TARGET2;
//	float4 vel	: SV_TARGET4;
//};*/
//
///*PixelOutputType*/float4 main(VertextoPixel PSIn):SV_TARGET
//{
//	/*float4 color = float4(0,0,0,1);
//	if(xFx.x!=1) 
//	{
//		float3 ref = normalize(PSIn.Position3D-PSIn.CamPos); 
//		color = lerp(float4(xHorizon.xyz,1), enviroTex.Sample(texSampler,ref), saturate(PSIn.AtmospherePos));
//	}
//
//	if(xSun.w){
//		float3 lightdirection = normalize(xSun.xyz);
//		float3 eyeVector = normalize(PSIn.CamPos - PSIn.Position3D);
//		float spec = dot(lightdirection, eyeVector);
//		spec = pow(abs(spec), (2-specular.a)*150 );
//		color.rgb += (spec * xSunColor.rgb);
//	}
//
//	if(xFx.y==1){
//		float avg=0;
//		avg+=color.r; avg+=color.g; avg+=color.b; avg/=3.0f;
//		color=avg;
//	}
//
//	if(xFx.z==1){
//		color.r=1-color.r;
//		color.g=1-color.g;
//		color.b=1-color.b;
//	}
//
//	return float4(color.rgb,1);*/
//
//	return enviroTex.Sample(texSampler,normalize(PSIn.Position3D-PSIn.CamPos));
//}