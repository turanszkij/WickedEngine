# Primitives

[← Back to Scripting API index](../index.md)

## Ray

```lua
    --- Creates a ray from an origin and a direction, with optional minimum and
    --- maximum distance along the ray.
    ---
    ---@param origin Vector
    ---@param direction Vector
    ---@param Tmin? number
    ---@param tmax? number
    ---
    ---@return Ray
    function Ray(origin, direction, Tmin, tmax) end

    --- A ray defined by an origin Vector and a normalized direction Vector. It
    --- can be used to intersect with other primitives or the scene.
    ---
    ---@class Ray
    ---
    ---@field Origin Vector
    ---@field Direction Vector
    local Ray = {}

    --- Returns whether the ray intersects the given AABB.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function Ray.Intersects(aabb) end

    --- Returns whether the ray intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean
    function Ray.Intersects(sphere) end

    --- Returns whether the ray intersects the given capsule.
    ---
    ---@param capsule Capsule
    ---
    ---@return boolean
    function Ray.Intersects(capsule) end

    --- Returns the point where the ray intersects the given plane.
    ---
    ---@param plane Vector
    ---
    ---@return Vector point
    function Ray.Intersects(plane) end

    --- Returns the origin.
    ---
    ---@return Vector
    function Ray.GetOrigin() end

    --- Returns the direction.
    ---
    ---@return Vector
    function Ray.GetDirection() end

    --- Sets the origin.
    ---
    ---@param vector Vector
    function Ray.SetOrigin(vector) end

    --- Sets the direction.
    ---
    ---@param vector Vector
    function Ray.SetDirection(vector) end

    --- Creates a ray from two points. Point a will be the ray origin, pointing
    --- towards point b.
    ---
    ---@param a Vector
    ---@param b Vector
    function Ray.CreateFromPoints(a, b) end

    --- Compute placement orientation matrix at intersection result. This matrix
    --- can be used to place entities in the scene oriented on the surface.
    ---
    ---@param position Vector
    ---@param normal Vector
    ---
    ---@return Matrix
    function Ray.GetPlacementOrientation(position, normal) end
```

## AABB

```lua
    --- If no argument is given, it will be infinitely inverse that can't
    --- intersect.
    ---
    ---@param min? Vector
    ---@param max Vector
    ---
    ---@return AABB
    function AABB(min, max) end

    --- Axis Aligned Bounding Box. Can be intersected with other primitives.
    ---
    ---@class AABB
    ---
    ---@field Min Vector
    ---@field Max Vector
    local AABB = {}


    --- Omit the z component for intersection check for more precise 2D
    --- intersection.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function AABB.Intersects2D(aabb) end

    --- Returns whether this AABB intersects the given AABB.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function AABB.Intersects(aabb) end

    --- Returns whether this AABB intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean
    function AABB.Intersects(sphere) end

    --- Returns whether this AABB intersects the given ray.
    ---
    ---@param ray Ray
    ---
    ---@return boolean
    function AABB.Intersects(ray) end

    --- Returns whether the given point is inside this AABB.
    ---
    ---@param point Vector
    ---
    ---@return boolean
    function AABB.Intersects(point) end

    --- Returns the minimum corner of the box.
    ---
    ---@return Vector
    function AABB.GetMin() end

    --- Returns the maximum corner of the box.
    ---
    ---@return Vector
    function AABB.GetMax() end

    --- Sets the minimum corner of the box.
    ---
    ---@param vector Vector
    function AABB.SetMin(vector) end

    --- Sets the maximum corner of the box.
    ---
    ---@param vector Vector
    function AABB.SetMax(vector) end

    --- Returns the center point of the box.
    ---
    ---@return Vector
    function AABB.GetCenter() end

    --- Returns the half-extents (half the size along each axis) of the box.
    ---
    ---@return Vector
    function AABB.GetHalfExtents() end

    --- Transforms the AABB with a matrix and returns the resulting conservative
    --- AABB.
    ---
    ---@param matrix Matrix
    ---
    ---@return AABB
    function AABB.Transform(matrix) end

    --- Get a matrix that represents the AABB as OBB (oriented bounding box).
    ---
    ---@return Matrix
    function AABB.GetAsBoxMatrix() end

    --- Projects the AABB to the screen, returns a 2D rectangle in UV-space as
    --- Vector(topleftX, topleftY, bottomrightX, bottomrightY), each value is in
    --- range [0, 1].
    ---
    ---@param ViewProjection Matrix
    ---
    ---@return Vector
    function AABB.ProjectToScreen(ViewProjection) end
```

