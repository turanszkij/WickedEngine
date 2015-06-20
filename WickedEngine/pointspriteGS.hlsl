cbuffer constatBuffer:register(c0){
	float4x4	xView;
	float4x4	xProjection;
	float4		xCamPos;
	float4		xAdd;
};

struct GS_INPUT{
	float4	pos			: SV_POSITION;
	float4  inSizOpMir	: TEXCOORD0;
	//float3	color		: COLOR;
	float	rot			: ROTATION;
};

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	float4 opaAddDarkSiz	: TEXCOORD1;
	//float3 col				: TEXCOORD2;
	float4 pp				: TEXCOORD2;
};

[maxvertexcount(4)]
void main(point GS_INPUT p[1], inout TriangleStream<VertextoPixel> triStream)
{
	VertextoPixel p1 = (VertextoPixel)0;
	float2x2 rot = float2x2(
		cos(p[0].rot),-sin(p[0].rot),
		sin(p[0].rot),cos(p[0].rot)
		);

    float3 normal			= p[0].pos - xCamPos;
	normal					= mul(normal, xView);

	float3 r = float3( mul(float2(0,1),rot),0 );
    float3 rightAxis		= cross(r, normal);
    float3 upAxis			= cross(normal, rightAxis);
    rightAxis				= normalize(rightAxis);
    upAxis					= normalize(upAxis);

    float4 rightVector		= float4(rightAxis.xyz, 1.0f);
    float4 upVector         = float4(upAxis.xyz, 1.0f);
	p[0].pos				= mul(p[0].pos, xView);


	float quadLength = p[0].inSizOpMir.x*0.5f;

	//p1.col = p[0].color;
	p1.opaAddDarkSiz = float4(saturate(p[0].inSizOpMir.y),xAdd.x,xAdd.y,p[0].inSizOpMir.x);

		//rightVector.xy=mul(rightVector.xy,rot);
		//upVector.xy=mul(upVector.xy,rot);

	p1.pos.w=1;
    p1.pos.xyz = p[0].pos+rightVector*(-quadLength)+upVector*(quadLength);
    p1.tex = float2(0.0f, 0.0f);
    p1.pos = mul(p1.pos, xProjection);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
	
	p1.pos.w=1;
    p1.pos.xyz = p[0].pos+rightVector*(-quadLength)+upVector*(-quadLength);
    p1.tex = float2(0.0f, 1.0f);
    p1.pos = mul(p1.pos, xProjection);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
	
	p1.pos.w=1;
    p1.pos.xyz = p[0].pos+rightVector*(quadLength)+upVector*(quadLength);
    p1.tex = float2(1.0f, 0.0f);
    p1.pos = mul(p1.pos, xProjection);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
	
	p1.pos.w=1;
    p1.pos.xyz = p[0].pos+rightVector*(quadLength)+upVector*(-quadLength);
    p1.tex = float2(1.0f, 1.0f);
    p1.pos = mul(p1.pos, xProjection);
	if(p[0].inSizOpMir.z==1) p1.tex.x=1-p1.tex.x;
	if(p[0].inSizOpMir.w==1) p1.tex.y=1-p1.tex.y;
	p1.pp = p1.pos;
    triStream.Append(p1);
}