#include "shader\\inc\\shader_include.hlsl"

#include "shader\\src\\gs\\quad.hlsl"

StructuredBuffer<float3> samplesKernel     : register( t1 );
Texture2D<float4> normalMap                : register( t2 );
Texture2D<float> depthMap                  : register( t3 );
Texture2D<float4> randomTexture            : register( t4 );

SamplerState noiseSampler : register( s0 );

float GetDepth(float2 frag)
{
    return depthMap.Load(  int3(frag.xy,0) ).x;
}

float GetLinearDepth(float depth)
{
    return DepthLinear(g_vFrustumNearFar.x, g_vFrustumNearFar.y, depth);
}

float GetAssessmentDepth(float depth)
{
	return 1 / sqrt(1.0 - depth);
}

float3 GetNormal(float2 frag)
{
	return normalMap.Load( int3(frag.xy,0) ).xyz;
}

float3 GetRandom(float2 frag)
{
    return normalize(randomTexture.Sample(noiseSampler, frag.xy/float2(4,4)).xyz);
}

float3 GetPosition(float2 frag, float depth)
{
    float2 ndc = float2(frag.xy/g_vFrustumParams.xy) * float2(2, -2) + float2(-1, 1); 

    float3 view_p = GetLinearDepth(depth) * float3(ndc.x * g_vFrustumParams.w/g_vFrustumParams.z, ndc.y * 1/g_vFrustumParams.z, 1);

    return mul( float4( view_p, 1.0 ), g_mInvView ).xyz;
}

float3 GetProjection(float3 position)
{
	 float4 projected_p = mul( float4(position, 1.0f), g_mWorldViewProjection);
     projected_p /= projected_p.w;
	 return float3((float2(0.5f, -0.5f) * projected_p.xy + float2(0.5f, 0.5f))*g_vFrustumParams.xy, projected_p.z);
}

#include "shader\\src\\ps\\PostProccess.hlsl"