#ifndef _FULLSCREEN_TRIANGLE_HF_
#define _FULLSCREEN_TRIANGLE_HF_

inline void FullScreenTriangle(in uint vertexID, out float4 pos)
{
	pos.x = (float)(vertexID / 2) * 4.0f - 1.0f;
	pos.y = (float)(vertexID % 2) * 4.0f - 1.0f;
	pos.z = 0.0f;
	pos.w = 1.0f;
}
inline void FullScreenTriangle(in uint vertexID, out float4 pos, out float2 tex)
{
	FullScreenTriangle(vertexID, pos);

	tex.x = (float)(vertexID / 2)*2.0f;
	tex.y = 1.0f - (float)(vertexID % 2)*2.0f;
}

#endif // _FULLSCREEN_TRIANGLE_HF_