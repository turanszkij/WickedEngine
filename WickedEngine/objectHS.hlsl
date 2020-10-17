#include "globals.hlsli"

#define g_f4Eye				g_xCamera_CamPos

struct HullInputType
{
	float4 pos								: POSITION;
	float4 color							: COLOR;
	float4 uvsets							: UVSETS;
	float4 atlas							: ATLAS;
	float4 nor								: NORMAL;
	float4 tan								: TANGENT;
	float4 posPrev							: POSITIONPREV;
};

//The ConstantOutputType structure is what will be the output from the patch constant function.
struct ConstantOutputType
{
	float fTessFactor[3] : SV_TessFactor;
    float fInsideTessFactor : SV_InsideTessFactor;

	float4 pos0 : POSITION0;
	float4 pos1 : POSITION1;
	float4 pos2 : POSITION2;

	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;

	float4 uvsets0 : UVSETS0;
	float4 uvsets1 : UVSETS1;
	float4 uvsets2 : UVSETS2;

	float4 atlas0 : ATLAS0;
	float4 atlas1 : ATLAS1;
	float4 atlas2 : ATLAS2;

	float4 nor0 : NORMAL0;
	float4 nor1 : NORMAL1;
	float4 nor2 : NORMAL2;

	float4 tan0 : TANGENT0;
	float4 tan1 : TANGENT1;
	float4 tan2 : TANGENT2;

	float4 posPrev0 : POSITIONPREV0;
	float4 posPrev1 : POSITIONPREV1;
	float4 posPrev2 : POSITIONPREV2;
};

//The HullOutputType structure is what will be the output from the hull shader.
struct HullOutputType
{
	float4 pos								: POSITION;
	float4 color							: COLOR;
	float4 uvsets							: UVSETS;
	float4 atlas							: ATLAS;
	float4 nor								: NORMAL;
	float4 tan								: TANGENT;
	float4 posPrev							: POSITIONPREV;
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
	ConstantOutputType Out;
    
	const float MODIFIER = 0.6f;

    float fDistance;
    float3 f3MidPoint;
    // Edge 0
	f3MidPoint = ( I[2].pos.xyz + I[0].pos.xyz) / 2.0f;
    fDistance = distance( f3MidPoint, g_f4Eye.xyz )*MODIFIER - g_f4TessFactors.z;
	Out.fTessFactor[0] = g_f4TessFactors.x * ( 1.0f - clamp( ( fDistance / g_f4TessFactors.w ), 0.0f, 1.0f - ( 1.0f / g_f4TessFactors.x ) ) );
    // Edge 1
	f3MidPoint = ( I[0].pos.xyz + I[1].pos.xyz) / 2.0f;
    fDistance = distance( f3MidPoint, g_f4Eye.xyz )*MODIFIER - g_f4TessFactors.z;
	Out.fTessFactor[1] = g_f4TessFactors.x * ( 1.0f - clamp( ( fDistance / g_f4TessFactors.w ), 0.0f, 1.0f - ( 1.0f / g_f4TessFactors.x ) ) );
    // Edge 2
	f3MidPoint = ( I[1].pos.xyz + I[2].pos.xyz) / 2.0f;
    fDistance = distance( f3MidPoint, g_f4Eye.xyz )*MODIFIER - g_f4TessFactors.z;
	Out.fTessFactor[2] = g_f4TessFactors.x * ( 1.0f - clamp( ( fDistance / g_f4TessFactors.w ), 0.0f, 1.0f - ( 1.0f / g_f4TessFactors.x ) ) );
    // Inside
	Out.fInsideTessFactor = ( Out.fTessFactor[0] + Out.fTessFactor[1] + Out.fTessFactor[2] ) / 3.0f;

	Out.pos0 = I[0].pos;
	Out.pos1 = I[1].pos;
	Out.pos2 = I[2].pos;

	Out.color0 = I[0].color;
	Out.color1 = I[1].color;
	Out.color2 = I[2].color;

	Out.uvsets0 = I[0].uvsets;
	Out.uvsets1 = I[1].uvsets;
	Out.uvsets2 = I[2].uvsets;

	Out.atlas0 = I[0].atlas;
	Out.atlas1 = I[1].atlas;
	Out.atlas2 = I[2].atlas;
    
	Out.nor0 = I[0].nor;
	Out.nor1 = I[1].nor;
	Out.nor2 = I[2].nor;

	Out.tan0 = I[0].tan;
	Out.tan1 = I[1].tan;
	Out.tan2 = I[2].tan;

	Out.posPrev0 = I[0].posPrev;
	Out.posPrev1 = I[1].posPrev;
	Out.posPrev2 = I[2].posPrev;
           
    return Out;
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
    HullOutputType Out;

	Out.pos				= patch[pointId].pos;
	Out.color			= patch[pointId].color;
	Out.uvsets			= patch[pointId].uvsets;
	Out.atlas			= patch[pointId].atlas;
    Out.nor				= patch[pointId].nor;
    Out.tan				= patch[pointId].tan;
	Out.posPrev			= patch[pointId].posPrev;

    return Out;
}