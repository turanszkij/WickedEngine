struct PixelInputType
{
	float4 pos						: SV_POSITION;
	float clip						: SV_ClipDistance0;
	float2 tex						: TEXCOORD0;
	float3 nor						: NORMAL;
	float4 pos2D					: SCREENPOSITION;
	float3 pos3D					: WORLDPOSITION;
	float3 cam						: CAMERAPOS;
	//float3 tan						: TANGENT;
	//float3 bin						: BINORMAL;
	//float3 EyeVec					: TEXCOORD1;
	float4 ReflectionMapSamplingPos : TEXCOORD1;
	//float4 ShadowMapSamplingPos[3]	: TEXCOORD3;
	//float4 pos2D					: TEXCOORD6;
	//float3 Position3D				: TEXCOORD7;
	float3 vel						: TEXCOORD2;
	//uint   mat						: MATERIALINDEX;
};

//cbuffer matBuffer:register(b2){
//	float4 diffuseColor;
//	float4 hasRefNorTexSpe;
//	float4 specular;
//	float4 refractionIndexMovingTexEnv;
//	uint shadeless;
//	uint specular_power;
//	uint toonshaded;
//	uint matIndex;
//	float emit;
//	float padding[3];
//};

cbuffer matBuffer:register(b2){
	float4 diffuseColor;
	uint hasRef,hasNor,hasTex,hasSpe;
	float4 specular;
	float refractionIndex;
	float2 movingTex;
	float metallic;
	uint shadeless;
	uint specular_power;
	uint toonshaded;
	uint matIndex;
	float emit;
	float padding[3];
};