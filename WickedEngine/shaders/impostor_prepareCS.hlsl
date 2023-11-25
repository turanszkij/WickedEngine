#include "globals.hlsli"

static const uint THREADCOUNT = 64;

static const float3 BILLBOARD[] =
{
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(1, 1, 0),
};

RWBuffer<uint> output_indices : register(u0);
RWByteAddressBuffer output_vertices_pos : register(u1);
RWBuffer<float4> output_vertices_nor : register(u2);
RWByteAddressBuffer output_impostor_data : register(u3);
RWStructuredBuffer<IndirectDrawArgsIndexedInstanced> output_indirect : register(u4);

struct ObjectCount
{
	uint count;
};
PUSHCONSTANT(push, ObjectCount);

[numthreads(THREADCOUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	[branch]
	if (DTid.x >= push.count)
		return;

	const uint instanceIndex = DTid.x;
	ShaderMeshInstance instance = load_instance(instanceIndex);
	ShaderGeometry geometry = load_geometry(instance.geometryOffset);

	float dist = distance(GetCamera().position, instance.center);
	const bool distance_culled = dist < (instance.fadeDistance - instance.radius);

	// Frustum culling:
	ShaderSphere sphere;
	sphere.center = instance.center;
	sphere.radius = instance.radius;

	const bool visible = geometry.impostorSliceOffset >= 0 && !distance_culled && GetCamera().frustum.intersects(sphere);

	// Optimization: reduce to 1 atomic operation per wave
	const uint waveAppendCount = WaveActiveCountBits(visible);
	uint waveOffset;
	if (WaveIsFirstLane() && waveAppendCount > 0)
	{
		InterlockedAdd(output_indirect[0].IndexCountPerInstance, waveAppendCount * 6u, waveOffset);
	}
	waveOffset = WaveReadLaneFirst(waveOffset);

	[branch]
	if (visible)
	{
		const uint indexOffset = waveOffset + WavePrefixSum(6u);
		const uint impostorOffset = indexOffset / 6u;
		const uint vertexOffset = impostorOffset * 4u;

		// Write out indices:
		output_indices[indexOffset + 0] = vertexOffset + 0; // provoking vertex prim0
		output_indices[indexOffset + 1] = vertexOffset + 1;
		output_indices[indexOffset + 2] = vertexOffset + 2;
		output_indices[indexOffset + 3] = vertexOffset + 3; // provoking vertex prim1
		output_indices[indexOffset + 4] = vertexOffset + 2;
		output_indices[indexOffset + 5] = vertexOffset + 1;

		// We rotate the billboard to face camera, but unlike emitted particles, 
		//	they don't rotate according to camera rotation, but the camera position relative
		//	to the impostor (at least for now)
		float3 origin = instance.center;
		float3 up = float3(0, 1, 0);
		float3 face = GetCamera().position - origin;
		face.y = 0; // only rotate around Y axis!
		face = normalize(face);
		float3 right = normalize(cross(face, up));

		// Decide which slice to show according to billboard facing direction:
		uint slice = uint(geometry.impostorSliceOffset);
		float angle = acos(dot(face.xz, float2(0, 1))) / PI;
		if (cross(face, float3(0, 0, 1)).y < 0)
		{
			angle = 2 - angle;
		}
		angle *= 0.5f;
		angle = saturate(angle - 0.0001);
		slice += uint(angle * impostorCaptureAngles) * 3;

		const float dither = max(0, instance.fadeDistance - dist) / instance.radius;

		// Write out per impostor data:
		uint2 data = 0;
		data.x |= slice & 0xFFFFFF;
		data.x |= (uint(dither * 255) & 0xFF) << 24u;
		data.y = instance.color;
		output_impostor_data.Store2(impostorOffset * sizeof(uint2), data);

		// Write out vertices:
		for (uint vertexID = 0; vertexID < 4; ++vertexID)
		{
			float3 pos = BILLBOARD[vertexID];
			pos = mul(pos, float3x3(right, up, face));
			pos *= instance.radius;
			pos += instance.center;
#ifdef __PSSL__
			output_vertices_pos.TypedStore<float3>((vertexOffset + vertexID) * sizeof(float3), pos);
#else
			output_vertices_pos.Store<float3>((vertexOffset + vertexID) * sizeof(float3), pos);
#endif // __PSSL__
			output_vertices_nor[vertexOffset + vertexID] = float4(face, 0);
		}
	}
}
