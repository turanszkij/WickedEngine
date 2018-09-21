#include "globals.hlsli"
#include "reconstructPositionHF.hlsli"
#include "tangentComputeHF.hlsli"
#include "packHF.hlsli"


struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};
struct PixelOutputType
{
	float4 col	: SV_TARGET0;
	//float4 nor	: SV_TARGET1;
};

PixelOutputType main(VertexToPixel PSIn)
{
	PixelOutputType Out = (PixelOutputType)0;

	float2 screenPos = PSIn.pos2D.xy / PSIn.pos2D.w * float2(0.5f,-0.5f) + 0.5f;
	float depth = texture_depth.Load(int3(PSIn.pos.xy,0)).r;
	float3 pos3D = getPosition(screenPos,depth);

	float3 clipSpace = mul(float4(pos3D, 1), xDecalVP).xyz;
	float3 projTex = clipSpace.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
	
	clip( ((saturate(projTex.x) == projTex.x) && (saturate(projTex.y) == projTex.y) && (saturate(projTex.z) == projTex.z))?1:-1 );

	//if (hasTexNor & 0x0000010){
	//	float3 normal = normalize(cross(ddx(pos3D), ddy(pos3D)));
	//	//clip( dot(normal,front)>-0.2?-1:1 ); //clip at oblique angle
	//	float4 nortex=texture_1.Sample(sampler_aniso_clamp,projTex.xy);
	//	float3 eyevector = normalize( eye - pos3D );
	//	if(nortex.a>0){
	//		float3 T, B;
	//		float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -projTex.xy, T, B);
	//		float3 bumpColor = 2.0f * nortex.rgb - 1.0f;
	//		//bumpColor.g*=-1;
	//		normal = normalize(mul(bumpColor, tangentFrame));
	//	}
	//	Out.nor.a=min(nortex.b,nortex.a);
	//	Out.nor.xyz=normal;
	//}
	if(hasTexNor & 0x0000001){
		Out.col=texture_0.Sample(sampler_objectshader,projTex.xy);
		Out.col.a*=opacity;
		float3 edgeBlend = clipSpace.xyz;
		edgeBlend = abs(edgeBlend);
		Out.col.a *= 1 - pow(max(max(edgeBlend.x, edgeBlend.y), edgeBlend.z), 8);
		//Out.col.a *= pow(saturate(-dot(normal,front)), 4);
		//ALPHATEST(Out.col.a)
		//if(hasTexNor & 0x0000010)
		//	Out.nor.a=Out.col.a;
	}

	//Out.nor = float4(encode(Out.nor.xyz), 0, Out.nor.a);

	return Out;
}