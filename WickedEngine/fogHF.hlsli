
//static const float fogFar=1,fogNear=0.9989,heightThreshold=0.05f;

inline float getFog(float3 depth, float3 pos3D, float3 fogSEH){
	return pow( saturate((depth - fogSEH.x) / (fogSEH.y - fogSEH.x)),clamp(pos3D.y*fogSEH.z,1,10000) );
}
inline float getFog(float3 depth, float3 fogSEH){
	return saturate((depth - fogSEH.x) / (fogSEH.y - fogSEH.x));
}
inline float3 applyFog(float3 color, float3 horizon, float fog){
	return lerp(color,horizon,saturate(fog));
}
