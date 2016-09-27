#ifndef _CULLING_SHADER_HF_
#define _CULLING_SHADER_HF_

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
	float4 view = mul(clip, g_xCamera_InvP);
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

#endif // _CULLING_SHADER_HF_
