#ifndef _CULLING_SHADER_HF_
#define _CULLING_SHADER_HF_
#include "lightCullingCSInterop.h"

CBUFFER(DispatchParams, CBSLOT_RENDERER_DISPATCHPARAMS)
{
	// Number of groups dispatched. (This parameter is not available as an HLSL system value!)
	uint3	xDispatchParams_numThreadGroups;
	uint	xDispatchParams_value0; // extra value / padding

					  // Total number of threads dispatched. (Also not available as an HLSL system value!)
					  // Note: This value may be less than the actual number of threads executed 
					  // if the screen size is not evenly divisible by the block size.
	uint3	xDispatchParams_numThreads;
	uint	xDispatchParams_value1; // extra value / padding
}

struct Plane
{
	float3	N;		// Plane normal.
	float	d;		// Distance to origin.
};
// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes a right-handed (counter-clockwise winding order) 
// coordinate system to determine the direction of the plane normal.
Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
	Plane plane;

	float3 v0 = p1 - p0;
	float3 v2 = p2 - p0;

	plane.N = normalize(cross(v0, v2));

	// Compute the distance to the origin using p0.
	plane.d = dot(plane.N, p0);

	return plane;
}
// Four planes of a view frustum (in view space).
// The planes are:
//  * Left,
//  * Right,
//  * Top,
//  * Bottom.
// The back and/or front planes can be computed from depth values in the 
// light culling compute shader.
struct Frustum
{
	Plane planes[4];	// left, right, top, bottom frustum planes.
};
// Convert clip space coordinates to view space
float4 ClipToView(float4 clip)
{
	// View space position.
	float4 view = mul(clip, g_xFrame_MainCamera_InvP);
	// Perspective projection.
	view = view / view.w;

	return view;
}
// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen)
{
	// Convert to normalized texture coordinates
	float2 texCoord = screen.xy / GetScreenResolution();

	// Convert to clip space
	float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

	return ClipToView(clip);
}
struct Sphere
{
	float3 c;	 // Center point.
	float r;	// Radius.
};
// Check to see if a sphere is fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool SphereInsidePlane(Sphere sphere, Plane plane)
{
	return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}
// Check to see of a light is partially contained within the frustum.
bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
	bool result = true;

	if (sphere.c.z + sphere.r < zNear || sphere.c.z - sphere.r > zFar)
	{
		result = false;
	}

	// Then check frustum planes
	for (int i = 0; i < 4 && result; i++)
	{
		if (SphereInsidePlane(sphere, frustum.planes[i]))
		{
			result = false;
		}
	}

	return result;
}
// Check to see if a point is fully behind (inside the negative halfspace of) a plane.
bool PointInsidePlane(float3 p, Plane plane)
{
	return dot(plane.N, p) - plane.d < 0;
}

struct Cone
{
	float3 T;   // Cone tip.
	float  h;   // Height of the cone.
	float3 d;   // Direction of the cone.
	float  r;   // bottom radius of the cone.
};
// Check to see if a cone if fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool ConeInsidePlane(Cone cone, Plane plane)
{
	// Compute the farthest point on the end of the cone to the positive space of the plane.
	float3 m = cross(cross(plane.N, cone.d), cone.d);
	float3 Q = cone.T + cone.d * cone.h - m * cone.r;

	// The cone is in the negative halfspace of the plane if both
	// the tip of the cone and the farthest point on the end of the cone to the 
	// positive halfspace of the plane are both inside the negative halfspace 
	// of the plane.
	return PointInsidePlane(cone.T, plane) && PointInsidePlane(Q, plane);
}
bool ConeInsideFrustum(Cone cone, Frustum frustum, float zNear, float zFar)
{
	bool result = true;

	Plane nearPlane = { float3(0, 0, 1), zNear };
	Plane farPlane = { float3(0, 0, -1), -zFar };

	// First check the near and far clipping planes.
	if (ConeInsidePlane(cone, nearPlane) || ConeInsidePlane(cone, farPlane))
	{
		result = false;
	}

	// Then check frustum planes
	for (int i = 0; i < 4 && result; i++)
	{
		if (ConeInsidePlane(cone, frustum.planes[i]))
		{
			result = false;
		}
	}

	return result;
}



static const float4 frustumCorners[4] = {
	float4(-1, -1, 1, 1),
	float4(1, -1, 1, 1),
	float4(-1, 1, 1, 1),
	float4(1, 1, 1, 1)
};
static const Frustum frustum = {
	ComputePlane(float3(0,0,0), frustumCorners[2].xyz, frustumCorners[0].xyz),
	ComputePlane(float3(0,0,0), frustumCorners[1].xyz, frustumCorners[3].xyz),
	ComputePlane(float3(0,0,0), frustumCorners[0].xyz, frustumCorners[1].xyz),
	ComputePlane(float3(0,0,0), frustumCorners[3].xyz, frustumCorners[2].xyz)
};

#endif // _CULLING_SHADER_HF_
