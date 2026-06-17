# Math Types

[← Back to Scripting API index](../index.md)

## Vector

```lua
    --- Constructs a 4-component vector. Any missing component defaults to 0.
    ---
    ---@param x? number
    ---@param y? number
    ---@param z? number
    ---@param w? number
    ---
    ---@return Vector
    function Vector(x, y, z, w) end

    --- A four component floating point vector. Provides efficient calculations
    --- with SIMD support.
    ---
    --- The global `vector` object exposes the same functions, so they can be
    --- called in a static style, e.g. `vector.Dot(a, b)`.
    ---
    ---@class Vector
    ---
    ---@field X number
    ---@field Y number
    ---@field Z number
    ---@field W number
    local Vector = {}

    --- Global helper exposing every Vector function for static-style calls,
    --- e.g. vector.Lerp(a, b, t).
    ---
    ---@type Vector
    vector = nil

    --- Returns the X component of the vector.
    ---
    ---@return number
    function Vector.GetX() end

    --- Returns the Y component of the vector.
    ---
    ---@return number
    function Vector.GetY() end

    --- Returns the Z component of the vector.
    ---
    ---@return number
    function Vector.GetZ() end

    --- Returns the W component of the vector.
    ---
    ---@return number
    function Vector.GetW() end

    --- Sets the X component of the vector.
    ---
    ---@param value number
    function Vector.SetX(value) end

    --- Sets the Y component of the vector.
    ---
    ---@param value number
    function Vector.SetY(value) end

    --- Sets the Z component of the vector.
    ---
    ---@param value number
    function Vector.SetZ(value) end

    --- Sets the W component of the vector.
    ---
    ---@param value number
    function Vector.SetW(value) end

    --- Returns the 3D length (magnitude) of a vector. Called with no argument
    --- it operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return number
    ---
    ---@overload fun(): number
    function Vector.Length(v) end

    --- Returns the squared 3D length of a vector (cheaper than Length). Called
    --- with no argument it operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return number
    ---
    ---@overload fun(): number
    function Vector.LengthSquared(v) end

    --- Returns the distance between two points.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.Distance(v1, v2) end

    --- Returns the squared distance between two points (cheaper than Distance).
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.DistanceSquared(v1, v2) end

    --- Returns a normalized (unit-length) copy of a vector. Called with no
    --- argument it operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Vector.Normalize(v) end

    --- Returns a normalized copy of a quaternion. Called with no argument it
    --- operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Vector.QuaternionNormalize(v) end

    --- Clamps every component of a vector between min and max. Called with two
    --- arguments it operates on the vector itself.
    ---
    ---@param v Vector
    ---@param min number
    ---@param max number
    ---
    ---@return Vector
    ---
    ---@overload fun(min: number, max: number): Vector
    function Vector.Clamp(v, min, max) end

    --- Transforms a vector by a matrix (4D, uses the w component).
    ---
    ---@param vec Vector
    ---@param matrix Matrix
    ---
    ---@return Vector
    function Vector.Transform(vec, matrix) end

    --- Transforms a 3D normal by a matrix (ignores translation).
    ---
    ---@param vec Vector
    ---@param matrix Matrix
    ---
    ---@return Vector
    function Vector.TransformNormal(vec, matrix) end

    --- Transforms a 3D coordinate by a matrix (applies the perspective divide).
    ---
    ---@param vec Vector
    ---@param matrix Matrix
    ---
    ---@return Vector
    function Vector.TransformCoord(vec, matrix) end

    --- Returns the component-wise sum of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    function Vector.Add(v1, v2) end

    --- Returns the component-wise difference of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    function Vector.Subtract(v1, v2) end

    --- Multiplies two vectors component-wise, or scales a vector by a scalar.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    ---
    ---@overload fun(v: Vector, f: number): Vector
    ---@overload fun(f: number, v: Vector): Vector
    function Vector.Multiply(v1, v2) end

    --- Returns the dot product of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.Dot(v1, v2) end

    --- Returns the cross product of two 3D vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    function Vector.Cross(v1, v2) end

    --- Linearly interpolates between two vectors by factor t in [0, 1].
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---@param t number
    ---
    ---@return Vector
    function Vector.Lerp(v1, v2, t) end

    --- Rotates a 3D vector by a quaternion.
    ---
    ---@param v Vector          the vector to rotate
    ---@param quaternion Vector the rotation quaternion
    ---
    ---@return Vector
    function Vector.Rotate(v, quaternion) end

    --- Returns a quaternion representing the identity (no) rotation.
    ---
    ---@return Vector
    function Vector.QuaternionIdentity() end

    --- Returns the inverse of a quaternion.
    ---
    ---@param quaternion Vector
    ---
    ---@return Vector
    function Vector.QuaternionInverse(quaternion) end

    --- Multiplies (concatenates) two quaternions.
    ---
    ---@param quaternion1 Vector
    ---@param quaternion2 Vector
    ---
    ---@return Vector
    function Vector.QuaternionMultiply(quaternion1, quaternion2) end

    --- Builds a quaternion from Euler angles packed as (roll, pitch, yaw).
    ---
    ---@param rotXYZ Vector
    ---
    ---@return Vector
    function Vector.QuaternionFromRollPitchYaw(rotXYZ) end

    --- Extracts Euler angles (roll, pitch, yaw) from a quaternion.
    ---
    ---@param quaternion Vector
    ---
    ---@return Vector
    function Vector.QuaternionToRollPitchYaw(quaternion) end

    --- Spherically interpolates between two quaternions by factor t.
    ---
    ---@param quaternion1 Vector
    ---@param quaternion2 Vector
    ---@param t number
    ---
    ---@return Vector
    function Vector.QuaternionSlerp(quaternion1, quaternion2, t) end

    --- Spherically interpolates between two quaternions by factor t. Same as
    --- QuaternionSlerp.
    ---
    ---@param quaternion1 Vector
    ---@param quaternion2 Vector
    ---@param t number
    ---
    ---@return Vector
    function Vector.Slerp(quaternion1, quaternion2, t) end

    --- Constructs a plane (as a Vector of coefficients) from a point and a
    --- normal.
    ---
    ---@param point Vector
    ---@param normal Vector
    ---
    ---@return Vector
    function Vector.PlaneFromPointNormal(point, normal) end

    --- Constructs a plane (as a Vector of coefficients) from three points.
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param c Vector
    ---
    ---@return Vector
    function Vector.PlaneFromPoints(a, b, c) end

    --- Computes the angle between two 3D vectors around the given axis, in the
    --- range [0, max_angle] (default 2*pi).
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param axis Vector
    ---@param max_angle? number
    ---
    ---@return number
    function Vector.GetAngle(a, b, axis, max_angle) end

    --- Computes the signed angle between two 3D vectors around the given axis.
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param axis Vector
    ---
    ---@return number
    function Vector.GetAngleSigned(a, b, axis) end
```

