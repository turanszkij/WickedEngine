#include "globals.hlsli"

TEXTURE2D(texture_0, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_1, float4, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_2, float4, TEXSLOT_ONDEMAND2);
TEXTURE2D(texture_3, float4, TEXSLOT_ONDEMAND3);
TEXTURE2D(texture_4, float4, TEXSLOT_ONDEMAND4);
TEXTURE2D(texture_5, float4, TEXSLOT_ONDEMAND5);
TEXTURE2D(texture_6, float4, TEXSLOT_ONDEMAND6);

struct InVert
{
	float4 pos					: SV_POSITION;
	nointerpolation uint vid	: VERTEXID;
};

struct VertextoPixel{
	float4 pos					: SV_POSITION;
	float3 texPos				: TEXCOORD0; // texture coordinates (xy) + offset(z)
	nointerpolation uint   sel	: TEXCOORD1; // texture selector
	nointerpolation float4 opa	: TEXCOORD2; // opacity + padding
};

// Append a screen space quad to the output stream:
inline void append(inout TriangleStream<VertextoPixel> triStream, VertextoPixel p1, uint selector, float2 posMod, float2 size)
{
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
}

// pre-baked offsets
// These values work well for me, but should be tweakable
static const float mods[] = { 1,0.55,0.4,0.1,-0.1,-0.3,-0.5 };

[maxvertexcount(4)]
void main(point InVert p[1], inout TriangleStream<VertextoPixel> triStream)
{
	VertextoPixel p1 = (VertextoPixel)0;

	// Determine flare size from texture dimensions
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

	flareSize /= GetScreenResolution();

	float referenceDepth = saturate(1 - xSunPos.z);

	// determine the flare opacity:
	// These values work well for me, but should be tweakable
	const float2 step = 1.0f / (GetInternalResolution()*xSunPos.z);
	const float2 range = 10.5f * step;
	float samples = 0.0f;
	float accdepth = 0.0f;
	for (float y = -range.y; y <= range.y; y += step.y)
	{
		for (float x = -range.x; x <= range.x; x += step.x)
		{
			samples += 1.0f;
			accdepth += texture_depth.SampleLevel(sampler_point_clamp, xSunPos.xy + float2(x, y), 0).r < referenceDepth ? 1 : 0;
		}
	}
	accdepth /= samples;

	p1.pos = float4(0, 0, 0, 1);
	p1.opa = float4(accdepth, 0, 0, 0);

	// Make a new flare if it is at least partially visible:
	[branch]if (accdepth > 0)
	{
		append(triStream, p1, p[0].vid, mods[p[0].vid], flareSize);
	}
}
