#include "globals.hlsli"

#define g_f4Eye				g_xCamera_CamPos

struct HullInputType
{
	float3 f3Position						: POSITION;
	float3 f3PositionPrev					: POSITIONPREV;
	float4 tex								: TEXCOORD0;
	float4 f4Normal							: NORMAL;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
};

//The ConstantOutputType structure is what will be the output from the patch constant function.
struct ConstantOutputType
{
	float fTessFactor[3] : SV_TessFactor;
    float fInsideTessFactor : SV_InsideTessFactor;

	float3 f3B0 : POSITION0;
	float3 f3B1 : POSITION1;
	float3 f3B2 : POSITION2;

	float3 f3PrevB0 : POSITIONPREV0;
	float3 f3PrevB1 : POSITIONPREV1;
	float3 f3PrevB2 : POSITIONPREV2;

	float4 f4N0 : NORMAL0;
	float4 f4N1 : NORMAL1;
	float4 f4N2 : NORMAL2;
};

//The HullOutputType structure is what will be the output from the hull shader.
struct HullOutputType
{
	float3 pos								: POSITION;
	float3 posPrev							: POSITIONPREV;
	float4 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
};


//--------------------------------------------------------------------------------------
// Returns the dot product between the viewing vector and the patch edge
//--------------------------------------------------------------------------------------
float GetEdgeDotProduct ( 
                        float3 f3EdgeNormal0,   // Normalized normal of the first control point of the given patch edge 
                        float3 f3EdgeNormal1,   // Normalized normal of the second control point of the given patch edge 
                        float3 f3ViewVector     // Normalized viewing vector
                        )
{
    float3 f3EdgeNormal = normalize( ( f3EdgeNormal0 + f3EdgeNormal1 ) * 0.5f );
    
    float fEdgeDotProduct = dot( f3EdgeNormal, f3ViewVector );

    return fEdgeDotProduct;
}
//--------------------------------------------------------------------------------------
// Returns the orientation adaptive tessellation factor (0.0f -> 1.0f)
//--------------------------------------------------------------------------------------
float GetOrientationAdaptiveScaleFactor ( 
                                        float fEdgeDotProduct,      // Dot product of edge normal with view vector
                                        float fSilhouetteEpsilon    // Epsilon to determine the range of values considered to be silhoutte
                                        )
{
    float fScale = 1.0f - abs( fEdgeDotProduct );
        
    fScale = saturate( ( fScale - fSilhouetteEpsilon ) / ( 1.0f - fSilhouetteEpsilon ) );

    return fScale;
}

////////////////////////////////////////////////////////////////////////////////
// Patch Constant Function
////////////////////////////////////////////////////////////////////////////////
ConstantOutputType PatchConstantFunction(InputPatch<HullInputType, 3> I)
{
	ConstantOutputType O = (ConstantOutputType)0;
    
	const float MODIFIER = 0.6f;

    float fDistance;
    float3 f3MidPoint;
    // Edge 0
    f3MidPoint = ( I[2].f3Position + I[0].f3Position ) / 2.0f;
    fDistance = distance( f3MidPoint, g_f4Eye.xyz )*MODIFIER - g_f4TessFactors.z;
    O.fTessFactor[0] = g_f4TessFactors.x * ( 1.0f - clamp( ( fDistance / g_f4TessFactors.w ), 0.0f, 1.0f - ( 1.0f / g_f4TessFactors.x ) ) );
    // Edge 1
    f3MidPoint = ( I[0].f3Position + I[1].f3Position ) / 2.0f;
    fDistance = distance( f3MidPoint, g_f4Eye.xyz )*MODIFIER - g_f4TessFactors.z;
    O.fTessFactor[1] = g_f4TessFactors.x * ( 1.0f - clamp( ( fDistance / g_f4TessFactors.w ), 0.0f, 1.0f - ( 1.0f / g_f4TessFactors.x ) ) );
    // Edge 2
    f3MidPoint = ( I[1].f3Position + I[2].f3Position ) / 2.0f;
    fDistance = distance( f3MidPoint, g_f4Eye.xyz )*MODIFIER - g_f4TessFactors.z;
    O.fTessFactor[2] = g_f4TessFactors.x * ( 1.0f - clamp( ( fDistance / g_f4TessFactors.w ), 0.0f, 1.0f - ( 1.0f / g_f4TessFactors.x ) ) );
    // Inside
    O.fInsideTessFactor = ( O.fTessFactor[0] + O.fTessFactor[1] + O.fTessFactor[2] ) / 3.0f;
    
    
    O.f3B0 = I[0].f3Position;
    O.f3B1 = I[1].f3Position;
    O.f3B2 = I[2].f3Position;

	O.f3PrevB0 = I[0].f3PositionPrev;
	O.f3PrevB1 = I[1].f3PositionPrev;
	O.f3PrevB2 = I[2].f3PositionPrev;
    
    O.f4N0 = I[0].f4Normal;
    O.f4N1 = I[1].f4Normal;
    O.f4N2 = I[2].f4Normal;
           
    return O;
}

////////////////////////////////////////////////////////////////////////////////
// Hull Shader
////////////////////////////////////////////////////////////////////////////////
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
HullOutputType main(InputPatch<HullInputType, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    HullOutputType Out = (HullOutputType)0;

	Out.pos				= patch[pointId].f3Position.xyz;
	Out.posPrev			= patch[pointId].f3PositionPrev.xyz;
	Out.tex				= patch[pointId].tex;
    Out.nor				= patch[pointId].f4Normal;
	Out.instanceColor	= patch[pointId].instanceColor.rgb;
	Out.dither			= patch[pointId].dither;

    return Out;
}