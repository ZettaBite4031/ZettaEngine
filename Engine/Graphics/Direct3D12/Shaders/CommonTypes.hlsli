#if !defined(ZETTA_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not directly include in shader files. Only include via Common.hlsli
#endif

struct GlobalShaderData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InvProjection;
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    
    float3 CameraPosition;
    float ViewWidth;
    
    float3 CameraDirection;
    float ViewHeight;
    
    uint NumDirectionalLights;
    float DeltaTime;
};

struct PerObjectData
{
    float4x4 World;
    float4x4 InvWorld;
    float4x4 WorldViewProjection;
};

struct Plane
{
    float3 Normal;
    float Distance;
};

struct Sphere
{
    float3 Center;
    float Radius;
};

struct Cone
{
    float3 Tip;
    float Height;
    float3 Direction;
    float Radius;
};

struct Frustum
{
    Plane Planes[4];
};

#ifndef __cplusplus
struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;
    uint3 GroupThreadID     : SV_GroupThreadID;
    uint3 DispatchThreadID  : SV_DispatchThreadID;
    uint  GroupIndex        : SV_GroupIndex;
};
#endif

struct LightCullingDispatchParameters
{
    uint2 NumThreadGroups;
    uint2 NumThreads;
    
    uint NumLights;
    uint DepthBufferSRV_Index;
};

struct LightCullingLightInfo
{
    float3 Position;
    float Range;
    
    float3 Direction;
    float ConeRadius;
    
    uint Type;
    float3 _padding;
};

// Contains light data that's formatted and ready to be copied
// to D3D constant/structured buffer as a continuous blob
struct LightParameters
{
    float3 Position;
    float Intensity;
    
    float3 Direction;
    uint Type;
    
    float3 Color;
    float Range;
    
    float3 Attenuation;
    float CosUmbra;
    
    float CosPenumbra;
    float3 _padding;
};

struct DirectionalLightParameters
{
    float3 Direction;
    float Intensity;
    
    float3 Color;
    float _padding;
};

#ifdef __cplusplus
static_assert((sizeof(PerObjectData) % 16) == 0, "PerObjectData must formatted in 16byte chunks without implicit padding");
static_assert((sizeof(LightParameters) % 16) == 0, "LightParameters must formatted in 16byte chunks without implicit padding");
static_assert((sizeof(LightCullingLightInfo) % 16) == 0, "LightCullingLightInfo must formatted in 16byte chunks without implicit padding");
static_assert((sizeof(DirectionalLightParameters) % 16) == 0, "DirectionalLightParameters must formatted in 16byte chunks without implicit padding");
#endif