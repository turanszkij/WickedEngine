cbuffer lightStatic:register(b0){
	float4x4 matProjInv;
}

#include "reconstructPositionHF.hlsli"
#include "tangentComputeHF.hlsli"

Texture2D<float> xSceneDepthMap:register(t1);
Texture2D<float4> xTexture:register(t2);
Texture2D<float4> xNormal:register(t3);
SamplerState texSampler:register(s0);

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};
struct PixelOutputType
{
	float4 col	: SV_TARGET0;
	float4 nor	: SV_TARGET1;
};

cbuffer decalCB:register(b1){
	float4x4 xDecalVP;
	int hasTexNor;
	float3 eye;
	float opacity;
	float3 front;
}

PixelOutputType main(VertexToPixel PSIn)
{
	PixelOutputType Out = (PixelOutputType)0;

	float2 screenPos;
		screenPos.x = PSIn.pos2D.x/PSIn.pos2D.w/2.0f + 0.5f;
		screenPos.y = -PSIn.pos2D.y/PSIn.pos2D.w/2.0f + 0.5f;
	float2 depthMapSize;
	xSceneDepthMap.GetDimensions(depthMapSize.x,depthMapSize.y);
	float depth = xSceneDepthMap.Load(int4(screenPos*depthMapSize,0,0)).r;
	float3 pos3D = getPosition(screenPos,depth);

	float4 projPos;
		projPos = mul(float4(pos3D,1),xDecalVP);
	float3 projTex;
		projTex.x = projPos.x/projPos.w/2.0f +0.5f;
		projTex.y = -projPos.y/projPos.w/2.0f +0.5f;
		projTex.z = projPos.z/projPos.w;
	clip( ((saturate(projTex.x) == projTex.x) && (saturate(projTex.y) == projTex.y) && (saturate(projTex.z) == projTex.z))?1:-1 );

	if(hasTexNor & 0x0000010){
		float3 normal = normalize( cross( ddx(pos3D),ddy(pos3D) ) );
		//clip( dot(normal,front)>-0.2?-1:1 ); //clip at oblique angle
		float4 nortex=xNormal.Sample(texSampler,projTex.xy);
		float3 eyevector = normalize( eye - pos3D );
		if(nortex.a>0){
			float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -projTex.xy);
			float3 bumpColor = 2.0f * nortex - 1.0f;
			//bumpColor.g*=-1;
			normal = normalize(mul(bumpColor, tangentFrame));
		}
		Out.nor.a=min(nortex.b,nortex.a);
		Out.nor.xyz=normal;
	}
	if(hasTexNor & 0x0000001){
		Out.col=xTexture.Sample(texSampler,projTex.xy);
		Out.col.a*=opacity;
		clip( Out.col.a<0.05?-1:1 );
		if(hasTexNor & 0x0000010)
			Out.nor.a=Out.col.a;
	}


	return Out;
}