#include "objectHF.hlsli"

struct ConstantOutput
{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;

	float3 b210 : CONTROLPOINT0;
	float3 b120 : CONTROLPOINT1;
	float3 b021 : CONTROLPOINT2;
	float3 b012 : CONTROLPOINT3;
	float3 b102 : CONTROLPOINT4;
	float3 b201 : CONTROLPOINT5;
	float3 b111 : CENTER;
};

#ifdef OBJECTSHADER_COMPILE_HS

ConstantOutput PatchConstantFunction(InputPatch<PixelInput, 3> patch)
{
	ConstantOutput output;

	// tessellation strength based on distance and orientation:
	[unroll]
	for (uint ie = 0; ie < 3; ++ie)
	{
#if 0
		output.edges[ie] = GetMesh().tessellation_factor;
#else
		float3 edge = patch[(ie + 1) % 3].pos.xyz - patch[ie].pos.xyz;
		float3 vec = (patch[(ie + 1) % 3].pos.xyz + patch[ie].pos.xyz) / 2 - GetCamera().position.xyz;
		float len = sqrt(dot(edge, edge) / dot(vec, vec));
		output.edges[(ie + 1) % 3] = max(1, len * GetMesh().tessellation_factor);
#endif
	}


#if 1
	// frustum culling:
	float rad = GetMaterial().displacementMapping;
	int culled[4];
	[unroll]
	for (int ip = 0; ip < 4; ++ip)
	{
		culled[ip] = 1;
		culled[ip] &= dot(GetCamera().frustum.planes[ip + 2], patch[0].pos) < -rad;
		culled[ip] &= dot(GetCamera().frustum.planes[ip + 2], patch[1].pos) < -rad;
		culled[ip] &= dot(GetCamera().frustum.planes[ip + 2], patch[2].pos) < -rad;
	}
	if (culled[0] || culled[1] || culled[2] || culled[3])
	{
		output.edges[0] = 0;
	}
#endif


	// compute the cubic geometry control points
	const float3 nor[] = {
		normalize(patch[0].nor),
		normalize(patch[1].nor),
		normalize(patch[2].nor),
	};
	output.b210 = ((2 * patch[0].pos.xyz) + patch[1].pos.xyz - (dot((patch[1].pos.xyz - patch[0].pos.xyz), nor[0]) * nor[0])) / 3.0;
	output.b120 = ((2 * patch[1].pos.xyz) + patch[0].pos.xyz - (dot((patch[0].pos.xyz - patch[1].pos.xyz), nor[1]) * nor[1])) / 3.0;
	output.b021 = ((2 * patch[1].pos.xyz) + patch[2].pos.xyz - (dot((patch[2].pos.xyz - patch[1].pos.xyz), nor[1]) * nor[1])) / 3.0;
	output.b012 = ((2 * patch[2].pos.xyz) + patch[1].pos.xyz - (dot((patch[1].pos.xyz - patch[2].pos.xyz), nor[2]) * nor[2])) / 3.0;
	output.b102 = ((2 * patch[2].pos.xyz) + patch[0].pos.xyz - (dot((patch[0].pos.xyz - patch[2].pos.xyz), nor[2]) * nor[2])) / 3.0;
	output.b201 = ((2 * patch[0].pos.xyz) + patch[2].pos.xyz - (dot((patch[2].pos.xyz - patch[0].pos.xyz), nor[0]) * nor[0])) / 3.0;
	float3 E = (output.b210 + output.b120 + output.b021 + output.b012 + output.b102 + output.b201) / 6.0;
	float3 V = (patch[0].pos.xyz + patch[1].pos.xyz + patch[2].pos.xyz) / 3.0;
	output.b111 = E + ((E - V) / 2.0);


	output.inside = (output.edges[0] + output.edges[1] + output.edges[2]) / 3.0;

	return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
//[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
PixelInput main(InputPatch<PixelInput, 3> patch, uint pointID : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
	return patch[pointID];
}
#endif // OBJECTSHADER_COMPILE_HS



#ifdef OBJECTSHADER_COMPILE_DS

[domain("tri")]
PixelInput main(ConstantOutput input, float3 uvw : SV_DomainLocation, const OutputPatch<PixelInput, 3> patch)
{
	PixelInput output = patch[0];

	const float u = uvw.x;
	const float v = uvw.y;
	const float w = uvw.z;
	const float uu = u * u;
	const float vv = v * v;
	const float ww = w * w;
	const float uu3 = uu * 3;
	const float vv3 = vv * 3;
	const float ww3 = ww * 3;

	// compute position from cubic control points and barycentric coords:
	output.pos.xyz = patch[0].pos.xyz * ww * w + patch[1].pos.xyz * uu * u + patch[2].pos.xyz * vv * v +
		input.b210 * ww3 * u + input.b120 * w * uu3 + input.b201 * ww3 * v + input.b021 * uu3 * v +
		input.b102 * w * vv3 + input.b012 * u * vv3 + input.b111 * 6.0 * w * u * v;

#ifdef OBJECTSHADER_USE_POSITIONPREV
	output.pre.xyz = patch[0].pre.xyz * ww * w + patch[1].pre.xyz * uu * u + patch[2].pre.xyz * vv * v +
		input.b210 * ww3 * u + input.b120 * w * uu3 + input.b201 * ww3 * v + input.b021 * uu3 * v +
		input.b102 * w * vv3 + input.b012 * u * vv3 + input.b111 * 6.0 * w * u * v;
#endif // OBJECTSHADER_USE_POSITIONPREV

#ifdef OBJECTSHADER_USE_UVSETS
	output.uvsets = w * patch[0].uvsets + u * patch[1].uvsets + v * patch[2].uvsets;
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_COLOR
	output.color = min16float4(w * patch[0].color + u * patch[1].color + v * patch[2].color);
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_ATLAS
	output.atl = min16float2(w * patch[0].atl + u * patch[1].atl + v * patch[2].atl);
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_NORMAL
	output.nor = min16float3(normalize(w * patch[0].nor + u * patch[1].nor + v * patch[2].nor));
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_TANGENT
	output.tan = min16float4(normalize(w * patch[0].tan + u * patch[1].tan + v * patch[2].tan));
#endif // OBJECTSHADER_USE_TANGENT

#ifdef OBJECTSHADER_USE_POSITION3D
	output.pos3D = output.pos.xyz;
#endif // OBJECTSHADER_USE_POSITION3D


#ifdef OBJECTSHADER_USE_NORMAL
#ifdef OBJECTSHADER_USE_UVSETS
	// displacement mapping:
	[branch]
	if (GetMaterial().displacementMapping > 0 && GetMaterial().textures[DISPLACEMENTMAP].IsValid())
	{
		float displacement = GetMaterial().textures[DISPLACEMENTMAP].SampleLevel(sampler_objectshader, output.uvsets, 0).r;
		displacement *= GetMaterial().displacementMapping;
		output.pos.xyz += normalize(float3(output.nor)) * displacement;
	}
#endif // OBJECTSHADER_USE_UVSETS
#endif // OBJECTSHADER_USE_NORMAL


#ifdef OBJECTSHADER_USE_CLIPPLANE
	output.clip = dot(output.pos, GetCamera().clip_plane);
#endif // OBJECTSHADER_USE_CLIPPLANE

#ifndef OBJECTSHADER_USE_NOCAMERA
	output.pos = mul(GetCamera().view_projection, output.pos);
#endif // OBJECTSHADER_USE_NOCAMERA

#ifdef OBJECTSHADER_USE_POSITIONPREV
#ifndef OBJECTSHADER_USE_NOCAMERA
	output.pre = mul(GetCamera().previous_view_projection, output.pre);
#endif // OBJECTSHADER_USE_NOCAMERA
#endif // OBJECTSHADER_USE_POSITIONPREV

	return output;
}

#endif // OBJECTSHADER_COMPILE_DS
