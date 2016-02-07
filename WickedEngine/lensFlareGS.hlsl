#include "globals.hlsli"

CBUFFER(LensFlareCB, CBSLOT_OTHER_LENSFLARE)
{
	float4		xSunPos;
	float4		xScreen;
};

struct InVert{
	float4 pos : SV_POSITION;
	nointerpolation uint vid : VERTEXID;
};

struct VertextoPixel{
	float4 pos				: SV_POSITION;
	float3 texPos			: TEXCOORD0;
	nointerpolation uint   sel				: TEXCOORD1;
	nointerpolation float4 opa				: TEXCOORD2;
};

//Texture2D<float> depth:register(t0);
//Texture2D flare[7]:register(t1);

inline void append(inout TriangleStream<VertextoPixel> triStream, VertextoPixel p1, uint selector, float2 posMod, float2 size){
	float2 pos = (xSunPos.xy-0.5)*float2(2,-2);
	float2 moddedPos = pos*posMod;
	float dis = distance(pos,moddedPos);

	p1.pos.xy=moddedPos+float2(-size.x,-size.y);
	p1.texPos.z=dis;
	p1.sel=selector;
	p1.texPos.xy=float2(0,0);
	triStream.Append(p1);
	
	p1.pos.xy=moddedPos+float2(-size.x,size.y);
	p1.texPos.xy=float2(0,1);
	triStream.Append(p1);

	p1.pos.xy=moddedPos+float2(size.x,-size.y);
	p1.texPos.xy=float2(1,0);
	triStream.Append(p1);
	
	p1.pos.xy=moddedPos+float2(size.x,size.y);
	p1.texPos.xy=float2(1,1);
	triStream.Append(p1);

	//triStream.RestartStrip();
}

#define VERTEXCOUNT 4

static const float mods[] = {1,0.55,0.4,0.1,-0.1,-0.3,-0.5};
//static const float size[] = {0.12,0.1,0.25,0.05,0.15,0.1,0.3};

[maxvertexcount(VERTEXCOUNT)]
void main(point InVert p[1], inout TriangleStream<VertextoPixel> triStream)
{
	VertextoPixel p1 = (VertextoPixel)0;

	float2 flareSize=float2(256,256);
	[branch]
	switch(p[0].vid){
	case 0:
		texture_0.GetDimensions(flareSize.x,flareSize.y);
		break;
	case 1:
		texture_1.GetDimensions(flareSize.x,flareSize.y);
		break;
	case 2:
		texture_2.GetDimensions(flareSize.x,flareSize.y);
		break;
	case 3:
		texture_3.GetDimensions(flareSize.x,flareSize.y);
		break;
	case 4:
		texture_4.GetDimensions(flareSize.x,flareSize.y);
		break;
	case 5:
		texture_5.GetDimensions(flareSize.x,flareSize.y);
		break;
	case 6:
		texture_6.GetDimensions(flareSize.x,flareSize.y);
		break;
	default:break;
	};
	
	
	float2 depthMapSize;
	texture_depth.GetDimensions(depthMapSize.x,depthMapSize.y);
	flareSize/=depthMapSize;

	const float range = 10.5f;
	float samples = 0.0f;
	float accdepth = 0.0f;
	for (float y = -range; y <= range; y += 1.0f)
		for (float x = -range; x <= range; x += 1.0f){
			samples+=1.0f;
			accdepth += (texture_depth.SampleCmpLevelZero(sampler_cmp_depth,xSunPos.xy+float2(x,y)/(depthMapSize*xSunPos.z),xSunPos.z).r ) ;
		}
	accdepth/=samples;

	p1.pos=float4(0,0,0,1);
	p1.opa=float4(accdepth,0,0,0);


	[branch]if( accdepth>0 )
		append(triStream,p1,p[0].vid,mods[p[0].vid],flareSize);
}