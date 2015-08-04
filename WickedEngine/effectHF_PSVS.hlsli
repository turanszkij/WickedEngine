struct PixelInputType
{
	float4 pos						: SV_POSITION;
	float clip						: SV_ClipDistance0;
	float2 tex						: TEXCOORD0;
	float3 nor						: NORMAL;
	float4 pos2D					: SCREENPOSITION;
	float3 pos3D					: WORLDPOSITION;
	float3 cam						: CAMERAPOS;
	float4 ReflectionMapSamplingPos : TEXCOORD1;
	float3 vel						: TEXCOORD2;
	float  ao						: AMBIENT_OCCLUSION;
	float  dither					: DITHER;
	float3 instanceColor			: INSTANCECOLOR;
};

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