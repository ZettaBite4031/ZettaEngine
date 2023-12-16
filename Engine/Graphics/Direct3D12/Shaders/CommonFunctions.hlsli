#if !defined(ZETTA_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not directly include in shader files. Only include via Common.hlsli
#endif

bool PointInsidePlane(float3 p, Plane plane)
{
    return dot(plane.Normal, p) - plane.Distance < 0;
}

bool SphereInsidePlane(Sphere sphere, Plane plane)
{
    return dot(plane.Normal, sphere.Center) - plane.Distance < -sphere.Radius;
}

bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
    bool res = true;
    if (sphere.Center.z - sphere.Radius > zNear || sphere.Center.z + sphere.Radius < zFar) res = false;
    for (int i = 0; i < 4 && res; i++) if (SphereInsidePlane(sphere, frustum.Planes[i])) res = false;
    return res;
}

bool ConeInsidePlane(Cone cone, Plane plane)
{
    float3 m = cross(cross(plane.Normal, cone.Direction), cone.Direction);
    float3 Q = cone.Tip + cone.Direction * cone.Height - m * cone.Radius;
    return PointInsidePlane(cone.Tip, plane) && PointInsidePlane(Q, plane);

}

bool ConeInsideFrustum(Cone cone, Frustum frustum, float zNear, float zFar)
{
    bool res = true;
    Plane nearPlane = { float3(0, 0, -1), -zNear };
    Plane farPlane = { float3(0, 0, 1), zFar };
    
    if (ConeInsidePlane(cone, nearPlane) || ConeInsidePlane(cone, farPlane)) res = false;
    for (int i = 0; i < 4 && res; i++) if (ConeInsidePlane(cone, frustum.Planes[i])) res = false;
    return res;
}

// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes right-handedness (CCW winding) for the
// coordinate system to determine the direction of the plane normal.
Plane
    ComputePlane(
    float3 p0, float3 p1, float3 p2)
{
    Plane plane;

    const float3 v0 = p1 - p0;
    const float3 v2 = p2 - p0;
    
    plane.Normal = normalize(cross(v0, v2)); // Swap v0 and v2 for left-handed winding order;
    plane.Distance = dot(plane.Normal, p0);

    return plane;
}

float4 ClipToView(float4 clip, float4x4 inverseProjection)
{
    float4 view = mul(inverseProjection, clip);
    view /= view.w;
    return view;
}

float4 ScreenToView(float4 screen, float2 invViewDimensions, float4x4 inverseProjection)
{
    float2 texCoord = screen.xy * invViewDimensions;
    
    float4 clip = float4(float2(texCoord.x, 1.f - texCoord.y) * 2.f - 1.f, screen.z, screen.w);

    return ClipToView(clip, inverseProjection);

}