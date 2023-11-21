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
    
    float DeltaTime;
};

struct PerObjectData
{
    float4x4 World;
    float4x4 InvWorld;
    float4x4 WorldViewProjection;
};

struct VertexOut
{
    float4 HomogeneousPosition  : SV_Position;
    float3 World                : POSITION;
    float3 Normal               : NORMAL;
    float3 Tangent              : TANGENT;
    float2 UV                   : TEXTURE;
};

struct PixelOut
{
    float4 Color : SV_Target0;
};

VertexOut TestShaderVS(in uint VertexIdx : SV_VertexID)
{
    VertexOut vsOut;
    
    vsOut.HomogeneousPosition = 0.f;
    vsOut.World = 0.f;
    vsOut.Normal = 0.f;
    vsOut.Tangent = 0.f;
    vsOut.UV = 0.f;
    
    return vsOut;
}

[earlydepthstencil]
PixelOut TestShaderPS(in VertexOut psIn)
{
    PixelOut psOut;
    psOut.Color = 0.f;
    
    return psOut;
}