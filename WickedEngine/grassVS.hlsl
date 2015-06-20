struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 nor : NORMAL;
	float4 tan : TANGENT;
};

VS_OUT main( float4 pos : POSITION, float4 nor : NORMAL, float4 tan : TANGENT)
{
	VS_OUT Out = (VS_OUT)0;

	Out.pos=pos;
	Out.nor=nor;
	Out.tan=tan;

	return Out;
}