## Sphere

```lua
    --- Creates a sphere from a center point and a radius.
    ---
    ---@param center Vector
    ---@param radius number
    ---
    ---@return Sphere
    function Sphere(center, radius) end

    --- Sphere defined by center Vector and radius. Can be intersected with
    --- other primitives.
    ---
    ---@class Sphere
    ---
    ---@field Center Vector
    ---@field Radius number
    local Sphere = {}

    --- Returns whether this sphere intersects the given AABB.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function Sphere.Intersects(aabb) end

    --- Returns whether this sphere intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean
    function Sphere.Intersects(sphere) end

    --- Returns whether this sphere intersects the given capsule.
    ---
    ---@param capsule Capsule
    ---
    ---@return boolean
    function Sphere.Intersects(capsule) end

    --- Returns whether this sphere intersects the given ray.
    ---
    ---@param ray Ray
    ---
    ---@return boolean
    function Sphere.Intersects(ray) end

    --- Returns whether the given point is inside this sphere.
    ---
    ---@param point Vector
    ---
    ---@return boolean
    function Sphere.Intersects(point) end

    --- Returns the center of the sphere.
    ---
    ---@return Vector
    function Sphere.GetCenter() end

    --- Returns the radius of the sphere.
    ---
    ---@return number
    function Sphere.GetRadius() end

    --- Sets the center of the sphere.
    ---
    ---@param value Vector
    function Sphere.SetCenter(value) end

    --- Sets the radius of the sphere.
    ---
    ---@param value number
    function Sphere.SetRadius(value) end

    --- Compute placement orientation matrix at intersection result. This matrix
    --- can be used to place entities in the scene oriented on the surface.
    ---
    ---@param position Vector
    ---@param normal Vector
    ---
    ---@return Matrix
    function Sphere.GetPlacementOrientation(position, normal) end
```

## Capsule

```lua
    --- Creates a capsule from a base point, a tip point and a radius.
    ---
    ---@param base Vector
    ---@param tip Vector
    ---@param radius number
    ---
    ---@return Capsule
    function Capsule(base, tip, radius) end

    --- It's like two spheres connected by a cylinder. Base and Tip are the two
    --- endpoints, radius is the cylinder's radius.
    ---
    ---@class Capsule
    ---
    ---@field Base Vector
    ---@field Tip Vector
    ---@field Radius number
    local Capsule = {}

    --- Returns whether this capsule intersects the given capsule.
    ---
    ---@param other Capsule
    ---
    ---@return boolean intersects
    ---@return Vector position
    ---@return Vector normal
    ---@return number depth
    function Capsule.Intersects(other) end

    --- Returns whether this capsule intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean intersects
    ---@return number depth
    ---@return Vector normal
    function Capsule.Intersects(sphere) end

    --- Returns whether this capsule intersects the given ray.
    ---
    ---@param ray Ray
    ---
    ---@return boolean
    function Capsule.Intersects(ray) end

    --- Returns whether the given point is inside this capsule.
    ---
    ---@param point Vector
    ---
    ---@return boolean
    function Capsule.Intersects(point) end

    --- Returns the base point of the capsule.
    ---
    ---@return Vector
    function Capsule.GetBase() end

    --- Returns the tip point of the capsule.
    ---
    ---@return Vector
    function Capsule.GetTip() end

    --- Returns the radius of the capsule.
    ---
    ---@return number
    function Capsule.GetRadius() end

    --- Sets the base point of the capsule.
    ---
    ---@param value Vector
    function Capsule.SetBase(value) end

    --- Sets the tip point of the capsule.
    ---
    ---@param value Vector
    function Capsule.SetTip(value) end

    --- Sets the radius of the capsule.
    ---
    ---@param value number
    function Capsule.SetRadius(value) end

    --- Compute placement orientation matrix at intersection result. This matrix
    --- can be used to place entities in the scene oriented on the surface.
    ---
    ---@param position Vector
    ---@param normal Vector
    ---
    ---@return Matrix
    function Capsule.GetPlacementOrientation(position, normal) end

    --- Returns the axis-aligned bounding box of the capsule.
    ---
    ---@return AABB
    function Capsule.GetAABB() end
```
