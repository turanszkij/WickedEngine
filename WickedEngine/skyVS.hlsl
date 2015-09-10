#include "icosphere.hlsli"

cbuffer skyCB:register(b3){
	float4x4 xV;
	float4x4 xP;
	float4x4 xPrevView;
	float4x4 xPrevProjection;
}

struct VSOut{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

VSOut main(uint vid : SV_VERTEXID)
{
	VSOut Out = (VSOut)0;
	Out.pos=Out.pos2D=mul(mul(float4(ICOSPHERE[vid],1),(float3x3)xV),xP);
	Out.nor=ICOSPHERE[vid];
	Out.pos2DPrev = mul(mul(float4(ICOSPHERE[vid], 1), (float3x3)xPrevView), xPrevProjection);
	return Out;
}

//VSOut main(uint vID : SV_VERTEXID)
//{
//	VSOut Out = (VSOut)0;
//	Out.pos=float4((vID%2-0.5)*2,((vID%4)/2-0.5)*2,1,1);
//	Out.tex=normalize( mul( mul(Out.pos,xP).xyz,(float3x3)xV ) );
//	return Out;
//}

/*struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	float3 Position3D		: TEXCOORD1;
	float3 CamPos			: TEXCOORD2;
	float  AtmospherePos	: TEXCOORD3;
};



VertextoPixel main(Input input)
{
	VertextoPixel Out = (VertextoPixel)0;
	
		float4x4 WORLD = float4x4(
				float4(input.wi0.x,input.wi1.x,input.wi2.x,0)
				,float4(input.wi0.y,input.wi1.y,input.wi2.y,0)
				,float4(input.wi0.z,input.wi1.z,input.wi2.z,0)
				,float4(input.wi0.w,input.wi1.w,input.wi2.w,1)
			);

	float4 pos = mul(input.pos,WORLD);

	Out.pos=mul( pos,xViewProjection );
	Out.tex=input.tex;

	Out.Position3D = mul( input.pos, WORLD);
	Out.CamPos     = xCamPos;
	Out.AtmospherePos = input.pos.y/0.3f;

	return Out;
}*/