## Matrix

```lua
    --- Constructs a 4x4 matrix from 16 row-major components. Any missing
    --- component defaults to 0; with no arguments the identity matrix is
    --- returned.
    ---
    ---@param m00? number
    ---@param m01? number
    ---@param m02? number
    ---@param m03? number
    ---@param m10? number
    ---@param m11? number
    ---@param m12? number
    ---@param m13? number
    ---@param m20? number
    ---@param m21? number
    ---@param m22? number
    ---@param m23? number
    ---@param m30? number
    ---@param m31? number
    ---@param m32? number
    ---@param m33? number
    ---
    ---@return Matrix
    function Matrix(
        m00, m01, m02, m03,
        m10, m11, m12, m13,
        m20, m21, m22, m23,
        m30, m31, m32, m33
    ) end

    --- A four by four matrix. Provides efficient calculations with SIMD
    --- support.
    ---
    --- The global `matrix` object exposes the same functions, so they can be
    --- called in a static style, e.g. `matrix.Multiply(a, b)`.
    ---
    ---@class Matrix
    local Matrix = {}

    --- Global helper exposing every Matrix function for static-style calls,
    --- e.g. matrix.Multiply(a, b).
    ---
    ---@type Matrix
    matrix = nil

    --- Returns the given row (0-3) of the matrix as a Vector.
    ---
    ---@param row? integer row index in [0, 3] (default 0)
    ---
    ---@return Vector
    function Matrix.GetRow(row) end

    --- Builds a translation matrix from a position vector (identity if
    --- omitted).
    ---
    ---@param vector? Vector
    ---
    ---@return Matrix
    function Matrix.Translation(vector) end

    --- Builds a rotation matrix from Euler angles packed as (roll, pitch, yaw).
    ---
    ---@param rollPitchYaw? Vector
    ---
    ---@return Matrix
    function Matrix.Rotation(rollPitchYaw) end

    --- Builds a rotation matrix around the X axis.
    ---
    ---@param angleInRadians? number
    ---
    ---@return Matrix
    function Matrix.RotationX(angleInRadians) end

    --- Builds a rotation matrix around the Y axis.
    ---
    ---@param angleInRadians? number
    ---
    ---@return Matrix
    function Matrix.RotationY(angleInRadians) end

    --- Builds a rotation matrix around the Z axis.
    ---
    ---@param angleInRadians? number
    ---
    ---@return Matrix
    function Matrix.RotationZ(angleInRadians) end

    --- Builds a rotation matrix from a quaternion.
    ---
    ---@param quaternion? Vector
    ---
    ---@return Matrix
    function Matrix.RotationQuaternion(quaternion) end

    --- Builds a scaling matrix from a per-axis Vector or a uniform scalar
    --- (identity if omitted).
    ---
    ---@param scaleXYZ? Vector
    ---
    ---@return Matrix
    ---
    ---@overload fun(scale: number): Matrix
    function Matrix.Scale(scaleXYZ) end

    --- Builds a left-handed view matrix looking from eye toward a direction.
    ---
    ---@param eye Vector
    ---@param direction Vector
    ---@param up? Vector up direction (default (0, 1, 0))
    ---
    ---@return Matrix
    function Matrix.LookTo(eye, direction, up) end

    --- Builds a left-handed view matrix looking from eye at a focus point.
    ---
    ---@param eye Vector
    ---@param focusPos Vector
    ---@param up? Vector up direction (default (0, 1, 0))
    ---
    ---@return Matrix
    function Matrix.LookAt(eye, focusPos, up) end

    --- Returns the product of two matrices.
    ---
    ---@param m1 Matrix
    ---@param m2 Matrix
    ---
    ---@return Matrix
    function Matrix.Multiply(m1, m2) end

    --- Returns the component-wise sum of two matrices.
    ---
    ---@param m1 Matrix
    ---@param m2 Matrix
    ---
    ---@return Matrix
    function Matrix.Add(m1, m2) end

    --- Returns the transpose of a matrix.
    ---
    ---@param m Matrix
    ---
    ---@return Matrix
    function Matrix.Transpose(m) end

    --- Returns the inverse of a matrix together with its determinant.
    ---
    ---@param m Matrix
    ---
    ---@return Matrix inverse
    ---@return number determinant
    function Matrix.Inverse(m) end

    --- Returns the forward direction of a matrix. Called with no argument it
    --- operates on the matrix itself.
    ---
    ---@param mat Matrix
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Matrix.GetForward(mat) end

    --- Returns the up direction of a matrix. Called with no argument it
    --- operates on the matrix itself.
    ---
    ---@param mat Matrix
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Matrix.GetUp(mat) end

    --- Returns the right direction of a matrix. Called with no argument it
    --- operates on the matrix itself.
    ---
    ---@param mat Matrix
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Matrix.GetRight(mat) end
```
