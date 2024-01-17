#include "Common.hlsli"

ConstantBuffer<GlobalShaderData>                    GlobalData      : register(b0, space0);
ConstantBuffer<LightCullingDispatchParameters>      ShaderParams    : register(b1, space0);
RWStructuredBuffer<Frustum>                         Frustums        : register(u0, space0);


#if USE_BOUNDING_SPHERES
// NOTE: `TILE_SIZE` is defined by the engine at compile time
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void ComputeGridFrustumsCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x >= ShaderParams.NumThreads.x || DispatchThreadID.y >= ShaderParams.NumThreads.y) return;

    const float2 invViewDimensions = TILE_SIZE / float2(GlobalData.ViewWidth, GlobalData.ViewHeight);
    const float2 topLeft = DispatchThreadID.xy * invViewDimensions;
    const float2 center = topLeft + (invViewDimensions * 0.5f);

    float3 topLeftVS = AntiprojectionUV(topLeft, 0, GlobalData.InvProjection).xyz;
    float3 centerVS = AntiprojectionUV(center, 0, GlobalData.InvProjection).xyz;
    
    const float farClipRcp = -GlobalData.InvProjection._m33;
    Frustum frustum = { normalize(centerVS), distance(centerVS, topLeftVS) * farClipRcp };
    
    Frustums[DispatchThreadID.x + (DispatchThreadID.y * ShaderParams.NumThreads.x)] = frustum;

}
#else
// NOTE: `TILE_SIZE` is defined by the engine at compile time
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void ComputeGridFrustumsCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint x = DispatchThreadID.x;
    const uint y = DispatchThreadID.y;

    // Return if thread is outside bounds of grid
    if (x >= ShaderParams.NumThreads.x || y >= ShaderParams.NumThreads.y)
        return;
    
    // Compute the 4 corner points of the far clipping plane for frustum vertices
    float4 screenspace[4];
    screenspace[0] = float4(float2(x + 0, y + 0) * TILE_SIZE, 0.f, 1.f);
    screenspace[1] = float4(float2(x + 1, y + 0) * TILE_SIZE, 0.f, 1.f);
    screenspace[2] = float4(float2(x + 0, y + 1) * TILE_SIZE, 0.f, 1.f);
    screenspace[3] = float4(float2(x + 1, y + 1) * TILE_SIZE, 0.f, 1.f);
    
    const float2 invViewDimensions = 1.f / float2(GlobalData.ViewWidth, GlobalData.ViewHeight);
    float3 viewspace[4];

    viewspace[0] = ScreenToView(screenspace[0], invViewDimensions, GlobalData.InvProjection).xyz;
    viewspace[1] = ScreenToView(screenspace[1], invViewDimensions, GlobalData.InvProjection).xyz;
    viewspace[2] = ScreenToView(screenspace[2], invViewDimensions, GlobalData.InvProjection).xyz;
    viewspace[3] = ScreenToView(screenspace[3], invViewDimensions, GlobalData.InvProjection).xyz;

    const float3 eyePos = (float3)0;
    Frustum frustum;
    frustum.Planes[0] = ComputePlane(viewspace[0], eyePos, viewspace[2]);
    frustum.Planes[1] = ComputePlane(viewspace[3], eyePos, viewspace[1]);
    frustum.Planes[2] = ComputePlane(viewspace[1], eyePos, viewspace[0]);
    frustum.Planes[3] = ComputePlane(viewspace[2], eyePos, viewspace[3]);
    
    Frustums[x + (y * ShaderParams.NumThreads.x)] = frustum;

}
#endif