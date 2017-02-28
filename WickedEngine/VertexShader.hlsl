struct VS_OUTPUT
{
	float4 Position   : SV_Position;
	float2 TexCoord   : TEXCOORD0;
};

VS_OUTPUT main( in float4 vPosition : POSITION, in float2 vTC0 : TEXCOORD0 )
{
	VS_OUTPUT Output;

	Output.Position = float4( vPosition.x, vPosition.y, 0.01f, 1.0f );
	Output.TexCoord = vTC0;

	return Output;
}