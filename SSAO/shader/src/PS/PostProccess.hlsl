#define MAX_SAMPLE_COUNT 16
#define RADIUS 1.0f
#define NORMAL_BIAS 0.0f;

float calculateOcclusion(in float3 position, in float3 sampleDir, in float radius, in float depth)
{
    float3 projectedPosition = GetProjection(position + sampleDir * radius);

	float projected = GetAssessmentDepth(projectedPosition.z);
	float readed =    GetAssessmentDepth(GetDepth(projectedPosition.xy));

    ///
	float diff = (readed - projected);

	float occlussion =  step(diff, 0); // (x >= y) ? 1 : 0
	float distanceCheck = min(1.0, radius / abs(depth - readed));

	return occlussion * distanceCheck;
    ///
}

float4 PS(in float4 pos: SV_POSITION):SV_TARGET
{ 
/*
    float  depth = GetDepth(pos.xy);//float2(256+0,576-1)

    //if(0.202<depth&&depth<0.2024)
    //    return float4(0,1,1,1);
    //return float4(0,0,0,1);    

    if(0.202<depth&&depth<0.203)
        return float4(1,0,0,1);
    else if(0.548<depth&&depth<0.5489)
        return float4(0,1,0,1);
    else if(0.890<depth&&depth<0.891)
        return float4(0,0,1,1);
    else if(depth<1.0)
       return float4(1,1,1,1);
    else   
       return float4(0,0,0,1);
*/
/*
    float  depth = GetDepth(pos.xy);

    float3 n = GetNormal(pos.xy);  

    if(0.865<n.x&&n.x<0.867)
        return float4(1,0,0,1);
    else if(0.706<n.x&&n.x<0.707)
        return float4(0,1,0,1);
    else if(0.99999<n.y&&n.y<=1.0)
        return float4(0,0,1,1);
    
    if(depth<1.0)
       return float4(1,1,1,1); 
    else          
       return float4(0,0,0,1); 
*/   
    float  depth =       GetDepth(pos.xy);
	float3 normal =     GetNormal(pos.xy);
	float3 position = GetPosition(pos.xy, depth) + normal * NORMAL_BIAS;
    float3 random =     GetRandom(pos.xy);

    float  assessmentDepth = GetAssessmentDepth(depth);

    ///
	float ssao = 0;

	[unroll]
	for(int i = 0; i < MAX_SAMPLE_COUNT; i++)
	{
		float3 sampleDir = reflect(samplesKernel[i], random);

		ssao += calculateOcclusion(position, sampleDir * sign(dot(sampleDir, normal)), RADIUS, assessmentDepth);
	}  

    float k = pow(1-(ssao / MAX_SAMPLE_COUNT), 2);
    ///

    return float4(float3(k, k, k), 1);
}
