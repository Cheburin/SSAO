#include "shader\\inc\\shader_include.hlsl"

#include "shader\\src\\gs\\quad.hlsl"

Texture2D colorMap                      : register( t2 );
Texture2D<float4> normalMap             : register( t3 );
Texture2D<float> depthMap               : register( t4 );

cbuffer Handling : register(b1)
{
	int Radius;
	int reserv0;
	int reserv1;
	int reserv2;

	float4 Weights[129];

	float DepthAnalysisFactor;
	int DepthAnalysis;
	int NormalAnalysis;
	int reserv5;
};

float GetDepth(float2 uv)
{
	return depthMap.Load( int3(uv.xy * g_vFrustumParams.xy, 0) ).x;
}

float3 GetNormal(float2 uv)
{
	return normalMap.Load( int3(uv.xy * g_vFrustumParams.xy, 0) ).xyz;
}

float4 GetColor(float2 uv)
{
	return colorMap.Load( int3(uv.xy * g_vFrustumParams.xy, 0) );
}

float LinearizeDepth(float depth)
{
	float2 NearFarValue = g_vFrustumNearFar;

	float z_n = 2.0 * depth - 1.0;
    return 2.0 * NearFarValue.x * NearFarValue.y / (NearFarValue.y + NearFarValue.x - z_n * (NearFarValue.y - NearFarValue.x));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 Blur(float2 UV, float2 SourcePixel, const float2 dir)
{
//
	float4 finalColor = GetColor(UV) * Weights[Radius].x;

	float lDepthC = DepthAnalysis ? LinearizeDepth(GetDepth(UV)) : 0;

	float3 normalC = GetNormal(UV);

	float totalAdditionalWeight = Weights[Radius].x;
	
	for(int i = 1; i <= Radius; i++)
	{
		float2 UVL = UV - SourcePixel * dir * i;
		float2 UVR = UV + SourcePixel * dir * i;

		float depthFactorR = 1.0f;
		float depthFactorL = 1.0f;
		float normalFactorL = 1.0f;
		float normalFactorR = 1.0f;
		
		[flatten]
		if(DepthAnalysis)
		{
			float lDepthR = LinearizeDepth(GetDepth(UVR));
			float lDepthL = LinearizeDepth(GetDepth(UVL));

			depthFactorR = saturate(1.0f / (abs(lDepthR - lDepthC) / DepthAnalysisFactor));
			depthFactorL = saturate(1.0f / (abs(lDepthL - lDepthC) / DepthAnalysisFactor));
		}

		[flatten]
		if(NormalAnalysis)
		{
			float3 normalR = GetNormal(UVR);
			float3 normalL = GetNormal(UVL);

			normalFactorL = saturate(max(0.0f, dot(normalC, normalL)));
			normalFactorR = saturate(max(0.0f, dot(normalC, normalR)));
		}

		float cwR = Weights[Radius + i].x * depthFactorR * normalFactorR;
		float cwL = Weights[Radius - i].x * depthFactorL * normalFactorL;

		finalColor += GetColor(UVR) * cwR;
		finalColor += GetColor(UVL) * cwL;

		totalAdditionalWeight += cwR;
		totalAdditionalWeight += cwL;
	}


	return finalColor / totalAdditionalWeight;
//
}

float4 HB(in float4 pos: SV_POSITION) : SV_TARGET
{
    float2 UV = pos.xy / g_vFrustumParams.xy;
	float2 SourcePixel = float2(1.0, 1.0) / g_vFrustumParams.xy;

	return Blur(UV, SourcePixel, float2(1, 0));
}

float4 VB(in float4 pos: SV_POSITION) : SV_TARGET
{
    float2 UV = pos.xy / g_vFrustumParams.xy;
	float2 SourcePixel = float2(1.0, 1.0) / g_vFrustumParams.xy;

	return Blur(UV, SourcePixel, float2(0, 1));
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////