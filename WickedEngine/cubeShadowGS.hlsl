struct GS_CUBEMAP_IN 
{ 
	float4 Pos		: SV_POSITION;    // World position 
    float2 Tex		: TEXCOORD0;         // Texture coord 
}; 
struct PS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;
	float3 pos3D	: POSITION3D;
    float2 Tex		: TEXCOORD0;         // Texture coord 
    uint RTIndex	: SV_RenderTargetArrayIndex; 
};


cbuffer constantBuffer:register(b0){
	float4x4 xViewProjection[6];
}

[maxvertexcount(18)] 
void main( triangle GS_CUBEMAP_IN input[3], inout TriangleStream<PS_CUBEMAP_IN> CubeMapStream ) 
{ 
    for( int f = 0; f < 6; ++f ) 
    { 
        // Compute screen coordinates 
        PS_CUBEMAP_IN output; 
        output.RTIndex = f; 
        for( int v = 0; v < 3; v++ ) 
        { 
            output.Pos = mul( input[v].Pos, xViewProjection[f] );
			output.pos3D = input[v].Pos;
            output.Tex = input[v].Tex;
            CubeMapStream.Append( output ); 
        } 
        CubeMapStream.RestartStrip(); 
    } 
}